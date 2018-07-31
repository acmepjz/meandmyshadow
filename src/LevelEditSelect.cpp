/*
 * Copyright (C) 2012-2013 Me and My Shadow
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
#include "GUIObject.h"
#include "GUIListBox.h"
#include "GUIScrollBar.h"
#include "InputManager.h"
#include "StatisticsManager.h"
#include "Game.h"
#include "GUIOverlay.h"
#include <algorithm>
#include <string>
#include <iostream>

#include "libs/tinyformat/tinyformat.h"

using namespace std;

LevelEditSelect::LevelEditSelect(ImageManager& imageManager, SDL_Renderer& renderer):LevelSelect(imageManager,renderer,_("Map Editor"),LevelPackManager::CUSTOM_PACKS){
	//Create the gui.
    createGUI(imageManager,renderer, true);
	
	//Set the levelEditGUIObjectRoot.
	levelEditGUIObjectRoot=GUIObjectRoot;
	
	//show level list
	changePack();
    refresh(imageManager, renderer);
}

LevelEditSelect::~LevelEditSelect(){
	selectedNumber=NULL;
}

void LevelEditSelect::createGUI(ImageManager& imageManager,SDL_Renderer &renderer, bool initial){
	if(initial){
		//The levelpack name text field.
        levelpackName=new GUITextBox(imageManager,renderer,280,104,240,32);
		levelpackName->eventCallback=this;
		levelpackName->visible=false;
		GUIObjectRoot->addChild(levelpackName);

		//Create the six buttons at the bottom of the screen.
		newPack = new GUIButton(imageManager, renderer, 0, 0, -1, 32, _("New Levelpack"));
		newPack->name = "cmdNewLvlpack";
		newPack->eventCallback = this;
		GUIObjectRoot->addChild(newPack);

		propertiesPack = new GUIButton(imageManager, renderer, 0, 0, -1, 32, _("Pack Properties"), 0, true, true, GUIGravityCenter);
		propertiesPack->name = "cmdLvlpackProp";
		propertiesPack->eventCallback = this;
		GUIObjectRoot->addChild(propertiesPack);

		removePack = new GUIButton(imageManager, renderer, 0, 0, -1, 32, _("Remove Pack"), 0, true, true, GUIGravityRight);
		removePack->name = "cmdRmLvlpack";
		removePack->eventCallback = this;
		GUIObjectRoot->addChild(removePack);

		move = new GUIButton(imageManager, renderer, 0, 0, -1, 32, _("Move Map"));
		move->name = "cmdMoveMap";
		move->eventCallback = this;
		//NOTE: Set enabled equal to the inverse of initial.
		//When resizing the window initial will be false and therefor the move button can stay enabled.
		move->enabled = false;
		GUIObjectRoot->addChild(move);

		remove = new GUIButton(imageManager, renderer, 0, 0, -1, 32, _("Remove Map"), 0, false, true, GUIGravityCenter);
		remove->name = "cmdRmMap";
		remove->eventCallback = this;
		GUIObjectRoot->addChild(remove);

		edit = new GUIButton(imageManager, renderer, 0, 0, -1, 32, _("Edit Map"), 0, false, true, GUIGravityRight);
		edit->name = "cmdEdit";
		edit->eventCallback = this;
		GUIObjectRoot->addChild(edit);
	}

	//Move buttons to correct position
	const int x1 = int(SCREEN_WIDTH*0.02), x2 = int(SCREEN_WIDTH*0.5), x3 = int(SCREEN_WIDTH*0.98);
	const int y1 = SCREEN_HEIGHT - 120, y2 = SCREEN_HEIGHT - 60;
	newPack->left = x1; newPack->top = y1;
	propertiesPack->left = x2; propertiesPack->top = y1;
	removePack->left = x3; removePack->top = y1;
	move->left = x1; move->top = y2;
	remove->left = x2; remove->top = y2;
	edit->left = x3; edit->top = y2;

	//Now update widgets and then check if they overlap
    GUIObjectRoot->render(renderer,0,0,false);
	if (propertiesPack->left - propertiesPack->gravityX < newPack->left + newPack->width ||
	   propertiesPack->left-propertiesPack->gravityX+propertiesPack->width > removePack->left-removePack->gravityX){
		newPack->smallFont = true;
		newPack->width = -1;
		
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

	// NOTE: this should not happen since the minimal width is 800 ???
	
	/*//Check again
    GUIObjectRoot->render(renderer,0,0,false);
	if (propertiesPack->left - propertiesPack->gravityX < newPack->left + newPack->width ||
	   propertiesPack->left-propertiesPack->gravityX+propertiesPack->width > removePack->left-removePack->gravityX){
		newPack->left = SCREEN_WIDTH*0.02;
		newPack->top = SCREEN_HEIGHT - 140;
		newPack->smallFont = false;
		newPack->width = -1;
		newPack->gravity = GUIGravityLeft;
		
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
	}*/
}

