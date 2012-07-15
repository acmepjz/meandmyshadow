/*
 * Copyright (C) 2012 Me and My Shadow
 *
 * This file is part of Me and My Shadow.
 *
 * Me and My Shadow is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Me and My Shadow is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Me and My Shadow.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "LevelEditSelect.h"
#include "GameState.h"
#include "Functions.h"
#include "FileManager.h"
#include "Globals.h"
#include "Objects.h"
#include "GUIObject.h"
#include "GUIListBox.h"
#include "GUIScrollBar.h"
#include "InputManager.h"
#include "Game.h"
#include "GUIOverlay.h"
#include <SDL/SDL_ttf.h>
#include <SDL/SDL.h>
#include <stdio.h>
#include <algorithm>
#include <string>
#include <iostream>

#include "libs/tinyformat/tinyformat.h"

using namespace std;

LevelEditSelect::LevelEditSelect():LevelSelect(_("Map Editor"),LevelPackManager::CUSTOM_PACKS){
	//Create the gui.
	createGUI(true);
	
	//Set the levelEditGUIObjectRoot.
	levelEditGUIObjectRoot=GUIObjectRoot;
	
	//show level list
	changePack();
	refresh();
}

LevelEditSelect::~LevelEditSelect(){
	selectedNumber=NULL;
}

void LevelEditSelect::createGUI(bool initial){
	if(initial){
		//The levelpack name text field.
		levelpackName=new GUIObject(280,104,240,32,GUIObjectTextBox);
		levelpackName->eventCallback=this;
		levelpackName->visible=false;
		GUIObjectRoot->childControls.push_back(levelpackName);
	}
	
	if(!initial){
		//Remove the previous buttons.
		for(int i=0;i<GUIObjectRoot->childControls.size();i++){
			if(GUIObjectRoot->childControls[i]->type==GUIObjectButton && GUIObjectRoot->childControls[i]->caption!=_("Back")){
				delete GUIObjectRoot->childControls[i];
				GUIObjectRoot->childControls.erase(GUIObjectRoot->childControls.begin()+i);
				i--;
			}
		}
	}
	
	//Create the six buttons at the bottom of the screen.
	GUIObject* obj=new GUIObject(SCREEN_WIDTH*0.02,SCREEN_HEIGHT-120,-1,32,GUIObjectButton,_("New Levelpack"));
	obj->name="cmdNewLvlpack";
	obj->eventCallback=this;
	GUIObjectRoot->childControls.push_back(obj);
	
	propertiesPack=new GUIObject(SCREEN_WIDTH*0.5,SCREEN_HEIGHT-120,-1,32,GUIObjectButton,_("Pack Properties"),0,true,true,GUIGravityCenter);
	propertiesPack->name="cmdLvlpackProp";
	propertiesPack->eventCallback=this;
	GUIObjectRoot->childControls.push_back(propertiesPack);
	
	removePack=new GUIObject(SCREEN_WIDTH*0.98,SCREEN_HEIGHT-120,-1,32,GUIObjectButton,_("Remove Pack"),0,true,true,GUIGravityRight);
	removePack->name="cmdRmLvlpack";
	removePack->eventCallback=this;
	GUIObjectRoot->childControls.push_back(removePack);
	
	move=new GUIObject(SCREEN_WIDTH*0.02,SCREEN_HEIGHT-60,-1,32,GUIObjectButton,_("Move Map"));
	move->name="cmdMoveMap";
	move->eventCallback=this;
	//NOTE: Set enabled equal to the inverse of initial.
	//When resizing the window initial will be false and therefor the move button can stay enabled.
	move->enabled=false;
	GUIObjectRoot->childControls.push_back(move);
	
	remove=new GUIObject(SCREEN_WIDTH*0.5,SCREEN_HEIGHT-60,-1,32,GUIObjectButton,_("Remove Map"),0,false,true,GUIGravityCenter);
	remove->name="cmdRmMap";
	remove->eventCallback=this;
	GUIObjectRoot->childControls.push_back(remove);
	
	edit=new GUIObject(SCREEN_WIDTH*0.98,SCREEN_HEIGHT-60,-1,32,GUIObjectButton,_("Edit Map"),0,false,true,GUIGravityRight);
	edit->name="cmdEdit";
	edit->eventCallback=this;
	GUIObjectRoot->childControls.push_back(edit);
	
	//Now update widgets and then check if they overlap
	GUIObjectRoot->render(0,0,false);
	if(propertiesPack->left-propertiesPack->gravityX < obj->left+obj->width ||
	   propertiesPack->left-propertiesPack->gravityX+propertiesPack->width > removePack->left-removePack->gravityX){
		obj->smallFont=true;
		obj->width=-1;
		
		propertiesPack->smallFont=true;
		propertiesPack->width=-1;
		
		removePack->smallFont=true;
		removePack->width=-1;
		
		move->smallFont=true;
		move->width=-1;
		
		remove->smallFont=true;
		remove->width=-1;
		
		edit->smallFont=true;
		edit->width=-1;
	}
	
	//Check again
	GUIObjectRoot->render(0,0,false);
	if(propertiesPack->left-propertiesPack->gravityX < obj->left+obj->width ||
	   propertiesPack->left-propertiesPack->gravityX+propertiesPack->width > removePack->left-removePack->gravityX){
		obj->left = SCREEN_WIDTH*0.02;
		obj->top = SCREEN_HEIGHT-140;
		obj->smallFont=false;
		obj->width=-1;
		obj->gravity = GUIGravityLeft;
		
		propertiesPack->left = SCREEN_WIDTH*0.02;
		propertiesPack->top = SCREEN_HEIGHT-100;
		propertiesPack->smallFont=false;
		propertiesPack->width=-1;
		propertiesPack->gravity = GUIGravityLeft;
		
		removePack->left = SCREEN_WIDTH*0.02;
		removePack->top = SCREEN_HEIGHT-60;
		removePack->smallFont=false;
		removePack->width=-1;
		removePack->gravity = GUIGravityLeft;
		
		move->left = SCREEN_WIDTH*0.98;
		move->top = SCREEN_HEIGHT-140;
		move->smallFont=false;
		move->width=-1;
		move->gravity = GUIGravityRight;
		
		remove->left = SCREEN_WIDTH*0.98;
		remove->top = SCREEN_HEIGHT-100;
		remove->smallFont=false;
		remove->width=-1;
		remove->gravity = GUIGravityRight;
		
		edit->left = SCREEN_WIDTH*0.98;
		edit->top = SCREEN_HEIGHT-60;
		edit->smallFont=false;
		edit->width=-1;
		edit->gravity = GUIGravityRight;
	}
}

void LevelEditSelect::changePack(){
	packName=levelpacks->item[levelpacks->value];
	if(packName=="Custom Levels"){
		//Disable some levelpack buttons.
		propertiesPack->enabled=false;
		removePack->enabled=false;
	}else{
		//Enable some levelpack buttons.
		propertiesPack->enabled=true;
		removePack->enabled=true;
	}
	
	//Set last levelpack.
	getSettings()->setValue("lastlevelpack",packName);
	
	//Now let levels point to the right pack.
	levels=getLevelPackManager()->getLevelPack(packName);
}

void LevelEditSelect::packProperties(bool newPack){
	//Open a message popup.
	GUIObject* root=new GUIObject((SCREEN_WIDTH-600)/2,(SCREEN_HEIGHT-320)/2,600,320,GUIObjectFrame,_("Properties"));
	GUIObject* obj;
	
	obj=new GUIObject(40,50,240,36,GUIObjectLabel,_("Name:"));
	root->childControls.push_back(obj);

	obj=new GUIObject(60,80,480,36,GUIObjectTextBox,packName.c_str());
	if(newPack)
		obj->caption="";
	obj->name="LvlpackName";
	root->childControls.push_back(obj);
	
	obj=new GUIObject(40,120,240,36,GUIObjectLabel,_("Description:"));
	root->childControls.push_back(obj);

	obj=new GUIObject(60,150,480,36,GUIObjectTextBox,levels->levelpackDescription.c_str());
	if(newPack)
		obj->caption="";
	obj->name="LvlpackDescription";
	root->childControls.push_back(obj);
	
	obj=new GUIObject(40,190,240,36,GUIObjectLabel,_("Congratulation text:"));
	root->childControls.push_back(obj);
	
	obj=new GUIObject(60,220,480,36,GUIObjectTextBox,levels->congratulationText.c_str());
	if(newPack)
		obj->caption="";
	obj->name="LvlpackCongratulation";
	root->childControls.push_back(obj);
	
	obj=new GUIObject(root->width*0.3,320-44,-1,36,GUIObjectButton,_("OK"),0,true,true,GUIGravityCenter);
	obj->name="cfgOK";
	obj->eventCallback=this;
	root->childControls.push_back(obj);
	obj=new GUIObject(root->width*0.7,320-44,-1,36,GUIObjectButton,_("Cancel"),0,true,true,GUIGravityCenter);
	obj->name="cfgCancel";
	obj->eventCallback=this;
	root->childControls.push_back(obj);
	
	//Create the gui overlay.
	GUIOverlay* overlay=new GUIOverlay(root);

	if(newPack){
		packName.clear();
	}
}

void LevelEditSelect::addLevel(){
	//Open a message popup.
	GUIObject* root=new GUIObject((SCREEN_WIDTH-600)/2,(SCREEN_HEIGHT-200)/2,600,200,GUIObjectFrame,_("Add level"));
	GUIObject* obj;
	
	obj=new GUIObject(40,80,240,36,GUIObjectLabel,_("File name:"));
	root->childControls.push_back(obj);
	
	char s[64];
	sprintf(s,"map%02d.map",levels->getLevelCount()+1);
	obj=new GUIObject(300,80,240,36,GUIObjectTextBox,s);
	obj->name="LvlFile";
	root->childControls.push_back(obj);

	obj=new GUIObject(root->width*0.3,200-44,-1,36,GUIObjectButton,_("OK"),0,true,true,GUIGravityCenter);
	obj->name="cfgAddOK";
	obj->eventCallback=this;
	root->childControls.push_back(obj);
	obj=new GUIObject(root->width*0.7,200-44,-1,36,GUIObjectButton,_("Cancel"),0,true,true,GUIGravityCenter);
	obj->name="cfgAddCancel";
	obj->eventCallback=this;
	root->childControls.push_back(obj);
	
	//Dim the screen using the tempSurface.
	GUIOverlay* overlay=new GUIOverlay(root);
}

void LevelEditSelect::moveLevel(){
	//Open a message popup.
	GUIObject* root=new GUIObject((SCREEN_WIDTH-600)/2,(SCREEN_HEIGHT-200)/2,600,200,GUIObjectFrame,_("Move level"));
	GUIObject* obj;
	
	obj=new GUIObject(40,60,240,36,GUIObjectLabel,_("Level: "));
	root->childControls.push_back(obj);
	
	obj=new GUIObject(300,60,240,36,GUIObjectTextBox,"1");
	obj->name="MoveLevel";
	root->childControls.push_back(obj);
	
	obj=new GUISingleLineListBox(root->width*0.5,110,240,36,true,true,GUIGravityCenter);
	obj->name="lstPlacement";
	vector<string> v;
	v.push_back(_("Before"));
	v.push_back(_("After"));
	v.push_back(_("Swap"));
	(dynamic_cast<GUISingleLineListBox*>(obj))->item=v;
	obj->value=0;
	root->childControls.push_back(obj);
	
	obj=new GUIObject(root->width*0.3,200-44,-1,36,GUIObjectButton,_("OK"),0,true,true,GUIGravityCenter);
	obj->name="cfgMoveOK";
	obj->eventCallback=this;
	root->childControls.push_back(obj);
	obj=new GUIObject(root->width*0.7,200-44,-1,36,GUIObjectButton,_("Cancel"),0,true,true,GUIGravityCenter);
	obj->name="cfgMoveCancel";
	obj->eventCallback=this;
	root->childControls.push_back(obj);
	
	//Create the gui overlay.
	GUIOverlay* overlay=new GUIOverlay(root);
}

void LevelEditSelect::refresh(bool change){
	int m=levels->getLevelCount();

	if(change){
		numbers.clear();
		
		//clear the selected level
		if(selectedNumber!=NULL){
			selectedNumber=NULL;
		}
		
		//Disable the level specific buttons.
		move->enabled=false;
		remove->enabled=false;
		edit->enabled=false;
		
		for(int n=0;n<=m;n++){
			numbers.push_back(Number());
		}
	}
	
	for(int n=0;n<m;n++){
		SDL_Rect box={(n%LEVELS_PER_ROW)*64+80,(n/LEVELS_PER_ROW)*64+184,0,0};
		numbers[n].init(n,box);
	}
	SDL_Rect box={(m%LEVELS_PER_ROW)*64+80,(m/LEVELS_PER_ROW)*64+184,0,0};
	numbers[m].init("+",box);
	
	m++; //including the "+" button
	if(m>LEVELS_DISPLAYED_IN_SCREEN){
		levelScrollBar->maxValue=(m-LEVELS_DISPLAYED_IN_SCREEN+LEVELS_PER_ROW-1)/LEVELS_PER_ROW;
		levelScrollBar->visible=true;
	}else{
		levelScrollBar->maxValue=0;
		levelScrollBar->visible=false;
	}
	if(!levels->levelpackDescription.empty())
		levelpackDescription->caption=_C(levels->getDictionaryManager(),levels->levelpackDescription);
	else
		levelpackDescription->caption="";
}

void LevelEditSelect::selectNumber(unsigned int number,bool selected){
	if(selected){
		levels->setCurrentLevel(number);
		setNextState(STATE_LEVEL_EDITOR);
		
		//Pick music from the current music list.
		getMusicManager()->pickMusic();
	}else{
		if(number==numbers.size()-1){
			addLevel();
		}else if(number>=0 && number<numbers.size()){
			selectedNumber=&numbers[number];
			
			//Enable the level specific buttons.
			//NOTE: We check if 'remove levelpack' is enabled, if not then it's the Levels levelpack.
			if(removePack->enabled)
				move->enabled=true;
			remove->enabled=true;
			edit->enabled=true;
		}
	}
}

void LevelEditSelect::render(){
	//Let the levelselect render.
	LevelSelect::render();
}

void LevelEditSelect::resize(){
	//Let the levelselect resize.
	LevelSelect::resize();
	
	//Create the GUI.
	createGUI(false);
	
	//NOTE: This is a workaround for buttons failing when resizing.
	if(packName=="Custom Levels"){
		removePack->enabled=false;
		propertiesPack->enabled=false;
	}
	if(selectedNumber)
		selectNumber(selectedNumber->getNumber(),false);
	
}

void LevelEditSelect::renderTooltip(unsigned int number,int dy){
	SDL_Color fg={0,0,0};
	SDL_Surface* name;
	
	if(number==(unsigned)levels->getLevelCount()){
		//Render the name of the level.
		name=TTF_RenderUTF8_Blended(fontText,_("Add level"),fg);
	}else{
		//Render the name of the level.
		name=TTF_RenderUTF8_Blended(fontText,_C(levels->getDictionaryManager(),levels->getLevelName(number)),fg);
	}
	
	//Check if name isn't null.
	if(name==NULL)
		return;
	
	//Now draw a square the size of the three texts combined.
	SDL_Rect r=numbers[number].box;
	r.y-=dy*64;
	r.w=name->w;
	r.h=name->h;
	
	//Make sure the tooltip doesn't go outside the window.
	if(r.y>SCREEN_HEIGHT-200){
		r.y-=name->h+4;
	}else{
		r.y+=numbers[number].box.h+2;
	}
	if(r.x+r.w>SCREEN_WIDTH-50)
		r.x=SCREEN_WIDTH-50-r.w;
	
	//Draw a rectange
	Uint32 color=0xFFFFFF00|240;
	drawGUIBox(r.x-5,r.y-5,r.w+10,r.h+10,screen,color);
	
	//Calc the position to draw.
	SDL_Rect r2=r;
	
	//Now we render the name if the surface isn't null.
	if(name!=NULL){
		//Draw the name.
		SDL_BlitSurface(name,NULL,screen,&r2);
	}
	
	//And free the surfaces.
	SDL_FreeSurface(name);
}

void LevelEditSelect::GUIEventCallback_OnEvent(std::string name,GUIObject* obj,int eventType){
	//NOTE: We check for the levelpack change to enable/disable some levelpack buttons.
	if(name=="cmdLvlPack"){
		//We call changepack and return to prevent the LevelSelect to undo what we did.
		changePack();
		refresh();
		return;
	}
	
	//Let the level select handle his GUI events.
	LevelSelect::GUIEventCallback_OnEvent(name,obj,eventType);
	
	//Check for the edit button.
	if(name=="cmdNewLvlpack"){
		//Create a new pack.
		packProperties(true);
	}else if(name=="cmdLvlpackProp"){
		//Show the pack properties.
		packProperties(false);
	}else if(name=="cmdRmLvlpack"){
		//Show an "are you sure" message.
		if(msgBox(_("Are you sure?"),MsgBoxYesNo,_("Remove prompt"))==MsgBoxYes){
			//Remove the directory.
			if(!removeDirectory(levels->levelpackPath.c_str())){
				cerr<<"ERROR: Unable to remove levelpack directory "<<levels->levelpackPath<<endl;
			}
			
			//Remove it from the vector (levelpack list).
			vector<string>::iterator it;
			it=find(levelpacks->item.begin(),levelpacks->item.end(),packName);
			if(it!=levelpacks->item.end()){
				levelpacks->item.erase(it);
			}
			
			//Remove it from the levelpackManager.
			getLevelPackManager()->removeLevelPack(packName);
			
			//And call changePack.
			levelpacks->value=levelpacks->item.size()-1;
			changePack();
			refresh();
		}
	}else if(name=="cmdMoveMap"){
		if(selectedNumber!=NULL){
			moveLevel();
		}
	}else if(name=="cmdRmMap"){
		if(selectedNumber!=NULL){
			if(packName!="Custom Levels"){
				if(!removeFile((levels->levelpackPath+"/"+levels->getLevel(selectedNumber->getNumber())->file).c_str())){
					cerr<<"ERROR: Unable to remove level "<<(levels->levelpackPath+"/"+levels->getLevel(selectedNumber->getNumber())->file).c_str()<<endl;
				}
				levels->removeLevel(selectedNumber->getNumber());
				levels->saveLevels(levels->levelpackPath+"/levels.lst");
			}else{
				//This is the levels levelpack so we just remove the file.
				if(!removeFile(levels->getLevel(selectedNumber->getNumber())->file.c_str())){
					cerr<<"ERROR: Unable to remove level "<<levels->getLevel(selectedNumber->getNumber())->file<<endl;
				}
				levels->removeLevel(selectedNumber->getNumber());
			}
			
			//And refresh the selection screen.
			refresh();
		}
	}else if(name=="cmdEdit"){
		if(selectedNumber!=NULL){
			levels->setCurrentLevel(selectedNumber->getNumber());
			setNextState(STATE_LEVEL_EDITOR);
			
			//Pick music from the current music list.
			getMusicManager()->pickMusic();
		}
	}
	
	//Check for levelpack properties events.
	if(name=="cfgOK"){
		//Now loop throught the children of the GUIObjectRoot in search of the fields.
		for(unsigned int i=0;i<GUIObjectRoot->childControls.size();i++){
			if(GUIObjectRoot->childControls[i]->name=="LvlpackName"){
				//Check if the name changed.
				if(packName!=GUIObjectRoot->childControls[i]->caption){
					//Delete the old one.
					if(!packName.empty()){
						if(!renameDirectory((getUserPath(USER_DATA)+"custom/levelpacks/"+packName).c_str(),(getUserPath(USER_DATA)+"custom/levelpacks/"+GUIObjectRoot->childControls[i]->caption).c_str())){
							cerr<<"ERROR: Unable to move levelpack directory "<<(getUserPath(USER_DATA)+"custom/levelpacks/"+packName)<<" to "<<(getUserPath(USER_DATA)+"custom/levelpacks/"+GUIObjectRoot->childControls[i]->caption)<<endl;
						}
						
						//Remove the old one from the levelpack manager.
						getLevelPackManager()->removeLevelPack(packName);
						
						//And the levelpack list.
						vector<string>::iterator it1;
						it1=find(levelpacks->item.begin(),levelpacks->item.end(),packName);
							if(it1!=levelpacks->item.end()){
								levelpacks->item.erase(it1);
							if((unsigned)levelpacks->value>levelpacks->item.size())
								levelpacks->value=levelpacks->item.size()-1;
						}
					}else{
						//It's a new levelpack so we need to change the levels array.
						LevelPack* pack=new LevelPack;
						levels=pack;

						//Now create the dirs.
						if(!createDirectory((getUserPath(USER_DATA)+"custom/levelpacks/"+GUIObjectRoot->childControls[i]->caption).c_str())){
							cerr<<"ERROR: Unable to create levelpack directory "<<(getUserPath(USER_DATA)+"custom/levelpacks/"+GUIObjectRoot->childControls[i]->caption)<<endl;
						}
						if(!createFile((getUserPath(USER_DATA)+"custom/levelpacks/"+GUIObjectRoot->childControls[i]->caption+"/levels.lst").c_str())){
							cerr<<"ERROR: Unable to create levelpack file "<<(getUserPath(USER_DATA)+"custom/levelpacks/"+GUIObjectRoot->childControls[i]->caption+"/levels.lst")<<endl;
						}
					}
					//And set the new name.
					packName=GUIObjectRoot->childControls[i]->caption;
					levels->levelpackName=packName;
					levels->levelpackPath=(getUserPath(USER_DATA)+"custom/levelpacks/"+packName+"/");
					
					//Also add the levelpack location
					getLevelPackManager()->addLevelPack(levels);
					levelpacks->item.push_back(GUIObjectRoot->childControls[i]->caption);
					levelpacks->value=levelpacks->item.size()-1;
					
					//And call changePack.
					changePack();
				}
			}
			if(GUIObjectRoot->childControls[i]->name=="LvlpackDescription"){
				levels->levelpackDescription=GUIObjectRoot->childControls[i]->caption;
			}
			if(GUIObjectRoot->childControls[i]->name=="LvlpackCongratulation"){
				levels->congratulationText=GUIObjectRoot->childControls[i]->caption;
			}
		}
		//Refresh the leveleditselect to show the correct information.
		refresh();
		
		//Save the configuration.
		levels->saveLevels(getUserPath(USER_DATA)+"custom/levelpacks/"+packName+"/levels.lst");
		getSettings()->setValue("lastlevelpack",packName);
		
		//Clear the gui.
		if(GUIObjectRoot){
			delete GUIObjectRoot;
			GUIObjectRoot=NULL;
		}
	}else if(name=="cfgCancel"){
		//Check if packName is empty, if so it was a new levelpack and we need to revert to an existing one.
		if(packName.empty()){
			packName=levelpacks->item[levelpacks->value];
			changePack();
		}
		
		//Clear the gui.
		if(GUIObjectRoot){
			delete GUIObjectRoot;
			GUIObjectRoot=NULL;
		}
	}
	
	//Check for add level events.
	if(name=="cfgAddOK"){
		//Check if the file name isn't null.
		//Now loop throught the children of the GUIObjectRoot in search of the fields.
		for(unsigned int i=0;i<GUIObjectRoot->childControls.size();i++){
			if(GUIObjectRoot->childControls[i]->name=="LvlFile"){
				if(GUIObjectRoot->childControls[i]->caption.empty()){
					msgBox(_("No file name given for the new level."),MsgBoxOKOnly,_("Missing file name"));
					return;
				}else{
					string tmp_caption = GUIObjectRoot->childControls[i]->caption;
					
					//Replace all spaces with a underline.
					size_t j;
					for(;(j=tmp_caption.find(" "))!=string::npos;){
						tmp_caption.replace(j,1,"_");
					}
					
					//If there isn't ".map" extension add it.
					size_t found=tmp_caption.find_first_of(".");
					if(found!=string::npos)
						tmp_caption.replace(tmp_caption.begin()+found+1,tmp_caption.end(),"map");
					else if (tmp_caption.substr(found+1)!="map")
						tmp_caption.append(".map");
					
					/* Create path and file in it */
					string path=(levels->levelpackPath+"/"+tmp_caption);
					if(packName=="Custom Levels"){
						path=(getUserPath(USER_DATA)+"/custom/levels/"+tmp_caption);
					}
					
					//First check if the file doesn't exist already.
					FILE* f;
					f=fopen(path.c_str(),"rb");
					
					//Check if it exists.
					if(f){
						//Close the file.
						fclose(f);
						
						//Let the currentState render once to prevent multiple GUI overlapping and prevent the screen from going black.
						currentState->render();
						levelEditGUIObjectRoot->render();
						
						//Notify the user.
						msgBox(("The file "+tmp_caption+" already exists.").c_str(),MsgBoxOKOnly,"Error");
						return;
					}
					
					if(!createFile(path.c_str())){
						cerr<<"ERROR: Unable to create level file "<<path<<endl;
					}
					levels->addLevel(path);
					if(packName!="Custom Levels")
						levels->saveLevels(getUserPath(USER_DATA)+"custom/levelpacks/"+packName+"/levels.lst");
					refresh();
					
					//Clear the gui.
					if(GUIObjectRoot){
						delete GUIObjectRoot;
						GUIObjectRoot=NULL;
						return;
					}
				}
			}
		}
	}else if(name=="cfgAddCancel"){
		//Clear the gui.
		if(GUIObjectRoot){
			delete GUIObjectRoot;
			GUIObjectRoot=NULL;
		}
	}
	
	//Check for move level events.
	if(name=="cfgMoveOK"){
		//Check if the entered level number is valid.
		//Now loop throught the children of the GUIObjectRoot in search of the fields.
		int level=0;
		int placement=0;
		for(unsigned int i=0;i<GUIObjectRoot->childControls.size();i++){
			if(GUIObjectRoot->childControls[i]->name=="MoveLevel"){
				level=atoi(GUIObjectRoot->childControls[i]->caption.c_str());
				if(level<=0 || level>levels->getLevelCount()){
					msgBox(_("The entered level number isn't valid!"),MsgBoxOKOnly,_("Illegal number"));
					return;
				}
			}
			if(GUIObjectRoot->childControls[i]->name=="lstPlacement"){
				placement=GUIObjectRoot->childControls[i]->value;
			}
		}
		
		//Now we execute the swap/move.
		//Check for the place before.
		if(placement==0){
			//We place the selected level before the entered level.
			levels->moveLevel(selectedNumber->getNumber(),level-1);
		}else if(placement==1){
			//We place the selected level after the entered level.
			if(level<selectedNumber->getNumber())
				levels->moveLevel(selectedNumber->getNumber(),level);
			else
				levels->moveLevel(selectedNumber->getNumber(),level+1);
		}else if(placement==2){
			//We swap the selected level with the entered level.
			levels->swapLevel(selectedNumber->getNumber(),level-1);
		}
		
		//And save the change.
		if(packName!="Custom Levels")
			levels->saveLevels(getUserPath(USER_DATA)+"custom/levelpacks/"+packName+"/levels.lst");
			
		refresh();
		
		//Clear the gui.
		if(GUIObjectRoot){
			delete GUIObjectRoot;
			GUIObjectRoot=NULL;
		}
	}else if(name=="cfgMoveCancel"){
		//Clear the gui.
		if(GUIObjectRoot){
			delete GUIObjectRoot;
			GUIObjectRoot=NULL;
		}
	}
}
