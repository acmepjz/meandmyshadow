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

#include "LevelPlaySelect.h"
#include "GameState.h"
#include "Functions.h"
#include "FileManager.h"
#include "Globals.h"
#include "LevelSelect.h"
#include "GUIObject.h"
#include "GUIListBox.h"
#include "GUIScrollBar.h"
#include "InputManager.h"
#include "ThemeManager.h"
#include "SoundManager.h"
#include "StatisticsManager.h"
#include "Game.h"
#include <stdio.h>
#include <string>
#include <sstream>
#include <iostream>

/////////////////////LEVEL SELECT/////////////////////
LevelPlaySelect::LevelPlaySelect(ImageManager& imageManager, SDL_Renderer& renderer)
    :LevelSelect(imageManager,renderer,_("Select Level")),
      levelInfoRender(imageManager,renderer,getDataPath(),*fontText,objThemes.getTextColor(false)){
	//Load the play button if needed.
    playButtonImage=imageManager.loadTexture(getDataPath()+"gfx/playbutton.png", renderer);
	
	//Create the gui.
    createGUI(imageManager,renderer, true);
	
	//Show level list
    refresh(imageManager,renderer);
}

LevelPlaySelect::~LevelPlaySelect(){
	play=NULL;
	
	//Clear the selected level.
	if(selectedNumber!=NULL){
		delete selectedNumber;
		selectedNumber=NULL;
	}
}

void LevelPlaySelect::createGUI(ImageManager& imageManager,SDL_Renderer &renderer, bool initial){
	//Create the play button.
	if(initial){
        play=new GUIButton(imageManager,renderer,SCREEN_WIDTH-240,SCREEN_HEIGHT-60,240,32,_("Play"));
	}else{
		play->left=SCREEN_WIDTH-240;
		play->top=SCREEN_HEIGHT-60;
	}
	play->name="cmdPlay";
	play->eventCallback=this;
	play->enabled=false;
	if(initial)
		GUIObjectRoot->addChild(play);
}

