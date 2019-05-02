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

#include <stdio.h>
#include <string>
#include <vector>
#include <map>
#include "StatisticsManager.h"
#include "StatisticsScreen.h"
#include "Globals.h"
#include "Functions.h"
#include "ThemeManager.h"
#include "InputManager.h"
#include "GUIListBox.h"
#include "GUIScrollBar.h"
#include "EasterEggScreen.h"
#include <SDL_ttf_fontfallback.h>

#include "libs/tinyformat/tinyformat.h"

using namespace std;

//GUI events are handled here.
//name: The name of the element that invoked the event.
//obj: Pointer to the object that invoked the event.
//eventType: Integer containing the type of event.
void StatisticsScreen::GUIEventCallback_OnEvent(ImageManager& imageManager, SDL_Renderer& renderer, std::string name,GUIObject* obj,int eventType){
	//Check what type of event it was.
	if(eventType==GUIEventClick){
		if(name=="cmdBack"){
			//Goto the main menu.
			setNextState(STATE_MENU);
		}
	} else if (eventType == GUIEventChange && name == "lstAchievements") {
		//NOTE: This code is only for testing purpose
		int index = lists[0]->value;
		if (index >= 0) statsMgr.newAchievement(achievementList[index].id, false);
		lists[0]->value = -1;
	}
}

//Constructor.
StatisticsScreen::StatisticsScreen(ImageManager& imageManager, SDL_Renderer& renderer){
	//Update in-game time.
	statsMgr.updatePlayTime();

	//Render the title.
	title = titleTextureFromText(renderer, _("Achievements and Statistics"), objThemes.getTextColor(false), SCREEN_WIDTH);


	//Create GUI.
    createGUI(imageManager, renderer);
}

//Destructor.
StatisticsScreen::~StatisticsScreen(){
	//Delete the GUI.
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}
}

