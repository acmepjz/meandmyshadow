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
#include "GUITextArea.h"
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

////////////////LEVEL PACK EDITOR////////////////////
class LevelPackEditor:public GUIEventCallback{
private:
	//The fileName of the levelpack file.
	string fileName;
	//Textbox for the description of the levelpack.
	GUIObject* txtLvPackName;
	//Listbox containing the levels.
	GUIListBox* lstLvPack;
	//The levelpack.
	Levels objLvPack;
	
	//Pointer to the textfield of the congratulationText configure popup.
	GUIObject* congratulationTextBox;
private:
	void updateListBox(){
		//First clear the list.
		lstLvPack->item.clear();
		
		//Now loop the levels
		for(int i=0;i<objLvPack.getLevelCount();i++){
			char s[32];
			sprintf(s,"%d.",i+1);
			lstLvPack->item.push_back(s+objLvPack.getLevelName(i)+"("+objLvPack.getLevelFile(i)+")");
		}
	}
	
	void addLevel(const string& s){
		//Prepare to load the level.
		TreeStorageNode obj;
		POASerializer objSerializer;
		
		//Parse the level file.
		if(objSerializer.loadNodeFromFile(processFileName(s).c_str(),&obj,true)){
			//Get the name of 
			string name;
			vector<string>& v=obj.attributes["name"];
			
			//Make sure that there's a name.
			if(v.size()>0)
				name=v[0];
			
			//And add the level to the levelpack.
			objLvPack.addLevel(s,name,lstLvPack->value);
			//Now update the list.
			updateListBox();
		}
	}
	
	void updateLevel(int lvl){
		TreeStorageNode obj;
		POASerializer objSerializer;
		if(objSerializer.loadNodeFromFile(processFileName(objLvPack.getLevelFile(lvl)).c_str(),&obj,true)){
			string name;
			vector<string>& v=obj.attributes["name"];
			if(v.size()>0) name=v[0];
			if(!name.empty()) objLvPack.setLevelName(lvl,name);
		}
	}
	
	void congratulationText(){
		//Pointer to the current GUIObjectRoot.
		//We keep it so we can put it back after closing the fileDialog.
		GUIObject* tmp=GUIObjectRoot;
	
		GUIObjectRoot=new GUIObject(100,(SCREEN_HEIGHT-200)/2,600,200,GUIObjectFrame,"Congratulations");
		GUIObject* obj;
		
		//NOTE: We reuse the objectProperty and secondProperty.
		obj=new GUIObject(40,40,240,36,GUIObjectLabel,"Text");
		GUIObjectRoot->childControls.push_back(obj);
		obj=new GUIObject(140,40,350,36,GUIObjectTextBox,objLvPack.congratulationText.c_str());
		congratulationTextBox=obj;
		GUIObjectRoot->childControls.push_back(obj);
		
		//Ok and cancel buttons.
		obj=new GUIObject(100,200-44,150,36,GUIObjectButton,"OK");
		obj->name="cmdCongratOK";
		obj->eventCallback=this;
		GUIObjectRoot->childControls.push_back(obj);
		obj=new GUIObject(350,200-44,150,36,GUIObjectButton,"Cancel");
		obj->name="cmdCongratCancel";
		obj->eventCallback=this;
		GUIObjectRoot->childControls.push_back(obj);
		
		//Let the currentState render once to prevent multiple GUI overlapping and prevent the screen from going black.
		currentState->render();
		
		//Now we keep rendering and updating the GUI.
		SDL_FillRect(tempSurface,NULL,0);
		SDL_SetAlpha(tempSurface,SDL_SRCALPHA,155);
		SDL_BlitSurface(tempSurface,NULL,screen,NULL);
		while(GUIObjectRoot){
			while(SDL_PollEvent(&event)) 
				GUIObjectHandleEvents(true);
			if(GUIObjectRoot)
				GUIObjectRoot->render();
			SDL_Flip(screen);
			SDL_Delay(30);
		}
		
		//Now set back the old GUI.
		GUIObjectRoot=tmp;
	}
public:
	//Constructor.
	LevelPackEditor(){}
	
	void show(){
		GUIObject* obj;
		GUIObject* tmp=GUIObjectRoot;
		
		//===
		GUIObjectRoot=new GUIObject(50,50,700,500,GUIObjectFrame,"Level Pack Editor");
		GUIObjectRoot->childControls.push_back(new GUIObject(8,20,184,36,GUIObjectLabel,"Level Pack Name"));
		txtLvPackName=new GUIObject(200,20,492,36,GUIObjectTextBox,"Untitled Level Pack");
		GUIObjectRoot->childControls.push_back(txtLvPackName);
		
		//The add level button.
		obj=new GUIObject(8,60,192,36,GUIObjectButton,"Add Level");
		obj->name="cmdAdd";
		obj->eventCallback=this;
		GUIObjectRoot->childControls.push_back(obj);
		//The remove level button.
		obj=new GUIObject(208,60,192,36,GUIObjectButton,"Remove Level");
		obj->name="cmdRemove";
		obj->eventCallback=this;
		GUIObjectRoot->childControls.push_back(obj);
		//The congratulation text.
		obj=new GUIObject(408,60,240,36,GUIObjectButton,"Congratulations Text");
		obj->name="cmdCongratulations";
		obj->eventCallback=this;
		GUIObjectRoot->childControls.push_back(obj);
		//The move up button.
		obj=new GUIObject(8,100,192,36,GUIObjectButton,"Move Up");
		obj->name="cmdMoveUp";
		obj->eventCallback=this;
		GUIObjectRoot->childControls.push_back(obj);
		//The move down button.
		obj=new GUIObject(208,100,192,36,GUIObjectButton,"Move Down");
		obj->name="cmdMoveDown";
		obj->eventCallback=this;
		GUIObjectRoot->childControls.push_back(obj);
		//The update level names button.
		obj=new GUIObject(408,100,240,36,GUIObjectButton,"Update Level Names");
		obj->name="cmdUpdate";
		obj->eventCallback=this;
		GUIObjectRoot->childControls.push_back(obj);
		
		//The levelpack list.
		lstLvPack=new GUIListBox(8,140,684,316);
		lstLvPack->name="lstLvPack";
		lstLvPack->eventCallback=this;
		GUIObjectRoot->childControls.push_back(lstLvPack);
		
		//The load levelpack button.
		obj=new GUIObject(8,460,192,36,GUIObjectButton,"Load Level Pack");
		obj->name="cmdLoad";
		obj->eventCallback=this;
		GUIObjectRoot->childControls.push_back(obj);
		//The save levelpack button.
		obj=new GUIObject(208,460,192,36,GUIObjectButton,"Save Level Pack");
		obj->name="cmdSave";
		obj->eventCallback=this;
		GUIObjectRoot->childControls.push_back(obj);
		
		//The exit button.
		obj=new GUIObject(564,460,128,36,GUIObjectButton,"Exit");
		obj->name="cmdExit";
		obj->eventCallback=this;
		GUIObjectRoot->childControls.push_back(obj);
		
		//GUI has been created.
		//Now dim the screen and keep rendering/updating the gui.
		SDL_FillRect(tempSurface,NULL,0);
		SDL_SetAlpha(tempSurface,SDL_SRCALPHA,155);
		SDL_BlitSurface(tempSurface,NULL,screen,NULL);
		while(GUIObjectRoot){
			while(SDL_PollEvent(&event))
				GUIObjectHandleEvents();
			if(GUIObjectRoot)
				GUIObjectRoot->render();
			SDL_Flip(screen);
			SDL_Delay(30);
		}
		//Set the old GUI back.
		GUIObjectRoot=tmp;
		
		//Done.
		return;
	}
	
