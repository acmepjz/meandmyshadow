/****************************************************************************
** Copyright (C) 2011 Luka Horvat <redreaper132 at gmail.com>
** Copyright (C) 2011 Edward Lii <edward_iii at myway.com>
** Copyright (C) 2011 O. Bahri Gordebak <gordebak at gmail.com>
**
**
** This file may be used under the terms of the GNU General Public
** License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
**  You should have received a copy of the GNU General Public License
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/
#include "GameState.h"
#include "Globals.h"
#include "Functions.h"
#include "FileManager.h"
#include "GameObjects.h"
#include "ThemeManager.h"
#include "Objects.h"
#include "Levels.h"
#include "LevelEditor.h"
#include "TreeStorageNode.h"
#include "POASerializer.h"
#include "GUIListBox.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef WIN32
#include <windows.h>
#include <shlobj.h>
#else
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#endif
using namespace std;

LevelEditor::LevelEditor():Game(false){
	LEVEL_WIDTH=800;
	LEVEL_HEIGHT=600;
	
	//Load an empty level.
	loadLevel(getDataPath()+"misc/Empty.map");
	
	//This will set some default settings.
	reset();
	
	//Load the toolbar.
	toolbar=loadImage(getDataPath()+"gfx/menu/toolbar.png");
	toolbarRect={205,555,410,50};
	
	//Load the selectionMark.
	selectionMark=loadImage(getDataPath()+"gfx/menu/selection.png");
	
	//Create the semi transparent surface.
	placement=SDL_CreateRGBSurface(SDL_SWSURFACE,800,600,32,0x000000FF,0x0000FF00,0x00FF0000,0);
	SDL_SetColorKey(placement,SDL_SRCCOLORKEY|SDL_RLEACCEL,SDL_MapRGB(placement->format,255,0,255));
	SDL_SetAlpha(placement,SDL_SRCALPHA,125);
}

LevelEditor::~LevelEditor(){
	for(unsigned int i=0;i<levelObjects.size();i++) delete levelObjects[i];
	levelObjects.clear();
	selection.clear();
	SDL_FreeSurface(placement);
	camera.x=0;
	camera.y=0;
}

void LevelEditor::reset(){
	//Set some default values.
	playMode=false;
	tool=ADD;
	currentType=0;
	pressedShift=false;
	dragging=false;
	selectionDrag=false;
	dragCenter=NULL;
	camera.x=0;
	camera.y=0;
	cameraXvel=0;
	cameraYvel=0;
	objectProperty=NULL;
	configuredObject=NULL;
	
	selection.clear();
	clipboard.clear();
}

void LevelEditor::saveLevel(string fileName){
	//Create the output stream and check if it starts.
	std::ofstream save(fileName.c_str());
	if(!save) return;

	//The dimensions of the level.
	int maxX=0;
	int maxY=0;

	//The storageNode to put the level data in before writing it away.
	TreeStorageNode node;
	char s[64];

	//The width of the level.
	maxX=LEVEL_WIDTH;
	sprintf(s,"%d",maxX);
	node.attributes["size"].push_back(s);

	//The height of the level.
	maxY=LEVEL_HEIGHT;
	sprintf(s,"%d",maxY);
	node.attributes["size"].push_back(s);

	//Loop through the gameObjects and save them.
	for(int o=0;o<(signed)levelObjects.size();o++){
		int objectType=levelObjects[o]->type;

		//Check if it's a legal gameObject type.
		if(objectType>=0 && objectType<TYPE_MAX){
			TreeStorageNode* obj1=new TreeStorageNode;
			node.subNodes.push_back(obj1);

			//It's a tile so name the node tile.
			obj1->name="tile";
			
			//Write away the type of the gameObject.
			sprintf(s,"%d",objectType);
			obj1->value.push_back(g_sBlockName[objectType]);

			//Get the box for the location of the gameObject.
			SDL_Rect box=levelObjects[o]->getBox(BoxType_Base);
			//Put the location in the storageNode.
			sprintf(s,"%d",box.x);
			obj1->value.push_back(s);
			sprintf(s,"%d",box.y);
			obj1->value.push_back(s);

			//Loop through the editor data and save it also.
			vector<pair<string,string> > obj;
			levelObjects[o]->getEditorData(obj);
			for(unsigned int i=0;i<obj.size();i++){
				if((!obj[i].first.empty()) && (!obj[i].second.empty())){
					obj1->attributes[obj[i].first].push_back(obj[i].second);
				}
			}
		}
	}

	//Create a POASerializer and write away the level node.
	POASerializer objSerializer;
	objSerializer.writeNode(&node,save,true,true);
}


///////////////EVENT///////////////////
void LevelEditor::handleEvents(){
	//Check if we need to quit, if so we enter the exit state.
	if(event.type==SDL_QUIT){
		setNextState(STATE_EXIT);
	}
	
	//If playing/testing we should the game handle the events.
	if(playMode){
		Game::handleEvents();
		
		//Also check if we should exit the playMode.
		if(event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_ESCAPE){
			//Reset the game and disable playMode.
			Game::reset();
			playMode=false;
			camera.x=cameraSave.x;
			camera.y=cameraSave.y;
		}
	}else{
		//Also check if we should exit the editor.
		if(event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_ESCAPE){
			//We exit the level editor.
			if(GUIObjectRoot){
				delete GUIObjectRoot;
				GUIObjectRoot=NULL;
			}
			setNextState(STATE_MENU);
		}
		
		//Also check if we should exit the editor.
		if(event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_RSHIFT){
			pressedShift=true;
		}
		if(event.type==SDL_KEYUP && event.key.keysym.sym==SDLK_RSHIFT){
			pressedShift=false;
		}
		
		//Check if delete is pressed.
		if(event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_DELETE){
			if(!selection.empty()){
				//Loop through the selected game objects.
				for(unsigned int o=0; o<selection.size(); o++){
					//First find the index in the levelObjects vector.
					std::vector<GameObject*>::iterator it;
					it=find(levelObjects.begin(),levelObjects.end(),selection[o]);
					levelObjects.erase(it);
					
					//Now delete the game object.
					delete selection[o];
				}
				
				//And clear the selection vector.
				selection.clear();
				dragCenter=NULL;
				selectionDrag=false;
			}
		}
		
		//Check for copy (Ctrl+c) or cut (Ctrl+x).
		if(event.type==SDL_KEYDOWN && (event.key.keysym.sym==SDLK_c || event.key.keysym.sym==SDLK_x) && (event.key.keysym.mod & KMOD_CTRL)){
			//Clear the current clipboard.
			clipboard.clear();
			
			//Check if the selection isn't empty.
			if(!selection.empty()){
				//Loop through the selection to find the left-top block.
				int x=selection[0]->getBox().x;
				int y=selection[0]->getBox().y;
				for(unsigned int o=1; o<selection.size(); o++){
					if(selection[o]->getBox().x<x || selection[o]->getBox().y<y){
						x=selection[o]->getBox().x;
						y=selection[o]->getBox().y;
					}
				}
			
				//Loop through the selection for the actual copying.
				for(unsigned int o=0; o<selection.size(); o++){
					//Get the editor data of the object.
					vector<pair<string,string> > obj;
					selection[o]->getEditorData(obj);
				
					//Loop through the editor data and convert it.
					map<string,string> objMap;
					for(unsigned int i=0;i<obj.size();i++){
						objMap[obj[i].first]=obj[i].second;
					}
					//Add some entries to the map.
					char s[64];
					sprintf(s,"%d",selection[o]->getBox().x-x);
					objMap["x"]=s;
					sprintf(s,"%d",selection[o]->getBox().y-y);
					objMap["y"]=s;
					sprintf(s,"%d",selection[o]->type);
					objMap["type"]=s;
					
					//And add the map to the clipboard vector.
					clipboard.push_back(objMap);
				
					if(event.key.keysym.sym==SDLK_x){
						//First find the index in the levelObjects vector.
						std::vector<GameObject*>::iterator it;
						it=find(levelObjects.begin(),levelObjects.end(),selection[o]);
						levelObjects.erase(it);
					
						//Now delete the game object.
						delete selection[o];
					}
				}
			
				//Only clear the selection when Ctrl+x;
				if(event.key.keysym.sym==SDLK_x){
					selection.clear();
					dragCenter=NULL;
					selectionDrag=false;
				}
			}
		}
		
		//Check for paste (Ctrl+v).
		if(event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_v && (event.key.keysym.mod & KMOD_CTRL)){
			//First make sure that the clipboard isn't empty.
			if(!clipboard.empty()){
				//Clear the current selection.
				selection.clear();
				
				//Get the current mouse location.
				int x,y;
				SDL_GetMouseState(&x,&y);
				x+=camera.x; y+=camera.y;
				
				//Apply snap to grid.
				if(!pressedShift){
					x=(x/50)*50;
					y=(y/50)*50;
				}else{
					x-=25;
					y-=25;
				}
				
				//Loop through the clipboard.
				for(unsigned int o=0;o<clipboard.size();o++){
					Block* block=new Block(0,0,atoi(clipboard[o]["type"].c_str()),this);
					block->setPosition(atoi(clipboard[o]["x"].c_str())+x,atoi(clipboard[o]["y"].c_str())+y);
					block->setEditorData(clipboard[o]);
					levelObjects.push_back(block);
					
					//Check if the block is outside the level size.
					if(block->getBox().x+50>LEVEL_WIDTH){
						LEVEL_WIDTH=block->getBox().x+50;
					}
					if(block->getBox().y+50>LEVEL_HEIGHT){
						LEVEL_HEIGHT=block->getBox().y+50;
					}
					
					//Also add the block to the selection.
					selection.push_back(block);
				}
			}
		}
		
		//Check if the return button is pressed.
		//If so run the configure tool.
		if(event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_RETURN){
			//Get the current mouse location.
			int x,y;
			SDL_GetMouseState(&x,&y);
			//Create the rectangle.
			SDL_Rect mouse={x+camera.x,y+camera.y,0,0};
			
			//Loop through the selected game objects.
			for(unsigned int o=0; o<levelObjects.size(); o++){
				//Check for collision.
				if(checkCollision(mouse,levelObjects[o]->getBox())){
					tool=CONFIGURE;
					//And invoke the event, we always say selected is true since for the configure tool it doesn't matter.
					onClickObject(levelObjects[o],false);
					//Break out of the for loop.
					break;
				}
			}
		}
		
		//Check for the arrow keys, used for moving the camera when playMode=false.
		Uint8* keyState=SDL_GetKeyState(NULL);
		cameraXvel=0;
		cameraYvel=0;
		if(keyState[SDLK_RIGHT]){
			if(pressedShift){
				cameraXvel+=10;
			}else{
				cameraXvel+=5;
			}
		}
		if(keyState[SDLK_LEFT]){
			if(pressedShift){
				cameraXvel-=10;
			}else{
				cameraXvel-=5;
			}
		}
		if(keyState[SDLK_UP]){
			if(pressedShift){
				cameraYvel-=10;
			}else{
				cameraYvel-=5;
			}
		}
		if(keyState[SDLK_DOWN]){
			if(pressedShift){
				cameraYvel+=10;
			}else{
				cameraYvel+=5;
			}
		}
		
		//Check if the left mouse button is pressed/holded.
		if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button==SDL_BUTTON_LEFT){
			pressedLeftMouse=true;
		}
		if(event.type==SDL_MOUSEBUTTONUP && event.button.button==SDL_BUTTON_LEFT){
			pressedLeftMouse=false;
			
			//We also need to check if dragging is true.
			if(dragging){
				//Set dragging false and call the onDrop event.
				dragging=false;
				int x,y;
				SDL_GetMouseState(&x,&y);
				//We call the drop event.
				onDrop(x+camera.x,y+camera.y);
			}
		}
		
		//Check if the mouse is dragging.
		if(pressedLeftMouse && event.type==SDL_MOUSEMOTION){
			if(abs(event.motion.xrel)+abs(event.motion.yrel)>=2){
				//Check if this is the start of the dragging.
				if(!dragging){
					//The mouse is moved enough so let's set dragging true.
					dragging=true;
					//Get the current mouse location.
					int x,y;
					SDL_GetMouseState(&x,&y);
					//We call the dragStart event.
					onDragStart(x+camera.x,y+camera.y);
				}else{
					//Dragging was already true meaning we call onDrag() instead of onDragStart().
					onDrag(event.motion.xrel,event.motion.yrel);
				}
			}
		}
		
		//Check if we scroll up, meaning the currentType++;
		if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button==SDL_BUTTON_WHEELUP){
			currentType++;
			if(currentType>=TYPE_MAX){
				currentType=0;
			}
		}
		//Check if we scroll down, meaning the currentType--;
		if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button==SDL_BUTTON_WHEELDOWN){
			currentType--;
			if(currentType<0){
				currentType=TYPE_MAX-1;
			}
		}
		
		//Check if we should enter playMode.
		if(event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_p){
			playMode=true;
			cameraSave.x=camera.x;
			cameraSave.y=camera.y;
		}
		//Check for tool shortcuts.
		if(event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_a){
			tool=ADD;
		}
		if(event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_s){
			tool=SELECT;
		}
		if(event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_d){
			tool=DELETE;
		}
		if(event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_w){
			tool=CONFIGURE;
		}
		
		//Check for certain events.
		//Get the current mouse location.
		int x,y;
		SDL_GetMouseState(&x,&y);
		//Create the rectangle.
		SDL_Rect mouse={x,y,0,0};
		
		//Check if the left mouse button is pressed.
		if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button==SDL_BUTTON_LEFT){
			//First make sure the mouse click isn't on the toolbar.
			if(checkCollision(mouse,toolbarRect)==false){
				//We didn't hit the toolbar so convert the mouse location to ingame location.
				mouse.x+=camera.x;
				mouse.y+=camera.y;
				
				//Boolean if there's a click event fired.
				bool event=false;
				//Loop through the objects to check collision.
				for(unsigned int o=0; o<levelObjects.size(); o++){
					if(checkCollision(levelObjects[o]->getBox(),mouse)==true){
						//We have collision meaning that we clicked on a object.
						std::vector<GameObject*>::iterator it;
						it=find(selection.begin(),selection.end(),levelObjects[o]);
						event=true;
						 
						if(it!=selection.end()){
							onClickObject(levelObjects[o],true);
						}else{
							onClickObject(levelObjects[o],false);
						}
					}
				}
				
				//If event is false then we clicked on void.
				if(!event){
					onClickVoid(mouse.x,mouse.y);
				}
			}
		}
		
		//Check if we should a new level. (Ctrl+n)
		if(event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_n && (event.key.keysym.mod & KMOD_CTRL)){
			reset();
			loadLevel(getDataPath()+"misc/Empty.map");
		}
		//Check if we should load a level. (Ctrl+o)
		if(event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_o && (event.key.keysym.mod & KMOD_CTRL)){
			string s="";
			if(fileDialog(s,"Load Level","map","%USER%/levels/\nAddon levels\n%DATA%/levels/\nMain levels",false,true)){
				loadLevel(processFileName(s));
			}
		}
		//Check if we should save the level. (Ctrl+s)
		if(event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_s && (event.key.keysym.mod & KMOD_CTRL)){
			string s=LevelName;
			if(fileDialog(s,"Save Level","map","%USER%/levels/",true,true)){
				saveLevel(processFileName(s));
			}
		}
	}
}

void LevelEditor::onClickObject(GameObject* obj,bool selected){
	switch(tool){
	  case SELECT:
	  case ADD:
	    //If it isn't selected then select it.
	    if(!selected){
			//First check if shift is pressed or not.
			if(!pressedShift){
				//First empty the current selection.
				selection.clear();
			}
			selection.push_back(obj);
	    }
	    break;
	  case DELETE:
	  {
	    //First we create an iterator.
	    std::vector<GameObject*>::iterator it;
	    
	    //Check if the object is selected, if so we first need to remove it from the selection.
	    if(selected){
			it=find(selection.begin(),selection.end(),obj);
			if(it!=selection.end()){
				selection.erase(it);
			}
	    }
	    
	    //Now we remove the object from the levelObjects.
	    it=find(levelObjects.begin(),levelObjects.end(),obj);
	    levelObjects.erase(it);
	    delete obj;
	    obj=NULL;
	    break;
	  }
	  case CONFIGURE:
	  {
	    //Check which type of object it is.
	    if(obj->type==TYPE_NOTIFICATION_BLOCK){
			//Open a message popup.
			//First delete any existing gui.
			if(GUIObjectRoot){
				delete GUIObjectRoot;
				GUIObjectRoot=NULL;
			}
			
			//Get the properties and check if 
			vector<pair<string,string> > objMap;
			obj->getEditorData(objMap);
			int m=objMap.size();
			if(m>0){
				//Set the object we configure.
				configuredObject=obj;
				
				//Now create the GUI.
				GUIObjectRoot=new GUIObject(100,(SCREEN_HEIGHT-180)/2,600,180,GUIObjectFrame,"Notification block");
				GUIObject* obj;
			
				obj=new GUIObject(10,40,240,36,GUIObjectLabel,"Enter message here:");
				GUIObjectRoot->ChildControls.push_back(obj);
				obj=new GUIObject(200,80,352,36,GUIObjectTextBox,objMap[1].second.c_str());
				//Set the textField.
				objectProperty=obj;
				GUIObjectRoot->ChildControls.push_back(obj);
			
				obj=new GUIObject(100,180-44,150,36,GUIObjectButton,"OK");
				obj->Name="cfgNotificationBlockOK";
				obj->EventCallback=this;
				GUIObjectRoot->ChildControls.push_back(obj);
				obj=new GUIObject(350,180-44,150,36,GUIObjectButton,"Cancel");
				obj->Name="cfgNotificationBlockCancel";
				obj->EventCallback=this;
				GUIObjectRoot->ChildControls.push_back(obj);

				//Draw screen to the tempSurface once.
				SDL_BlitSurface(screen,NULL,tempSurface,NULL);
				SDL_FillRect(screen,NULL,0);
				SDL_SetAlpha(tempSurface,SDL_SRCALPHA, 100);
				SDL_BlitSurface(tempSurface,NULL,screen,NULL);
			
				while(GUIObjectRoot){
					while(SDL_PollEvent(&event)) GUIObjectHandleEvents();
					if(GUIObjectRoot) GUIObjectRoot->render();
					SDL_Flip(screen);
					SDL_Delay(30);
				}
			}
	    }
	    if(obj->type==TYPE_CONVEYOR_BELT || obj->type==TYPE_SHADOW_CONVEYOR_BELT){
			//Open a message popup.
			//First delete any existing gui.
			if(GUIObjectRoot){
				delete GUIObjectRoot;
				GUIObjectRoot=NULL;
			}
			
			//Get the properties and check if 
			vector<pair<string,string> > objMap;
			obj->getEditorData(objMap);
			int m=objMap.size();
			if(m>0){
				//Set the object we configure.
				configuredObject=obj;
				
				//Now create the GUI.
				string s;
				if(obj->type==TYPE_CONVEYOR_BELT){
					s="Shadow Conveyor belt";
				}else{
				  	s="Conveyor belt";
				}
				  
				GUIObjectRoot=new GUIObject(100,(SCREEN_HEIGHT-180)/2,600,180,GUIObjectFrame,s.c_str());
				GUIObject* obj;
			
				obj=new GUIObject(10,40,240,36,GUIObjectLabel,"Enter speed here:");
				GUIObjectRoot->ChildControls.push_back(obj);
				obj=new GUIObject(200,80,352,36,GUIObjectTextBox,objMap[2].second.c_str());
				//Set the textField.
				objectProperty=obj;
				GUIObjectRoot->ChildControls.push_back(obj);
			
				obj=new GUIObject(100,180-44,150,36,GUIObjectButton,"OK");
				obj->Name="cfgConveyorBlockOK";
				obj->EventCallback=this;
				GUIObjectRoot->ChildControls.push_back(obj);
				obj=new GUIObject(350,180-44,150,36,GUIObjectButton,"Cancel");
				obj->Name="cfgConveyorBlockCancel";
				obj->EventCallback=this;
				GUIObjectRoot->ChildControls.push_back(obj);

				//Draw screen to the tempSurface once.
				SDL_BlitSurface(screen,NULL,tempSurface,NULL);
				SDL_FillRect(screen,NULL,0);
				SDL_SetAlpha(tempSurface,SDL_SRCALPHA, 100);
				SDL_BlitSurface(tempSurface,NULL,screen,NULL);
			
				while(GUIObjectRoot){
					while(SDL_PollEvent(&event)) GUIObjectHandleEvents();
					if(GUIObjectRoot) GUIObjectRoot->render();
					SDL_Flip(screen);
					SDL_Delay(30);
				}
			}
	    }
	    break;
	  }
	  default:
	    break;	    
	}
}

void LevelEditor::onClickVoid(int x,int y){
	switch(tool){
	  case SELECT:
	  {
	    //We need to clear the selection.
	    selection.clear();
	    break;
	  }
	  case ADD:
	  {
	      //We need to clear the selection.
	      selection.clear();
	      
	      //Now place an object.
	      //Apply snap to grid.
	      if(!pressedShift){
			x=(x/50)*50;
			y=(y/50)*50;
	      }else{
			x-=25;
			y-=25;
	      }
	      levelObjects.push_back(new Block(x,y,currentType,this));
	      
	      //Check if the block is outside the level size.
	      if(x+50>LEVEL_WIDTH){
			LEVEL_WIDTH=x+50;
	      }
	      if(y+50>LEVEL_HEIGHT){
			LEVEL_HEIGHT=y+50;
	      }
	      break;
	  }
	  default:
	    break;	    
	}	
}

void LevelEditor::onDragStart(int x,int y){
	switch(tool){
	  case SELECT:
	  case ADD:
	  {
	    //We can drag the selection so check if the selection isn't empty.
	    if(!selection.empty()){
		//The selection isn't empty so search the dragCenter.
		//Create a mouse rectangle.
		SDL_Rect mouse={x,y,0,0};
		
		//Loop through the objects to check collision.
		for(unsigned int o=0; o<selection.size(); o++){
			if(checkCollision(selection[o]->getBox(),mouse)==true){
				//We have collision so set the dragCenter.
				dragCenter=selection[o];
				selectionDrag=true;
			}
		}
	    }
	    break;
	  }
	  default:
	    break;	    
	}	
}

void LevelEditor::onDrag(int dx,int dy){
	switch(tool){
	  case DELETE:
	  {
		//No matter what we delete the item the mouse is above.
		//Get the current mouse location.
		int x,y;
		SDL_GetMouseState(&x,&y);
		//Create the rectangle.
		SDL_Rect mouse={x+camera.x,y+camera.y,0,0};
		
		//Loop through the objects to check collision.
		for(unsigned int o=0; o<levelObjects.size(); o++){
			if(checkCollision(levelObjects[o]->getBox(),mouse)==true){
				//We have collision meaning that we are on hovering above an object.
				std::vector<GameObject*>::iterator it;
				it=find(selection.begin(),selection.end(),levelObjects[o]);
				
				//Check if the object is in the selection.
				if(it!=selection.end()){
					//It is so we delete it.
					selection.erase(it);
				}
				
				//Now we remove the object from the levelObjects.
				GameObject* obj=levelObjects[o];
				levelObjects.erase(levelObjects.begin()+o);
				delete obj;
				obj=NULL;
			}
		}
	    break;
	  }
	  default:
	    break;	    
	}	
}

void LevelEditor::onDrop(int x,int y){
	switch(tool){
	  case SELECT:
	  case ADD:
	  {
	      //Check if the drag center isn't null.
	      if(dragCenter==NULL) return;
	      //The location of the dragCenter.
	      SDL_Rect r=dragCenter->getBox();
	      //Apply snap to grid.
	      if(!pressedShift){
			x=(x/50)*50;
			y=(y/50)*50;
	      }else{
			x-=25;
			y-=25;
	      }

	      //Loop through the selection.
	      for(unsigned int o=0; o<selection.size(); o++){
			SDL_Rect r1=selection[o]->getBox();
			//We need to place the object at his drop place.
			selection[o]->setPosition((r1.x-r.x)+x,(r1.y-r.y)+y);
			//Check if the block outside the level size.
			if((r1.x-r.x)+x+50>LEVEL_WIDTH){
				LEVEL_WIDTH=(r1.x-r.x)+x+50;
			}
			if((r1.y-r.y)+y+50>LEVEL_HEIGHT){
				LEVEL_HEIGHT=(r1.y-r.y)+y+50;
			}
	      }
	      
	      //Make sure the dragCenter is null and set selectionDrag false.
	      dragCenter=NULL;
	      selectionDrag=false;
	      break;
	  }
	  default:
	    break;    
	}
}

void LevelEditor::onCameraMove(int dx,int dy){
	switch(tool){
	  case DELETE:
	  {
		//Only delete when the left mouse button is pressed.
		if(pressedLeftMouse){
			//Get the current mouse location.
			int x,y;
			SDL_GetMouseState(&x,&y);
			//Create the rectangle.
			SDL_Rect mouse={x+camera.x,y+camera.y,0,0};
		
			//Loop through the objects to check collision.
			for(unsigned int o=0; o<levelObjects.size(); o++){
				if(checkCollision(levelObjects[o]->getBox(),mouse)==true){
					//We have collision meaning that we are on hovering above an object.
					std::vector<GameObject*>::iterator it;
					it=find(selection.begin(),selection.end(),levelObjects[o]);
				
					//Check if the object is in the selection.
					if(it!=selection.end()){
						//It is so we delete it.
						selection.erase(it);
					}
				
					//Now we remove the object from the levelObjects.
					GameObject* obj=levelObjects[o];
					levelObjects.erase(levelObjects.begin()+o);
					delete obj;
					obj=NULL;
				}
			}
		}
	    break;
	  }
	  default:
	    break; 
	}
}


void LevelEditor::GUIEventCallback_OnEvent(std::string name,GUIObject* obj,int eventType){
	//Check for GUI events.
	//Notification block configure events.
	if(name=="cfgNotificationBlockOK"){
		if(GUIObjectRoot){
			//Set the message of the notification block.
			std::map<std::string,std::string> editorData;
			editorData["message"]=objectProperty->Caption;
			configuredObject->setEditorData(editorData);
			
			//And delete the GUI.
			objectProperty=NULL;
			configuredObject=NULL;
			delete GUIObjectRoot;
			GUIObjectRoot=NULL;
		}
	}
	if(name=="cfgNotificationBlockCancel"){
		if(GUIObjectRoot){
			//Delete the GUI.
			objectProperty=NULL;
			configuredObject=NULL;
			delete GUIObjectRoot;
			GUIObjectRoot=NULL;
		}
	}
	//Conveyor belt block configure events.
	if(name=="cfgConveyorBlockOK"){
		if(GUIObjectRoot){
			//Set the message of the notification block.
			std::map<std::string,std::string> editorData;
			editorData["speed"]=objectProperty->Caption;
			configuredObject->setEditorData(editorData);
			
			//And delete the GUI.
			objectProperty=NULL;
			configuredObject=NULL;
			delete GUIObjectRoot;
			GUIObjectRoot=NULL;
		}
	}
	if(name=="cfgConveyorBlockCancel"){
		if(GUIObjectRoot){
			//Delete the GUI.
			objectProperty=NULL;
			configuredObject=NULL;
			delete GUIObjectRoot;
			GUIObjectRoot=NULL;
		}
	}
}

////////////////LOGIC////////////////////
void LevelEditor::logic(){
	if(playMode){
		//PlayMode so let the game do it's logic.
		Game::logic();
	}else{
		//Move the camera.
		if(cameraXvel!=0 || cameraYvel!=0){
			camera.x+=cameraXvel;
			camera.y+=cameraYvel;
			//Call the onCameraMove event.
			onCameraMove(cameraXvel,cameraYvel);
		}		
		//Move the camera with the mouse.
		setCamera();
		
		//It isn't playMode so the mouse should be checked.
		if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button==SDL_BUTTON_LEFT){
			int x,y;
			SDL_GetMouseState(&x,&y);
			SDL_Rect mouse={x,y,0,0};
	
			//We loop through the number of tools + the number of buttons.
			for(int t=0; t<NUMBER_TOOLS+4; t++){
				SDL_Rect toolRect={205+(t*40)+(t*10),555,40,40};
		
				if(checkCollision(mouse,toolRect)==true){
					//Don't do this when it's higher than NUMBER_TOOLS.
					if(t<NUMBER_TOOLS){
						tool=(Tools)t;
					}else{
						//The selected button isn't a tool.
						//Now check which button it is.
						if(t==NUMBER_TOOLS){
							playMode=true;
							cameraSave.x=camera.x;
							cameraSave.y=camera.y;
						}
						if(t==NUMBER_TOOLS+2){
							string s=LevelName;
							if(fileDialog(s,"Save Level","map","%USER%/levels/",true,true)){
								saveLevel(processFileName(s));
							}
						}
						if(t==NUMBER_TOOLS+3){
							string s="";
							if(fileDialog(s,"Load Level","map","%USER%/levels/\nAddon levels\n%DATA%/levels/\nMain levels",false,true)){
								loadLevel(processFileName(s));
							}
						}
					}
				}
			}
		}
	}
}

/////////////////RENDER//////////////////////
void LevelEditor::render(){
	//Always let the game render the game.
	Game::render();
	
	//Only render extra stuff like the toolbar, selection, etc.. when not in playMode.
	if(!playMode){
		//Render the selectionmarks.
		//TODO: Check if block is in sight.
		for(unsigned int o=0; o<selection.size(); o++){
			//Get the location to draw.
			SDL_Rect r=selection[o]->getBox();
			r.x-=camera.x;
			r.y-=camera.y;
		
			//Draw the selectionMarks.
			applySurface(r.x,r.y,selectionMark,screen,NULL);
			applySurface(r.x+r.w-5,r.y,selectionMark,screen,NULL);
			applySurface(r.x,r.y+r.h-5,selectionMark,screen,NULL);
			applySurface(r.x+r.w-5,r.y+r.h-5,selectionMark,screen,NULL);
		}
		
		//Clear the placement surface.
		SDL_FillRect(placement,NULL,0x00FF00FF);
		
		//Draw the dark areas marking the outside of the level.
		SDL_Rect r;
		if(camera.x<0){
			//Draw left side.
			r.x=0;
			r.y=0;
			r.w=0-camera.x;
			r.h=600;
			SDL_FillRect(placement,&r,0);
		}
		if(camera.x>LEVEL_WIDTH-800){
			//Draw right side.
			r.x=LEVEL_WIDTH-camera.x;
			r.y=0;
			r.w=800-(LEVEL_WIDTH-camera.x);
			r.h=600;
			SDL_FillRect(placement,&r,0);  
		}
		if(camera.y<0){
			//Draw the top.
			r.x=0;
			r.y=0;
			r.w=800;
			r.h=0-camera.y;
			SDL_FillRect(placement,&r,0);
		}
		if(camera.y>LEVEL_HEIGHT-600){
			//Draw the bottom.
			r.x=0;
			r.y=LEVEL_HEIGHT-camera.y;
			r.w=800;
			r.h=600-(LEVEL_HEIGHT-camera.y);
			SDL_FillRect(placement,&r,0);
		}
		
		//Check if we should draw on the placement surface.
		if(selectionDrag){
			showSelectionDrag();
		}else{
			if(tool==ADD){
				showCurrentObject();
			}
		}
		
		//Draw the level borders.
		drawRect(-camera.x,-camera.y,LEVEL_WIDTH,LEVEL_HEIGHT);
		
		//Render the placement surface.
		applySurface(0,0,placement,screen,NULL);
		  
		//On top of all render the toolbar.
		applySurface(195,550,toolbar,screen,NULL);
	
		//Draw a rectangle around the current tool.
		drawRect(205+(tool*40)+(tool*10),555,40,40);
	}
}

void LevelEditor::showCurrentObject(){
	//Get the current mouse location.
	int x,y;
	SDL_GetMouseState(&x,&y);
	//Create the rectangle.
	SDL_Rect mouse={x+camera.x,y+camera.y,0,0};

	//Check if we should snap the block to grid or not.
	if(!pressedShift){
		mouse.x=(mouse.x/50)*50;
		mouse.y=(mouse.y/50)*50;
	}else{
		mouse.x-=25;
		mouse.y-=25;
	}

	//Check if the currentType is a legal type.
	if(currentType>=0 && currentType<TYPE_MAX){
		ThemeBlock* obj=objThemes.getBlock(currentType);
		if(obj){
			obj->editorPicture.draw(placement,mouse.x-camera.x,mouse.y-camera.y);
		}
	}
}

void LevelEditor::showSelectionDrag(){
	//Get the current mouse location.
	int x,y;
	SDL_GetMouseState(&x,&y);
	//Create the rectangle.
	SDL_Rect mouse={x+camera.x,y+camera.y,0,0};

	//Check if we should snap the block to grid or not.
	if(!pressedShift){
		mouse.x=(mouse.x/50)*50;
		mouse.y=(mouse.y/50)*50;
	}else{
		mouse.x-=25;
		mouse.y-=25;
	}

	//Check if the drag center isn't null.
	if(dragCenter==NULL) return;
	//The location of the dragCenter.
	SDL_Rect r=dragCenter->getBox();
	
	//Loop through the selection.
	//TODO: Check if block is in sight.
	for(unsigned int o=0; o<selection.size(); o++){
		ThemeBlock* obj=objThemes.getBlock(selection[o]->type);
		if(obj){
			SDL_Rect r1=selection[o]->getBox();
			obj->editorPicture.draw(placement,(r1.x-r.x)+mouse.x-camera.x,(r1.y-r.y)+mouse.y-camera.y);
		}
	}
}