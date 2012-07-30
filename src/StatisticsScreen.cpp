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
#include <SDL/SDL_image.h>

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

#define DRAW_PLAYER_STATISTICS(name,var,format) { \
	surface=TTF_RenderUTF8_Blended(fontGUISmall,name,themeTextColor); \
	SDL_SetAlpha(surface,0,0xFF); \
	applySurface(0,y,surface,stats,NULL); \
	y+=surface->h; \
	SDL_FreeSurface(surface); \
	sprintf(s,format,statsMgr.player##var+statsMgr.shadow##var); \
	surface=TTF_RenderUTF8_Blended(fontText,s,themeTextColor); \
	SDL_SetAlpha(surface,0,0xFF); \
	applySurface(400-surface->w,y-surface->h,surface,stats,NULL); \
	SDL_FreeSurface(surface); \
	sprintf(s,format,statsMgr.player##var); \
	surface=TTF_RenderUTF8_Blended(fontText,s,themeTextColor); \
	SDL_SetAlpha(surface,0,0xFF); \
	applySurface(520-surface->w,y-surface->h,surface,stats,NULL); \
	SDL_FreeSurface(surface); \
	sprintf(s,format,statsMgr.shadow##var); \
	surface=TTF_RenderUTF8_Blended(fontText,s,themeTextColor); \
	SDL_SetAlpha(surface,0,0xFF); \
	applySurface(640-surface->w,y-surface->h,surface,stats,NULL); \
	SDL_FreeSurface(surface); \
}

#define DRAW_MISC_STATISTICS_1(name1,var1,format1) { \
	surface=TTF_RenderUTF8_Blended(fontGUISmall,name1,themeTextColor); \
	SDL_SetAlpha(surface,0,0xFF); \
	applySurface(0,y,surface,stats,NULL); \
	x=surface->w+8; \
	y+=surface->h; \
	SDL_FreeSurface(surface); \
	sprintf(s,format1,statsMgr.var1); \
	surface=TTF_RenderUTF8_Blended(fontText,s,themeTextColor); \
	SDL_SetAlpha(surface,0,0xFF); \
	applySurface(x,y-surface->h,surface,stats,NULL); \
	SDL_FreeSurface(surface); \
}

//we are so lazy that we just use height of the first one, ignore second one
#define DRAW_MISC_STATISTICS_2(name1,var1,format1,name2,var2,format2) { \
	surface=TTF_RenderUTF8_Blended(fontGUISmall,name1,themeTextColor); \
	SDL_SetAlpha(surface,0,0xFF); \
	applySurface(0,y,surface,stats,NULL); \
	x=surface->w+8; \
	y+=surface->h; \
	SDL_FreeSurface(surface); \
	sprintf(s,format1,statsMgr.var1); \
	surface=TTF_RenderUTF8_Blended(fontText,s,themeTextColor); \
	SDL_SetAlpha(surface,0,0xFF); \
	applySurface(x,y-surface->h,surface,stats,NULL); \
	SDL_FreeSurface(surface); \
	surface=TTF_RenderUTF8_Blended(fontGUISmall,name2,themeTextColor); \
	SDL_SetAlpha(surface,0,0xFF); \
	applySurface(320,y-surface->h,surface,stats,NULL); \
	x=surface->w+328; \
	SDL_FreeSurface(surface); \
	sprintf(s,format2,statsMgr.var2); \
	surface=TTF_RenderUTF8_Blended(fontText,s,themeTextColor); \
	SDL_SetAlpha(surface,0,0xFF); \
	applySurface(x,y-surface->h,surface,stats,NULL); \
	SDL_FreeSurface(surface); \
}