	void GUIEventCallback_OnEvent(std::string name,GUIObject* obj,int eventType){
		if(name=="cmdExit"){
			//Delete the GUI.
			if(GUIObjectRoot){
				delete GUIObjectRoot;
				GUIObjectRoot=NULL;
			}
		}else if(name=="cmdLoad"){
			//Let the currentState render once to prevent multiple GUI overlapping and prevent the screen from going black.
			currentState->render();
			
			//Show the fileDialog.
			string s=fileName;
			if(fileDialog(s,"Load Level Pack","","%USER%/custom/levelpacks/\nMy levelpacks\n%USER%/levelpacks/\nAddon levelpacks\n%DATA%/levelpacks/\nMain levelpacks",false,true,false)){
				if(!objLvPack.loadLevels(processFileName(s+"/levels.lst"),"")){
					msgBox("Can't load level pack:\n"+s,MsgBoxOKOnly,"Error");
					s="";
				}
				txtLvPackName->caption=objLvPack.levelpackDescription;
				lstLvPack->value=-1;
				updateListBox();
				fileName=s;
			}
		}else if(name=="cmdSave"){
			//Let the currentState render once to prevent multiple GUI overlapping and prevent the screen from going black.
			currentState->render();
			
			//Show the fileDialog.
			string s=fileName;
			if(fileDialog(s,"Save Level Pack","","%USER%/custom/levelpacks/",true,true,false)){
				objLvPack.levelpackDescription=txtLvPackName->caption;
				createDirectory(processFileName(s).c_str());
				
				objLvPack.saveLevels(s+"/levels.lst");
				fileName=s+"/levels.lst";
			}
		}else if(name=="cmdAdd"){
			//Let the currentState render once to prevent multiple GUI overlapping and prevent the screen from going black.
			currentState->render();
			
			//Show the fileDialog.
			string s;
			if(fileDialog(s,"Load Level","map","%USER%/custom/levels/\nMy levels\n%USER%/levels/\nAddon levels\n%DATA%/levels/\nMain levels",false,true))
				addLevel(s);
		}else if(name=="cmdCongratulations"){
			//Show the congratulationText edit popup.
			congratulationText();
		}else if(name=="cmdMoveUp"){
			//Get the current location.
			int i=lstLvPack->value;
			
			//Check if it can move up.
			if(i>0&&i<objLvPack.getLevelCount()){
				//Swap the two levels.
				objLvPack.swapLevel(i,i-1);
				//Change the selected item to the correct one.
				lstLvPack->value=i-1;
				//Update the list.
				updateListBox();
			}
		}else if(name=="cmdMoveDown"){
			//Get the current location.
			int i=lstLvPack->value;
			
			//Check if it can move up.
			if(i>=0&&i<objLvPack.getLevelCount()-1){
				//Swap the two levels.
				objLvPack.swapLevel(i,i+1);
				//Change the selected item to the correct one.
				lstLvPack->value=i+1;
				//Update the list.
				updateListBox();
			}
		}else if(name=="cmdRemove"){
			//Get the current location.
			int i=lstLvPack->value;
			
			//Check if it exists.
			if(i>=0&&i<objLvPack.getLevelCount()){
				//Remove it and update the list.
				objLvPack.removeLevel(i);
				updateListBox();
			}
		}else if(name=="cmdUpdate"){
			//Loop through the levels and update them.
			for(int i=0;i<objLvPack.getLevelCount();i++)
				updateLevel(i);
			//Let the currentState render once to prevent multiple GUI overlapping and prevent the screen from going black.
			currentState->render();
			
			//Show the user that it has been done.
			msgBox("OK!",MsgBoxOKOnly,"");
			//Update the list.
			updateListBox();
		}else if(name=="cmdCongratOK"){
			//Congratulation text configure menu, ok button.
			//Set the text.
			objLvPack.congratulationText=congratulationTextBox->caption;
			congratulationTextBox=NULL;
			
			//And delete the GUI.
			if(GUIObjectRoot){
				delete GUIObjectRoot;
				GUIObjectRoot=NULL;
			}
		}else if(name=="cmdCongratCancel"){
			//Delete the GUI.
			if(GUIObjectRoot){
				delete GUIObjectRoot;
				GUIObjectRoot=NULL;
			}
		}
		
	}
};

/////////////////MovingPosition////////////////////////////
MovingPosition::MovingPosition(int x,int y,int time){
	this->x=x;
	this->y=y;
	this->time=time;
}

MovingPosition::~MovingPosition(){}

void MovingPosition::updatePosition(int x,int y){
	this->x=x;
	this->y=y;
}


/////////////////LEVEL EDITOR//////////////////////////////
LevelEditor::LevelEditor():Game(false){
	LEVEL_WIDTH=800;
	LEVEL_HEIGHT=600;
	
	
	//Load an empty level.
	loadLevel(getDataPath()+"misc/Empty.map");
	
	//This will set some default settings.
	reset();
	
	//Load the toolbar.
	toolbar=loadImage(getDataPath()+"gfx/menu/toolbar.png");
	SDL_Rect tmp={155,555,510,50};
	toolbarRect=tmp;
	
	//Load the selectionMark.
	selectionMark=loadImage(getDataPath()+"gfx/menu/selection.png");
	
	//Load the movingMark.
	movingMark=loadImage(getDataPath()+"gfx/menu/moving.png");
	
	//Create the semi transparent surface.
	placement=SDL_CreateRGBSurface(SDL_SWSURFACE|SDL_SRCALPHA,800,600,32,0x000000FF,0x0000FF00,0x00FF0000,0);
	SDL_SetColorKey(placement,SDL_SRCCOLORKEY|SDL_RLEACCEL,SDL_MapRGB(placement->format,255,0,255));
	SDL_SetAlpha(placement,SDL_SRCALPHA,125);
}