void LevelEditSelect::changePack(){
	packName=levelpacks->item[levelpacks->value].second;
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
	getSettings()->setValue("lastlevelpack",levelpacks->getName());
	
	//Now let levels point to the right pack.
	levels=getLevelPackManager()->getLevelPack(levelpacks->getName());

	//invalidate the tooltip
	toolTip.number = -1;
}

void LevelEditSelect::packProperties(ImageManager& imageManager,SDL_Renderer& renderer, bool newPack){
	//Open a message popup.
    GUIObject* root=new GUIFrame(imageManager,renderer,(SCREEN_WIDTH-600)/2,(SCREEN_HEIGHT-390)/2,600,390,_("Properties"));
	GUIObject* obj;
	
    obj=new GUILabel(imageManager,renderer,40,50,240,36,_("Name:"));
	root->addChild(obj);

    obj=new GUITextBox(imageManager,renderer,60,80,480,36,packName.c_str());
	if(newPack)
		obj->caption="";
	obj->name="LvlpackName";
	root->addChild(obj);
	
    obj=new GUILabel(imageManager,renderer,40,120,240,36,_("Description:"));
	root->addChild(obj);

    obj=new GUITextBox(imageManager,renderer,60,150,480,36,levels->levelpackDescription.c_str());
	if(newPack)
		obj->caption="";
	obj->name="LvlpackDescription";
	root->addChild(obj);
	
    obj=new GUILabel(imageManager,renderer,40,190,240,36,_("Congratulation text:"));
	root->addChild(obj);
	
    obj=new GUITextBox(imageManager,renderer,60,220,480,36,levels->congratulationText.c_str());
	if(newPack)
		obj->caption="";
	obj->name="LvlpackCongratulation";
	root->addChild(obj);

	obj = new GUILabel(imageManager, renderer, 40, 260, 240, 36, _("Music list:"));
	root->addChild(obj);

	obj = new GUITextBox(imageManager, renderer, 60, 290, 480, 36, levels->levelpackMusicList.c_str());
	if (newPack)
		obj->caption = "";
	obj->name = "LvlpackMusic";
	root->addChild(obj);

    obj=new GUIButton(imageManager,renderer,root->width*0.3,390-44,-1,36,_("OK"),0,true,true,GUIGravityCenter);
	obj->name="cfgOK";
	obj->eventCallback=this;
	root->addChild(obj);
    obj=new GUIButton(imageManager,renderer,root->width*0.7,390-44,-1,36,_("Cancel"),0,true,true,GUIGravityCenter);
	obj->name="cfgCancel";
	obj->eventCallback=this;
	root->addChild(obj);
	
	//Create the gui overlay.
	//NOTE: We don't need to store a pointer since it will auto cleanup itself.
    new GUIOverlay(renderer,root);

	if(newPack){
		packName.clear();
	}
}

void LevelEditSelect::addLevel(ImageManager& imageManager,SDL_Renderer& renderer){
	//Open a message popup.
    GUIObject* root=new GUIFrame(imageManager,renderer,(SCREEN_WIDTH-600)/2,(SCREEN_HEIGHT-200)/2,600,200,_("Add level"));
	GUIObject* obj;
	
    obj=new GUILabel(imageManager,renderer,40,80,240,36,_("File name:"));
	root->addChild(obj);
	
	char s[64];
    SDL_snprintf(s,64,"map%02d.map",levels->getLevelCount()+1);
    obj=new GUITextBox(imageManager,renderer,300,80,240,36,s);
	obj->name="LvlFile";
	root->addChild(obj);

    obj=new GUIButton(imageManager,renderer,root->width*0.3,200-44,-1,36,_("OK"),0,true,true,GUIGravityCenter);
	obj->name="cfgAddOK";
	obj->eventCallback=this;
	root->addChild(obj);
    obj=new GUIButton(imageManager,renderer,root->width*0.7,200-44,-1,36,_("Cancel"),0,true,true,GUIGravityCenter);
	obj->name="cfgAddCancel";
	obj->eventCallback=this;
	root->addChild(obj);
	
    //Dim the screen using the tempSurface.
	//NOTE: We don't need to store a pointer since it will auto cleanup itself.
    new GUIOverlay(renderer,root);
}

