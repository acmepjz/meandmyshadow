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
#ifdef __APPLE__
#include <SDL_image/SDL_image.h>
#include <SDL_gfx/SDL_gfxPrimitives.h>
#else
#include <SDL/SDL_image.h>
#include <SDL/SDL_gfxPrimitives.h>
#endif

using namespace std;

//GUI events are handled here.
//name: The name of the element that invoked the event.
//obj: Pointer to the object that invoked the event.
//eventType: Integer containing the type of event.
void StatisticsScreen::GUIEventCallback_OnEvent(std::string name,GUIObject* obj,int eventType){
	//Check what type of event it was.
	if(eventType==GUIEventClick){
		if(name=="cmdBack"){
			//Goto the main menu.
			setNextState(STATE_MENU);
		}
	}
}

//Constructor.
StatisticsScreen::StatisticsScreen(){
	//Update in-game time.
	statsMgr.updatePlayTime();

	//Render the title.
	title=TTF_RenderUTF8_Blended(fontTitle,_("Achievements and Statistics"),themeTextColor);

	//Create GUI.
	createGUI();
}

//Destructor.
StatisticsScreen::~StatisticsScreen(){
	//Delete the GUI.
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}
	
	//Free images.
	SDL_FreeSurface(title);
}

//we are so lazy that we just use height of the first text, ignore the others
#define DRAW_PLAYER_STATISTICS(name,var,fmt) { \
	surface=TTF_RenderUTF8_Blended(fontGUISmall,name,themeTextColor); \
	stats=SDL_CreateRGBSurface(SCREEN_FLAGS,w,surface->h, /* create new surface */ \
		screen->format->BitsPerPixel,screen->format->Rmask,screen->format->Gmask,screen->format->Bmask,0); \
	SDL_FillRect(stats,NULL,-1); \
	applySurface(4,0,surface,stats,NULL); \
	y=surface->h; \
	SDL_FreeSurface(surface); \
	sprintf(s,fmt,statsMgr.player##var+statsMgr.shadow##var); \
	surface=TTF_RenderUTF8_Blended(fontText,s,themeTextColor); \
	applySurface(w-260-surface->w,(y-surface->h)/2,surface,stats,NULL); \
	SDL_FreeSurface(surface); \
	sprintf(s,fmt,statsMgr.player##var); \
	surface=TTF_RenderUTF8_Blended(fontText,s,themeTextColor); \
	applySurface(w-140-surface->w,(y-surface->h)/2,surface,stats,NULL); \
	SDL_FreeSurface(surface); \
	sprintf(s,fmt,statsMgr.shadow##var); \
	surface=TTF_RenderUTF8_Blended(fontText,s,themeTextColor); \
	applySurface(w-20-surface->w,(y-surface->h)/2,surface,stats,NULL); \
	SDL_FreeSurface(surface); \
	list->addItem("",stats); /* add it to list box */ \
}

//we are so lazy that we just use height of the first text, ignore the others
template <class T1>
static SDL_Surface* drawMiscStatistics1(int w,GUIListBox *list,const char* name1,const T1 var1,const char* format1){
	//create new surface
	SDL_Surface* surface=TTF_RenderUTF8_Blended(fontGUISmall,name1,themeTextColor);

	SDL_Surface* stats=SDL_CreateRGBSurface(SCREEN_FLAGS,w,surface->h,
		screen->format->BitsPerPixel,screen->format->Rmask,screen->format->Gmask,screen->format->Bmask,0);
	SDL_FillRect(stats,NULL,-1);

	applySurface(4,0,surface,stats,NULL);
	int x=surface->w+8;
	int y=surface->h;
	SDL_FreeSurface(surface);

	//draw value
	char s[1024];
	sprintf(s,format1,var1);
	surface=TTF_RenderUTF8_Blended(fontText,s,themeTextColor);
	applySurface(x,(y-surface->h)/2,surface,stats,NULL);
	SDL_FreeSurface(surface);

	//add it to list box
	list->addItem("",stats);

	//over
	return stats;
}

//we are so lazy that we just use height of the first text, ignore the others
template <class T1,class T2>
static void drawMiscStatistics2(int w,GUIListBox *list,const char* name1,const T1 var1,const char* format1,const char* name2,const T2 var2,const char* format2){
	SDL_Surface* stats=drawMiscStatistics1(w,list,name1,var1,format1);

	//Check if the width is enough
	if(w>=800){
		//draw name
		SDL_Surface* surface=TTF_RenderUTF8_Blended(fontGUISmall,name2,themeTextColor);
		applySurface(w/2-8,stats->h-surface->h,surface,stats,NULL);
		int x=surface->w+w/2;
		SDL_FreeSurface(surface);

		//draw value
		char s[1024];
		sprintf(s,format2,var2);
		surface=TTF_RenderUTF8_Blended(fontText,s,themeTextColor);
		applySurface(x,(stats->h-surface->h)/2,surface,stats,NULL);
		SDL_FreeSurface(surface);
	}else{
		//Split into two rows
		drawMiscStatistics1(w,list,name2,var2,format2);
	}
}

//Method that will create the GUI.
void StatisticsScreen::createGUI(){
	//Create the root element of the GUI.
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}
	GUIObjectRoot=new GUIObject(0,0,SCREEN_WIDTH,SCREEN_HEIGHT);
	
	//Create back button.
	GUIObject* obj=new GUIButton(SCREEN_WIDTH*0.5,SCREEN_HEIGHT-60,-1,36,_("Back"),0,true,true,GUIGravityCenter);
	obj->name="cmdBack";
	obj->eventCallback=this;
	GUIObjectRoot->addChild(obj);

	//Create list box.
	listBox=new GUISingleLineListBox((SCREEN_WIDTH-500)/2,104,500,32);
	listBox->addItem(_("Achievements"));
	listBox->addItem(_("Statistics"));
	listBox->value=0;
	GUIObjectRoot->addChild(listBox);
	
	//Create list box for achievements.
	GUIListBox *list=new GUIListBox(64,150,SCREEN_WIDTH-128,SCREEN_HEIGHT-150-72);
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
		SDL_Surface *surface=statsMgr.createAchievementSurface(&achievementList[idx],NULL,&r,false,lpt);

		if(surface!=NULL){
			hlineRGBA(surface,0,surface->w,0,0,0,0,32);
			hlineRGBA(surface,0,surface->w,surface->h-1,0,0,0,128);
			hlineRGBA(surface,0,surface->w,surface->h-2,0,0,0,32);
			list->addItem("",surface);
		}
	}

	//Now create list box for statistics.
	list=new GUIListBox(64,150,SCREEN_WIDTH-128,SCREEN_HEIGHT-150-72,true,false);
	list->selectable=false;
	GUIObjectRoot->addChild(list);
	lists.push_back(list);

	//Load needed pictures.
	//FIXME: hard-coded image path
	SDL_Surface* bmPlayer=loadImage(getDataPath()+"themes/Cloudscape/characters/player.png");
	SDL_Surface* bmShadow=loadImage(getDataPath()+"themes/Cloudscape/characters/shadow.png");
	SDL_Surface* bmMedal=loadImage(getDataPath()+"gfx/medals.png");

	//Render stats.
	char s[64],s2[64];
	SDL_Surface *stats,*surface,*h_bar;
	SDL_Rect r;
	int x,y,w=SCREEN_WIDTH-128;
	Uint32 clr=SDL_MapRGB(screen->format,themeTextColor.r,themeTextColor.g,themeTextColor.b);

	//The horizontal bar.
	h_bar=SDL_CreateRGBSurface(SDL_SWSURFACE,w,2,
		screen->format->BitsPerPixel,screen->format->Rmask,screen->format->Gmask,screen->format->Bmask,0);
	SDL_FillRect(h_bar,NULL,clr);

	//Player and shadow specific statistics
	//The header.
	stats=SDL_CreateRGBSurface(SCREEN_FLAGS,w,44,
		screen->format->BitsPerPixel,screen->format->Rmask,screen->format->Gmask,screen->format->Bmask,0);
	SDL_FillRect(stats,NULL,-1);

	surface=TTF_RenderUTF8_Blended(fontGUISmall,_("Total"),themeTextColor);
	applySurface(w-260-surface->w,stats->h-surface->h,surface,stats,NULL);
	SDL_FreeSurface(surface);
	//FIXME: hard-coded player and shadow images
	r.x=0;r.y=0;r.w=23;r.h=40;
	applySurface(w-140-r.w,stats->h-40,bmPlayer,stats,&r);
	applySurface(w-20-r.w,stats->h-40,bmShadow,stats,&r);

	list->addItem("",stats);

	//Each items.
	DRAW_PLAYER_STATISTICS(_("Traveling distance (m)"),TravelingDistance,"%0.1f");
	DRAW_PLAYER_STATISTICS(_("Jump times"),Jumps,"%d");
	DRAW_PLAYER_STATISTICS(_("Die times"),Dies,"%d");
	DRAW_PLAYER_STATISTICS(_("Squashed times"),Squashed,"%d");

	//Game specific statistics.
	list->addItem("",SDL_DisplayFormat(h_bar));

	drawMiscStatistics2(w,list,_("Recordings:"),statsMgr.recordTimes,"%d",_("Switch pulled times:"),statsMgr.switchTimes,"%d");
	drawMiscStatistics1(w,list,_("Swap times:"),statsMgr.swapTimes,"%d");
	drawMiscStatistics2(w,list,_("Save times:"),statsMgr.saveTimes,"%d",_("Load times:"),statsMgr.loadTimes,"%d");

	//Level specific statistics
	list->addItem("",SDL_DisplayFormat(h_bar));

	surface=TTF_RenderUTF8_Blended(fontGUISmall,_("Completed levels:"),themeTextColor);
	stats=SDL_CreateRGBSurface(SCREEN_FLAGS,w,surface->h,
		screen->format->BitsPerPixel,screen->format->Rmask,screen->format->Gmask,screen->format->Bmask,0);
	SDL_FillRect(stats,NULL,-1);

	applySurface(4,0,surface,stats,NULL);
	x=surface->w+8;
	y=surface->h;
	SDL_FreeSurface(surface);

	sprintf(s,"%d",statsMgr.completedLevels);
	surface=TTF_RenderUTF8_Blended(fontText,s,themeTextColor);
	applySurface(x,(y-surface->h)/2,surface,stats,NULL);
	SDL_FreeSurface(surface);

	sprintf(s,"%d",statsMgr.completedLevels-statsMgr.goldLevels-statsMgr.silverLevels);
	surface=TTF_RenderUTF8_Blended(fontText,s,themeTextColor);
	applySurface(w-260-surface->w,(y-surface->h)/2,surface,stats,NULL);
	r.x=0;r.y=0;r.w=30;r.h=30;
	applySurface(w-260-surface->w-30,(y-30)/2,bmMedal,stats,&r);
	SDL_FreeSurface(surface);

	sprintf(s,"%d",statsMgr.silverLevels);
	surface=TTF_RenderUTF8_Blended(fontText,s,themeTextColor);
	applySurface(w-140-surface->w,(y-surface->h)/2,surface,stats,NULL);
	r.x+=30;
	applySurface(w-140-surface->w-30,(y-30)/2,bmMedal,stats,&r);
	SDL_FreeSurface(surface);

	sprintf(s,"%d",statsMgr.goldLevels);
	surface=TTF_RenderUTF8_Blended(fontText,s,themeTextColor);
	applySurface(w-20-surface->w,(y-surface->h)/2,surface,stats,NULL);
	r.x+=30;
	applySurface(w-20-surface->w-30,(y-30)/2,bmMedal,stats,&r);
	SDL_FreeSurface(surface);

	list->addItem("",stats);

	//Other statistics.
	list->addItem("",SDL_DisplayFormat(h_bar));

	sprintf(s,"%02d:%02d:%02d",statsMgr.playTime/3600,(statsMgr.playTime/60)%60,statsMgr.playTime%60);
	sprintf(s2,"%02d:%02d:%02d",statsMgr.levelEditTime/3600,(statsMgr.levelEditTime/60)%60,statsMgr.levelEditTime%60);
	drawMiscStatistics2(w,list,_("In-game time:"),s,"%s",_("Level editing time:"),s2,"%s");

	drawMiscStatistics1(w,list,_("Created levels:"),statsMgr.createdLevels,"%d");

	//Clean up
	SDL_FreeSurface(h_bar);
}

//In this method all the key and mouse events should be handled.
//Note: The GUIEvents won't be handled here.
void StatisticsScreen::handleEvents(){
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
void StatisticsScreen::logic(){
}

//This method handles all the rendering.
void StatisticsScreen::render(){
	//Draw background.
	objThemes.getBackground(true)->draw(screen);
	objThemes.getBackground(true)->updateAnimation();

	//Draw title.
	applySurface((SCREEN_WIDTH-title->w)/2,40-TITLE_FONT_RAISE,title,screen,NULL);

	//Draw statistics.
	int value=listBox->value;
	for(unsigned int i=0;i<lists.size();i++){
		lists[i]->visible=(i==value);
	}
}

//Method that will be called when the screen size has been changed in runtime.
void StatisticsScreen::resize(){
	//Recreate the gui to fit the new resolution.
	createGUI();
}