//we are so lazy that we just use height of the first text, ignore the others
#define DRAW_PLAYER_STATISTICS(name,var,fmt) { \
    SurfacePtr surface(TTF_RenderUTF8_Blended(fontGUISmall,name,objThemes.getTextColor(true))); \
    SurfacePtr stats = createSurface(w,surface->h); \
    SDL_FillRect(stats.get(),NULL,-1); \
    applySurface(4,0,surface.get(),stats.get(),NULL); \
    y=surface->h; \
    surface.reset(TTF_RenderUTF8_Blended(fontText, \
		tfm::format(fmt,statsMgr.player##var+statsMgr.shadow##var).c_str(), \
		objThemes.getTextColor(true))); \
    applySurface(w-260-surface->w,(y-surface->h),surface.get(),stats.get(),NULL); \
    surface.reset(TTF_RenderUTF8_Blended(fontText, \
		tfm::format(fmt,statsMgr.player##var).c_str(), \
		objThemes.getTextColor(true))); \
    applySurface(w-140-surface->w,(y-surface->h),surface.get(),stats.get(),NULL); \
    surface.reset(TTF_RenderUTF8_Blended(fontText, \
		tfm::format(fmt,statsMgr.shadow##var).c_str(), \
		objThemes.getTextColor(true))); \
    applySurface(w-20-surface->w,(y-surface->h),surface.get(),stats.get(),NULL); \
    list->addItem(renderer,"",textureFromSurface(renderer, std::move(stats))); /* add it to list box */ \
}

//Add an item to the listbox, that displays "name1", and "var1" formatted with "format"
//we are so lazy that we just use height of the first text, ignore the others
template <class T1>
static void drawMiscStatistics1(SDL_Renderer& renderer, int w,GUIListBox *list,const char* name1,const T1 var1,const char* format1){
	//create new surface
    SurfacePtr nameSurface(TTF_RenderUTF8_Blended(fontGUISmall,name1,objThemes.getTextColor(true)));

    SurfacePtr stats=createSurface(w, nameSurface->h);
    SDL_FillRect(stats.get(),NULL,-1);

    applySurface(4,0,nameSurface.get(),stats.get(),NULL);
    const int x=nameSurface->w+8;
    const int y=nameSurface->h;

    SurfacePtr formatSurface(TTF_RenderUTF8_Blended(fontText,
		tfm::format(format1,var1).c_str(),
		objThemes.getTextColor(true)));
    //NOTE: SDL2 port. Not halving the y value here as this ends up looking better.
    applySurface(x,y-formatSurface->h,formatSurface.get(),stats.get(),NULL);

	//add it to list box
    list->addItem(renderer, "",textureFromSurface(renderer, std::move(stats)));

	//over
    //return stats;
}

//NOTE: Disabled this for the SDL2 port for now. It looks a bit off anyhow.
//Might want to make a more general method that draws as many "cells" as there is space.
//Draws two stats on one line if there is space.
//we are so lazy that we just use height of the first text, ignore the others
/*template <class T1,class T2>
static void drawMiscStatistics2(int w,GUIListBox *list,const char* name1,const T1 var1,const char* format1,const char* name2,const T2 var2,const char* format2){
	SDL_Surface* stats=drawMiscStatistics1(w,list,name1,var1,format1);

	//Check if the width is enough
	if(w>=800){
		//draw name
		SDL_Surface* surface=TTF_RenderUTF8_Blended(fontGUISmall,name2,objThemes.getTextColor(true));
		applySurface(w/2-8,stats->h-surface->h,surface,stats,NULL);
		int x=surface->w+w/2;
		SDL_FreeSurface(surface);

		//draw value
		char s[1024];
		//FIXME: Use tfm::format instead of sprintf to enable locale support
		FIXME_sprintf(s,format2,var2);
		surface=TTF_RenderUTF8_Blended(fontText,s,objThemes.getTextColor(true));
		applySurface(x,(stats->h-surface->h)/2,surface,stats,NULL);
		SDL_FreeSurface(surface);
	}else{
		//Split into two rows
		drawMiscStatistics1(w,list,name2,var2,format2);
	}
}*/

void StatisticsScreen::addAchievements(ImageManager& imageManager, SDL_Renderer &renderer, GUIListBox *list, bool revealUnknownAchievements) {
	for (int idx = 0; achievementList[idx].id != NULL; ++idx) {
		time_t *lpt = NULL;

		map<string, OwnedAchievement>::iterator it = statsMgr.achievements.find(achievementList[idx].id);
		if (it != statsMgr.achievements.end()) {
			lpt = &it->second.achievedTime;
		}

		AchievementInfo info = achievementList[idx];
		if (revealUnknownAchievements) {
			if (info.displayStyle == ACHIEVEMENT_HIDDEN || info.displayStyle == ACHIEVEMENT_TITLE) {
				info.displayStyle = ACHIEVEMENT_ALL;
			}
		}

		SDL_Rect r;
		r.x = r.y = 0;
		r.w = list->width - 16;
		auto surface = statsMgr.createAchievementSurface(renderer, &info, &r, false, lpt);

		if (surface){
			list->addItem(renderer, "", surface);
		}
	}
}

//Method that will create the GUI.
void StatisticsScreen::createGUI(ImageManager& imageManager, SDL_Renderer &renderer){
	//Create the root element of the GUI.
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}
    GUIObjectRoot=new GUIObject(imageManager,renderer,0,0,SCREEN_WIDTH,SCREEN_HEIGHT);
	
	//Create back button.
    GUIObject* obj=new GUIButton(imageManager,renderer,SCREEN_WIDTH*0.5,SCREEN_HEIGHT-60,-1,36,_("Back"),0,true,true,GUIGravityCenter);
	obj->name="cmdBack";
	obj->eventCallback=this;
	GUIObjectRoot->addChild(obj);

	//Create list box.
    listBox=new GUISingleLineListBox(imageManager,renderer,(SCREEN_WIDTH-500)/2,104,500,32);
	listBox->addItem(_("Achievements"));
	listBox->addItem(_("Statistics"));
	listBox->value=0;
	GUIObjectRoot->addChild(listBox);
	
	//Create list box for achievements.
    GUIListBox *list=new GUIListBox(imageManager,renderer,64,150,SCREEN_WIDTH-128,SCREEN_HEIGHT-150-72);
	list->name = "lstAchievements"; // debug only
	list->eventCallback = this; // debug only
	list->selectable=false;
	GUIObjectRoot->addChild(list);
	lists.clear();
	lists.push_back(list);
	
	addAchievements(imageManager, renderer, list);

	//Now create list box for statistics.
    list=new GUIListBox(imageManager,renderer,64,150,SCREEN_WIDTH-128,SCREEN_HEIGHT-150-72,true,false);
	list->selectable=false;
	GUIObjectRoot->addChild(list);
	lists.push_back(list);

	//Load needed pictures.
	//FIXME: hard-coded image path
    //TODO: Might want to consider not caching these as most other stuff use textures now.
    SDL_Surface* bmPlayer=imageManager.loadImage(getDataPath()+"themes/Cloudscape/characters/player.png");
    SDL_Surface* bmShadow=imageManager.loadImage(getDataPath()+"themes/Cloudscape/characters/shadow.png");
    SDL_Surface* bmMedal=imageManager.loadImage(getDataPath()+"gfx/medals.png");

	SDL_Rect r;
	int x,y,w=SCREEN_WIDTH-128;
    SharedTexture h_bar = [&](){
        //The horizontal bar.
        SurfacePtr h_bar(createSurface(w,2));
		SDL_Color c = objThemes.getTextColor(true);
		Uint32 clr=SDL_MapRGB(h_bar->format,c.r,c.g,c.b);
        SDL_FillRect(h_bar.get(),NULL,clr);
        return textureFromSurface(renderer, std::move(h_bar));
    }();

	//Player and shadow specific statistics
	//The header.
    {
        SurfacePtr stats = createSurface(w, 44);
        SDL_FillRect(stats.get(),NULL,-1);

        SurfacePtr surface(TTF_RenderUTF8_Blended(fontGUISmall,_("Total"),objThemes.getTextColor(true)));
        applySurface(w-260-surface->w,stats->h-surface->h,surface.get(),stats.get(),NULL);
        //FIXME: hard-coded player and shadow images
        r.x=0;r.y=0;r.w=23;r.h=40;
        applySurface(w-140-r.w,stats.get()->h-40,bmPlayer,stats.get(),&r);
        applySurface(w-20-r.w,stats.get()->h-40,bmShadow,stats.get(),&r);

        list->addItem(renderer, "",textureFromSurface(renderer, std::move(stats)));
    }

	//Each items.
    {
        DRAW_PLAYER_STATISTICS(_("Traveling distance (m)"),TravelingDistance,"%0.1f");
        DRAW_PLAYER_STATISTICS(_("Jump times"),Jumps,"%d");
        DRAW_PLAYER_STATISTICS(_("Die times"),Dies,"%d");
        DRAW_PLAYER_STATISTICS(_("Squashed times"),Squashed,"%d");
    }

	//Game specific statistics.
    list->addItem(renderer, "",h_bar);

    auto drawMiscStats = [&](const char* name1,const int var1,const char* format1) {
        drawMiscStatistics1(renderer, w, list, name1, var1, format1);
    };

    drawMiscStats(_("Recordings:"),statsMgr.recordTimes,"%d");
    drawMiscStats(_("Switch pulled times:"),statsMgr.switchTimes,"%d");
    drawMiscStats(_("Swap times:"),statsMgr.swapTimes,"%d");
    drawMiscStats(_("Save times:"),statsMgr.saveTimes,"%d");
    drawMiscStats(_("Load times:"),statsMgr.loadTimes,"%d");
	drawMiscStats(_("Collectibles collected:"), statsMgr.collectibleCollected, "%d");

	//Level specific statistics
    list->addItem(renderer, "",h_bar);

	auto drawLevelStats = [&](const char* name1, int completed, int total, int gold, int silver) {
        SurfacePtr surface(TTF_RenderUTF8_Blended(fontGUISmall,name1,objThemes.getTextColor(true)));
        SurfacePtr stats = createSurface(w, surface->h);
        SDL_FillRect(stats.get(),NULL,-1);

        applySurface(4,0,surface.get(),stats.get(),NULL);
        x=surface->w+8;
        y=surface->h;

        surface.reset(TTF_RenderUTF8_Blended(fontText,
			tfm::format("%d/%d", completed, total).c_str(),
			objThemes.getTextColor(true)));

        applySurface(x,(y-surface->h),surface.get(),stats.get(),NULL);

		surface.reset(TTF_RenderUTF8_Blended(fontText,
			tfm::format("%d", completed - gold - silver).c_str(),
			objThemes.getTextColor(true)));
        applySurface(w-260-surface->w,(y-surface->h),surface.get(),stats.get(),NULL);
        r.x=0;r.y=0;r.w=30;r.h=30;
        applySurface(w-260-surface->w-30,(y-30)/2,bmMedal,stats.get(),&r);

		surface.reset(TTF_RenderUTF8_Blended(fontText,
			tfm::format("%d", silver).c_str(),
			objThemes.getTextColor(true)));
        applySurface(w-140-surface->w,(y-surface->h),surface.get(),stats.get(),NULL);
        r.x+=30;
        applySurface(w-140-surface->w-30,(y-30)/2,bmMedal,stats.get(),&r);

		surface.reset(TTF_RenderUTF8_Blended(fontText,
			tfm::format("%d", gold).c_str(),
			objThemes.getTextColor(true)));
        applySurface(w-20-surface->w,(y-surface->h),surface.get(),stats.get(),NULL);
        r.x+=30;
        applySurface(w-20-surface->w-30,(y-30)/2,bmMedal,stats.get(),&r);

        list->addItem(renderer,"",textureFromSurface(renderer, std::move(stats)));
	};

	drawLevelStats(_("Completed levels:"), statsMgr.completedLevels, statsMgr.totalLevels, statsMgr.goldLevels, statsMgr.silverLevels);
	drawLevelStats(_("Official levels:"), statsMgr.completedLevelsByCategory[MAIN],
		statsMgr.totalLevelsByCategory[MAIN], statsMgr.goldLevelsByCategory[MAIN], statsMgr.silverLevelsByCategory[MAIN]);
	drawLevelStats(_("Addon levels:"), statsMgr.completedLevelsByCategory[ADDON],
		statsMgr.totalLevelsByCategory[ADDON], statsMgr.goldLevelsByCategory[ADDON], statsMgr.silverLevelsByCategory[ADDON]);
	drawLevelStats(_("Custom levels:"), statsMgr.completedLevelsByCategory[CUSTOM],
		statsMgr.totalLevelsByCategory[CUSTOM], statsMgr.goldLevelsByCategory[CUSTOM], statsMgr.silverLevelsByCategory[CUSTOM]);

	//Levelpack specific statistics
	list->addItem(renderer, "", h_bar);

	drawLevelStats(_("Completed levelpacks:"), statsMgr.completedLevelpacks, statsMgr.totalLevelpacks, statsMgr.goldLevelpacks, statsMgr.silverLevelpacks);
	drawLevelStats(_("Official levelpacks:"), statsMgr.completedLevelpacksByCategory[MAIN],
		statsMgr.totalLevelpacksByCategory[MAIN], statsMgr.goldLevelpacksByCategory[MAIN], statsMgr.silverLevelpacksByCategory[MAIN]);
	drawLevelStats(_("Addon levelpacks:"), statsMgr.completedLevelpacksByCategory[ADDON],
		statsMgr.totalLevelpacksByCategory[ADDON], statsMgr.goldLevelpacksByCategory[ADDON], statsMgr.silverLevelpacksByCategory[ADDON]);
	drawLevelStats(_("Custom levelpacks:"), statsMgr.completedLevelpacksByCategory[CUSTOM],
		statsMgr.totalLevelpacksByCategory[CUSTOM], statsMgr.goldLevelpacksByCategory[CUSTOM], statsMgr.silverLevelpacksByCategory[CUSTOM]);

	//Other statistics.
    list->addItem(renderer, "",h_bar);

    drawMiscStatistics1(renderer,w,list,_("In-game time:"),
		tfm::format("%02d:%02d:%02d", statsMgr.playTime / 3600, (statsMgr.playTime / 60) % 60, statsMgr.playTime % 60),
		"%s");
    drawMiscStatistics1(renderer,w,list,_("Level editing time:"),
		tfm::format("%02d:%02d:%02d", statsMgr.levelEditTime / 3600, (statsMgr.levelEditTime / 60) % 60, statsMgr.levelEditTime % 60),
		"%s");

    drawMiscStats(_("Created levels:"),statsMgr.createdLevels,"%d");

	drawMiscStatistics1(renderer, w, list, _("Achievement achieved:"),
		tfm::format("%d/%d", statsMgr.getCurrentNumberOfAchievements(), statsMgr.getTotalAchievements()),
		"%s");
}

//In this method all the key and mouse events should be handled.
//NOTE: The GUIEvents won't be handled here.
void StatisticsScreen::handleEvents(ImageManager& imageManager, SDL_Renderer& renderer){
	//Check if we need to quit, if so enter the exit state.
	if(event.type==SDL_QUIT){
		setNextState(STATE_EXIT);
	}

	//Check horizontal movement
	int value = listBox->value;
	if (inputMgr.isKeyDownEvent(INPUTMGR_RIGHT)){
		isKeyboardOnly = true;
		value++;
		if (value >= (int)listBox->item.size()) value = 0;
	} else if (inputMgr.isKeyDownEvent(INPUTMGR_LEFT)){
		isKeyboardOnly = true;
		value--;
		if (value < 0) value = listBox->item.size() - 1;
	}
	listBox->value = value;
	
	//Check vertical movement
	if (value >= 0 && value < (int)lists.size()) {
		if (inputMgr.isKeyDownEvent(INPUTMGR_UP)){
			isKeyboardOnly = true;
			lists[value]->scrollScrollbar(-1);
		} else if (inputMgr.isKeyDownEvent(INPUTMGR_DOWN)){
			isKeyboardOnly = true;
			lists[value]->scrollScrollbar(1);
		}
	}

	//Yet another cheat "ls -la" which reveals all unknown achievements
	static char input[6];
	static int inputLen = 0;
	if (value == 0) {
		if (event.type == SDL_KEYDOWN) {
			if (event.key.keysym.sym >= 32 && event.key.keysym.sym <= 126) {
				if (inputLen < sizeof(input)) input[inputLen] = event.key.keysym.sym;
				inputLen++;
			} else {
				if (event.key.keysym.sym == SDLK_RETURN && inputLen == 6 &&
					input[0] == 'l' && input[1] == 's' && input[2] == ' ' && input[3] == '-' && input[4] == 'l' && input[5] == 'a')
				{
					if (easterEggScreen(imageManager, renderer)) {
						//new achievement
						statsMgr.newAchievement("cheat");

						//reload achievement list with hidden achievements revealed
						lists[0]->clearItems();
						lists[0]->selectable = true; // debug only
						addAchievements(imageManager, renderer, lists[0], true);
					}
				}
				inputLen = 0;
			}
		}
	} else {
		inputLen = 0;
	}

	//Check if the escape button is pressed, if so go back to the main menu.
	if(inputMgr.isKeyDownEvent(INPUTMGR_ESCAPE)){
		setNextState(STATE_MENU);
	}
}

//All the logic that needs to be done should go in this method.
void StatisticsScreen::logic(ImageManager&, SDL_Renderer&){
}

//This method handles all the rendering.
void StatisticsScreen::render(ImageManager&, SDL_Renderer& renderer){
	//Draw background.
    objThemes.getBackground(true)->draw(renderer);
	objThemes.getBackground(true)->updateAnimation();

	//Draw title.
    drawTitleTexture(SCREEN_WIDTH, *title, renderer);

	//Draw statistics.
	int value=listBox->value;
	for(unsigned int i=0;i<lists.size();i++){
		lists[i]->visible=(i==value);
	}
}

//Method that will be called when the screen size has been changed in runtime.
void StatisticsScreen::resize(ImageManager &imageManager, SDL_Renderer &renderer){
	//Recreate the gui to fit the new resolution.
    createGUI(imageManager, renderer);
}