void LevelEditSelect::moveLevel(ImageManager& imageManager,SDL_Renderer& renderer){
	//Open a message popup.
    GUIObject* root=new GUIFrame(imageManager,renderer,(SCREEN_WIDTH-600)/2,(SCREEN_HEIGHT-200)/2,600,200,_("Move level"));
	GUIObject* obj;
	
    obj=new GUILabel(imageManager,renderer,40,60,240,36,_("Level: "));
	root->addChild(obj);
	
    obj=new GUITextBox(imageManager,renderer,300,60,240,36,"1");
	obj->name="MoveLevel";
	root->addChild(obj);
	
    obj=new GUISingleLineListBox(imageManager,renderer,root->width*0.5,110,240,36,true,true,GUIGravityCenter);
	obj->name="lstPlacement";
	vector<string> v;
	v.push_back(_("Before"));
	v.push_back(_("After"));
	v.push_back(_("Swap"));
	(dynamic_cast<GUISingleLineListBox*>(obj))->addItems(v);
	obj->value=0;
	root->addChild(obj);
	
    obj=new GUIButton(imageManager,renderer,root->width*0.3,200-44,-1,36,_("OK"),0,true,true,GUIGravityCenter);
	obj->name="cfgMoveOK";
	obj->eventCallback=this;
	root->addChild(obj);
    obj=new GUIButton(imageManager,renderer,root->width*0.7,200-44,-1,36,_("Cancel"),0,true,true,GUIGravityCenter);
	obj->name="cfgMoveCancel";
	obj->eventCallback=this;
	root->addChild(obj);
	
	//Create the gui overlay.
	//NOTE: We don't need to store a pointer since it will auto cleanup itself.
    new GUIOverlay(renderer,root);
}

void LevelEditSelect::refresh(ImageManager& imageManager, SDL_Renderer& renderer, bool change){
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
            numbers.emplace_back(imageManager, renderer);
		}
	}
	
	for(int n=0;n<m;n++){
		SDL_Rect box={(n%LEVELS_PER_ROW)*64+80,(n/LEVELS_PER_ROW)*64+184,0,0};
        numbers[n].init(renderer,n,box);
	}
	SDL_Rect box={(m%LEVELS_PER_ROW)*64+80,(m/LEVELS_PER_ROW)*64+184,0,0};
    numbers[m].init(renderer,"+",box,m);

	m++; //including the "+" button
	if(m>LEVELS_DISPLAYED_IN_SCREEN){
		levelScrollBar->maxValue=(m-LEVELS_DISPLAYED_IN_SCREEN+LEVELS_PER_ROW-1)/LEVELS_PER_ROW;
		levelScrollBar->visible=true;
	}else{
		levelScrollBar->maxValue=0;
		levelScrollBar->visible=false;
	}
	if(!levels->levelpackDescription.empty())
		levelpackDescription->caption=_CC(levels->getDictionaryManager(),levels->levelpackDescription);
	else
		levelpackDescription->caption="";
}

