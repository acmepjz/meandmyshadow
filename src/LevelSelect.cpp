/*
 * Copyright (C) 2011-2013 Me and My Shadow
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

#include "GameState.h"
#include "Functions.h"
#include "FileManager.h"
#include "Globals.h"
#include "LevelSelect.h"
#include "GUIObject.h"
#include "GUIListBox.h"
#include "GUIScrollBar.h"
#include "InputManager.h"
#include <stdio.h>
#include <string>
#include <sstream>
#include <iostream>

#include "libs/tinyformat/tinyformat.h"

using namespace std;

////////////////////NUMBER////////////////////////
Number::Number(ImageManager& imageManager, SDL_Renderer& renderer){
	image=NULL;
	number=0;
	medal=0;
	selected=false;
	locked=false;
	
	//Set the default dimensions.
	box.x=0;
	box.y=0;
	box.h=50;
	box.w=50;
	
	//Load the medals image.	
    medals=imageManager.loadTexture(getDataPath()+"gfx/medals.png", renderer);
    //To make sure it can be added to a vector, stop here rather than generate a massive error.
    static_assert(std::is_move_constructible<Number>::value, "Not move constructable!");
}

void Number::init(SDL_Renderer& renderer,int number,SDL_Rect box){
	//First set the number and update our status.
	this->number=number;

	//Write our text, number+1 since the counting doens't start with 0, but with 1.
	std::stringstream text;
	number++;
	text<<number;

	//Create the text image.
    const SDL_Color black={0,0,0,0};
	//Create the text image.
	//Also check which font to use, if the number is higher than 100 use the small font.
    image = textureFromText(renderer,*fontGUI,text.str().c_str(),black);

	//Set the new location of the number.
	this->box.x=box.x;
	this->box.y=box.y;
	
	//Load background blocks.
	objThemes.getBlock(TYPE_BLOCK,true)->createInstance(&block);
	block.changeState("unlocked");
	
	objThemes.getBlock(TYPE_SHADOW_BLOCK,true)->createInstance(&blockLocked);
	blockLocked.changeState("locked");
}

void Number::init(SDL_Renderer& renderer,std::string text,SDL_Rect box){
	//First set the number and update our status.
	this->number=-1;

	//Create the text image.
	SDL_Color black={0,0,0};
    image = textureFromText(renderer,*fontGUI,text.c_str(),black);

	//Set the new location of the number.
	this->box.x=box.x;
	this->box.y=box.y;
	
	//Load background blocks.
	objThemes.getBlock(TYPE_BLOCK,true)->createInstance(&block);
	block.changeState("unlocked");
	
	objThemes.getBlock(TYPE_SHADOW_BLOCK,true)->createInstance(&blockLocked);
	blockLocked.changeState("locked");
}

void Number::show(SDL_Renderer& renderer, int dy){
	//First draw the background, also apply the yOffset(dy).
	if(!locked)
        block.draw(renderer,box.x,box.y-dy);
	else
        blockLocked.draw(renderer,box.x,box.y-dy);
	//Now draw the text image over the background.
	//We draw it centered inside the box.
    applyTexture(box.x+25-(textureWidth(*image)/2),box.y-dy,image,renderer);

	//Draw the selection mark.
	if(selected){
        drawGUIBox(box.x,box.y-dy,50,50,renderer,0xFFFFFF23);
	}
	
	//Draw the medal.
    if(medal>0&&medals){
        const SDL_Rect srcRect={(medal-1)*30,0,30,30};
        const SDL_Rect dstRect={box.x+30,(box.y+30)-dy,30,30};
        SDL_RenderCopy(&renderer,medals.get(),&srcRect,&dstRect);
	}
}

void Number::setLocked(bool locked){
	this->locked=locked;
}

void Number::setMedal(int medal){
	this->medal=medal;
}


/////////////////////LEVEL SELECT/////////////////////
LevelSelect::LevelSelect(ImageManager& imageManager,SDL_Renderer& renderer, const char* titleText, LevelPackManager::LevelPackLists packType){
	//clear the selected level
	selectedNumber=NULL;
	
	//Calculate the LEVELS_PER_ROW and LEVEL_ROWS if they aren't calculated already.
	calcRows();
	
	//Render the title.
    title=textureFromText(renderer,*fontTitle,titleText,themeTextColor);

	//create GUI (test only)
	GUIObject* obj;
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}

    GUIObjectRoot=new GUIObject(imageManager,renderer,0,0,SCREEN_WIDTH,SCREEN_HEIGHT);

	//the level select scroll bar
    levelScrollBar=new GUIScrollBar(imageManager,renderer,SCREEN_WIDTH*0.9,184,16,SCREEN_HEIGHT-344,ScrollBarVertical,0,0,0,1,4,true,false);
	GUIObjectRoot->addChild(levelScrollBar);

	//level pack description
    levelpackDescription=new GUILabel(imageManager,renderer,0,140,SCREEN_WIDTH,32,"",0,true,true,GUIGravityCenter);
	GUIObjectRoot->addChild(levelpackDescription);

    levelpacks=new GUISingleLineListBox(imageManager,renderer,(SCREEN_WIDTH-500)/2,104,500,32);
	levelpacks->name="cmdLvlPack";
	levelpacks->eventCallback=this;
	vector<pair<string,string> > v=getLevelPackManager()->enumLevelPacks(packType);
	levelpacks->addItems(v);
	levelpacks->value=0;

	//Check if we can find the lastlevelpack.
	for(vector<pair<string,string> >::iterator i=v.begin(); i!=v.end(); ++i){
		if(i->first==getSettings()->getValue("lastlevelpack")){
			levelpacks->value=i-v.begin();
		}
	}

	//Load the progress.
	levels=getLevelPackManager()->getLevelPack(v[levelpacks->value].first);
	levels->loadProgress();
	
	//And add the levelpack single line listbox to the GUIObjectRoot.
	GUIObjectRoot->addChild(levelpacks);
	
    obj=new GUIButton(imageManager,renderer,20,20,-1,32,_("Back"));
	obj->name="cmdBack";
	obj->eventCallback=this;
	GUIObjectRoot->addChild(obj);
	
	section=1;
}

LevelSelect::~LevelSelect(){
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}
	levelScrollBar=NULL;
	levelpackDescription=NULL;
	
	selectedNumber=NULL;
}

void LevelSelect::calcRows(){
	//Calculate the number of rows and the number of levels per row.
	LEVELS_PER_ROW=(SCREEN_WIDTH*0.8)/64;
	int LEVEL_ROWS=(SCREEN_HEIGHT-344)/64;
	LEVELS_DISPLAYED_IN_SCREEN=LEVELS_PER_ROW*LEVEL_ROWS;
}

void LevelSelect::selectNumberKeyboard(ImageManager& imageManager, SDL_Renderer& renderer, int x,int y){
	if(section==2){
		//Move selection
		int realNumber=0;
		if(selectedNumber)
			realNumber=selectedNumber->getNumber()+x+(y*LEVELS_PER_ROW);
		
		//If selection is outside of the map grid, change section
		if(realNumber<0 || realNumber>(int)numbers.size()-1){
			section=1;
			for(int i=0;i<levels->getLevelCount();i++){
				numbers[i].selected=false;
                refresh(imageManager, renderer);
			}
		}else{
			//If not, move selection
			if(!numbers[realNumber].getLocked()){
				for(int i=0;i<levels->getLevelCount();i++){
					numbers[i].selected=(i==realNumber);
				}
                selectNumber(imageManager,renderer,realNumber,false);
			}
		}
	}else if(section==1){
		//Loop through levelpacks and update GUI
		levelpacks->value+=x;
		
		if(levelpacks->value<0){
			levelpacks->value=levelpacks->item.size()-1;
		}else if(levelpacks->value>(int)levelpacks->item.size()-1){
			levelpacks->value=0;
		}
		
        GUIEventCallback_OnEvent(imageManager,renderer,"cmdLvlPack",static_cast<GUIObject*>(levelpacks),0);
		
		//If up is pressed, change section
		if(y==1){
			section=2;
            selectNumber(imageManager,renderer,0,false);
			numbers[0].selected=true;
		}
	}else{
		section=clamp(section+y,0,2);
	}
}

void LevelSelect::handleEvents(ImageManager& imageManager, SDL_Renderer& renderer){
	//Check for an SDL_QUIT event.
	if(event.type==SDL_QUIT){
		setNextState(STATE_EXIT);
	}
	
	//Check for a mouse click.
	if(event.type==SDL_MOUSEBUTTONUP && event.button.button==SDL_BUTTON_LEFT){
        checkMouse(imageManager, renderer);
	}
	
	//Check focus movement
	if(inputMgr.isKeyDownEvent(INPUTMGR_RIGHT)){
        selectNumberKeyboard(imageManager, renderer, 1,0);
	}else if(inputMgr.isKeyDownEvent(INPUTMGR_LEFT)){
        selectNumberKeyboard(imageManager, renderer, -1,0);
	}else if(inputMgr.isKeyDownEvent(INPUTMGR_UP)){
        selectNumberKeyboard(imageManager, renderer, 0,-1);
	}else if(inputMgr.isKeyDownEvent(INPUTMGR_DOWN)){
        selectNumberKeyboard(imageManager, renderer, 0,1);
	}
	
	//Check if enter is pressed
	if(section==2 && inputMgr.isKeyUpEvent(INPUTMGR_SELECT)){
        selectNumber(imageManager,renderer,selectedNumber->getNumber(),true);
	}
	
	//Check if escape is pressed.
	if(inputMgr.isKeyUpEvent(INPUTMGR_ESCAPE)){
		setNextState(STATE_MENU);
	}
	
	//Check for scrolling down and up.
//	if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button==SDL_BUTTON_WHEELDOWN && levelScrollBar){
	if(event.type==SDL_MOUSEWHEEL && levelScrollBar){
			if(levelScrollBar->value<levelScrollBar->maxValue) {
				//TODO - tweak the scroll amount
				levelScrollBar->value += event.wheel.y;
                if(levelScrollBar->value < 0) {
                    levelScrollBar->value = 0;
                }
			}
	}
//		return;
//	}else if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button==SDL_BUTTON_WHEELUP && levelScrollBar){
//		if(levelScrollBar->value>0) levelScrollBar->value--;
//		return;
//	}
}

void LevelSelect::checkMouse(ImageManager &imageManager, SDL_Renderer &renderer){
	int x,y,dy=0,m=numbers.size();
	
	//Get the current mouse location.
	SDL_GetMouseState(&x,&y);

	//Check if there's a scrollbar, if so get the value.
	if(levelScrollBar)
		dy=levelScrollBar->value;
	//Upper bound of levels we'd like to display.
	if(m>dy*LEVELS_PER_ROW+LEVELS_DISPLAYED_IN_SCREEN)
		m=dy*LEVELS_PER_ROW+LEVELS_DISPLAYED_IN_SCREEN;
	y+=dy*64;

	SDL_Rect mouse={x,y,0,0};

	for(int n=dy*LEVELS_PER_ROW; n<m; n++){
		if(!numbers[n].getLocked()){
			if(checkCollision(mouse,numbers[n].box)==true){
				if(numbers[n].selected){
                    selectNumber(imageManager, renderer, n,true);
				}else{
					//Select current level
					for(int i=0;i<levels->getLevelCount();i++){
						numbers[i].selected=(i==n);
					}
                    selectNumber(imageManager, renderer,n,false);
				}
				section=2;
				break;
			}
		}
	}
}

void LevelSelect::logic(ImageManager&, SDL_Renderer&){}

void LevelSelect::render(ImageManager&, SDL_Renderer& renderer){
	int x,y,dy=0,m=numbers.size();
	int idx=-1;
	
	//Get the current mouse location.
	SDL_GetMouseState(&x,&y);

	if(levelScrollBar)
		dy=levelScrollBar->value;
	//Upper bound of levels we'd like to display.
	if(m>dy*LEVELS_PER_ROW+LEVELS_DISPLAYED_IN_SCREEN)
		m=dy*LEVELS_PER_ROW+LEVELS_DISPLAYED_IN_SCREEN;
	y+=dy*64;

	SDL_Rect mouse={x,y,0,0};

	//Draw background.
    objThemes.getBackground(true)->draw(renderer);
	objThemes.getBackground(true)->updateAnimation();
	//Draw the title.
    drawTitleTexture(SCREEN_WIDTH, *title, renderer);

	//Loop through the level blocks and draw them.
	for(int n=dy*LEVELS_PER_ROW;n<m;n++){
        numbers[n].show(renderer,dy*64);
		if(numbers[n].getLocked()==false && checkCollision(mouse,numbers[n].box)==true)
			idx=n;
	}
	
	//Show the tool tip text.
	if(idx>=0){
        renderTooltip(renderer,idx,dy);
	}
}

void LevelSelect::resize(ImageManager& imageManager,SDL_Renderer& renderer){
	calcRows();
    refresh(imageManager,renderer,false);
	
	//NOTE: We don't need to recreate the listbox and the back button, only resize the list.
	levelpacks->left=(SCREEN_WIDTH-500)/2;
	levelpackDescription->width = SCREEN_WIDTH;
}

void LevelSelect::GUIEventCallback_OnEvent(ImageManager& imageManager, SDL_Renderer& renderer, std::string name,GUIObject* obj,int eventType){
	if(name=="cmdLvlPack"){
		getSettings()->setValue("lastlevelpack",static_cast<GUISingleLineListBox*>(obj)->item[obj->value].first);
	}else if(name=="cmdBack"){
		setNextState(STATE_MENU);
		return;
	}else{
		return;
	}

	//new: reset the level list scroll bar
	if(levelScrollBar)
		levelScrollBar->value=0;

	levels=getLevelPackManager()->getLevelPack(static_cast<GUISingleLineListBox*>(obj)->item[obj->value].first);
	
	//Load the progress file.
	levels->loadProgress();
	
	//And refresh the numbers.
    refresh(imageManager, renderer);
}