void LevelPlaySelect::refresh(ImageManager& imageManager, SDL_Renderer& renderer, bool /*change*/){
	const int m=levels->getLevelCount();
	numbers.clear();
    levelInfoRender.resetText(renderer, *fontText, objThemes.getTextColor(false));

	//Create the non selected number.
	if (selectedNumber == NULL){
		selectedNumber = new Number(imageManager, renderer);
	}
	SDL_Rect box={40,SCREEN_HEIGHT-130,50,50};
    selectedNumber->init(renderer," ",box);
	selectedNumber->setLocked(true);
	selectedNumber->setMedal(0);
	
	bestTimeFilePath.clear();
	bestRecordingFilePath.clear();	
	
	//Disable the play button.
	play->enabled=false;

	for(int n=0; n<m; n++){
        numbers.emplace_back(imageManager, renderer);
	}

	for(int n=0; n<m; n++){
        SDL_Rect box={(n%LEVELS_PER_ROW)*64+static_cast<int>(SCREEN_WIDTH*0.2)/2,(n/LEVELS_PER_ROW)*64+184,0,0};
        numbers[n].init(renderer,n,box);
		numbers[n].setLocked(levels->getLocked(n));
		int medal=levels->getLevel(n)->won;
		if(medal){
			if(levels->getLevel(n)->targetTime<0 || levels->getLevel(n)->time<=levels->getLevel(n)->targetTime)
				medal++;
			if(levels->getLevel(n)->targetRecordings<0 || levels->getLevel(n)->recordings<=levels->getLevel(n)->targetRecordings)
				medal++;
		}
		numbers[n].setMedal(medal);
	}

	if(m>LEVELS_DISPLAYED_IN_SCREEN){
		levelScrollBar->maxValue=(m-LEVELS_DISPLAYED_IN_SCREEN+(LEVELS_PER_ROW-1))/LEVELS_PER_ROW;
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

void LevelPlaySelect::selectNumber(ImageManager& imageManager, SDL_Renderer& renderer, unsigned int number,bool selected){
	if (selected) {
		if (number >= 0 && number < levels->getLevelCount()) {
			levels->setCurrentLevel(number);
			setNextState(STATE_GAME);
		}
	}else{
        displayLevelInfo(imageManager, renderer,number);
	}
}

void LevelPlaySelect::checkMouse(ImageManager &imageManager, SDL_Renderer &renderer){
	int x,y;
	
	//Get the current mouse location.
	SDL_GetMouseState(&x,&y);
	
	//Check if we should replay the record.
	if(selectedNumber!=NULL){
		SDL_Rect mouse={x,y,0,0};
		if(!bestTimeFilePath.empty()){
			SDL_Rect box={SCREEN_WIDTH-420,SCREEN_HEIGHT-130,372,32};
			if(checkCollision(box,mouse)){
				Game::recordFile=bestTimeFilePath;
				levels->setCurrentLevel(selectedNumber->getNumber());
				setNextState(STATE_GAME);
				return;
			}
		}
		if(!bestRecordingFilePath.empty()){
			SDL_Rect box={SCREEN_WIDTH-420,SCREEN_HEIGHT-98,372,32};
			if(checkCollision(box,mouse)){
				Game::recordFile=bestRecordingFilePath;
				levels->setCurrentLevel(selectedNumber->getNumber());
				setNextState(STATE_GAME);
				return;
			}
		}
	}
	
	//Call the base method from the super class.
    LevelSelect::checkMouse(imageManager, renderer);
}

void LevelPlaySelect::displayLevelInfo(ImageManager& imageManager, SDL_Renderer& renderer, int number){
	//Update currently selected level
	if(selectedNumber==NULL){
        selectedNumber=new Number(imageManager, renderer);
	}
	SDL_Rect box={40,SCREEN_HEIGHT-130,50,50};

	if (number >= 0 && number < levels->getLevelCount()) {
		selectedNumber->init(renderer, number, box);
		selectedNumber->setLocked(false);

		//Show level medal
		int medal = levels->getLevel(number)->won;
		int time = levels->getLevel(number)->time;
		int targetTime = levels->getLevel(number)->targetTime;
		int recordings = levels->getLevel(number)->recordings;
		int targetRecordings = levels->getLevel(number)->targetRecordings;

		if (medal){
			if (targetTime < 0){
				medal = -1;
			} else{
				if (targetTime < 0 || time <= targetTime)
					medal++;
				if (targetRecordings < 0 || recordings <= targetRecordings)
					medal++;
			}
		}
		selectedNumber->setMedal(medal);
		std::string levelTime;
		std::string levelRecs;

		//Show best time and recordings
		if (medal){
			char s[64];

			if (time > 0)
				if (targetTime>0)
					sprintf(s, "%-.2fs / %-.2fs", time / 40.0f, targetTime / 40.0f);
				else
					sprintf(s, "%-.2fs / -", time / 40.0f);
			else
				s[0] = '\0';
			levelTime = s;

			if (recordings >= 0)
				if (targetRecordings >= 0)
					sprintf(s, "%5d / %d", recordings, targetRecordings);
				else
					sprintf(s, "%5d / -", recordings);
			else
				s[0] = '\0';
			levelRecs = s;
		} else{
			levelTime = "- / -";
			levelRecs = "- / -";
		}

		//Show the play button.
		play->enabled = true;

		//Check if there is auto record file
		levels->getLevelAutoSaveRecordPath(number, bestTimeFilePath, bestRecordingFilePath, false);
		if (!bestTimeFilePath.empty()){
			FILE *f;
			f = fopen(bestTimeFilePath.c_str(), "rb");
			if (f == NULL){
				bestTimeFilePath.clear();
			} else{
				fclose(f);
			}
		}
		if (!bestRecordingFilePath.empty()){
			FILE *f;
			f = fopen(bestRecordingFilePath.c_str(), "rb");
			if (f == NULL){
				bestRecordingFilePath.clear();
			} else{
				fclose(f);
			}
		}

		//Show level description
		const std::string& levelDescription = levels->getLevelName(number);

		levelInfoRender.update(renderer, *fontText, objThemes.getTextColor(false),
			levelDescription, levelTime, levelRecs);
	} else {
		levelInfoRender.resetText(renderer, *fontText, objThemes.getTextColor(false));

		selectedNumber->init(renderer, " ", box);
		selectedNumber->setLocked(true);
		selectedNumber->setMedal(0);

		bestTimeFilePath.clear();
		bestRecordingFilePath.clear();

		//Disable the play button.
		play->enabled = false;
	}
}

void LevelPlaySelect::handleEvents(ImageManager& imageManager, SDL_Renderer& renderer){
	//Call handleEvents() of base class.
	LevelSelect::handleEvents(imageManager, renderer);

	//Check if the cheat code is input which is used to skip locked level.
	//NOTE: The cheat code is NOT in plain text, since we don't want you to find it out immediately.
	//NOTE: If you type it wrong, please press a key which is NOT a-z before retype it (as the code suggests).
	if (event.type == SDL_KEYDOWN) {
		static Uint32 hash = 0;
		if (event.key.keysym.sym >= SDLK_a && event.key.keysym.sym <= SDLK_z) {
			Uint32 c = event.key.keysym.sym - SDLK_a + 1;
			hash = hash * 1296096U + c;
			if (hash == 498506457U) {
				if (selectedNumber) {
					int n = selectedNumber->getNumber();
					if (n >= 0 && n < (int)numbers.size() - 1 && numbers[n + 1].getLocked()) {
						//unlock the level temporarily
						numbers[n + 1].setLocked(false);

						//play a sound effect
						getSoundManager()->playSound("hit");

						//new achievement
						statsMgr.newAchievement("cheat");
					}
				}
				hash = 0;
			}
		} else {
			hash = 0;
		}
	}

	if (section == 3) {
		//Check focus movement
		if (inputMgr.isKeyDownEvent(INPUTMGR_DOWN) || inputMgr.isKeyDownEvent(INPUTMGR_RIGHT)){
			isKeyboardOnly = true;
			section2++;
		} else if (inputMgr.isKeyDownEvent(INPUTMGR_UP) || inputMgr.isKeyDownEvent(INPUTMGR_LEFT)){
			isKeyboardOnly = true;
			section2--;
		}
		if (section2 > 3) section2 = 1;
		else if (section2 < 1) section2 = 3;

		//Check if enter is pressed
		if (inputMgr.isKeyUpEvent(INPUTMGR_SELECT) && selectedNumber) {
			int n = selectedNumber->getNumber();
			if (n >= 0) {
				switch (section2) {
				case 1:
					if (!bestTimeFilePath.empty()) {
						Game::recordFile = bestTimeFilePath;
						levels->setCurrentLevel(n);
						setNextState(STATE_GAME);
					}
					break;
				case 2:
					if (!bestRecordingFilePath.empty()) {
						Game::recordFile = bestRecordingFilePath;
						levels->setCurrentLevel(n);
						setNextState(STATE_GAME);
					}
					break;
				case 3:
					selectNumber(imageManager, renderer, n, true);
					break;
				}
			}
		}
	}
}

void LevelPlaySelect::render(ImageManager& imageManager, SDL_Renderer &renderer){
	//First let the levelselect render.
    LevelSelect::render(imageManager,renderer);
	
	int x,y,dy=0;
	
	//Get the current mouse location.
	SDL_GetMouseState(&x,&y);

	if(levelScrollBar)
		dy=levelScrollBar->value;
	//Upper bound of levels we'd like to display.
	y+=dy*64;
	
	SDL_Rect mouse={x,y,0,0};
	
	//Show currently selected level (if any)
	if(selectedNumber!=NULL){
        selectedNumber->show(renderer, 0);
		
        //Only show the replay button if the level is completed (won).
		if(selectedNumber->getNumber()>=0 && selectedNumber->getNumber()<levels->getLevelCount()) {
			if(levels->getLevel(selectedNumber->getNumber())->won){
				if(!bestTimeFilePath.empty()){

					SDL_Rect r={0,0,32,32};
                    const SDL_Rect box={SCREEN_WIDTH-420,SCREEN_HEIGHT-130,372,32};
					
					if (isKeyboardOnly ? (section == 3 && section2 == 1) : checkCollision(box, mouse)){
						r.x = 32;
						drawGUIBox(box.x, box.y, box.w, box.h, renderer, 0xFFFFFF40);
					}
                    const SDL_Rect dstRect = {SCREEN_WIDTH-80,SCREEN_HEIGHT-130,r.w,r.h};
                    SDL_RenderCopy(&renderer,playButtonImage.get(),&r, &dstRect);
				}
				
				if(!bestRecordingFilePath.empty()){
					SDL_Rect r={0,0,32,32};
                    const SDL_Rect box={SCREEN_WIDTH-420,SCREEN_HEIGHT-98,372,32};
					
					if (isKeyboardOnly ? (section == 3 && section2 == 2) : checkCollision(box, mouse)){
						r.x = 32;
						drawGUIBox(box.x, box.y, box.w, box.h, renderer, 0xFFFFFF40);
					}
					
                    const SDL_Rect dstRect = {SCREEN_WIDTH-80,SCREEN_HEIGHT-98,r.w,r.h};
                    SDL_RenderCopy(&renderer,playButtonImage.get(),&r, &dstRect);
				}
			}
		}

		levelInfoRender.render(renderer);
	}

	//Draw highlight for play button.
	if (isKeyboardOnly && play && play->enabled) {
		play->state = (section == 3 && section2 == 3) ? 1 : 0;
	}
}

void LevelPlaySelect::renderTooltip(SDL_Renderer &renderer, unsigned int number, int dy){
    if (!toolTip.name || toolTip.number != number) {
        const int SLEN = 64;
        char s[SLEN];

        //Render the name of the level.
		toolTip.name = textureFromText(renderer, *fontText, _CC(levels->getDictionaryManager(), levels->getLevelName(number)), objThemes.getTextColor(true));
        toolTip.time=nullptr;
        toolTip.recordings=nullptr;
        toolTip.number=number;

        //The time it took.
        if(levels->getLevel(number)->time>0){
            SDL_snprintf(s,SLEN,"%-.2fs",levels->getLevel(number)->time/40.0f);
			toolTip.time = textureFromText(renderer, *fontText, s, objThemes.getTextColor(true));
        }

        //The number of recordings it took.
        if(levels->getLevel(number)->recordings>=0){
            SDL_snprintf(s,SLEN,"%d",levels->getLevel(number)->recordings);
			toolTip.recordings = textureFromText(renderer, *fontText, s, objThemes.getTextColor(true));
        }
    }
	
    const SDL_Rect nameSize = rectFromTexture(*toolTip.name);
	//Now draw a square the size of the three texts combined.
	SDL_Rect r=numbers[number].box;
	r.y-=dy*64;
    if(toolTip.time && toolTip.recordings){
        const int recW = textureWidth(*toolTip.recordings);
        const int timeW = textureWidth(*toolTip.time);
        r.w=(nameSize.w)>(25+timeW+40+recW)?(nameSize.w):(25+timeW+40+recW);
        r.h=nameSize.h+5+20;
	}else{
        r.w=nameSize.w;
        r.h=nameSize.h;
	}
	
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
	//Increase the height to leave a gap between name and stats.
	r2.y+=30;
    if(toolTip.time){
		//Now draw the time.
        applyTexture(r2.x,r2.y,levelInfoRender.timeIcon,renderer);
		r2.x+=25;
        applyTexture(r2.x, r2.y, toolTip.time, renderer);
        r2.x+=textureWidth(*toolTip.time)+15;
	}
    if(toolTip.recordings){
		//Now draw the recordings.
        applyTexture(r2.x,r2.y,levelInfoRender.recordingsIcon,renderer);
		r2.x+=25;
        applyTexture(r2.x, r2.y, toolTip.recordings, renderer);
	}
}

void LevelPlaySelect::resize(ImageManager &imageManager, SDL_Renderer &renderer){
	//Let the LevelSelect do his stuff.
    LevelSelect::resize(imageManager, renderer);
	
	//Now create our gui again.
    createGUI(imageManager,renderer, false);
}

void LevelPlaySelect::GUIEventCallback_OnEvent(ImageManager& imageManager, SDL_Renderer& renderer, std::string name,GUIObject* obj,int eventType){
	//Let the level select handle his GUI events.
    LevelSelect::GUIEventCallback_OnEvent(imageManager,renderer,name,obj,eventType);
	
	//Check for the play button.
	if(name=="cmdPlay"){
		if(selectedNumber!=NULL){
			levels->setCurrentLevel(selectedNumber->getNumber());
			setNextState(STATE_GAME);
		}
	}
}