void LevelEditSelect::selectNumber(ImageManager& imageManager, SDL_Renderer& renderer, unsigned int number, bool selected){
	if (selected) {
		if (number >= 0 && number < levels->getLevelCount()) {
			levels->setCurrentLevel(number);
			setNextState(STATE_LEVEL_EDITOR);
		} else {
			addLevel(imageManager, renderer);
		}
	}else{
		move->enabled = false;
		remove->enabled = false;
		edit->enabled = false;
		selectedNumber = NULL;
		if (number == numbers.size() - 1){
			if (isKeyboardOnly) {
				selectedNumber = &numbers[number];
			} else {
				addLevel(imageManager, renderer);
			}
		} else if (number >= 0 && number < levels->getLevelCount()) {
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

void LevelEditSelect::handleEvents(ImageManager& imageManager, SDL_Renderer& renderer){
	//Call handleEvents() of base class.
	LevelSelect::handleEvents(imageManager, renderer);

	if (section == 3) {
		//Check focus movement
		if (inputMgr.isKeyDownEvent(INPUTMGR_RIGHT)){
			isKeyboardOnly = true;
			section2++;
		} else if (inputMgr.isKeyDownEvent(INPUTMGR_LEFT)){
			isKeyboardOnly = true;
			section2--;
		} else if (inputMgr.isKeyDownEvent(INPUTMGR_UP)){
			isKeyboardOnly = true;
			section2 -= 3;
		} else if (inputMgr.isKeyDownEvent(INPUTMGR_DOWN)){
			isKeyboardOnly = true;
			section2 += 3;
		}
		if (section2 > 6) section2 -= 6;
		else if (section2 < 1) section2 += 6;

		//Check if enter is pressed
		if (isKeyboardOnly && inputMgr.isKeyDownEvent(INPUTMGR_SELECT) && section2 >= 1 && section2 <= 6) {
			GUIButton *buttons[6] = {
				newPack, propertiesPack, removePack, move, remove, edit
			};
			GUIEventCallback_OnEvent(imageManager, renderer, buttons[section2 - 1]->name, buttons[section2 - 1], GUIEventClick);
		}
	}
}

void LevelEditSelect::render(ImageManager& imageManager,SDL_Renderer &renderer){
	//Let the levelselect render.
    LevelSelect::render(imageManager,renderer);

	//Draw highlight in keyboard only mode.
	if (isKeyboardOnly) {
		GUIButton *buttons[6] = {
			newPack, propertiesPack, removePack, move, remove, edit
		};
		for (int i = 0; i < 6; i++) {
			buttons[i]->state = (section == 3 && section2 - 1 == i) ? 1 : 0;
		}
	}
}

void LevelEditSelect::resize(ImageManager& imageManager, SDL_Renderer &renderer){
	//Let the levelselect resize.
    LevelSelect::resize(imageManager, renderer);
	
	//Create the GUI.
    createGUI(imageManager,renderer, false);
	
	//NOTE: This is a workaround for buttons failing when resizing.
	if(packName=="Custom Levels"){
		removePack->enabled=false;
		propertiesPack->enabled=false;
	}
	if(selectedNumber)
        selectNumber(imageManager, renderer, selectedNumber->getNumber(),false);
	
}

void LevelEditSelect::renderTooltip(SDL_Renderer& renderer,unsigned int number,int dy){

    if (!toolTip.name || toolTip.number != number) {
		SDL_Color fg = objThemes.getTextColor(true);
        toolTip.number = number;

		if (number < (unsigned int)levels->getLevelCount()){
			//Render the name of the level.
			toolTip.name = textureFromText(renderer, *fontText, _CC(levels->getDictionaryManager(), levels->getLevelName(number)), fg);
		} else {
			//Add level button
			toolTip.name = textureFromText(renderer, *fontText, _("Add level"), fg);
		}
    }
	
	//Check if name isn't null.
    if(!toolTip.name)
		return;
	
	//Now draw a square the size of the three texts combined.
	SDL_Rect r=numbers[number].box;
	r.y-=dy*64;
    const SDL_Rect nameSize = rectFromTexture(*toolTip.name);
    r.w=nameSize.w;
    r.h=nameSize.h;
	
	//Make sure the tooltip doesn't go outside the window.
	if(r.y>SCREEN_HEIGHT-200){
        r.y-=nameSize.h+4;
	}else{
		r.y+=numbers[number].box.h+2;
	}
	if(r.x+r.w>SCREEN_WIDTH-50)
		r.x=SCREEN_WIDTH-50-r.w;
	
	//Draw a rectange
	Uint32 color=0xFFFFFFFF;
    drawGUIBox(r.x-5,r.y-5,r.w+10,r.h+10,renderer,color);
	
	//Calc the position to draw.
	SDL_Rect r2=r;
	
	//Now we render the name if the surface isn't null.
    if(toolTip.name){
		//Draw the name.
        applyTexture(r2.x, r2.y, toolTip.name, renderer);
	}
}

void LevelEditSelect::GUIEventCallback_OnEvent(ImageManager& imageManager, SDL_Renderer& renderer, std::string name,GUIObject* obj,int eventType){
	//NOTE: We check for the levelpack change to enable/disable some levelpack buttons.
	if(name=="cmdLvlPack"){
		//We call changepack and return to prevent the LevelSelect to undo what we did.
		changePack();
        refresh(imageManager, renderer);
		return;
	}
	
	//Let the level select handle his GUI events.
    LevelSelect::GUIEventCallback_OnEvent(imageManager,renderer,name,obj,eventType);
	
	//Check for the edit button.
	if(name=="cmdNewLvlpack"){
		//Create a new pack.
        packProperties(imageManager,renderer, true);
	}else if(name=="cmdLvlpackProp"){
		//Show the pack properties.
        packProperties(imageManager,renderer, false);
	}else if(name=="cmdRmLvlpack"){
		//Show an "are you sure" message.
        if(msgBox(imageManager,renderer,tfm::format(_("Are you sure remove the level pack '%s'?"),packName),MsgBoxYesNo,_("Remove prompt"))==MsgBoxYes){
			//Remove the directory.
			if(!removeDirectory(levels->levelpackPath.c_str())){
				cerr<<"ERROR: Unable to remove levelpack directory "<<levels->levelpackPath<<endl;
			}
			
			//Remove it from the vector (levelpack list).
			vector<pair<string,string> >::iterator it;
			for(it=levelpacks->item.begin();it!=levelpacks->item.end();++it){
				if(it->second==packName)
					levelpacks->item.erase(it);
			}
			
			//Remove it from the levelpackManager.
			getLevelPackManager()->removeLevelPack(levels->levelpackPath);
			
			//And call changePack.
			levelpacks->value=levelpacks->item.size()-1;
			changePack();
            refresh(imageManager, renderer);
		}
	}else if(name=="cmdMoveMap"){
		if(selectedNumber!=NULL){
            moveLevel(imageManager,renderer);
		}
	}else if(name=="cmdRmMap"){
		if(selectedNumber!=NULL){
			//Show an "are you sure" message.
			if (msgBox(imageManager, renderer, tfm::format(_("Are you sure remove the map '%s'?"), levels->getLevel(selectedNumber->getNumber())->name), MsgBoxYesNo, _("Remove prompt")) != MsgBoxYes) {
				return;
			}
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
            refresh(imageManager, renderer);
		}
	}else if(name=="cmdEdit"){
		if(selectedNumber!=NULL){
			levels->setCurrentLevel(selectedNumber->getNumber());
			setNextState(STATE_LEVEL_EDITOR);
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
						getLevelPackManager()->removeLevelPack(levelpacks->getName());
						
						//And the levelpack list.
						vector<pair<string,string> >::iterator it1;
						for(it1=levelpacks->item.begin();it1!=levelpacks->item.end();++it1){
							if(it1!=levelpacks->item.end()){
								levelpacks->item.erase(it1);
								break;
							}
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
					levelpacks->addItem(levels->levelpackPath,GUIObjectRoot->childControls[i]->caption);
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
			if (GUIObjectRoot->childControls[i]->name == "LvlpackMusic"){
				levels->levelpackMusicList = GUIObjectRoot->childControls[i]->caption;
			}
		}
		//Refresh the leveleditselect to show the correct information.
        refresh(imageManager, renderer);
		
		//Save the configuration.
		levels->saveLevels(getUserPath(USER_DATA)+"custom/levelpacks/"+packName+"/levels.lst");
		getSettings()->setValue("lastlevelpack",levels->levelpackPath);
		
		//Clear the gui.
		if(GUIObjectRoot){
			delete GUIObjectRoot;
			GUIObjectRoot=NULL;
		}
	}else if(name=="cfgCancel"){
		//Check if packName is empty, if so it was a new levelpack and we need to revert to an existing one.
		if(packName.empty()){
			packName=levelpacks->item[levelpacks->value].second;
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
                    msgBox(imageManager,renderer,_("No file name given for the new level."),MsgBoxOKOnly,_("Missing file name"));
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
                        currentState->render(imageManager,renderer);
                        levelEditGUIObjectRoot->render(renderer);
						
						//Notify the user.
                        msgBox(imageManager,renderer,string("The file "+tmp_caption+" already exists."),MsgBoxOKOnly,"Error");
						return;
					}
					
					if(!createFile(path.c_str())){
						cerr<<"ERROR: Unable to create level file "<<path<<endl;
					}else{
						//Update statistics.
						statsMgr.newAchievement("create1");
						if((++statsMgr.createdLevels)>=50) statsMgr.newAchievement("create50");
					}
					levels->addLevel(path);
					//NOTE: Also add the level to the levels levelpack in case of custom levels.
					if(packName=="Custom Levels"){
						LevelPack* levelsPack=getLevelPackManager()->getLevelPack("Levels/");
						if(levelsPack){
							levelsPack->addLevel(path);
							levelsPack->setLocked(levelsPack->getLevelCount()-1);
						}else{
							cerr<<"ERROR: Unable to add level to Levels levelpack"<<endl;
						}
					}
					if(packName!="Custom Levels")
						levels->saveLevels(getUserPath(USER_DATA)+"custom/levelpacks/"+packName+"/levels.lst");
                    refresh(imageManager, renderer);
					
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
                    msgBox(imageManager,renderer,_("The entered level number isn't valid!"),MsgBoxOKOnly,_("Illegal number"));
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
			
        refresh(imageManager, renderer);
		
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