LevelEditor::~LevelEditor(){
	//Loop through the levelObjects and delete them.
	for(unsigned int i=0;i<levelObjects.size();i++)
		delete levelObjects[i];
	levelObjects.clear();
	selection.clear();
	
	//Free the placement surface.
	SDL_FreeSurface(placement);
	
	//Reset the camera.
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
	secondObjectProperty=NULL;
	configuredObject=NULL;
	linking=false;
	linkingTrigger=NULL;
	currentId=0;
	movingBlock=NULL;
	moving=false;
	movingSpeed=10;
	levelName="";
	levelFile="";
	levelTheme="";
	tooltip=-1;
	
	//Set the player and shadow in the top left corner.
	player.setPosition(0,0);
	shadow.setPosition(0,0);
	
	selection.clear();
	clipboard.clear();
	triggers.clear();
	movingBlocks.clear();
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

	//The name of the level.
	if(!levelName.empty())
		node.attributes["name"].push_back(levelName);
	
	//The leveltheme.
	if(!levelTheme.empty())
		node.attributes["theme"].push_back(levelTheme);
	
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
			obj1->value.push_back(blockName[objectType]);

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
			Game::reset(true);
			playMode=false;
			camera.x=cameraSave.x;
			camera.y=cameraSave.y;	
		}
	}else{
		//Also check if we should exit the editor.
		if(event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_ESCAPE){
			//Before we quit ask a make sure question.
			if(msgBox("Are you sure you want to quit?",MsgBoxYesNo,"Quit prompt")==MsgBoxYes){
				//We exit the level editor.
				if(GUIObjectRoot){
					delete GUIObjectRoot;
					GUIObjectRoot=NULL;
				}
				setNextState(STATE_MENU);
			}
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
				 while(!selection.empty()){
					//Remove the objects in the selection.
					removeObject(selection[0]);
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
					
					//Overwrite the id to prevent triggers, portals, buttons, movingblocks, etc. from malfunctioning.
					//We give an empty string as id, which is invalid and thus suitable.
					objMap["id"]="";
					//Do the same for destination if the type is portal.
					if(selection[o]->type==TYPE_PORTAL){
						objMap["destination"]="";
					}
					
					//And add the map to the clipboard vector.
					clipboard.push_back(objMap);
				
					if(event.key.keysym.sym==SDLK_x){
						//Cutting means deleting the game object.
						removeObject(selection[o]);
						o--;
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
				x+=camera.x;
				y+=camera.y;
				
				//Apply snap to grid.
				if(!pressedShift){
					snapToGrid(&x,&y);
				}else{
					x-=25;
					y-=25;
				}
				
				//Integers containing the diff of the x that occurs when placing a block outside the level size on the top or left.
				//We use it to compensate the corrupted x and y locations of the other clipboard blocks.
				int diffX=0;
				int diffY=0;
				
				
				//Loop through the clipboard.
				for(unsigned int o=0;o<clipboard.size();o++){
					Block* block=new Block(0,0,atoi(clipboard[o]["type"].c_str()),this);
					block->setPosition(atoi(clipboard[o]["x"].c_str())+x+diffX,atoi(clipboard[o]["y"].c_str())+y+diffY);
					block->setEditorData(clipboard[o]);
					
					if(block->getBox().x<0){
						//A block on the left side of the level, meaning we need to shift everything.
						//First calc the difference.
						diffX+=(0-(block->getBox().x));
					}
					if(block->getBox().y<0){
						//A block on the left side of the level, meaning we need to shift everything.
						//First calc the difference.
						diffY+=(0-(block->getBox().y));
					}
					
					//And add the object using the addObject method.
					addObject(block);
					
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
					//Invoke the onEnterObject.
					onEnterObject(levelObjects[o]);
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
			//Only change the current type when using the add tool.
			if(tool==ADD){
				currentType++;
				if(currentType>=EDITOR_ORDER_MAX){
					currentType=0;
				}
			}
			//When in configure mode.
			if(tool==CONFIGURE){
				movingSpeed++;
				//The movingspeed is capped at 100.
				if(movingSpeed>100){
					movingSpeed=100;
				}
			}
		}
		//Check if we scroll down, meaning the currentType--;
		if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button==SDL_BUTTON_WHEELDOWN){
			//Only change the current type when using the add tool.
			if(tool==ADD){
				currentType--;
				if(currentType<0){
					currentType=EDITOR_ORDER_MAX-1;
				}
			}
			//When in configure mode.
			if(tool==CONFIGURE){
				movingSpeed--;
				if(movingSpeed<=0){
					movingSpeed=1;
				}
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
			//We clear the selection since that can't be used in the deletion tool.
			selection.clear();
			tool=REMOVE;
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
		
		//First make sure the mouse isn't above the toolbar.
		if(checkCollision(mouse,toolbarRect)==false){
			//We didn't hit the toolbar so convert the mouse location to ingame location.
			mouse.x+=camera.x;
			mouse.y+=camera.y;
			
			//Boolean if there's a click event fired.
			bool clickEvent=false;
			//Check if a mouse button is pressed.
			if(event.type==SDL_MOUSEBUTTONDOWN){
				//Loop through the objects to check collision.
				for(unsigned int o=0; o<levelObjects.size(); o++){
					if(checkCollision(levelObjects[o]->getBox(),mouse)==true){
						//We have collision meaning that the mouse is above an object.
						std::vector<GameObject*>::iterator it;
						it=find(selection.begin(),selection.end(),levelObjects[o]);
						
						//Set event true since there's a click event.
						clickEvent=true;
						
						//Check if the clicked object is in the selection or not.
						if(it!=selection.end()){
							if(event.button.button==SDL_BUTTON_LEFT){
								onClickObject(levelObjects[o],true);
							}else if(event.button.button==SDL_BUTTON_RIGHT){
								onRightClickObject(levelObjects[o],true);
							}
						}else{
							if(event.button.button==SDL_BUTTON_LEFT){
								onClickObject(levelObjects[o],false);
							}else if(event.button.button==SDL_BUTTON_RIGHT){
								onRightClickObject(levelObjects[o],false);
							}
						}
					}					
				}
			}
			
			//If event is false then we clicked on void.
			if(!clickEvent){
				if(event.type==SDL_MOUSEBUTTONDOWN){
					if(event.button.button==SDL_BUTTON_LEFT){
						//Left mouse button on void.
						onClickVoid(mouse.x,mouse.y);
					}else if(event.button.button==SDL_BUTTON_RIGHT && tool==CONFIGURE){
						//Stop linking.
						linking=false;
						linkingTrigger=NULL;
						
						//Write the path to the moving block.
						if(moving){
							std::map<std::string,std::string> editorData;
							char s[64], s0[64];
						
							sprintf(s,"%d",movingBlocks[movingBlock].size());
							editorData["MovingPosCount"]=s;
							//Loop through the positions.
							for(unsigned int o=0;o<movingBlocks[movingBlock].size();o++){
								sprintf(s0+1,"%d",o);
								sprintf(s,"%d",movingBlocks[movingBlock][o].x);
								s0[0]='x';
								editorData[s0]=s;
								sprintf(s,"%d",movingBlocks[movingBlock][o].y);
								s0[0]='y';
								editorData[s0]=s;
								sprintf(s,"%d",movingBlocks[movingBlock][o].time);
								s0[0]='t';
								editorData[s0]=s;
							}
							movingBlock->setEditorData(editorData);
						
							//Stop moving.
							moving=false;
							movingBlock=NULL;
						}
					}
				}
			}
		}
		
		//Check for backspace when moving to remove a movingposition.
		if(moving && event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_BACKSPACE){
			if(movingBlocks[movingBlock].size()>0){
				movingBlocks[movingBlock].pop_back();
			}
		}
		
		//Check for the tab key, level settings.
		if(event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_TAB){
			//Show the levelSettings.
			levelSettings();
		}
		
		//Check if we should a new level. (Ctrl+n)
		if(event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_n && (event.key.keysym.mod & KMOD_CTRL)){
			reset();
			loadLevel(getDataPath()+"misc/Empty.map");
		}
		//Check if we should load a level. (Ctrl+o)
		if(event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_o && (event.key.keysym.mod & KMOD_CTRL)){
			string s="";
			if(fileDialog(s,"Load Level","map","%USER%/custom/levels/\nMy levels\n%USER%/levels/\nAddon levels\n%DATA%/levels/\nMain levels",false,true)){
				reset();
				loadLevel(processFileName(s));
				postLoad();
			}
		}
		//Check if we should save the level (Ctrl+s) or save levelpack (Ctrl+Shift+s).
		if(event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_s && (event.key.keysym.mod & KMOD_CTRL)){
			//Check if shift was pressed or not.
			if(event.key.keysym.mod & KMOD_SHIFT){
				//Levelpack save.
				LevelPackEditor objEditor;
				objEditor.show();
			}else{
				//Normal save, open the the filedialog.
				string s=fileNameFromPath(levelFile);
				if(fileDialog(s,"Save Level","map","%USER%/custom/levels/",true,true)){
					saveLevel(processFileName(s));
					levelFile=processFileName(s);
				}
			}
		}
	}
}

void LevelEditor::levelSettings(){
	//It isn't so open a popup asking for a name.
	//First delete any existing gui.
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}
	
	GUIObjectRoot=new GUIObject(100,(SCREEN_HEIGHT-200)/2,600,200,GUIObjectFrame,"Level settings");
	GUIObject* obj;
	
	//NOTE: We reuse the objectProperty and secondProperty.
	obj=new GUIObject(40,40,240,36,GUIObjectLabel,"Name:");
	GUIObjectRoot->childControls.push_back(obj);
	obj=new GUIObject(140,40,350,36,GUIObjectTextBox,levelName.c_str());
	objectProperty=obj;
	GUIObjectRoot->childControls.push_back(obj);
	
	obj=new GUIObject(40,90,240,36,GUIObjectLabel,"Theme:");
	GUIObjectRoot->childControls.push_back(obj);
	obj=new GUIObject(140,90,350,36,GUIObjectTextBox,"");
	secondObjectProperty=obj;
	GUIObjectRoot->childControls.push_back(obj);
	
	//Ok and cancel buttons.
	obj=new GUIObject(100,200-44,150,36,GUIObjectButton,"OK");
	obj->name="lvlSettingsOK";
	obj->eventCallback=this;
	GUIObjectRoot->childControls.push_back(obj);
	obj=new GUIObject(350,200-44,150,36,GUIObjectButton,"Cancel");
	obj->name="lvlSettingsCancel";
	obj->eventCallback=this;
	GUIObjectRoot->childControls.push_back(obj);
	
	//Now we keep rendering and updating the GUI.
	SDL_FillRect(tempSurface,NULL,0);
	SDL_SetAlpha(tempSurface,SDL_SRCALPHA,155);
	SDL_BlitSurface(tempSurface,NULL,screen,NULL);
	while(GUIObjectRoot){
		while(SDL_PollEvent(&event)) 
			GUIObjectHandleEvents(true);
		if(GUIObjectRoot)
			GUIObjectRoot->render();
		SDL_Flip(screen);
		SDL_Delay(30);
	}
}

void LevelEditor::postLoad(){
	//We need to find the triggers.
	for(unsigned int o=0;o<levelObjects.size();o++){
		//Get the editor data.
		vector<pair<string,string> > objMap;
		levelObjects[o]->getEditorData(objMap);
		
		//Check for the highest id.
		for(unsigned int i=0;i<objMap.size();i++){
			if(objMap[i].first=="id"){
				unsigned int id=atoi(objMap[i].second.c_str());
				if(id>=currentId){
					currentId=id+1;
				}
			}
		}
		
		switch(levelObjects[o]->type){
			case TYPE_BUTTON:
			case TYPE_SWITCH:
			{
				//Add the object to the triggers vector.
				vector<GameObject*> linked;
				triggers[levelObjects[o]]=linked;
				//Now loop through the levelObjects in search for objects with the same id.
				for(unsigned int oo=0;oo<levelObjects.size();oo++){
					//Check if it isn't the same object but has the same id.
					if(o!=oo && (dynamic_cast<Block*>(levelObjects[o]))->id==(dynamic_cast<Block*>(levelObjects[oo]))->id){
						//Add the object to the link vector of the trigger.
						triggers[levelObjects[o]].push_back(levelObjects[oo]);
					}
				}
				break;
			}
			case TYPE_PORTAL:
			{
				//Add the object to the triggers vector.
				vector<GameObject*> linked;
				triggers[levelObjects[o]]=linked;
				
				//If the destination is empty we return.
				if((dynamic_cast<Block*>(levelObjects[o]))->destination.empty()){
					return;
				}
				
				//Now loop through the levelObjects in search for objects with the same id as destination.
				for(unsigned int oo=0;oo<levelObjects.size();oo++){
					//Check if it isn't the same object but has the same id.
					if(o!=oo && (dynamic_cast<Block*>(levelObjects[o]))->destination==(dynamic_cast<Block*>(levelObjects[oo]))->id){
						//Add the object to the link vector of the trigger.
						triggers[levelObjects[o]].push_back(levelObjects[oo]);
					}
				}
				break;
			}
			case TYPE_MOVING_BLOCK:
			case TYPE_MOVING_SHADOW_BLOCK:
			case TYPE_MOVING_SPIKES:
			{
				//Add the object to the movingBlocks vector.
				vector<MovingPosition> positions;
				movingBlocks[levelObjects[o]]=positions;
				
				//Get the number of entries of the editor data.
				int m=objMap.size();
				
				//Check if the editor data isn't empty.
				if(m>0){
					//Integer containing the positions.
					int pos=0;
					int currentPos=0;
					
					//Get the number of movingpositions.
					pos=atoi(objMap[1].second.c_str());
					
					while(currentPos<pos){
						int x=atoi(objMap[currentPos*3+4].second.c_str());
						int y=atoi(objMap[currentPos*3+5].second.c_str());
						int t=atoi(objMap[currentPos*3+6].second.c_str());
						
						//Create a new movingPosition.
						MovingPosition position(x,y,t);
						movingBlocks[levelObjects[o]].push_back(position);
						
						//Increase currentPos by one.
						currentPos++;
					}
				}
				
				break;
			}
			default:
			  break;
		}
	}
}

void LevelEditor::snapToGrid(int* x,int* y){
	//Check if the x location is negative.
	if(*x<0){
		*x=-((abs(*x-50)/50)*50);
	}else{
		*x=(*x/50)*50;
	}
	
	//Now the y location.
	if(*y<0){
		*y=-((abs(*y-50)/50)*50);
	}else{
		*y=(*y/50)*50;
	}
}

void LevelEditor::onClickObject(GameObject* obj,bool selected){
	switch(tool){
	  //NOTE: We put CONFIGURE above ADD and SELECT to use the same method of selection.
	  //Meaning there's no break at the end of CONFIGURE.
	  case CONFIGURE:
	  {
	    //Check if we are linking.
	    if(linking){
			//Check if the obj is valid to link to.
			switch(obj->type){
				case TYPE_CONVEYOR_BELT:
				case TYPE_SHADOW_CONVEYOR_BELT:
				case TYPE_MOVING_BLOCK:
				case TYPE_MOVING_SHADOW_BLOCK:
				case TYPE_MOVING_SPIKES:
				{
					//It's only valid when not linking a portal.
					if(linkingTrigger->type==TYPE_PORTAL){
						//You can't link a portal to moving blocks, etc.
						//Stop linking and return.
						linkingTrigger=NULL;
						linking=false;
						return;
					}
					break;
				}
				case TYPE_PORTAL:
				{
					//Make sure that the linkingTrigger is also a portal.
					if(linkingTrigger->type!=TYPE_PORTAL){
						//The linkingTrigger isn't a portal so stop linking and return.
						linkingTrigger=NULL;
						linking=false;
						return;
					}
					break;
				}
				default:
					//It isn't valid so stop linking and return.
					linkingTrigger=NULL;
					linking=false;
					return;
				break;
			}
			
			//Check if the linkingTrigger can handle multiple or only one link.
			switch(linkingTrigger->type){
				case TYPE_PORTAL:
				{
					//Portals can only link to one so remove all existing links.
					triggers[linkingTrigger].clear();
					triggers[linkingTrigger].push_back(obj);
					break;
				}
				default:
				{
					//The most can handle multiple links.
					triggers[linkingTrigger].push_back(obj);
					break;
				}
			}
			
			//Check if it's a portal.
			if(linkingTrigger->type==TYPE_PORTAL){
				//Portals need to get the id of the other instead of give it's own id.
				vector<pair<string,string> > objMap;
				obj->getEditorData(objMap);
				int m=objMap.size();
				if(m>0){
					std::map<std::string,std::string> editorData;
					char s[64];
					sprintf(s,"%d",atoi(objMap[0].second.c_str()));
					editorData["destination"]=s;
					linkingTrigger->setEditorData(editorData);
				}
			}else{
				//Give the object the same id as the trigger.
				vector<pair<string,string> > objMap;
				linkingTrigger->getEditorData(objMap);
				int m=objMap.size();
				if(m>0){
					std::map<std::string,std::string> editorData;
					char s[64];
					sprintf(s,"%d",atoi(objMap[0].second.c_str()));
					editorData["id"]=s;
					obj->setEditorData(editorData);
				}
			}

			
			//We return to prevent configuring stuff like conveyor belts, etc...
			linking=false;
			linkingTrigger=NULL;
			return;
	    }
	    
	    //If we're moving add a movingposition.
	    if(moving){
			//Get the current mouse location.
			int x,y;
			SDL_GetMouseState(&x,&y);
			x+=camera.x;
			y+=camera.y;
			
			//Apply snap to grid.
			if(!pressedShift){
				snapToGrid(&x,&y);
			}else{
				x-=25;
				y-=25;
			}
			
			x-=movingBlock->getBox().x;
			y-=movingBlock->getBox().y;
			
			//Calculate the length.
			//First get the delta x and y.
			int dx,dy;
			if(movingBlocks[movingBlock].empty()){
				dx=x;
				dy=y;
			}else{
				dx=x-movingBlocks[movingBlock].back().x;
				dy=y-movingBlocks[movingBlock].back().y;
			}
			
			double length=sqrt(double(dx*dx+dy*dy));
			movingBlocks[movingBlock].push_back(MovingPosition(x,y,(int)(length*(10/(double)movingSpeed))));
	    }
	  }
	  case SELECT:
	  case ADD:
	  {
		//Check if object is already selected.
		if(!selected){
			//First check if shift is pressed or not.
			if(!pressedShift){
				//Clear the selection.
				selection.clear();
			}
		
			//Add the object to the selection.
			selection.push_back(obj);
		}
	    break;
	  }
	  case REMOVE:
	  {
	    //Remove the object.
	    removeObject(obj);
	    break;
	  }
	  default:
	    break;	    
	}
}

void LevelEditor::onRightClickObject(GameObject* obj,bool selected){
	switch(tool){
	  case CONFIGURE:
	  {
		//Make sure we aren't doing anything special.
		if(moving || linking)
			break;

		//Check if it's a trigger.
		if(obj->type==TYPE_PORTAL || obj->type==TYPE_BUTTON || obj->type==TYPE_SWITCH){
			//Set linking true.
			linking=true;
			linkingTrigger=obj;
		}
		
		//Check if it's a moving block.
		if(obj->type==TYPE_MOVING_BLOCK || obj->type==TYPE_MOVING_SHADOW_BLOCK || obj->type==TYPE_MOVING_SPIKES){
			//Set moving true.
			moving=true;
			movingBlock=obj;
		}
		break;
	  }
	  case SELECT:
	  case ADD:
	  {
		//We deselect the object if it's selected.
		if(selected){
			std::vector<GameObject*>::iterator it;
			it=find(selection.begin(),selection.end(),obj);
			
			//Remove the object from selection.
			if(it!=selection.end()){
				selection.erase(it);
			}
		}else{
			//It wasn't a selected object so switch to configure mode.
			//Check if it's the right type of object.
			if(obj->type==TYPE_MOVING_BLOCK || obj->type==TYPE_MOVING_SHADOW_BLOCK || obj->type==TYPE_MOVING_SPIKES || 
				obj->type==TYPE_PORTAL || obj->type==TYPE_BUTTON || obj->type==TYPE_SWITCH){
				tool=CONFIGURE;
				onRightClickObject(obj,selected);
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
			snapToGrid(&x,&y);
	      }else{
			x-=25;
			y-=25;
	      }
	      addObject(new Block(x,y,editorTileOrder[currentType],this));
	      break;
	  }
	  case CONFIGURE:
	  {
	      //We need to clear the selection.
	      selection.clear();
	      
	      //If we're linking we should stop, user abort.
	      if(linking){
			linking=false;
			linkingTrigger=NULL;
			//And return.
			return;
	      }
	      
	      //If we're moving we should add a point.
	      if(moving){
			//Apply snap to grid.
			if(!pressedShift){
				snapToGrid(&x,&y);
			}else{
				x-=25;
				y-=25;
			}
			
			x-=movingBlock->getBox().x;
			y-=movingBlock->getBox().y;
			
			//Calculate the length.
			//First get the delta x and y.
			int dx,dy;
			if(movingBlocks[movingBlock].empty()){
				dx=x;
				dy=y;
			}else{
				dx=x-movingBlocks[movingBlock].back().x;
				dy=y-movingBlocks[movingBlock].back().y;
			}
			
			double length=sqrt(double(dx*dx+dy*dy));
			movingBlocks[movingBlock].push_back(MovingPosition(x,y,(int)(length*(10/(double)movingSpeed))));
			
			//And return.
			return;
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
	  case CONFIGURE:
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
	  case REMOVE:
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
				//Remove the object.
				removeObject(levelObjects[o]);
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
	  case CONFIGURE:
	  {
	      //Check if the drag center isn't null.
	      if(dragCenter==NULL) return;
	      //The location of the dragCenter.
	      SDL_Rect r=dragCenter->getBox();
	      //Apply snap to grid.
	      if(!pressedShift){
			snapToGrid(&x,&y);
	      }else{
			x-=25;
			y-=25;
	      }

	      //Loop through the selection.
	      for(unsigned int o=0; o<selection.size(); o++){
			SDL_Rect r1=selection[o]->getBox();
			//We need to place the object at his drop place.
			moveObject(selection[o],(r1.x-r.x)+x,(r1.y-r.y)+y);
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
	  case REMOVE:
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
					//Remove the object.
					removeObject(levelObjects[o]);
				}
			}
		}
	    break;
	  }
	  default:
	    break; 
	}
}

void LevelEditor::onEnterObject(GameObject* obj){
	switch(tool){
	  case CONFIGURE:
	  {
	    //Check if the type is an moving block.
	    if(obj->type==TYPE_MOVING_BLOCK || obj->type==TYPE_MOVING_SHADOW_BLOCK || obj->type==TYPE_MOVING_SPIKES){
			//Open a message popup.
			//First delete any existing gui.
			if(GUIObjectRoot){
				delete GUIObjectRoot;
				GUIObjectRoot=NULL;
			}
			
			//Get the properties.
			vector<pair<string,string> > objMap;
			obj->getEditorData(objMap);
			int m=objMap.size();
			if(m>0){
				//Set the object we configure.
				configuredObject=obj;
				
				//Now create the GUI.
				string s;
				switch(obj->type){
				  case TYPE_MOVING_BLOCK:
					s="Moving block";
				    break;
				  case TYPE_MOVING_SHADOW_BLOCK:
					s="Moving shadow block";
				    break;
				  case TYPE_MOVING_SPIKES:
					s="Moving spikes";
				    break;

				}
				GUIObjectRoot=new GUIObject(100,(SCREEN_HEIGHT-200)/2,600,200,GUIObjectFrame,s.c_str());
				GUIObject* obj;
			
				obj=new GUIObject(40,40,240,36,GUIObjectCheckBox,"Enabled",(objMap[2].second!="1"));
				obj->name="cfgMovingBlockEnabled";
				obj->eventCallback=this;
				objectProperty=obj;
				GUIObjectRoot->childControls.push_back(obj);
				
				obj=new GUIObject(300,40,240,36,GUIObjectCheckBox,"Loop",(objMap[3].second!="0"));
				obj->name="cfgMovingBlockLoop";
				obj->eventCallback=this;
				secondObjectProperty=obj;
				GUIObjectRoot->childControls.push_back(obj);
				
				obj=new GUIObject(40,80,160,36,GUIObjectButton,"Clear path");
				obj->name="cfgMovingBlockClrPath";
				obj->eventCallback=this;
				GUIObjectRoot->childControls.push_back(obj);
				
				obj=new GUIObject(230,80,160,36,GUIObjectButton,"Make path");
				obj->name="cfgMovingBlockMakePath";
				obj->eventCallback=this;
				GUIObjectRoot->childControls.push_back(obj);
				
				obj=new GUIObject(100,200-44,150,36,GUIObjectButton,"OK");
				obj->name="cfgMovingBlockOK";
				obj->eventCallback=this;
				GUIObjectRoot->childControls.push_back(obj);
				obj=new GUIObject(350,200-44,150,36,GUIObjectButton,"Cancel");
				obj->name="cfgCancel";
				obj->eventCallback=this;
				GUIObjectRoot->childControls.push_back(obj);

				//Dim the screen using the tempSurface.
				SDL_FillRect(tempSurface,NULL,0);
				SDL_SetAlpha(tempSurface,SDL_SRCALPHA,155);
				SDL_BlitSurface(tempSurface,NULL,screen,NULL);
			
				while(GUIObjectRoot){
					while(SDL_PollEvent(&event)) GUIObjectHandleEvents(true);
					if(GUIObjectRoot) GUIObjectRoot->render();
					SDL_Flip(screen);
					SDL_Delay(30);
				}
			}
	    }
	    
	    //Check which type of object it is.
	    if(obj->type==TYPE_NOTIFICATION_BLOCK){
			//Open a message popup.
			//First delete any existing gui.
			if(GUIObjectRoot){
				delete GUIObjectRoot;
				GUIObjectRoot=NULL;
			}
			
			//Get the properties.
			vector<pair<string,string> > objMap;
			obj->getEditorData(objMap);
			int m=objMap.size();
			if(m>0){
				//Set the object we configure.
				configuredObject=obj;
				
				//Now create the GUI.
				GUIObjectRoot=new GUIObject(100,(SCREEN_HEIGHT-250)/2,600,250,GUIObjectFrame,"Notification block");
				GUIObject* obj;
			
				obj=new GUIObject(40,40,240,36,GUIObjectLabel,"Enter message here:");
				GUIObjectRoot->childControls.push_back(obj);
				obj=new GUITextArea(50,80,500,100);
				string tmp=objMap[1].second.c_str();
				//Change \n with the characters '\n'.
				while(tmp.find("\\n")!=string::npos){
					tmp=tmp.replace(tmp.find("\\n"),2,"\n");
				}
				obj->caption=tmp.c_str();
				//Set the textField.
				objectProperty=obj;
				GUIObjectRoot->childControls.push_back(obj);
			
				obj=new GUIObject(100,250-44,150,36,GUIObjectButton,"OK");
				obj->name="cfgNotificationBlockOK";
				obj->eventCallback=this;
				GUIObjectRoot->childControls.push_back(obj);
				obj=new GUIObject(350,250-44,150,36,GUIObjectButton,"Cancel");
				obj->name="cfgCancel";
				obj->eventCallback=this;
				GUIObjectRoot->childControls.push_back(obj);

				//Dim the screen using the tempSurface.
				SDL_FillRect(tempSurface,NULL,0);
				SDL_SetAlpha(tempSurface,SDL_SRCALPHA,155);
				SDL_BlitSurface(tempSurface,NULL,screen,NULL);
			
				while(GUIObjectRoot){
					while(SDL_PollEvent(&event)) GUIObjectHandleEvents(true);
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
				  
				GUIObjectRoot=new GUIObject(100,(SCREEN_HEIGHT-200)/2,600,200,GUIObjectFrame,s.c_str());
				GUIObject* obj;
			
				obj=new GUIObject(40,40,240,36,GUIObjectCheckBox,"Enabled",(objMap[1].second!="1"));
				obj->name="cfgConveyorBlockEnabled";
				obj->eventCallback=this;
				objectProperty=obj;
				GUIObjectRoot->childControls.push_back(obj);

				obj=new GUIObject(40,70,240,36,GUIObjectLabel,"Enter speed here:");
				GUIObjectRoot->childControls.push_back(obj);
				obj=new GUIObject(200,110,352,36,GUIObjectTextBox,objMap[2].second.c_str());
				//Set the textField.
				secondObjectProperty=obj;
				GUIObjectRoot->childControls.push_back(obj);
			
				
				obj=new GUIObject(100,200-44,150,36,GUIObjectButton,"OK");
				obj->name="cfgConveyorBlockOK";
				obj->eventCallback=this;
				GUIObjectRoot->childControls.push_back(obj);
				obj=new GUIObject(350,200-44,150,36,GUIObjectButton,"Cancel");
				obj->name="cfgCancel";
				obj->eventCallback=this;
				GUIObjectRoot->childControls.push_back(obj);

				//Dim the screen using the tempSurface.
				SDL_FillRect(tempSurface,NULL,0);
				SDL_SetAlpha(tempSurface,SDL_SRCALPHA,155);
				SDL_BlitSurface(tempSurface,NULL,screen,NULL);
			
				while(GUIObjectRoot){
					while(SDL_PollEvent(&event)) GUIObjectHandleEvents(true);
					if(GUIObjectRoot) GUIObjectRoot->render();
					SDL_Flip(screen);
					SDL_Delay(30);
				}
			}
	    }
	    
	    if(obj->type==TYPE_PORTAL){
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
				GUIObjectRoot=new GUIObject(100,(SCREEN_HEIGHT-200)/2,600,200,GUIObjectFrame,"Portal");
				GUIObject* obj;
			
				obj=new GUIObject(40,40,240,36,GUIObjectCheckBox,"Automatic",(objMap[1].second=="1"));
				obj->name="cfgPortalAutomatic";
				obj->eventCallback=this;
				objectProperty=obj;
				GUIObjectRoot->childControls.push_back(obj);
				
				obj=new GUIObject(40,80,160,36,GUIObjectButton,"Select target");
				obj->name="cfgPortalLink";
				obj->eventCallback=this;
				GUIObjectRoot->childControls.push_back(obj);
				
				obj=new GUIObject(230,80,160,36,GUIObjectButton,"Remove target");
				obj->name="cfgPortalUnlink";
				obj->eventCallback=this;
				GUIObjectRoot->childControls.push_back(obj);

				obj=new GUIObject(100,200-44,150,36,GUIObjectButton,"OK");
				obj->name="cfgPortalOK";
				obj->eventCallback=this;
				GUIObjectRoot->childControls.push_back(obj);
				obj=new GUIObject(350,200-44,150,36,GUIObjectButton,"Cancel");
				obj->name="cfgCancel";
				obj->eventCallback=this;
				GUIObjectRoot->childControls.push_back(obj);
				
				//Dim the screen using the tempSurface.
				SDL_FillRect(tempSurface,NULL,0);
				SDL_SetAlpha(tempSurface,SDL_SRCALPHA,155);
				SDL_BlitSurface(tempSurface,NULL,screen,NULL);
			
				while(GUIObjectRoot){
					while(SDL_PollEvent(&event))
						GUIObjectHandleEvents(true);
					if(GUIObjectRoot)
						GUIObjectRoot->render();
					SDL_Flip(screen);
					SDL_Delay(30);
				}
			}
	    }
	    
	    if(obj->type==TYPE_BUTTON || obj->type==TYPE_SWITCH){
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
				if(obj->type==TYPE_BUTTON){
					s="Button";
				}else{
					s="Switch";
				}
				GUIObjectRoot=new GUIObject(100,(SCREEN_HEIGHT-200)/2,600,200,GUIObjectFrame,s.c_str());
				GUIObject* obj;
			
				obj=new GUIObject(40,40,240,36,GUIObjectLabel,"Behaviour");
				obj->name="cfgTriggerBehaviour";
				GUIObjectRoot->childControls.push_back(obj);
				
				obj=new GUISingleLineListBox(250,40,300,36);
				obj->name="lstBehaviour";
				vector<string> v;
				v.push_back("on");
				v.push_back("off");
				v.push_back("toggle");
				(dynamic_cast<GUISingleLineListBox*>(obj))->item=v;
				
				//Get the current behaviour.
				if(objMap[1].second=="on"){
					obj->value=0;
				}else if(objMap[1].second=="off"){
					obj->value=1;
				}else{
					//There's no need to check for the last one, since it's also the default.
					obj->value=2;
				}
				objectProperty=obj;
				GUIObjectRoot->childControls.push_back(obj);
				
				obj=new GUIObject(40,80,160,36,GUIObjectButton,"Select targets");
				obj->name="cfgTriggerLink";
				obj->eventCallback=this;
				GUIObjectRoot->childControls.push_back(obj);
				
				obj=new GUIObject(230,80,160,36,GUIObjectButton,"Remove targets");
				obj->name="cfgTriggerUnlink";
				obj->eventCallback=this;
				GUIObjectRoot->childControls.push_back(obj);

				obj=new GUIObject(100,200-44,150,36,GUIObjectButton,"OK");
				obj->name="cfgTriggerOK";
				obj->eventCallback=this;
				GUIObjectRoot->childControls.push_back(obj);
				obj=new GUIObject(350,200-44,150,36,GUIObjectButton,"Cancel");
				obj->name="cfgCancel";
				obj->eventCallback=this;
				GUIObjectRoot->childControls.push_back(obj);
				
				//Dim the screen using the tempSurface.
				SDL_FillRect(tempSurface,NULL,0);
				SDL_SetAlpha(tempSurface,SDL_SRCALPHA,155);
				SDL_BlitSurface(tempSurface,NULL,screen,NULL);
			
				while(GUIObjectRoot){
					while(SDL_PollEvent(&event))
						GUIObjectHandleEvents(true);
					if(GUIObjectRoot)
						GUIObjectRoot->render();
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

void LevelEditor::addObject(GameObject* obj){
	//If it's a player or shadow start then we need to remove the previous one.
	if(obj->type==TYPE_START_PLAYER || obj->type==TYPE_START_SHADOW){
		//Loop through the levelObjects.
		for(unsigned int o=0; o<levelObjects.size(); o++){
			//Check if the type is the same.
			if(levelObjects[o]->type==obj->type){
				removeObject(levelObjects[o]);
			}
		}
	}
  
	//Add it to the levelObjects.
	levelObjects.push_back(obj);
	
	//Check if the object is inside the level dimensions.
	if(obj->getBox().x+50>LEVEL_WIDTH){
		LEVEL_WIDTH=obj->getBox().x+50;
	}
	if(obj->getBox().y+50>LEVEL_HEIGHT){
		LEVEL_HEIGHT=obj->getBox().y+50;
	}
	if(obj->getBox().x<0){
		//A block on the left side of the level, meaning we need to shift everything.
		//First calc the difference.
		int diff=(0-(obj->getBox().x));
		
		for(unsigned int o=0; o<levelObjects.size(); o++){
			moveObject(levelObjects[o],levelObjects[o]->getBox().x+diff,levelObjects[o]->getBox().y);
		}
		  
		//The level grows with the difference, 0-(x+50).
		LEVEL_WIDTH+=diff;
		camera.x+=diff;
	}
	if(obj->getBox().y<0){
		//A block on the left side of the level, meaning we need to shift everything.
		//First calc the difference.
		int diff=(0-(obj->getBox().y));
		
		for(unsigned int o=0; o<levelObjects.size(); o++){
			moveObject(levelObjects[o],levelObjects[o]->getBox().x,levelObjects[o]->getBox().y+diff);
		}
		  
		//The level grows with the difference, 0-(x+50).
		LEVEL_WIDTH+=diff;
		camera.y+=diff;
	}
	
	//GameObject type specific stuff.
	switch(obj->type){
		case TYPE_BUTTON:
		case TYPE_SWITCH:
		case TYPE_PORTAL:
		{
			//Add the object to the triggers.
			vector<GameObject*> linked;
			triggers[obj]=linked;
			
			//Give it it's own id.
			std::map<std::string,std::string> editorData;
			char s[64];
			sprintf(s,"%d",currentId);
			currentId++;
			editorData["id"]=s;
			obj->setEditorData(editorData);
			break;
		}
		case TYPE_MOVING_BLOCK:
		case TYPE_MOVING_SHADOW_BLOCK:
		case TYPE_MOVING_SPIKES:
		{
			//Add the object to the moving blocks.
			vector<MovingPosition> positions;
			movingBlocks[obj]=positions;
			
			//Get the editor data.
			vector<pair<string,string> > objMap;
			obj->getEditorData(objMap);
			
			//Get the number of entries of the editor data.
			int m=objMap.size();
			
			//Check if the editor data isn't empty.
			if(m>0){
				//Integer containing the positions.
				int pos=0;
				int currentPos=0;
				
				//Get the number of movingpositions.
				pos=atoi(objMap[1].second.c_str());
				
				while(currentPos<pos){
					int x=atoi(objMap[currentPos*3+4].second.c_str());
					int y=atoi(objMap[currentPos*3+5].second.c_str());
					int t=atoi(objMap[currentPos*3+6].second.c_str());
					
					//Create a new movingPosition.
					MovingPosition position(x,y,t);
					movingBlocks[obj].push_back(position);
					
					//Increase currentPos by one.
					currentPos++;
				}
			}
		
			//Give it it's own id.
			std::map<std::string,std::string> editorData;
			char s[64];
			sprintf(s,"%d",currentId);
			currentId++;
			editorData["id"]=s;
			obj->setEditorData(editorData);
			break;
		}
		default:
		  break;
	}
}

void LevelEditor::moveObject(GameObject* obj,int x,int y){
	//Set the obj at it's new position.
	obj->setPosition(x,y);
  
	//Check if the object is inside the level dimensions.
	//If not let the level grow.
	if(obj->getBox().x+50>LEVEL_WIDTH){
		LEVEL_WIDTH=obj->getBox().x+50;
	}
	if(obj->getBox().y+50>LEVEL_HEIGHT){
		LEVEL_HEIGHT=obj->getBox().y+50;
	}
	if(obj->getBox().x<0){
		//A block on the left side of the level, meaning we need to shift everything.
		//First calc the difference.
		int diff=(0-(obj->getBox().x));
		
		for(unsigned int o=0; o<levelObjects.size(); o++){
			moveObject(levelObjects[o],levelObjects[o]->getBox().x+diff,levelObjects[o]->getBox().y);
		}
		  
		//The level grows with the difference, 0-(x+50).
		LEVEL_WIDTH+=diff;
		camera.x+=diff;
	}
	if(obj->getBox().y<0){
		//A block on the left side of the level, meaning we need to shift everything.
		//First calc the difference.
		int diff=(0-(obj->getBox().y));
		
		for(unsigned int o=0; o<levelObjects.size(); o++){
			moveObject(levelObjects[o],levelObjects[o]->getBox().x,levelObjects[o]->getBox().y+diff);
		}
		  
		//The level grows with the difference, 0-(x+50).
		LEVEL_WIDTH+=diff;
		camera.y+=diff;
	}
	
	//If the object is a player or shadow start then change the start position of the player or shadow.
	if(obj->type==TYPE_START_PLAYER){
		//Center the player horizontally.
  		player.fx=obj->getBox().x+(50-23)/2;
		player.fy=obj->getBox().y;
		//Now reset the player to get him to it's new start position.
		player.reset(true);
	}
	if(obj->type==TYPE_START_SHADOW){
		//Center the shadow horizontally.
  		shadow.fx=obj->getBox().x+(50-23)/2;
		shadow.fy=obj->getBox().y;
		//Now reset the shadow to get him to it's new start position.
		shadow.reset(true);
	}
}

void LevelEditor::removeObject(GameObject* obj){
	std::vector<GameObject*>::iterator it;
	std::map<GameObject*,vector<GameObject*> >::iterator mapIt;
	
	//Check if the object is in the selection.
	it=find(selection.begin(),selection.end(),obj);
	if(it!=selection.end()){
		//It is so we delete it.
		selection.erase(it);
	}
	
	//Check if the object is in the triggers.
	mapIt=triggers.find(obj);
	if(mapIt!=triggers.end()){
		//It is so we remove it.
		triggers.erase(mapIt);
	}
	
	//Boolean if it could be a target.
	if(obj->type==TYPE_MOVING_BLOCK || obj->type==TYPE_MOVING_SHADOW_BLOCK || obj->type==TYPE_MOVING_SPIKES
		|| obj->type==TYPE_CONVEYOR_BELT || obj->type==TYPE_SHADOW_CONVEYOR_BELT || obj->type==TYPE_PORTAL){
		for(mapIt=triggers.begin();mapIt!=triggers.end();++mapIt){
			//Now loop the target vector.
			for(unsigned int o=0;o<(*mapIt).second.size();o++){
				//Check if the obj is in the target vector.
				if((*mapIt).second[o]==obj){
					(*mapIt).second.erase(find((*mapIt).second.begin(),(*mapIt).second.end(),obj));
					o--;
				}
			}
		}
	}
	
	//Check if the object is in the movingObjects.
	std::map<GameObject*,vector<MovingPosition> >::iterator movIt;
	movIt=movingBlocks.find(obj);
	if(movIt!=movingBlocks.end()){
		//It is so we remove it.
		movingBlocks.erase(movIt);
	}
	
	//Now we remove the object from the levelObjects.
	it=find(levelObjects.begin(),levelObjects.end(),obj);
	if(it!=levelObjects.end()){
		levelObjects.erase(it);
	}
	delete obj;
	obj=NULL;
}

void LevelEditor::GUIEventCallback_OnEvent(std::string name,GUIObject* obj,int eventType){
	//Check for GUI events.
	//Notification block configure events.
	if(name=="cfgNotificationBlockOK"){
		if(GUIObjectRoot){
			//Set the message of the notification block.
			std::map<std::string,std::string> editorData;
			editorData["message"]=objectProperty->caption;
			configuredObject->setEditorData(editorData);
			
			//And delete the GUI.
			objectProperty=NULL;
			secondObjectProperty=NULL;
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
			editorData["speed"]=secondObjectProperty->caption;
			editorData["disabled"]=(objectProperty->value==0)?"1":"0";
			configuredObject->setEditorData(editorData);
			
			//And delete the GUI.
			objectProperty=NULL;
			secondObjectProperty=NULL;
			configuredObject=NULL;
			delete GUIObjectRoot;
			GUIObjectRoot=NULL;
		}
	}
	//Moving block configure events.
	if(name=="cfgMovingBlockOK"){
		if(GUIObjectRoot){
			//Set if the moving block is enabled/disabled.
			std::map<std::string,std::string> editorData;
			editorData["disabled"]=(objectProperty->value==0)?"1":"0";
			editorData["loop"]=(secondObjectProperty->value==1)?"1":"0";
			configuredObject->setEditorData(editorData);
			
			//And delete the GUI.
			objectProperty=NULL;
			secondObjectProperty=NULL;
			configuredObject=NULL;
			delete GUIObjectRoot;
			GUIObjectRoot=NULL;
		}
	}
	if(name=="cfgMovingBlockClrPath"){
		if(GUIObjectRoot){
			//Set the message of the notification block.
			std::map<std::string,std::string> editorData;
			editorData["MovingPosCount"]="0";
			configuredObject->setEditorData(editorData);
			
			std::map<GameObject*,vector<MovingPosition> >::iterator it;
			it=movingBlocks.find(configuredObject);
			if(it!=movingBlocks.end()){
				(*it).second.clear();
			}
			
			//And delete the GUI.
			objectProperty=NULL;
			secondObjectProperty=NULL;
			configuredObject=NULL;
			delete GUIObjectRoot;
			GUIObjectRoot=NULL;
		}
	}
	if(name=="cfgMovingBlockMakePath"){
		if(GUIObjectRoot){
			//Set moving.
			moving=true;
			movingBlock=configuredObject;
			
			//And delete the GUI.
			objectProperty=NULL;
			secondObjectProperty=NULL;
			configuredObject=NULL;
			delete GUIObjectRoot;
			GUIObjectRoot=NULL;
		}
	}
	//Portal block configure events.
	if(name=="cfgPortalOK"){
		if(GUIObjectRoot){
			//Set the message of the notification block.
			std::map<std::string,std::string> editorData;
			editorData["automatic"]=(objectProperty->value==1)?"1":"0";
			configuredObject->setEditorData(editorData);
			
			//And delete the GUI.
			objectProperty=NULL;
			secondObjectProperty=NULL;
			configuredObject=NULL;
			delete GUIObjectRoot;
			GUIObjectRoot=NULL;
		}
	}
	if(name=="cfgPortalLink"){
		//We set linking true.
		linking=true;
		linkingTrigger=configuredObject;
		
		//And delete the GUI.
		objectProperty=NULL;
		secondObjectProperty=NULL;
		configuredObject=NULL;
		if(GUIObjectRoot){
			delete GUIObjectRoot;
		}
		GUIObjectRoot=NULL;
	}
	if(name=="cfgPortalUnlink"){
		std::map<GameObject*,vector<GameObject*> >::iterator it;
		it=triggers.find(configuredObject);
		if(it!=triggers.end()){
			//Remove the targets.
			(*it).second.clear();
		}
		
		//We give the portal a new id to prevent activating unlinked targets.
		std::map<std::string,std::string> editorData;
		char s[64];
		sprintf(s,"%d",currentId);
		currentId++;
		editorData["id"]=s;
		configuredObject->setEditorData(editorData);
		
		//And delete the GUI.
		objectProperty=NULL;
		secondObjectProperty=NULL;
		configuredObject=NULL;
		if(GUIObjectRoot){
			delete GUIObjectRoot;
		}
		GUIObjectRoot=NULL;
	}
	//Trigger block configure events.
	if(name=="cfgTriggerOK"){
		if(GUIObjectRoot){
			//Set the message of the notification block.
			std::map<std::string,std::string> editorData;
			editorData["behaviour"]=(dynamic_cast<GUISingleLineListBox*>(objectProperty))->item[objectProperty->value];
			configuredObject->setEditorData(editorData);
			
			//And delete the GUI.
			objectProperty=NULL;
			secondObjectProperty=NULL;
			configuredObject=NULL;
			delete GUIObjectRoot;
			GUIObjectRoot=NULL;
		}
	}
	if(name=="cfgTriggerLink"){
		//We set linking true.
		linking=true;
		linkingTrigger=configuredObject;
		
		//And delete the GUI.
		objectProperty=NULL;
		secondObjectProperty=NULL;
		configuredObject=NULL;
		if(GUIObjectRoot){
			delete GUIObjectRoot;
		}
		GUIObjectRoot=NULL;
	}
	if(name=="cfgTriggerUnlink"){
		std::map<GameObject*,vector<GameObject*> >::iterator it;
		it=triggers.find(configuredObject);
		if(it!=triggers.end()){
			//Remove the targets.
			(*it).second.clear();
		}
		
		//We give the trigger a new id to prevent activating unlinked targets.
		std::map<std::string,std::string> editorData;
		char s[64];
		sprintf(s,"%d",currentId);
		currentId++;
		editorData["id"]=s;
		configuredObject->setEditorData(editorData);
		
		//And delete the GUI.
		objectProperty=NULL;
		secondObjectProperty=NULL;
		configuredObject=NULL;
		if(GUIObjectRoot){
			delete GUIObjectRoot;
		}
		GUIObjectRoot=NULL;
	}
	
	//Cancel.
	if(name=="cfgCancel"){
		if(GUIObjectRoot){
			//Delete the GUI.
			objectProperty=NULL;
			secondObjectProperty=NULL;
			configuredObject=NULL;
			delete GUIObjectRoot;
			GUIObjectRoot=NULL;
		}
	}
	
	//LevelSetting events.
	if(name=="lvlSettingsOK"){
		levelName=objectProperty->caption;
		levelTheme=secondObjectProperty->caption;
		
		//And delete the GUI.
		if(GUIObjectRoot){
			objectProperty=NULL;
			secondObjectProperty=NULL;
			delete GUIObjectRoot;
			GUIObjectRoot=NULL;
		}
	}
	if(name=="lvlSettingsCancel"){
		if(GUIObjectRoot){
			//Delete the GUI.
			objectProperty=NULL;
			secondObjectProperty=NULL;
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
		tooltip=-1;
		//Get the mouse location.
		int x,y;
		SDL_GetMouseState(&x,&y);
		SDL_Rect mouse={x,y,0,0};
	
		//We loop through the number of tools + the number of buttons.
		for(int t=0; t<NUMBER_TOOLS+6; t++){
			SDL_Rect toolRect={155+(t*40)+(t*10),555,40,40};
		
			//Check for collision.
			if(checkCollision(mouse,toolRect)==true){
				//Set the tooltip tool.
				tooltip=t;
				
				//Check if there's a mouse click.
				if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button==SDL_BUTTON_LEFT){
					if(t<NUMBER_TOOLS){
						tool=(Tools)t;
					}else{
						//The selected button isn't a tool.
						//Now check which button it is.
						if(t==NUMBER_TOOLS){
							playMode=true;
							cameraSave.x=camera.x;
							cameraSave.y=camera.y;
							
							if(tool==CONFIGURE){
								//Also stop linking or moving.
								if(linking){
									linking=false;
									linkingTrigger=NULL;
								}
								
								if(moving){
									//Write the path to the moving block.
									std::map<std::string,std::string> editorData;
									char s[64], s0[64];
									
									sprintf(s,"%d",movingBlocks[movingBlock].size());
									editorData["MovingPosCount"]=s;
									//Loop through the positions.
									for(unsigned int o=0;o<movingBlocks[movingBlock].size();o++){
										sprintf(s0+1,"%d",o);
										sprintf(s,"%d",movingBlocks[movingBlock][o].x);
										s0[0]='x';
										editorData[s0]=s;
										sprintf(s,"%d",movingBlocks[movingBlock][o].y);
										s0[0]='y';
										editorData[s0]=s;
										sprintf(s,"%d",movingBlocks[movingBlock][o].time);
										s0[0]='t';
										editorData[s0]=s;
									}
									movingBlock->setEditorData(editorData);
									
									moving=false;
									movingBlock=NULL;
								}
							}
						}
						if(t==NUMBER_TOOLS+2){
							//Levelsettings.
							levelSettings();
						}
						if(t==NUMBER_TOOLS+3){
							//Levelpack save.
							LevelPackEditor objEditor;
							objEditor.show();
						}
						if(t==NUMBER_TOOLS+4){
							string s=fileNameFromPath(levelFile);
							if(fileDialog(s,"Save Level","map","%USER%/custom/levels/",true,true)){
								saveLevel(processFileName(s));
								levelFile=processFileName(s);
							}
						}
						if(t==NUMBER_TOOLS+5){
							string s="";
							if(fileDialog(s,"Load Level","map","%USER%/custom/levels/\nMy levels\n%USER%/levels/\nAddon levels\n%DATA%/levels/\nMain levels",false,true)){
								reset();
								loadLevel(processFileName(s));
								postLoad();
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
			if(tool==CONFIGURE){
				showConfigure();
			}
		}
		
		//Draw the level borders.
		drawRect(-camera.x,-camera.y,LEVEL_WIDTH,LEVEL_HEIGHT,screen);
		
		//Render the placement surface.
		applySurface(0,0,placement,screen,NULL);
		
		//Render the hud layer.
		renderHUD();
		
		//On top of all render the toolbar.
		applySurface(145,550,toolbar,screen,NULL);
		//Now render a tooltip.
		if(tooltip>=0){
			//The back and foreground colors.
			SDL_Color bg={255,255,255},fg={0,0,0};
			
			//Tool specific text.
			SDL_Surface* tip=NULL;
			switch(tooltip){
				case 0:
					tip=TTF_RenderText_Shaded(fontSmall,"Select",fg,bg);
					break;
				case 1:
					tip=TTF_RenderText_Shaded(fontSmall,"Add",fg,bg);
					break;
				case 2:
					tip=TTF_RenderText_Shaded(fontSmall,"Delete",fg,bg);
					break;
				case 3:
					tip=TTF_RenderText_Shaded(fontSmall,"Configure",fg,bg);
					break;
				case 4:
					tip=TTF_RenderText_Shaded(fontSmall,"Play",fg,bg);
					break;
				case 6:
					tip=TTF_RenderText_Shaded(fontSmall,"Level settings",fg,bg);
					break;
				case 7:
					tip=TTF_RenderText_Shaded(fontSmall,"Levelpack editor",fg,bg);
					break;
				case 8:
					tip=TTF_RenderText_Shaded(fontSmall,"Save level",fg,bg);
					break;
				case 9:
					tip=TTF_RenderText_Shaded(fontSmall,"Load level",fg,bg);
					break;
				default:
					break;
			}
			
			if(tip!=NULL){
				SDL_Rect r={155+(tooltip*40)+(tooltip*10),555,40,40};
				r.y=550-tip->h;
				if(r.x+tip->w>SCREEN_WIDTH-50)
					r.x=SCREEN_WIDTH-50-tip->w;
				SDL_BlitSurface(tip,NULL,screen,&r);
				r.x--;
				r.y--;
				r.w=tip->w+1;
				r.h=1;
				SDL_FillRect(screen,&r,0);
				SDL_Rect r1={r.x,r.y,1,tip->h+1};
				SDL_FillRect(screen,&r1,0);
				r1.x+=r.w;
				SDL_FillRect(screen,&r1,0);
				r.y+=r1.h;
				SDL_FillRect(screen,&r,0);
				SDL_FreeSurface(tip);
			}
		}
		
		//Draw a rectangle around the current tool.
		drawRect(155+(tool*40)+(tool*10),555,40,40,screen);
	}
}

void LevelEditor::renderHUD(){
	//Switch the tool.
	switch(tool){
	case CONFIGURE:
		//If moving show the moving speed in the top right corner.
		if(moving){
			SDL_Rect r={620,0,180,30};
			SDL_FillRect(screen,&r,0);
			//Shrink the rectangle by one pixel and fill with white leaving an one pixel border.
			r.x+=1;
			r.w-=2;
			r.h-=1;
			SDL_FillRect(screen,&r,0xFFFFFF);
			
			//Now render the text.
			SDL_Color black={0,0,0,0};
			SDL_Color white={255,255,255,255};
			char s[64];
			sprintf(s,"%d",movingSpeed);
			SDL_Surface* bm=TTF_RenderText_Shaded(fontSmall,("Movespeed: "+string(s)).c_str(),black,white);
			
			r.x+=2;
			r.y+=2;
			
			//Draw the text and free the surface.
			SDL_BlitSurface(bm,NULL,screen,&r);
			SDL_FreeSurface(bm);
		}
		break;
	default:
		break;
	}
}

void LevelEditor::showCurrentObject(){
	//Get the current mouse location.
	int x,y;
	SDL_GetMouseState(&x,&y);
	x+=camera.x;
	y+=camera.y;

	//Check if we should snap the block to grid or not.
	if(!pressedShift){
		snapToGrid(&x,&y);
	}else{
		x-=25;
		y-=25;
	}

	//Check if the currentType is a legal type.
	if(currentType>=0 && currentType<EDITOR_ORDER_MAX){
		ThemeBlock* obj=objThemes.getBlock(editorTileOrder[currentType]);
		if(obj){
			obj->editorPicture.draw(placement,x-camera.x,y-camera.y);
		}
	}
}

void LevelEditor::showSelectionDrag(){
	//Get the current mouse location.
	int x,y;
	SDL_GetMouseState(&x,&y);
	//Create the rectangle.
	x+=camera.x;
	y+=camera.y;

	//Check if we should snap the block to grid or not.
	if(!pressedShift){
		snapToGrid(&x,&y);
	}else{
		x-=25;
		y-=25;
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
			obj->editorPicture.draw(placement,(r1.x-r.x)+x-camera.x,(r1.y-r.y)+y-camera.y);
		}
	}
}

void LevelEditor::showConfigure(){
	//arrow animation value. go through 0-65535 and loops.
	static unsigned short arrowAnimation=0;
	arrowAnimation++;

	//Draw the trigger lines.
	{
		map<GameObject*,vector<GameObject*> >::iterator it;
		for(it=triggers.begin();it!=triggers.end();it++){
			//Check if the trigger has linked targets.
			if(!(*it).second.empty()){
				//The location of the trigger.
				SDL_Rect r=(*it).first->getBox();
			
				//Loop through the targets.
				for(unsigned int o=0;o<(*it).second.size();o++){
					//Get the location of the target.
					SDL_Rect r1=(*it).second[o]->getBox();
				
					//Draw the line from the center of the trigger to the center of the target.
					drawLineWithArrow(r.x-camera.x+25,r.y-camera.y+25,r1.x-camera.x+25,r1.y-camera.y+25,placement,0,32,arrowAnimation%32);

					//Also draw two selection marks.
					applySurface(r.x-camera.x+25-2,r.y-camera.y+25-2,selectionMark,screen,NULL);
					applySurface(r1.x-camera.x+25-2,r1.y-camera.y+25-2,selectionMark,screen,NULL);
				}
			}
		}
	
		//Draw a line to the mouse from the linkingTrigger when linking.
		if(linking){
			//Get the current mouse location.
			int x,y;
			SDL_GetMouseState(&x,&y);
	  
			//Draw the line from the center of the trigger to mouse.
			drawLineWithArrow(linkingTrigger->getBox().x-camera.x+25,linkingTrigger->getBox().y-camera.y+25,x,y,placement,0,32,arrowAnimation%32);
		}
	}
	
	//Draw the moving positions.
	map<GameObject*,vector<MovingPosition> >::iterator it;
	for(it=movingBlocks.begin();it!=movingBlocks.end();it++){
		//Check if the block has positions.
		if(!(*it).second.empty()){
			//The location of the moving block.
			SDL_Rect block=(*it).first->getBox();
			block.x+=25-camera.x;
			block.y+=25-camera.y;
			
			//The location of the previous position.
			//The first time it's the moving block's position self.
			SDL_Rect r=block;
			
			//Loop through the positions.
			for(unsigned int o=0;o<(*it).second.size();o++){
				//Draw the line from the center of the previous position to the center of the position.
				//x and y are the coordinates for the current moving position.
				int x=block.x+(*it).second[o].x;
				int y=block.y+(*it).second[o].y;
				
				//Check if we need to draw line
				double dx=r.x-x;
				double dy=r.y-y;
				double d=sqrt(dx*dx+dy*dy);
				if(d>0.001f){
					if(it->second[o].time>0){
						//Calculate offset to contain the moving speed.
						int offset=int(d*arrowAnimation/it->second[o].time)%32;
						drawLineWithArrow(r.x,r.y,x,y,placement,0,32,offset);
					}else{
						//time==0 ???? so don't draw arrow at all
						drawLine(r.x,r.y,x,y,placement);
					}
				}
				
				//And draw a marker at the end.
				applySurface(x-13,y-13,movingMark,screen,NULL);
				
				//Get the box of the previous position.
				SDL_Rect tmp={x,y,0,0};
				r=tmp;
			}
		}
	}
	
	//Draw a line to the mouse from the previous moving pos.
	if(moving){
		//Get the current mouse location.
		int x,y;
		SDL_GetMouseState(&x,&y);
		
		//Check if we should snap the block to grid or not.
		if(!pressedShift){
			x+=camera.x;
			y+=camera.y;
			snapToGrid(&x,&y);
			x-=camera.x;
			y-=camera.y;
		}else{
			x-=25;
			y-=25;
		}

		int posX,posY;
		
		//Check if there are moving positions for the moving block.
		if(!movingBlocks[movingBlock].empty()){
			//Draw the line from the center of the previouse moving positions to mouse.
			posX=movingBlocks[movingBlock].back().x;
			posY=movingBlocks[movingBlock].back().y;
			
			posX-=camera.x;
			posY-=camera.y;
			
			posX+=movingBlock->getBox().x;
			posY+=movingBlock->getBox().y;
		}else{
			//Draw the line from the center of the movingblock to mouse.
			posX=movingBlock->getBox().x-camera.x;
			posY=movingBlock->getBox().y-camera.y;
		}

		//Calculate offset to contain the moving speed.
		int offset=int(double(arrowAnimation)*movingSpeed/10.0)%32;

		drawLineWithArrow(posX+25,posY+25,x+25,y+25,placement,0,32,offset);
		applySurface(x+12,y+12,movingMark,screen,NULL);
	}

}

//Filling the order array
const int LevelEditor::editorTileOrder[EDITOR_ORDER_MAX]={
	TYPE_BLOCK,
	TYPE_SHADOW_BLOCK,
	TYPE_SPIKES,
	TYPE_FRAGILE,
	TYPE_MOVING_BLOCK,
	TYPE_MOVING_SHADOW_BLOCK,
	TYPE_MOVING_SPIKES,
	TYPE_CONVEYOR_BELT,
	TYPE_SHADOW_CONVEYOR_BELT,
	TYPE_BUTTON,
	TYPE_SWITCH,
	TYPE_PORTAL,
	TYPE_SWAP,
	TYPE_CHECKPOINT,
	TYPE_NOTIFICATION_BLOCK,
	TYPE_START_PLAYER,
	TYPE_START_SHADOW,
	TYPE_EXIT
};
