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
#include "GUIOverlay.h"
#include "InputManager.h"
#include "ThemeManager.h"
#include "MD5.h"
#include "SoundManager.h"
#include "StatisticsManager.h"
#include "Game.h"
#include <stdio.h>
#include <string.h>
#include <string>
#include <sstream>
#include <iostream>

#include <SDL_ttf.h>

#include "libs/tinyformat/tinyformat.h"

class ReplayListOverlay : public GUIOverlay {
private:
	GUIListBox *list;

public:
	ReplayListOverlay(SDL_Renderer &renderer, GUIObject* root, GUIListBox *list)
		: GUIOverlay(renderer, root), list(list)
	{
	}

	void handleEvents(ImageManager& imageManager, SDL_Renderer& renderer) override {
		GUIOverlay::handleEvents(imageManager, renderer);

		//Do our own stuff.
		if (!list) return;

		//Check vertical movement
		if (inputMgr.isKeyDownEvent(INPUTMGR_UP)){
			isKeyboardOnly = true;
			list->value--;
			if (list->value < 0) list->value = 0;

			//FIXME: ad-hoc stupid code
			list->scrollScrollbar(0xC0000000);
			list->scrollScrollbar(list->value);
		} else if (inputMgr.isKeyDownEvent(INPUTMGR_DOWN)){
			isKeyboardOnly = true;
			list->value++;
			if (list->value >= (int)list->item.size()) list->value = list->item.size() - 1;

			//FIXME: ad-hoc stupid code
			list->scrollScrollbar(0xC0000000);
			list->scrollScrollbar(list->value);
		}

		if (isKeyboardOnly && list->eventCallback && inputMgr.isKeyDownEvent(INPUTMGR_SELECT) && list->value >= 0 && list->value<(int)list->item.size()) {
			list->eventCallback->GUIEventCallback_OnEvent(imageManager, renderer, list->name, list, GUIEventChange); // ???
		}
	}
};

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
	replayList = NULL;
	
	//Clear the selected level.
	if(selectedNumber!=NULL){
		delete selectedNumber;
		selectedNumber=NULL;
	}
}