//Constructor.
StatisticsScreen::StatisticsScreen(){
	//Load needed pictures.
	//Note: we don't use ImageManager because we need to process these pictures.
	SDL_Surface *bmPlayer=IMG_Load((getDataPath()+"themes/Cloudscape/player.png").c_str());
	SDL_Surface *bmShadow=IMG_Load((getDataPath()+"themes/Cloudscape/shadow.png").c_str());
	SDL_Surface *bmMedal=IMG_Load((getDataPath()+"gfx/medals.png").c_str());

	//disable the alpha channel
	SDL_SetAlpha(bmPlayer,0,0xFF);
	SDL_SetAlpha(bmShadow,0,0xFF);
	SDL_SetAlpha(bmMedal,0,0xFF);

	//Render the title.
	title=TTF_RenderUTF8_Blended(fontTitle,_("Statistics and Achievements"),themeTextColor);

	//Render stats.
    Uint32 rmask, gmask, bmask, amask;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	rmask = 0xff000000;
	gmask = 0x00ff0000;
    bmask = 0x0000ff00;
	amask = 0x000000ff;
#else
	rmask = 0x000000ff;
	gmask = 0x0000ff00;
    bmask = 0x00ff0000;
	amask = 0xff000000;
#endif
	stats=SDL_CreateRGBSurface(SDL_SWSURFACE,640,400,32,rmask,gmask,bmask,amask);

	char s[64];
	SDL_Surface *surface;
	SDL_Rect r;
	int x,y=0;
	Uint32 clr=SDL_MapRGB(stats->format,themeTextColor.r,themeTextColor.g,themeTextColor.b);

	//Player and shadow specific statistics
	surface=TTF_RenderUTF8_Blended(fontGUISmall,_("Total"),themeTextColor);
	SDL_SetAlpha(surface,0,0xFF);
	applySurface(400-surface->w,40-surface->h,surface,stats,NULL);
	SDL_FreeSurface(surface);
	r.x=0;r.y=0;r.w=23;r.h=40;
	applySurface(520-r.w,0,bmPlayer,stats,&r);
	applySurface(640-r.w,0,bmShadow,stats,&r);
	y+=40;

	DRAW_PLAYER_STATISTICS(_("Traveling distance (m)"),TravelingDistance,"%0.2f");
	DRAW_PLAYER_STATISTICS(_("Jump times"),Jumps,"%d");
	DRAW_PLAYER_STATISTICS(_("Die times"),Dies,"%d");

	//Game specific statistics
	r.x=0;r.y=y;r.w=stats->w;r.h=2;
	SDL_FillRect(stats,&r,clr);
	y+=2;

	DRAW_MISC_STATISTICS_2(_("Record times:"),recordTimes,"%d",_("Switch pulled times:"),switchTimes,"%d");
	DRAW_MISC_STATISTICS_1(_("Swap times:"),swapTimes,"%d");

	//Level specific statistics
	r.x=0;r.y=y;r.w=stats->w;r.h=2;
	SDL_FillRect(stats,&r,clr);
	y+=2;

	surface=TTF_RenderUTF8_Blended(fontGUISmall,_("Completed levels:"),themeTextColor);
	SDL_SetAlpha(surface,0,0xFF);
	applySurface(0,y,surface,stats,NULL);
	x=surface->w+8;
	y+=surface->h;
	SDL_FreeSurface(surface);

	sprintf(s,"%d",statsMgr.completedLevels);
	surface=TTF_RenderUTF8_Blended(fontText,s,themeTextColor);
	SDL_SetAlpha(surface,0,0xFF);
	applySurface(x,y-surface->h,surface,stats,NULL);
	SDL_FreeSurface(surface);

	sprintf(s,"%d",statsMgr.completedLevels-statsMgr.goldLevels-statsMgr.silverLevels);
	surface=TTF_RenderUTF8_Blended(fontText,s,themeTextColor);
	SDL_SetAlpha(surface,0,0xFF);
	applySurface(400-surface->w,y-surface->h,surface,stats,NULL);
	r.x=0;r.y=0;r.w=30;r.h=30;
	applySurface(400-surface->w-30,y-30,bmMedal,stats,&r);
	SDL_FreeSurface(surface);

	sprintf(s,"%d",statsMgr.silverLevels);
	surface=TTF_RenderUTF8_Blended(fontText,s,themeTextColor);
	SDL_SetAlpha(surface,0,0xFF);
	applySurface(520-surface->w,y-surface->h,surface,stats,NULL);
	r.x+=30;
	applySurface(520-surface->w-30,y-30,bmMedal,stats,&r);
	SDL_FreeSurface(surface);

	sprintf(s,"%d",statsMgr.goldLevels);
	surface=TTF_RenderUTF8_Blended(fontText,s,themeTextColor);
	SDL_SetAlpha(surface,0,0xFF);
	applySurface(640-surface->w,y-surface->h,surface,stats,NULL);
	r.x+=30;
	applySurface(640-surface->w-30,y-30,bmMedal,stats,&r);
	SDL_FreeSurface(surface);

	//Other statistics
	r.x=0;r.y=y;r.w=stats->w;r.h=2;
	SDL_FillRect(stats,&r,clr);
	y+=2;

	surface=TTF_RenderUTF8_Blended(fontGUISmall,_("Playing time:"),themeTextColor);
	SDL_SetAlpha(surface,0,0xFF);
	applySurface(0,y,surface,stats,NULL);
	x=surface->w+8;
	y+=surface->h;
	SDL_FreeSurface(surface);

	sprintf(s,"%02d:%02d:%02d",statsMgr.playTime/3600,(statsMgr.playTime/60)%60,statsMgr.playTime%60);
	surface=TTF_RenderUTF8_Blended(fontText,s,themeTextColor);
	SDL_SetAlpha(surface,0,0xFF);
	applySurface(x,y-surface->h,surface,stats,NULL);
	SDL_FreeSurface(surface);

	surface=TTF_RenderUTF8_Blended(fontGUISmall,_("Level editing time:"),themeTextColor);
	SDL_SetAlpha(surface,0,0xFF);
	applySurface(320,y-surface->h,surface,stats,NULL);
	x=surface->w+328;
	SDL_FreeSurface(surface);

	sprintf(s,"%02d:%02d:%02d",statsMgr.levelEditTime/3600,(statsMgr.levelEditTime/60)%60,statsMgr.levelEditTime%60);
	surface=TTF_RenderUTF8_Blended(fontText,s,themeTextColor);
	SDL_SetAlpha(surface,0,0xFF);
	applySurface(x,y-surface->h,surface,stats,NULL);
	SDL_FreeSurface(surface);

	DRAW_MISC_STATISTICS_1(_("Created levels:"),createdLevels,"%d");

	//Free loaded surface
	SDL_FreeSurface(bmPlayer);
	SDL_FreeSurface(bmShadow);
	SDL_FreeSurface(bmMedal);

	//Create GUI
	achievements=NULL;
	createGUI();
}

