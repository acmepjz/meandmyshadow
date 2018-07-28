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
#include <SDL_ttf.h>
#include <array>

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
	}
}

//Constructor.
StatisticsScreen::StatisticsScreen(ImageManager& imageManager, SDL_Renderer& renderer){
	//Update in-game time.
	statsMgr.updatePlayTime();

	//Render the title.
    //title=TTF_RenderUTF8_Blended(fontTitle,_("Achievements and Statistics"),objThemes.getTextColor(false));
    title = textureFromText(renderer, *fontTitle,_("Achievements and Statistics"),objThemes.getTextColor(false));


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
    SurfacePtr surface(TTF_RenderUTF8_Blended(fontGUISmall,name,objThemes.getTextColor(false))); \
    SurfacePtr stats = createSurface(w,surface->h); \
    SDL_FillRect(stats.get(),NULL,-1); \
    applySurface(4,0,surface.get(),stats.get(),NULL); \
    y=surface->h; \
    SDL_snprintf(formatString.data(),formatString.size(),fmt,statsMgr.player##var+statsMgr.shadow##var); \
    surface.reset(TTF_RenderUTF8_Blended(fontText,formatString.data(),objThemes.getTextColor(false))); \
    applySurface(w-260-surface->w,(y-surface->h)/2,surface.get(),stats.get(),NULL); \
    SDL_snprintf(formatString.data(),formatString.size(),fmt,statsMgr.player##var); \
    surface.reset(TTF_RenderUTF8_Blended(fontText,formatString.data(),objThemes.getTextColor(false))); \
    applySurface(w-140-surface->w,(y-surface->h)/2,surface.get(),stats.get(),NULL); \
    SDL_snprintf(formatString.data(),formatString.size(),fmt,statsMgr.shadow##var); \
    surface.reset(TTF_RenderUTF8_Blended(fontText,formatString.data(),objThemes.getTextColor(false))); \
    applySurface(w-20-surface->w,(y-surface->h)/2,surface.get(),stats.get(),NULL); \
    list->addItem(renderer,"",textureFromSurface(renderer, std::move(stats))); /* add it to list box */ \
}

//Add an item to the listbox, that displays "name1", and "var1" formatted with "format"
//we are so lazy that we just use height of the first text, ignore the others
template <class T1>
static void drawMiscStatistics1(SDL_Renderer& renderer, int w,GUIListBox *list,const char* name1,const T1 var1,const char* format1){
	//create new surface
    SurfacePtr nameSurface(TTF_RenderUTF8_Blended(fontGUISmall,name1,objThemes.getTextColor(false)));

    SurfacePtr stats=createSurface(w, nameSurface->h);
    SDL_FillRect(stats.get(),NULL,-1);

    applySurface(4,0,nameSurface.get(),stats.get(),NULL);
    const int x=nameSurface->w+8;
    const int y=nameSurface->h;

	//draw value
    //char s[1024];
    std::array<char, 1024> s;
    SDL_snprintf(s.data(),s.size(),format1,var1);
    SurfacePtr formatSurface(TTF_RenderUTF8_Blended(fontText,s.data(),objThemes.getTextColor(false)));
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
		SDL_Surface* surface=TTF_RenderUTF8_Blended(fontGUISmall,name2,objThemes.getTextColor(false));
		applySurface(w/2-8,stats->h-surface->h,surface,stats,NULL);
		int x=surface->w+w/2;
		SDL_FreeSurface(surface);

		//draw value
		char s[1024];
		sprintf(s,format2,var2);
		surface=TTF_RenderUTF8_Blended(fontText,s,objThemes.getTextColor(false));
		applySurface(x,(stats->h-surface->h)/2,surface,stats,NULL);
		SDL_FreeSurface(surface);
	}else{
		//Split into two rows
		drawMiscStatistics1(w,list,name2,var2,format2);
	}
}*/

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
	list->selectable=false;
	GUIObjectRoot->addChild(list);
	lists.clear();
	lists.push_back(list);
	
	for(int idx=0;achievementList[idx].id!=NULL;++idx){
		time_t *lpt=NULL;

		map<string,OwnedAchievement>::iterator it=statsMgr.achievements.find(achievementList[idx].id);
		if(it!=statsMgr.achievements.end()){
			lpt=&it->second.achievedTime;
		}
		
		SDL_Rect r;
		r.x=r.y=0;
		r.w=list->width-16;
        auto surface= statsMgr.createAchievementSurface(renderer, &achievementList[idx],&r,false,lpt);

        if(surface){
		//FIXME - this is broken with SDL2 as the function now operates on a renderer, not sure how best to fix it
/*			hlineRGBA(surface,0,surface->w,0,0,0,0,32);
			hlineRGBA(surface,0,surface->w,surface->h-1,0,0,0,128);
			hlineRGBA(surface,0,surface->w,surface->h-2,0,0,0,32);*/
            list->addItem(renderer, "",surface);
		}
	}

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

	//Render stats.
    //char s[64],s2[64];
    std::array<char, 64> formatString;

	SDL_Rect r;
	int x,y,w=SCREEN_WIDTH-128;
    SharedTexture h_bar = [&](){
        //The horizontal bar.
        SurfacePtr h_bar(createSurface(w,2));
        Uint32 clr=SDL_MapRGB(h_bar->format,objThemes.getTextColor(false).r,objThemes.getTextColor(false).g,objThemes.getTextColor(false).b);
        SDL_FillRect(h_bar.get(),NULL,clr);
        return textureFromSurface(renderer, std::move(h_bar));
    }();

	//Player and shadow specific statistics
	//The header.
    {
        SurfacePtr stats = createSurface(w, 44);
        SDL_FillRect(stats.get(),NULL,-1);

        SurfacePtr surface(TTF_RenderUTF8_Blended(fontGUISmall,_("Total"),objThemes.getTextColor(false)));
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

	//Level specific statistics
    list->addItem(renderer, "",h_bar);
    {
        SurfacePtr surface(TTF_RenderUTF8_Blended(fontGUISmall,_("Completed levels:"),objThemes.getTextColor(false)));
        SurfacePtr stats = createSurface(w, surface->h);
        SDL_FillRect(stats.get(),NULL,-1);

        applySurface(4,0,surface.get(),stats.get(),NULL);
        x=surface->w+8;
        y=surface->h;

        SDL_snprintf(formatString.data(), formatString.size(),"%d",statsMgr.completedLevels);
        surface.reset(TTF_RenderUTF8_Blended(fontText,formatString.data(),objThemes.getTextColor(false)));

        applySurface(x,(y-surface->h),surface.get(),stats.get(),NULL);

        SDL_snprintf(formatString.data(), formatString.size(),"%d",statsMgr.completedLevels-statsMgr.goldLevels-statsMgr.silverLevels);
        surface.reset(TTF_RenderUTF8_Blended(fontText,formatString.data(),objThemes.getTextColor(false)));
        applySurface(w-260-surface->w,(y-surface->h)/2,surface.get(),stats.get(),NULL);
        r.x=0;r.y=0;r.w=30;r.h=30;
        applySurface(w-260-surface->w-30,(y-30)/2,bmMedal,stats.get(),&r);

        SDL_snprintf(formatString.data(), formatString.size(),"%d",statsMgr.silverLevels);
        surface.reset(TTF_RenderUTF8_Blended(fontText,formatString.data(),objThemes.getTextColor(false)));
        applySurface(w-140-surface->w,(y-surface->h)/2,surface.get(),stats.get(),NULL);
        r.x+=30;
        applySurface(w-140-surface->w-30,(y-30)/2,bmMedal,stats.get(),&r);

        SDL_snprintf(formatString.data(), formatString.size(),"%d",statsMgr.goldLevels);
        surface.reset(TTF_RenderUTF8_Blended(fontText,formatString.data(),objThemes.getTextColor(false)));
        applySurface(w-20-surface->w,(y-surface->h)/2,surface.get(),stats.get(),NULL);
        r.x+=30;
        applySurface(w-20-surface->w-30,(y-30)/2,bmMedal,stats.get(),&r);

        list->addItem(renderer,"",textureFromSurface(renderer, std::move(stats)));
    }

	//Other statistics.
    list->addItem(renderer, "",h_bar);

    SDL_snprintf(formatString.data(), formatString.size(),"%02d:%02d:%02d",statsMgr.playTime/3600,(statsMgr.playTime/60)%60,statsMgr.playTime%60);
    drawMiscStatistics1(renderer,w,list,_("In-game time:"),formatString.data(),"%s");
    SDL_snprintf(formatString.data(), formatString.size(),"%02d:%02d:%02d",statsMgr.levelEditTime/3600,(statsMgr.levelEditTime/60)%60,statsMgr.levelEditTime%60);
    drawMiscStatistics1(renderer,w,list,_("Level editing time:"),formatString.data(),"%s");

    drawMiscStats(_("Created levels:"),statsMgr.createdLevels,"%d");
}

//In this method all the key and mouse events should be handled.
//NOTE: The GUIEvents won't be handled here.
void StatisticsScreen::handleEvents(ImageManager&, SDL_Renderer&){
	//Check if we need to quit, if so enter the exit state.
	if(event.type==SDL_QUIT){
		setNextState(STATE_EXIT);
	}

	//Check if the escape button is pressed, if so go back to the main menu.
	if(inputMgr.isKeyUpEvent(INPUTMGR_ESCAPE)){
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