void LevelPlaySelect::createGUI(ImageManager& imageManager,SDL_Renderer &renderer, bool initial){
	//Create the play button.
	if(initial){
        play=new GUIButton(imageManager,renderer,SCREEN_WIDTH-60,SCREEN_HEIGHT-60,-1,32,_("Play"),0,true,true,GUIGravityRight);
		replayList = new GUIButton(imageManager, renderer, 60, SCREEN_HEIGHT - 60, -1, 32, _("More replays"), 0, true, true, GUIGravityLeft);
	} else{
		play->left=SCREEN_WIDTH-60;
		play->top=SCREEN_HEIGHT-60;
		play->width = -1;
		replayList->left = 60;
		replayList->top = SCREEN_HEIGHT - 60;
		play->width = -1;
	}
	play->name="cmdPlay";
	play->eventCallback=this;
	play->enabled=false;
	replayList->name = "cmdReplayList";
	replayList->eventCallback = this;
	replayList->enabled = false;
	if (initial) {
		GUIObjectRoot->addChild(play);
		GUIObjectRoot->addChild(replayList);
	}
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
	replayList->enabled = false;

	for(int n=0; n<m; n++){
        numbers.emplace_back(imageManager, renderer);
	}

	for(int n=0; n<m; n++){
        SDL_Rect box={(n%LEVELS_PER_ROW)*64+static_cast<int>(SCREEN_WIDTH*0.2)/2,(n/LEVELS_PER_ROW)*64+184,0,0};
        numbers[n].init(renderer,n,box);
		numbers[n].setLocked(n>0 && levels->getLocked(n));
		int medal = levels->getLevel(n)->getMedal();
		numbers[n].setMedal(medal);
	}

	if(m>LEVELS_DISPLAYED_IN_SCREEN){
		levelScrollBar->maxValue=(m-LEVELS_DISPLAYED_IN_SCREEN+(LEVELS_PER_ROW-1))/LEVELS_PER_ROW;
		levelScrollBar->visible=true;
	}else{
		levelScrollBar->maxValue=0;
		levelScrollBar->visible=false;
	}
	if (levels->levelpackPath == LEVELS_PATH || levels->levelpackPath == CUSTOM_LEVELS_PATH)
		levelpackDescription->caption = _("Individual levels which are not contained in any level packs");
	else if (!levels->levelpackDescription.empty())
		levelpackDescription->caption = _CC(levels->getDictionaryManager(), levels->levelpackDescription);
	else
		levelpackDescription->caption = "";
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
			if(pointOnRect(mouse, box)){
				Game::recordFile=bestTimeFilePath;
				levels->setCurrentLevel(selectedNumber->getNumber());
				setNextState(STATE_GAME);
				return;
			}
		}
		if(!bestRecordingFilePath.empty()){
			SDL_Rect box={SCREEN_WIDTH-420,SCREEN_HEIGHT-98,372,32};
			if(pointOnRect(mouse, box)){
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
		LevelPack::Level *level = levels->getLevel(number);
		int medal = level->getMedal();
		int time = level->time;
		int targetTime = level->targetTime;
		int recordings = level->recordings;
		int targetRecordings = level->targetRecordings;

		selectedNumber->setMedal(medal);

		//Check if there is auto-saved record file
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

		//If there exists any auto-saved record file, and the MD5 of record is not set,
		//then we assume the MD5 is not changed and the record file is updated from old version of MnMS.
		if (!bestTimeFilePath.empty() || !bestRecordingFilePath.empty()) {
			bool b = true;
			for (int i = 0; i < 16; i++) {
				if (level->md5InLevelProgress[i]) {
					b = false;
					break;
				}
			}
			if (b) {
				//Just copy level MD5 to md5InLevelProgress.
				memcpy(level->md5InLevelProgress, level->md5Digest, sizeof(level->md5Digest));
			}
		}

		//Check if MD5 is changed
		bool md5Changed = false;

		for (int i = 0; i < 16; i++) {
			if (level->md5Digest[i] != level->md5InLevelProgress[i]) {
				md5Changed = true;
				break;
			}
		}

		//Show best time and recordings
		std::string levelTime;
		std::string levelRecs;
		if (medal){
			if (time >= 0) {
				if (targetTime >= 0)
					levelTime = tfm::format("%-.2fs / %-.2fs", time / 40.0, targetTime / 40.0);
				else
					levelTime = tfm::format("%-.2fs / -", time / 40.0);
				if (md5Changed) {
					levelTime += " ";
					/// TRANSLATORS: This means best time or recordings are outdated due to level MD5 changed. Please make it short since there are not enough spaces.
					levelTime += _("(old)");
				}
			} else
				levelTime.clear();

			if (recordings >= 0) {
				if (targetRecordings >= 0)
					levelRecs = tfm::format("%5d / %d", recordings, targetRecordings);
				else
					levelRecs = tfm::format("%5d / -", recordings);
				if (md5Changed) {
					levelRecs += " ";
					/// TRANSLATORS: This means best time or recordings are outdated due to level MD5 changed. Please make it short since there are not enough spaces.
					levelRecs += _("(old)");
				}
			} else
				levelRecs.clear();
		} else{
			levelTime = "- / -";
			levelRecs = "- / -";
		}

		//Show the play button.
		play->enabled = true;
		replayList->enabled = true;

		//Show level description
		levelInfoRender.update(renderer, *fontText, objThemes.getTextColor(false),
			_CC(levels->getDictionaryManager(), levels->getLevelName(number)), levelTime, levelRecs);
	} else {
		levelInfoRender.resetText(renderer, *fontText, objThemes.getTextColor(false));

		selectedNumber->init(renderer, " ", box);
		selectedNumber->setLocked(true);
		selectedNumber->setMedal(0);

		bestTimeFilePath.clear();
		bestRecordingFilePath.clear();

		//Disable the play button.
		play->enabled = false;
		replayList->enabled = false;
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
		if (section2 > 4) section2 = 1;
		else if (section2 < 1) section2 = 4;

		//Check if enter is pressed
		if (isKeyboardOnly && inputMgr.isKeyDownEvent(INPUTMGR_SELECT) && selectedNumber) {
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
					displayReplayList(imageManager, renderer, n);
					break;
				case 4:
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

		const int num = selectedNumber->getNumber();
		bool arcade = false;
		
        //Only show the replay button if the level is completed (won).
		if(num>=0 && num<levels->getLevelCount()) {
			auto lev = levels->getLevel(num);
			arcade = lev->arcade;
			if(lev->won){
				if(!bestTimeFilePath.empty()){

					SDL_Rect r={0,0,32,32};
                    const SDL_Rect box={SCREEN_WIDTH-408,SCREEN_HEIGHT-130,360,32};
					
					if (isKeyboardOnly ? (section == 3 && section2 == 1) : pointOnRect(mouse, box)){
						r.x = 32;
						drawGUIBox(box.x, box.y, box.w, box.h, renderer, 0xFFFFFF40);
					}
                    const SDL_Rect dstRect = {SCREEN_WIDTH-80,SCREEN_HEIGHT-130,r.w,r.h};
                    SDL_RenderCopy(&renderer,playButtonImage.get(),&r, &dstRect);
				}
				
				if(!bestRecordingFilePath.empty()){
					SDL_Rect r={0,0,32,32};
                    const SDL_Rect box={SCREEN_WIDTH-408,SCREEN_HEIGHT-98,360,32};
					
					if (isKeyboardOnly ? (section == 3 && section2 == 2) : pointOnRect(mouse, box)){
						r.x = 32;
						drawGUIBox(box.x, box.y, box.w, box.h, renderer, 0xFFFFFF40);
					}
					
                    const SDL_Rect dstRect = {SCREEN_WIDTH-80,SCREEN_HEIGHT-98,r.w,r.h};
                    SDL_RenderCopy(&renderer,playButtonImage.get(),&r, &dstRect);
				}
			}
		}

		levelInfoRender.render(renderer, arcade);
	}

	//Draw highlight for play button.
	if (isKeyboardOnly) {
		if (play && play->enabled) {
			play->state = (section == 3 && section2 == 4) ? 1 : 0;
		}
		if (replayList && replayList->enabled) {
			replayList->state = (section == 3 && section2 == 3) ? 1 : 0;
		}
	}
}

void LevelPlaySelect::renderTooltip(SDL_Renderer &renderer, unsigned int number, int dy){
    if (!toolTip.name || toolTip.number != number) {
        //Render the name of the level.
		toolTip.name = textureFromText(renderer, *fontText, _CC(levels->getDictionaryManager(), levels->getLevelName(number)), objThemes.getTextColor(true));
        toolTip.time=nullptr;
        toolTip.recordings=nullptr;
        toolTip.number=number;

        //The time it took.
        if(levels->getLevel(number)->time>0){
			toolTip.time = textureFromText(renderer, *fontText,
				tfm::format("%-.2fs", levels->getLevel(number)->time / 40.0).c_str(),
				objThemes.getTextColor(true));
        }

        //The number of recordings it took.
        if(levels->getLevel(number)->recordings>=0){
			toolTip.recordings = textureFromText(renderer, *fontText,
				tfm::format("%d", levels->getLevel(number)->recordings).c_str(),
				objThemes.getTextColor(true));
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
		if (levels->getLevel(number)->arcade) {
			levelInfoRender.collectable.draw(renderer, r2.x - 16, r2.y - 16);
		} else {
			applyTexture(r2.x, r2.y, levelInfoRender.recordingsIcon, renderer);
		}
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
		if(selectedNumber){
			levels->setCurrentLevel(selectedNumber->getNumber());
			setNextState(STATE_GAME);
		}
	} else if (name == "cmdReplayList") {
		if (selectedNumber){
			displayReplayList(imageManager, renderer, selectedNumber->getNumber());
		}
	} else if (name == "cmdCancel") {
		if (GUIObjectRoot) {
			delete GUIObjectRoot;
			GUIObjectRoot = NULL;
		}
	} else if (name == "lstReplays") {
		//Check which type of event.
		if (GUIObjectRoot && eventType == GUIEventChange) {
			if (auto list = dynamic_cast<GUIListBox*>(GUIObjectRoot->getChild("lstReplays"))) {
				//Make sure an item is selected.
				if (list->value >= 0 && list->value < (int)list->item.size()) {
					Game::recordFile = list->item[list->value];

					delete GUIObjectRoot;
					GUIObjectRoot = NULL;
				}

				//Deselect it.
				list->value = -1;
			}
		}
	}
}

void LevelPlaySelect::displayReplayList(ImageManager &imageManager, SDL_Renderer &renderer, int number) {
	//Get levelpack autosave record path
	std::string path = levels->getLevelpackAutoSaveRecordPath(false);

	//Get prefix
	std::string prefix = levels->getLevelAutoSaveRecordPrefix(number);

	//Enumerate and filter replays
	std::vector<std::string> files = enumAllFiles(path, "mnmsrec");
	for (int i = files.size() - 1; i >= 0; i--) {
		if (files[i].size() < prefix.size() || files[i].substr(0, prefix.size()) != prefix) {
			files.erase(files.begin() + i);
		}
	}

	if (files.empty()) {
		//There are no replays
		msgBox(imageManager, renderer, _("There are no replays for this level."), MsgBoxOKOnly, _("Error"));
		return;
	}

	const std::string levelMD5 = Md5::toString(levels->getLevelMD5(number));
	const bool levelArcade = levels->getLevel(number)->arcade;

	//Create a root object.
	GUIObject* root = new GUIFrame(imageManager, renderer, (SCREEN_WIDTH - 600) / 2, (SCREEN_HEIGHT - 500) / 2, 600, 500, _("More replays"));

	GUIListBox* list = new GUIListBox(imageManager, renderer, 40, 80, 520, 340);
	list->name = "lstReplays";
	list->eventCallback = this;
	root->addChild(list);

	SDL_Color color = objThemes.getTextColor(true);
	SDL_Color grayed = {
		(Uint8)(128 + color.r / 2),
		(Uint8)(128 + color.g / 2),
		(Uint8)(128 + color.b / 2),
	};

	for (int i = 0, m = files.size(); i < m; i++) {
		std::string s = files[i].substr(prefix.size() + 1, files[i].size() - prefix.size() - 9);

		std::string version;
		bool err = false;
		if (s.size() >= 33 && s[32] == '-') {
			for (int lp = 0; lp < 32; lp++) {
				char c = s[lp];
				if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')) {
					version.push_back(c);
				} else if (c >= 'A' && c <= 'F') {
					version.push_back(c + ('a' - 'A'));
				} else {
					err = true;
					break;
				}
			}
		} else {
			err = true;
		}

		size_t lps = s.find_first_of('-');
		if (err) {
			if (lps == std::string::npos) version.clear();
			else version = s.substr(0, lps);
		}
		s = s.substr(lps + 1);

		if (s == "best-time") {
			s = _("Best time");
		} else if (s == "best-recordings") {
			if (levelArcade) {
				s = _("Best collectibles");
			} else {
				s = _("Best recordings");
			}
		}

		//Create a surface.
		SurfacePtr surf(createSurface(list->width, 48));

		//Create description text.
		{
			SurfacePtr tmp(TTF_RenderUTF8_Blended(fontText, s.c_str(), color));
			SDL_SetSurfaceBlendMode(tmp.get(), SDL_BLENDMODE_NONE);
			applySurface(240, 12, tmp.get(), surf.get(), NULL);
		}

		//Create version text.
		{
			std::string ver;
			if (err) {
				/// TRANSLATORS: This means the replay file has unknown version (file name doesn't contain MD5).
				ver = _("Unknown version");
			} else {
				ver = (version == levelMD5) ?
					/// TRANSLATORS: This means the replay file matches the level (different MD5).
					_("Current version") :
					/// TRANSLATORS: This means the replay file doesn't match the level (different MD5).
					_("Outdated version");
			}

			SurfacePtr tmp(TTF_RenderUTF8_Blended(fontText, ver.c_str(), color));
			SDL_SetSurfaceBlendMode(tmp.get(), SDL_BLENDMODE_NONE);
			applySurface(4, version.empty() ? 12 : 2, tmp.get(), surf.get(), NULL);
		}

		//Create MD5 text.
		if (!version.empty()) {
			SurfacePtr tmp(TTF_RenderUTF8_Blended(fontMono, version.c_str(), grayed));
			SDL_SetSurfaceBlendMode(tmp.get(), SDL_BLENDMODE_NONE);
			applySurface(4, 24, tmp.get(), surf.get(), NULL);
		}

		//Done creating surface
		list->addItem(renderer, path + files[i], textureFromSurface(renderer, std::move(surf)));
	}

	GUIObject* obj = new GUIButton(imageManager, renderer, 300, 500 - 44, -1, 36, _("Close"), 0, true, true, GUIGravityCenter);
	obj->name = "cmdCancel";
	obj->eventCallback = this;
	root->addChild(obj);

	Game::recordFile.clear();

	(new ReplayListOverlay(renderer, root, list))->enterLoop(imageManager, renderer, true, false);

	if (!Game::recordFile.empty()) {
		levels->setCurrentLevel(number);
		setNextState(STATE_GAME);
	}
}