//Destructor.
StatisticsScreen::~StatisticsScreen(){
	//Delete the GUI.
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}
	
	//Free images
	SDL_FreeSurface(title);
	SDL_FreeSurface(stats);
	SDL_FreeSurface(achievements);
}

//Method that will create the GUI for the options menu.
void StatisticsScreen::createGUI(){
	//Draw achievements
	if(achievements) SDL_FreeSurface(achievements);

	vector<SDL_Surface*> surfaces;
	int w=SCREEN_WIDTH-128-16,h=0;

	for(map<string,AchievementInfo*>::iterator it=statsMgr.achievements.begin();
		it!=statsMgr.achievements.end();it++)
	{
		SDL_Rect r={0,0,w,0};
		SDL_Surface *surface=statsMgr.createAchievementSurface(it->second,NULL,&r,false);
		if(surface!=NULL){
			surfaces.push_back(surface);
			h+=r.h;
		}
	}

	if(surfaces.empty()){
		achievements=TTF_RenderUTF8_Blended(fontText,_("You don't have any achievements."),themeTextColor);
	}else{
		achievements=SDL_CreateRGBSurface(SDL_HWSURFACE,w,h,
			screen->format->BitsPerPixel,screen->format->Rmask,screen->format->Gmask,screen->format->Bmask,0);

		h=0;
		for(unsigned int i=0;i<surfaces.size();i++){
			SDL_Rect r={0,h,0,0};
			SDL_BlitSurface(surfaces[i],NULL,achievements,&r);
			h+=surfaces[i]->h;
			SDL_FreeSurface(surfaces[i]);
		}
	}

	//Create the root element of the GUI.
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}
	GUIObjectRoot=new GUIObject(0,0,SCREEN_WIDTH,SCREEN_HEIGHT,GUIObjectNone);
	
	//Create back button.
	GUIObject* obj=new GUIObject(SCREEN_WIDTH*0.5,SCREEN_HEIGHT-60,-1,36,GUIObjectButton,_("Back"),0,true,true,GUIGravityCenter);
	obj->name="cmdBack";
	obj->eventCallback=this;
	GUIObjectRoot->addChild(obj);

	//Create list box
	listBox=new GUISingleLineListBox((SCREEN_WIDTH-500)/2,104,500,32);
	listBox->item.push_back(_("Statistics"));
	listBox->item.push_back(_("Achievements"));
	listBox->value=0;
	GUIObjectRoot->addChild(listBox);

	//Create vertical scrollbar.
	h-=SCREEN_HEIGHT-144-80;
	if(h<0) h=0;
	scrollbarV=new GUIScrollBar(SCREEN_WIDTH-64-16,144,16,SCREEN_HEIGHT-144-80,1,0,0,h,16,SCREEN_HEIGHT-144-80,true,false);
	GUIObjectRoot->addChild(scrollbarV);
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
	
	//Check for scrolling down and up with mouse scroll wheel.
	if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button==SDL_BUTTON_WHEELDOWN && scrollbarV->visible){
		if(scrollbarV->value<scrollbarV->maxValue)
			scrollbarV->value+=scrollbarV->smallChange*4;
		if(scrollbarV->value>scrollbarV->maxValue)
			scrollbarV->value=scrollbarV->maxValue;
		return;
	}else if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button==SDL_BUTTON_WHEELUP && scrollbarV->visible){
		if(scrollbarV->value>0)
			scrollbarV->value-=scrollbarV->smallChange*4;
		if(scrollbarV->value<0)
			scrollbarV->value=0;
		return;
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

	switch(listBox->value){
	case 0:
		//statistics
		scrollbarV->visible=false;
		applySurface((SCREEN_WIDTH-stats->w)/2,144,stats,screen,NULL);
		break;
	case 1:
		//achievements
		{
			scrollbarV->visible=(scrollbarV->maxValue>0);

			SDL_Rect r1={0,scrollbarV->value,achievements->w,SCREEN_HEIGHT-144-80};
			SDL_Rect r2={64,144,0,0};
			SDL_BlitSurface(achievements,&r1,screen,&r2);
		}
		break;
	}
}

//Method that will be called when the screen size has been changed in runtime.
void StatisticsScreen::resize(){
	//Recreate the gui to fit the new resolution.
	createGUI();
}
