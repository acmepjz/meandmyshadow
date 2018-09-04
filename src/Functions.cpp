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

#include <stdio.h>
#include <math.h>
#include <locale.h>
#include <algorithm>
#include <SDL.h>
#include <SDL_mixer.h>
#include <SDL_syswm.h>
#include <SDL_ttf.h>
#include <string>
#include "Globals.h"
#include "Functions.h"
#include "FileManager.h"
#include "GameObjects.h"
#include "LevelPack.h"
#include "TitleMenu.h"
#include "OptionsMenu.h"
#include "CreditsMenu.h"
#include "LevelEditSelect.h"
#include "LevelEditor.h"
#include "Game.h"
#include "LevelPlaySelect.h"
#include "Addons.h"
#include "InputManager.h"
#include "ImageManager.h"
#include "MusicManager.h"
#include "SoundManager.h"
#include "ScriptExecutor.h"
#include "LevelPackManager.h"
#include "ThemeManager.h"
#include "GUIListBox.h"
#include "GUIOverlay.h"
#include "StatisticsManager.h"
#include "StatisticsScreen.h"
#include "Cursors.h"
#include "ScriptAPI.h"

#include "libs/tinyformat/tinyformat.h"
#include "libs/tinygettext/tinygettext.hpp"
#include "libs/tinygettext/log.hpp"
#include "libs/findlocale/findlocale.h"

using namespace std;

#ifdef WIN32
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#define TO_UTF8(SRC, DEST) WideCharToMultiByte(CP_UTF8, 0, SRC, -1, DEST, sizeof(DEST), NULL, NULL)
#define TO_UTF16(SRC, DEST) MultiByteToWideChar(CP_UTF8, 0, SRC, -1, DEST, sizeof(DEST)/sizeof(DEST[0]))
#else
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#endif

//Initialise the musicManager.
//The MusicManager is used to prevent loading music files multiple times and for playing/fading music.
MusicManager musicManager;

//Initialise the soundManager.
//The SoundManager is used to keep track of the sfx in the game.
SoundManager soundManager;

//Initialise the levelPackManager.
//The LevelPackManager is used to prevent loading levelpacks multiple times and for the game to know which levelpacks there are.
LevelPackManager levelPackManager;

//Map containing changed settings using command line arguments.
map<string,string> tmpSettings;
//Pointer to the settings object.
//It is used to load and save the settings file and change the settings.
Settings* settings=nullptr;

SDL_Renderer* sdlRenderer=nullptr;

std::string ngettext(const std::string& message,const std::string& messageplural,int num) {
	if (dictionaryManager) {
		return dictionaryManager->get_dictionary().translate_plural(message, messageplural, num);
	} else {
		//Assume it's of English plural rule
		return (num != 1) ? messageplural : message;
	}
}

void applySurface(int x,int y,SDL_Surface* source,SDL_Surface* dest,SDL_Rect* clip){
	//The offset is needed to draw at the right location.
	SDL_Rect offset;
	offset.x=x;
	offset.y=y;
	
	//Let SDL do the drawing of the surface.
	SDL_BlitSurface(source,clip,dest,&offset);
}

void drawRect(int x,int y,int w,int h,SDL_Renderer& renderer,Uint32 color){
	//NOTE: We let SDL_gfx render it.
    SDL_SetRenderDrawColor(&renderer,color >> 24,color >> 16,color >> 8,255);
    //rectangleRGBA(&renderer,x,y,x+w,y+h,color >> 24,color >> 16,color >> 8,255);
    const SDL_Rect r{x,y,w,h};
    SDL_RenderDrawRect(&renderer,&r);
}

//Draw a box with anti-aliased borders using SDL_gfx.
void drawGUIBox(int x,int y,int w,int h,SDL_Renderer& renderer,Uint32 color){
    SDL_Renderer* rd = &renderer;
    //FIXME, this may get the wrong color on system with different endianness.
	//Fill content's background color from function parameter
    SDL_SetRenderDrawColor(rd,color >> 24,color >> 16,color >> 8,color >> 0);
    {
        const SDL_Rect r{x+1,y+1,w-2,h-2};
        SDL_RenderFillRect(rd, &r);
    }
    SDL_SetRenderDrawColor(rd,0,0,0,255);
	//Draw first black borders around content and leave 1 pixel in every corner
    SDL_RenderDrawLine(rd,x+1,y,x+w-2,y);
    SDL_RenderDrawLine(rd,x+1,y+h-1,x+w-2,y+h-1);
    SDL_RenderDrawLine(rd,x,y+1,x,y+h-2);
    SDL_RenderDrawLine(rd,x+w-1,y+1,x+w-1,y+h-2);
	
	//Fill the corners with transperent color to create anti-aliased borders
    SDL_SetRenderDrawColor(rd,0,0,0,160);
    SDL_RenderDrawPoint(rd,x,y);
    SDL_RenderDrawPoint(rd,x,y+h-1);
    SDL_RenderDrawPoint(rd,x+w-1,y);
    SDL_RenderDrawPoint(rd,x+w-1,y+h-1);

	//Draw second lighter border around content
    SDL_SetRenderDrawColor(rd,0,0,0,64);
    {
        const SDL_Rect r{x+1,y+1,w-2,h-2};
        SDL_RenderDrawRect(rd,&r);
    }

    SDL_SetRenderDrawColor(rd,0,0,0,50);

	//Create anti-aliasing in corners of second border
    SDL_RenderDrawPoint(rd,x+1,y+1);
    SDL_RenderDrawPoint(rd,x+1,y+h-2);
    SDL_RenderDrawPoint(rd,x+w-2,y+1);
    SDL_RenderDrawPoint(rd,x+w-2,y+h-2);
}

void drawLine(int x1,int y1,int x2,int y2,SDL_Renderer& renderer,Uint32 color){
    SDL_SetRenderDrawColor(&renderer,color >> 24,color >> 16,color >> 8,255);
	//NOTE: We let SDL_gfx render it.
    //lineRGBA(&renderer,x1,y1,x2,y2,color >> 24,color >> 16,color >> 8,255);
    SDL_RenderDrawLine(&renderer,x1,y1,x2,y2);
}

void drawLineWithArrow(int x1,int y1,int x2,int y2,SDL_Renderer& renderer,Uint32 color,int spacing,int offset,int xsize,int ysize){
	//Draw line first
    drawLine(x1,y1,x2,y2,renderer,color);

	//calc delta and length
	double dx=x2-x1;
	double dy=y2-y1;
	double length=sqrt(dx*dx+dy*dy);
	if(length<0.001) return;

	//calc the unit vector
	dx/=length; dy/=length;

	//Now draw arrows on it
	for(double p=offset;p<length;p+=spacing){
		drawLine(int(x1+p*dx+0.5),int(y1+p*dy+0.5),
            int(x1+(p-xsize)*dx-ysize*dy+0.5),int(y1+(p-xsize)*dy+ysize*dx+0.5),renderer,color);
		drawLine(int(x1+p*dx+0.5),int(y1+p*dy+0.5),
            int(x1+(p-xsize)*dx+ysize*dy+0.5),int(y1+(p-xsize)*dy-ysize*dx+0.5),renderer,color);
	}
}

ScreenData creationFailed() {
	return ScreenData{ nullptr };
}

ScreenData createScreen(){
	//Check if we are going fullscreen.
	if(settings->getBoolValue("fullscreen"))
		pickFullscreenResolution();
	
	//Set the screen_width and height.
    SCREEN_WIDTH=atoi(settings->getValue("width").c_str());
    SCREEN_HEIGHT=atoi(settings->getValue("height").c_str());

	//Update the camera.
	camera.w=SCREEN_WIDTH;
	camera.h=SCREEN_HEIGHT;
	
    //Set the flags.
    Uint32 flags = 0;
    Uint32 currentFlags = SDL_GetWindowFlags(sdlWindow);

//#if !defined(ANDROID)
//		flags |= SDL_DOUBLEBUF;
//#endif
    if(settings->getBoolValue("fullscreen")) {
        flags|=SDL_WINDOW_FULLSCREEN; //TODO with SDL2 we can also do SDL_WINDOW_FULLSCREEN_DESKTOP
    }
    else if(settings->getBoolValue("resizable"))
        flags|=SDL_WINDOW_RESIZABLE;

    //Create the window and renderer if they don't exist and check if there weren't any errors.
    if (!sdlWindow && !sdlRenderer) {
        SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, flags, &sdlWindow, &sdlRenderer);
        if(!sdlWindow || !sdlRenderer){
            std::cerr <<  "FATAL ERROR: SDL_CreateWindowAndRenderer failed.\nError: " << SDL_GetError() << std::endl;
            return creationFailed();
        }

        SDL_SetRenderDrawBlendMode(sdlRenderer, SDL_BlendMode::SDL_BLENDMODE_BLEND);

        // White background so we see the menu on failure.
        SDL_SetRenderDrawColor(sdlRenderer, 255, 255, 255, 255);
    } else if (sdlWindow) {
        // Try changing to/from fullscreen
        if(SDL_SetWindowFullscreen(sdlWindow, flags & SDL_WINDOW_FULLSCREEN) != 0) {
            std::cerr << "WARNING: Failed to switch to fullscreen: " << SDL_GetError() << std::endl;
        };
        currentFlags = SDL_GetWindowFlags(sdlWindow);

        // Change fullscreen resolution
        if((currentFlags & SDL_WINDOW_FULLSCREEN ) || (currentFlags & SDL_WINDOW_FULLSCREEN_DESKTOP)) {
            SDL_DisplayMode m{0,0,0,0,nullptr};
            SDL_GetWindowDisplayMode(sdlWindow,&m);
            m.w = SCREEN_WIDTH;
            m.h = SCREEN_HEIGHT;
            if(SDL_SetWindowDisplayMode(sdlWindow, &m) != 0) {
                std::cerr << "WARNING: Failed to set display mode: " << SDL_GetError() << std::endl;
            }
        } else {
            SDL_SetWindowSize(sdlWindow, SCREEN_WIDTH, SCREEN_HEIGHT);
        }
    }
	
	//Now configure the newly created window (if windowed).
	if(settings->getBoolValue("fullscreen")==false)
		configureWindow();
	
	//Set the the window caption.
	SDL_SetWindowTitle(sdlWindow, ("Me and My Shadow "+version).c_str());
	//FIXME Seems to be obsolete
	//	SDL_EnableUNICODE(1);
	
	//Nothing went wrong so return true.
    return ScreenData{sdlRenderer};
}

vector<_res> getResolutionList(){
	//Vector that will hold the resolutions to choose from.
	vector<_res> resolutionList;

	//Enumerate available resolutions using SDL_ListModes()
	//NOTE: we enumerate fullscreen resolutions because
	// windowed resolutions always can be arbitrary
	if(resolutionList.empty()){
//		SDL_Rect **modes=SDL_ListModes(NULL,SDL_FULLSCREEN|SCREEN_FLAGS|SDL_ANYFORMAT);
		//NOTe - currently only using the first display (0)
		int numDisplayModes = SDL_GetNumDisplayModes(0);

		if(numDisplayModes < 1){
			cerr<<"ERROR: Can't enumerate available screen resolutions."
				" Use predefined screen resolutions list instead."<<endl;

			static const _res predefinedResolutionList[] = {
				{800,600},
				{1024,600},
				{1024,768},
				{1152,864},
				{1280,720},
				{1280,768},
				{1280,800},
				{1280,960},
				{1280,1024},
				{1360,768},
				{1366,768},
				{1440,900},
				{1600,900},
				{1600,1200},
				{1680,1080},
				{1920,1080},
				{1920,1200},
				{2560,1440},
				{3840,2160}
			};

			//Fill the resolutionList.
			for(unsigned int i=0;i<sizeof(predefinedResolutionList)/sizeof(_res);i++){
				resolutionList.push_back(predefinedResolutionList[i]);
			}
		}else{
			//Fill the resolutionList.

			for(int i=0;i < numDisplayModes; ++i){
				SDL_DisplayMode mode;
				int error = SDL_GetDisplayMode(0, i, &mode);
				if(error < 0) {
					//We failed to get a display mode. Should we crash here?
					std::cerr << "ERROR: Failed to get display mode " << i << " " << std::endl;
				}
				//Check if the resolution is higher than the minimum (800x600).
				if(mode.w >= 800 && mode.h >= 600){
					_res res={mode.w, mode.h};
					resolutionList.push_back(res);
				}
			}
			//Reverse it so that we begin with the lowest resolution.
			reverse(resolutionList.begin(),resolutionList.end());
		}
	}

	//Return the resolution list.
	return resolutionList;
}

void pickFullscreenResolution(){
	//Get the resolution list.
	vector<_res> resolutionList=getResolutionList();
	
	//The resolution that will hold the final result, we start with the minimum (800x600).
	_res closestMatch={800,600};
	int width=atoi(getSettings()->getValue("width").c_str());
	//int height=atoi(getSettings()->getValue("height").c_str());
	
	//Now loop through the resolutionList.
	for(int i=0;i<(int)resolutionList.size();i++){
			//The delta between the closestMatch and the resolution from the list.
			int dM=(closestMatch.w-resolutionList[i].w);
			//The delta between the target width and the resolution from the list.
			int dT=(width-resolutionList[i].w);
			
			//Since the resolutions are getting higher the lower (more negative) the further away it is.
			//That's why we check if the deltaMatch is lower than the the deltaTarget.
			if((dM)<(dT)){
				closestMatch.w=resolutionList[i].w;
				closestMatch.h=resolutionList[i].h;
			}
	}
	
	//Now set the resolution to the closest match.
	char s[64];
	sprintf(s,"%d",closestMatch.w);
	getSettings()->setValue("width",s);
	sprintf(s,"%d",closestMatch.h);
	getSettings()->setValue("height",s);
}

void configureWindow(){
	//We only need to configure the window if it's resizable.
	if(!getSettings()->getBoolValue("resizable"))
		return;

	//We use a new function in SDL2 to restrict minimum window size
	SDL_SetWindowMinimumSize(sdlWindow, 800, 600);
}

void onVideoResize(ImageManager& imageManager, SDL_Renderer &renderer){
	//Check if the resize event isn't malformed.
	if(event.window.data1<=0 || event.window.data2<=0)
		return;

	//Check the size limit.
    //TODO: SDL2 porting note: This may break on systems non-X11 or Windows systems as the window size won't be limited
    //there.
    if(event.window.data1<800)
		event.window.data1=800;
    if(event.window.data2<600)
		event.window.data2=600;

	//Check if it really resizes.
	if(SCREEN_WIDTH==event.window.data1 && SCREEN_HEIGHT==event.window.data2)
		return;

	char s[32];
	
	//Set the new width and height.
    SDL_snprintf(s,32,"%d",event.window.data1);
	getSettings()->setValue("width",s);
    SDL_snprintf(s,32,"%d",event.window.data2);
	getSettings()->setValue("height",s);
    //FIXME: THIS doesn't work properly.
	//Do resizing.
    SCREEN_WIDTH = event.window.data1;
    SCREEN_HEIGHT = event.window.data2;
    //Update the camera.
    camera.w=SCREEN_WIDTH;
    camera.h=SCREEN_HEIGHT;
	
	//Tell the theme to resize.
    if(!loadTheme(imageManager,renderer,""))
		return;
	
	//And let the currentState update it's GUI to the new resolution.
    currentState->resize(imageManager, renderer);
}

ScreenData init(){
	//Initialze SDL.
	if(SDL_Init(SDL_INIT_EVERYTHING)==-1) {
        std::cerr << "FATAL ERROR: SDL_Init failed\nError: " << SDL_GetError() << std::endl;
        return creationFailed();
	}

	//Initialze SDL_mixer (audio).
    //Note for SDL2 port: Changed frequency from 22050 to 44100.
    //22050 caused some sound artifacts on my system, and I'm not sure
    //why one would use it in this day and age anyhow.
    //unless it's for compatability with some legacy system.
    if(Mix_OpenAudio(44100,MIX_DEFAULT_FORMAT,2,1024)==-1){
        std::cerr << "FATAL ERROR: Mix_OpenAudio failed\nError: " << Mix_GetError() << std::endl;
        return creationFailed();
	}
	//Set the volume.
	Mix_Volume(-1,atoi(settings->getValue("sound").c_str()));

	//Increase the number of channels.
	soundManager.setNumberOfChannels(48);

	//Initialze SDL_ttf (fonts).
	if(TTF_Init()==-1){
        std::cerr << "FATAL ERROR: TTF_Init failed\nError: " << TTF_GetError() << std::endl;
        return creationFailed();
	}

	//Create the screen.
    ScreenData screenData(createScreen());
    if(!screenData) {
        return creationFailed();
    }

	//Load key config. Then initialize joystick support.
	inputMgr.loadConfig();
	inputMgr.openAllJoysitcks();
	
	//Init tinygettext for translations for the right language
	dictionaryManager = new tinygettext::DictionaryManager();
	dictionaryManager->add_directory(getDataPath()+"locale");
	dictionaryManager->set_charset("UTF-8");
	
	//Check if user have defined own language. If not, find it out for the player using findlocale
	string lang=getSettings()->getValue("lang");
	if(lang.length()>0){
		printf("Locale set by user to %s\n",lang.c_str());
		language=lang;
	}else{
		FL_Locale *locale;
		FL_FindLocale(&locale,FL_MESSAGES);
		printf("Locale isn't set by user: %s\n",locale->lang);

		language=locale->lang;
		if(locale->country!=NULL){
			language+=string("_")+string(locale->country);
		}
		if(locale->variant!=NULL){
			language+=string("@")+string(locale->variant);
		}

		FL_FreeLocale(&locale);
	}
	//Now set the language in the dictionaryManager.
	dictionaryManager->set_language(tinygettext::Language::from_name(language));

	//Disable annoying 'Couldn't translate: blah blah blah'
	tinygettext::Log::set_log_info_callback(NULL);
	
	//Set time format to the user-preference of the system.
	setlocale(LC_TIME,"");

	//Create the types of blocks.
	for(int i=0;i<TYPE_MAX;i++){
		Game::blockNameMap[Game::blockName[i]]=i;
	}

	//Structure that holds the event type/name pair.
	struct EventTypeName{
		int type;
		const char* name;
	};
	//Create the types of game object event types.
	{
		const EventTypeName types[]={
			{GameObjectEvent_PlayerWalkOn,"playerWalkOn"},
			{GameObjectEvent_PlayerIsOn,"playerIsOn"},
			{GameObjectEvent_PlayerLeave,"playerLeave"},
			{GameObjectEvent_OnCreate,"onCreate"},
			{GameObjectEvent_OnEnterFrame,"onEnterFrame"},
			{ GameObjectEvent_OnPlayerInteraction, "onPlayerInteraction" },
			{GameObjectEvent_OnToggle,"onToggle"},
			{GameObjectEvent_OnSwitchOn,"onSwitchOn"},
			{GameObjectEvent_OnSwitchOff,"onSwitchOff"},
			{0,NULL}
		};
		for(int i=0;types[i].name;i++){
			Game::gameObjectEventNameMap[types[i].name]=types[i].type;
			Game::gameObjectEventTypeMap[types[i].type]=types[i].name;
		}
	}
	//Create the types of level event types.
	{
		const EventTypeName types[]={
			{LevelEvent_OnCreate,"onCreate"},
			{LevelEvent_OnSave,"onSave"},
			{LevelEvent_OnLoad,"onLoad"},
			{0,NULL}
		};
		for(int i=0;types[i].name;i++){
			Game::levelEventNameMap[types[i].name]=types[i].type;
			Game::levelEventTypeMap[types[i].type]=types[i].name;
		}
	}

	//Nothing went wrong so we return true.
    return screenData;
}

static TTF_Font* loadFont(const char* name,int size){
	TTF_Font* tmpFont=TTF_OpenFont((getDataPath()+"font/"+name+".ttf").c_str(),size);
	if(tmpFont){
		return tmpFont;
	}else{
#if defined(ANDROID)
		//Android has built-in DroidSansFallback.ttf. (?)
		return TTF_OpenFont("/system/fonts/DroidSansFallback.ttf",size);
#else
		return TTF_OpenFont((getDataPath()+"font/DroidSansFallback.ttf").c_str(),size);
#endif
	}
}

bool loadFonts(){
	//Load the fonts.
	//NOTE: This is a separate method because it will be called separately when re-initing in case of language change.
	
	//First close the fonts if needed.
	if(fontTitle)
		TTF_CloseFont(fontTitle);
	if(fontGUI)
		TTF_CloseFont(fontGUI);
	if(fontGUISmall)
		TTF_CloseFont(fontGUISmall);
	if(fontText)
		TTF_CloseFont(fontText);
	if(fontMono)
		TTF_CloseFont(fontMono);
  	
	/// TRANSLATORS: Font used in GUI:
	///  - Use "knewave" for languages using Latin and Latin-derived alphabets
	///  - "DroidSansFallback" can be used for non-Latin writing systems
	fontTitle=loadFont(_("knewave"),55);
	fontGUI=loadFont(_("knewave"),32);
	fontGUISmall=loadFont(_("knewave"),24);
	/// TRANSLATORS: Font used for normal text:
	///  - Use "Blokletters-Viltstift" for languages using Latin and Latin-derived alphabets
	///  - "DroidSansFallback" can be used for non-Latin writing systems
	fontText=loadFont(_("Blokletters-Viltstift"),16);
	fontMono=loadFont("VeraMono",12);
	if(fontTitle==NULL || fontGUI==NULL || fontGUISmall==NULL || fontText==NULL || fontMono==NULL){
		printf("ERROR: Unable to load fonts! \n");
		return false;
	}
	
	//Nothing went wrong so return true.
	return true;
}

//Generate small arrows used for some GUI widgets.
static void generateArrows(SDL_Renderer& renderer){
	TTF_Font* fontArrow=loadFont(_("knewave"),18);
	
    arrowLeft1=textureFromText(renderer,*fontArrow,"<",objThemes.getTextColor(false));
    arrowRight1=textureFromText(renderer,*fontArrow,">",objThemes.getTextColor(false));
    arrowLeft2=textureFromText(renderer,*fontArrow,"<",objThemes.getTextColor(true));
    arrowRight2=textureFromText(renderer,*fontArrow,">",objThemes.getTextColor(true));
	
	TTF_CloseFont(fontArrow);
}

bool loadTheme(ImageManager& imageManager,SDL_Renderer& renderer,std::string name){
	//Load default fallback theme if it isn't loaded yet
	if(objThemes.themeCount()==0){
        if(objThemes.appendThemeFromFile(getDataPath()+"themes/Cloudscape/theme.mnmstheme", imageManager, renderer)==NULL){
			printf("ERROR: Can't load default theme file\n");
			return false;
		}
	}
	
	//Resize background or load specific theme
	bool success=true;
	if(name==""||name.empty()){
		objThemes.scaleToScreen();
	}else{
		string theme=processFileName(name);
        if(objThemes.appendThemeFromFile(theme+"/theme.mnmstheme", imageManager, renderer)==NULL){
			printf("ERROR: Can't load theme %s\n",theme.c_str());
			success=false;
		}
	}
	
    generateArrows(renderer);
	
	//Everything went fine so return true.
	return success;
}

static SDL_Cursor* loadCursor(const char* image[]){
	int i,row,col;
	//The array that holds the data (0=white 1=black)
	Uint8 data[4*32];
	//The array that holds the alpha mask (0=transparent 1=visible)
	Uint8 mask[4*32];
	//The coordinates of the hotspot of the cursor.
	int hotspotX, hotspotY;
	
	i=-1;
	//Loop through the rows and columns.
	//NOTE: We assume a cursor size of 32x32.
	for(row=0;row<32;++row){
		for(col=0; col<32;++col){
			if(col % 8) {
				data[i]<<=1;
				mask[i]<<=1;
			}else{
				++i;
				data[i]=mask[i]=0;
			}
			switch(image[4+row][col]){
				case '+':
					data[i] |= 0x01;
					mask[i] |= 0x01;
					break;
				case '.':
					mask[i] |= 0x01;
					break;
				default:
					break;
			}
		}
	}
	//Get the hotspot x and y locations from the last line of the cursor.
	sscanf(image[4+row],"%d,%d",&hotspotX,&hotspotY);
	return SDL_CreateCursor(data,mask,32,32,hotspotX,hotspotY);
}

bool loadFiles(ImageManager& imageManager, SDL_Renderer& renderer){
	//Load the fonts.
	if(!loadFonts())
		return false;
	
	//Show a loading screen
	{
        int w = 0,h = 0;
        SDL_GetRendererOutputSize(&renderer, &w, &h);
        SDL_Color fg={255,255,255,0};
		TexturePtr loadingTexture = titleTextureFromText(renderer, _("Loading..."), fg, w);
        SDL_Rect loadingRect = rectFromTexture(*loadingTexture);
        loadingRect.x = (w-loadingRect.w)/2;
        loadingRect.y = (h-loadingRect.h)/2;
        SDL_RenderCopy(sdlRenderer, loadingTexture.get(), NULL, &loadingRect);
		SDL_RenderPresent(sdlRenderer);
        SDL_RenderClear(sdlRenderer);
	}

	musicManager.destroy();
	//Load the music and play it.
	if(musicManager.loadMusic((getDataPath()+"music/menu.music")).empty()){
		printf("WARNING: Unable to load background music! \n");
	}
	musicManager.playMusic("menu",false);

	//Load all the music lists from the data and user data path.
	{
		vector<string> musicLists=enumAllFiles((getDataPath()+"music/"),"list",true);
		for(unsigned int i=0;i<musicLists.size();i++)
			getMusicManager()->loadMusicList(musicLists[i]);
		musicLists=enumAllFiles((getUserPath(USER_DATA)+"music/"),"list",true);
		for(unsigned int i=0;i<musicLists.size();i++)
			getMusicManager()->loadMusicList(musicLists[i]);
	}
	//Set the list to the configured one.
	getMusicManager()->setMusicList(getSettings()->getValue("musiclist"));
	
	//Check if music is enabled.
	if(getSettings()->getBoolValue("music"))
		getMusicManager()->setEnabled();
	
	//Load the sound effects
	soundManager.loadSound((getDataPath()+"sfx/jump.wav").c_str(),"jump");
	soundManager.loadSound((getDataPath()+"sfx/hit.wav").c_str(),"hit");
	soundManager.loadSound((getDataPath()+"sfx/checkpoint.wav").c_str(),"checkpoint");
	soundManager.loadSound((getDataPath()+"sfx/swap.wav").c_str(),"swap");
    soundManager.loadSound((getDataPath()+"sfx/toggle.ogg").c_str(),"toggle");
	soundManager.loadSound((getDataPath()+"sfx/error.wav").c_str(),"error");
	soundManager.loadSound((getDataPath()+"sfx/collect.wav").c_str(),"collect");
	soundManager.loadSound((getDataPath()+"sfx/achievement.ogg").c_str(),"achievement");

	//Load the cursor images from the Cursor.h file.
	cursors[CURSOR_POINTER]=loadCursor(pointer);
	cursors[CURSOR_CARROT]=loadCursor(ibeam);
	cursors[CURSOR_DRAG]=loadCursor(closedhand);
	cursors[CURSOR_SIZE_HOR]=loadCursor(size_hor);
	cursors[CURSOR_SIZE_VER]=loadCursor(size_ver);
	cursors[CURSOR_SIZE_FDIAG]=loadCursor(size_fdiag);
	cursors[CURSOR_SIZE_BDIAG]=loadCursor(size_bdiag);
	cursors[CURSOR_REMOVE]=loadCursor(remove_cursor);
	cursors[CURSOR_POINTING_HAND] = loadCursor(pointing_hand);
	//Set the default cursor right now.
	SDL_SetCursor(cursors[CURSOR_POINTER]);

	levelPackManager.destroy();
	//Now sum up all the levelpacks.
	vector<string> v=enumAllDirs(getDataPath()+"levelpacks/");
	for(vector<string>::iterator i=v.begin(); i!=v.end(); ++i){
		levelPackManager.loadLevelPack(getDataPath()+"levelpacks/"+*i);
	}
	v=enumAllDirs(getUserPath(USER_DATA)+"levelpacks/");
	for(vector<string>::iterator i=v.begin(); i!=v.end(); ++i){
		levelPackManager.loadLevelPack(getUserPath(USER_DATA)+"levelpacks/"+*i);
	}
	v=enumAllDirs(getUserPath(USER_DATA)+"custom/levelpacks/");
	for(vector<string>::iterator i=v.begin(); i!=v.end(); ++i){
		levelPackManager.loadLevelPack(getUserPath(USER_DATA)+"custom/levelpacks/"+*i);
	}
	//Now we add a special levelpack that will contain the levels not in a levelpack.
	LevelPack* levelsPack=new LevelPack;
	levelsPack->levelpackName="Levels";
	levelsPack->levelpackPath="Levels/";
	levelsPack->type=COLLECTION;
	LevelPack* customLevelsPack=new LevelPack;
	customLevelsPack->levelpackName="Custom Levels";
	customLevelsPack->levelpackPath="Custom Levels/";
	customLevelsPack->type=COLLECTION;

	//List the main levels and add them one for one.
	v = enumAllFiles(getDataPath() + "levels/");
	for (vector<string>::iterator i = v.begin(); i != v.end(); ++i){
		levelsPack->addLevel(getDataPath() + "levels/" + *i);
		levelsPack->setLocked(levelsPack->getLevelCount() - 1);
	}
	//List the addon levels and add them one for one.
	v=enumAllFiles(getUserPath(USER_DATA)+"levels/");
	for(vector<string>::iterator i=v.begin(); i!=v.end(); ++i){
		levelsPack->addLevel(getUserPath(USER_DATA)+"levels/"+*i);
		levelsPack->setLocked(levelsPack->getLevelCount()-1);
	}
	//List the custom levels and add them one for one.
	v=enumAllFiles(getUserPath(USER_DATA)+"custom/levels/");
	for(vector<string>::iterator i=v.begin(); i!=v.end(); ++i){
		levelsPack->addLevel(getUserPath(USER_DATA)+"custom/levels/"+*i);
		levelsPack->setLocked(levelsPack->getLevelCount()-1);
		
		customLevelsPack->addLevel(getUserPath(USER_DATA)+"custom/levels/"+*i);
		customLevelsPack->setLocked(customLevelsPack->getLevelCount()-1);
	}
	
	//Add them to the manager.
	levelPackManager.addLevelPack(levelsPack);
	levelPackManager.addLevelPack(customLevelsPack);

	//Load statistics
    statsMgr.loadPicture(renderer, imageManager);
    statsMgr.registerAchievements(imageManager);
	statsMgr.loadFile(getUserPath(USER_CONFIG)+"statistics");

	//Do something ugly and slow
	statsMgr.reloadCompletedLevelsAndAchievements();
	statsMgr.reloadOtherAchievements();
	
	//Load the theme, both menu and default.
	//NOTE: Loading theme may fail and returning false would stop everything, default theme will be used instead.
    if (!loadTheme(imageManager,renderer,getSettings()->getValue("theme"))){
		getSettings()->setValue("theme","%DATA%/themes/Cloudscape");
		saveSettings();
	}
	
	//Nothing failed so return true.
	return true;
}

bool loadSettings(){
	//Check the version of config file.
	int version = 0;
	std::string cfgV05 = getUserPath(USER_CONFIG) + "meandmyshadow_V0.5.cfg";
	std::string cfgV04 = getUserPath(USER_CONFIG) + "meandmyshadow.cfg";

	if (fileExists(cfgV05.c_str())) {
		//We find a config file of current version.
		version = 0x000500;
	} else if (fileExists(cfgV04.c_str())) {
		//We find a config file of V0.4 version or earlier.
		copyFile(cfgV04.c_str(), cfgV05.c_str());
		version = 0x000400;
	} else {
		//No config file found, just create a new one.
		version = 0x000500;
	}
	settings=new Settings(cfgV05);
	settings->parseFile(version);
	
	//Now apply settings changed through command line arguments, if any.
	map<string,string>::iterator it;
	for(it=tmpSettings.begin();it!=tmpSettings.end();++it){
		settings->setValue(it->first,it->second);
	}
	tmpSettings.clear();
  
	//Always return true?
	return true;
}

bool saveSettings(){
	return settings->save();
}

Settings* getSettings(){
	return settings;
}

MusicManager* getMusicManager(){
	return &musicManager;
}

SoundManager* getSoundManager(){
	return &soundManager;
}

LevelPackManager* getLevelPackManager(){
	return &levelPackManager;
}

void flipScreen(SDL_Renderer& renderer){
    // Render the data from the back buffer.
    SDL_RenderPresent(&renderer);
}

void clean(){
	//Save statistics
	statsMgr.saveFile(getUserPath(USER_CONFIG)+"statistics");	

	//We delete the settings.
	if(settings){
		delete settings;
		settings=NULL;
	}
	
	//Delete dictionaryManager.
	delete dictionaryManager;
	
	//Get rid of the currentstate.
	//NOTE: The state is probably already deleted by the changeState function.
	if(currentState)
		delete currentState;

	//Destroy the GUI if present.
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}
	
    //These calls to destroy makes sure stuff is
    //deleted before SDL is uninitialised (as these managers are stack allocated
    //globals.)
	
	//Destroy the musicManager.
	musicManager.destroy();

	//Destroy all sounds
	soundManager.destroy();

	//Destroy the cursors.
	for(int i=0;i<CURSOR_MAX;i++){
		SDL_FreeCursor(cursors[i]);
		cursors[i]=NULL;
	}

	//Destroy the levelPackManager.
	levelPackManager.destroy();
	levels=NULL;
	
	//Close all joysticks.
	inputMgr.closeAllJoysticks();
	
	//Close the fonts and quit SDL_ttf.
	TTF_CloseFont(fontTitle);
	TTF_CloseFont(fontGUI);
	TTF_CloseFont(fontGUISmall);
	TTF_CloseFont(fontText);
	TTF_CloseFont(fontMono);
	TTF_Quit();
	
	//Remove the temp surface.
    SDL_DestroyRenderer(sdlRenderer);
    SDL_DestroyWindow(sdlWindow);
    arrowLeft1.reset(nullptr);
    arrowLeft2.reset(nullptr);
    arrowRight1.reset(nullptr);
    arrowRight2.reset(nullptr);
	
	//Stop audio.and quit
	Mix_CloseAudio();
    //SDL2 porting note. Not sure why this was only done on apple.
//#ifndef __APPLE__
	Mix_Quit();
//#endif
	//And finally quit SDL.
	SDL_Quit();
}

void setNextState(int newstate){
	//Only change the state when we aren't already exiting.
	if(nextState!=STATE_EXIT){
		nextState=newstate;
	}
}

void changeState(ImageManager& imageManager, SDL_Renderer& renderer, int fade){

	//Check if there's a nextState.
	if(nextState!=STATE_NULL){
		//Fade out, if fading is enabled.
		if (currentState && settings->getBoolValue("fading")) {
			for (; fade >= 0; fade -= 17) {
				currentState->render(imageManager, renderer);
				//TODO: Shouldn't the gamestate take care of rendering the GUI?
				if (GUIObjectRoot) GUIObjectRoot->render(renderer);

				dimScreen(renderer, static_cast<Uint8>(255 - fade));

				//draw new achievements (if any) as overlay
				statsMgr.render(imageManager, renderer);

				flipScreen(renderer);

				SDL_Delay(1000/FPS);
			}
		}

		//Delete the currentState.
		delete currentState;
		currentState=NULL;

		//Set the currentState to the nextState.
		stateID=nextState;
		nextState=STATE_NULL;

		//Init the state.
		switch(stateID){
		case STATE_GAME:
			{
				currentState=NULL;
                Game* game=new Game(renderer, imageManager);
				currentState=game;
				//Check if we should load record file or a level.
				if(!Game::recordFile.empty()){
                    game->loadRecord(imageManager,renderer,Game::recordFile.c_str());
					Game::recordFile.clear();
				}else{
                    game->loadLevel(imageManager,renderer,levels->getLevelFile());
					levels->saveLevelProgress();
				}
			}
			break;
		case STATE_MENU:
            currentState=new Menu(imageManager, renderer);
			break;
		case STATE_LEVEL_SELECT:
            currentState=new LevelPlaySelect(imageManager, renderer);
			break;
		case STATE_LEVEL_EDIT_SELECT:
            currentState=new LevelEditSelect(imageManager, renderer);
			break;
		case STATE_LEVEL_EDITOR:
			{
				currentState=NULL;
                LevelEditor* levelEditor=new LevelEditor(renderer, imageManager);
				currentState=levelEditor;
				//Load the selected level.
                levelEditor->loadLevel(imageManager,renderer,levels->getLevelFile());
			}
			break;
		case STATE_OPTIONS:
            currentState=new Options(imageManager, renderer);
			break;
		case STATE_ADDONS:
            currentState=new Addons(renderer, imageManager);
			break;
		case STATE_CREDITS:
            currentState=new Credits(imageManager,renderer);
			break;
		case STATE_STATISTICS:
            currentState=new StatisticsScreen(imageManager,renderer);
			break;
		}
		//NOTE: STATE_EXIT isn't mentioned, meaning that currentState is null.
		//This way the game loop will break and the program will exit.
	}
}

void musicStoppedHook(){
	//We just call the musicStopped method of the MusicManager.
	musicManager.musicStopped();
}

void channelFinishedHook(int channel){
	soundManager.channelFinished(channel);
}

bool checkCollision(const SDL_Rect& a,const SDL_Rect& b){
	//Check if the left side of box a isn't past the right side of b.
	if(a.x>=b.x+b.w){
		return false;
	}
	//Check if the right side of box a isn't left of the left side of b.
	if(a.x+a.w<=b.x){
		return false;
	}
	//Check if the top side of box a isn't under the bottom side of b.
	if(a.y>=b.y+b.h){
		return false;
	}
	//Check if the bottom side of box a isn't above the top side of b.
	if(a.y+a.h<=b.y){
		return false;
	}

	//We have collision.
	return true;
}

bool pointOnRect(const SDL_Rect& point, const SDL_Rect& rect) {
	if(point.x >= rect.x && point.x < rect.x + rect.w
		&& point.y >= rect.y && point.y < rect.y + rect.h) {
		return true;
	}
	return false;
}

int parseArguments(int argc, char** argv){
	//Loop through all arguments.
	//We start at one since 0 is the command itself.
	for(int i=1;i<argc;i++){
		string argument=argv[i];
		
		//Check if the argument is the data-dir.
		if(argument=="--data-dir"){
			//We need a second argument so we increase i.
			i++;
			if(i>=argc){
				printf("ERROR: Missing argument for command '%s'\n\n",argument.c_str());
				return -1;
			}
			
			//Configure the dataPath with the given path.
			dataPath=argv[i];
			if(!getDataPath().empty()){
				char c=dataPath[dataPath.size()-1];
				if(c!='/'&&c!='\\') dataPath+="/";
			}
		}else if(argument=="--user-dir"){
			//We need a second argument so we increase i.
			i++;
			if(i>=argc){
				printf("ERROR: Missing argument for command '%s'\n\n",argument.c_str());
				return -1;
			}
			
			//Configure the userPath with the given path.
			userPath=argv[i];
			if(!userPath.empty()){
				char c=userPath[userPath.size()-1];
				if(c!='/'&&c!='\\') userPath+="/";
			}
		}else if(argument=="-f" || argument=="-fullscreen" || argument=="--fullscreen"){
			tmpSettings["fullscreen"]="1";
		}else if(argument=="-w" || argument=="-windowed" || argument=="--windowed"){
			tmpSettings["fullscreen"]="0";
		}else if(argument=="-mv" || argument=="-music" || argument=="--music"){
			//We need a second argument so we increase i.
			i++;
			if(i>=argc){
				printf("ERROR: Missing argument for command '%s'\n\n",argument.c_str());
				return -1;
			}
			
			//Now set the music volume.
			tmpSettings["music"]=argv[i];
		}else if(argument=="-sv" || argument=="-sound" || argument=="--sound"){
			//We need a second argument so we increase i.
			i++;
			if(i>=argc){
				printf("ERROR: Missing argument for command '%s'\n\n",argument.c_str());
				return -1;
			}
			
			//Now set sound volume.
			tmpSettings["sound"]=argv[i];
		}else if(argument=="-set" || argument=="--set"){
			//We need a second and a third argument so we increase i.
			i+=2;
			if(i>=argc){
				printf("ERROR: Missing argument for command '%s'\n\n",argument.c_str());
				return -1;
			}
			
			//And set the setting.
			tmpSettings[argv[i-1]]=argv[i];
		}else if(argument=="-v" || argument=="-version" || argument=="--version"){
			//Print the version.
			printf("%s\n",version.c_str());
			return 0;
		}else if(argument=="-h" || argument=="-help" || argument=="--help"){
			//If the help is requested we'll return false without printing an error.
			//This way the usage/help text will be printed.
			return -1;
		}else{
			//Any other argument is unknow so we return false.
			printf("ERROR: Unknown argument %s\n\n",argument.c_str());
			return -1;
		}
	}
	
	//If everything went well we can return true.
	return 1;
}

//Special structure that will recieve the GUIEventCallbacks of the messagebox.
struct msgBoxHandler:public GUIEventCallback{
public:
	//Integer containing the ret(urn) value of the messageBox.
	int ret;
public:
	//Constructor.
	msgBoxHandler():ret(0){}
	
    void GUIEventCallback_OnEvent(ImageManager& imageManager, SDL_Renderer& renderer, std::string name,GUIObject* obj,int eventType){
		//Make sure it's a click event.
		if(eventType==GUIEventClick){
			//Set the return value.
			ret=obj->value;
			
			//After a click event we can delete the GUI.
			if(GUIObjectRoot){
				delete GUIObjectRoot;
				GUIObjectRoot=NULL;
			}
		}
	}
};

msgBoxResult msgBox(ImageManager& imageManager,SDL_Renderer& renderer, const string& prompt,msgBoxButtons buttons,const string& title){
	//Create the event handler.
	msgBoxHandler objHandler;
	//The GUI objects.
	GUIObject* obj;
	
	//Create the GUIObjectRoot, the height and y location is temp.
	//It depends on the content what it will be.
    GUIObject* root=new GUIFrame(imageManager,renderer,(SCREEN_WIDTH-600)/2,200,600,200,title.c_str());
	
	//Integer containing the current y location used to grow dynamic depending on the content.
	int y=50;
	
	//Now process the prompt.
	{
		//NOTE: We shouldn't modify the cotents in the c_str() of a string,
		//since it's said that at least in g++ the std::string is copy-on-write
		//hence if we modify the content it may break

		//The copy of the prompt.
		std::vector<char> copyOfPrompt(prompt.begin(), prompt.end());

		//Append another '\0' to it.
		copyOfPrompt.push_back(0);

		//Pointer to the string.
		char* lps = &(copyOfPrompt[0]);
		//Pointer to a character.
		char* lp=NULL;
		
		//We keep looping forever.
		//The only way out is with the break statement.
		for(;;){
			//As long as it's still the same sentence we continue.
			//It will stop when there's a newline or end of line.
			for(lp=lps;*lp!='\n'&&*lp!='\r'&&*lp!=0;lp++);
			
			//Store the character we stopped on. (End or newline)
			char c=*lp;
			//Set the character in the string to 0, making lps a string containing one sentence.
			*lp=0;
			
			//Add a GUIObjectLabel with the sentence.
            root->addChild(new GUILabel(imageManager,renderer,0,y,root->width,25,lps,0,true,true,GUIGravityCenter));
			//Increase y with 25, about the height of the text.
			y+=25;
			
			//Check the stored character if it was a stop.
			if(c==0){
				//It was so break out of the for loop.
				lps=lp;
				break;
			}
			//It wasn't meaning more will follow.
			//We set lps to point after the "newline" forming a new string.
			lps=lp+1;
		}
	}
	//Add 70 to y to leave some space between the content and the buttons.
	y+=70;
	//Recalc the size of the message box.
	root->top=(SCREEN_HEIGHT-y)/2;
	root->height=y;
	
	//Now we need to add the buttons.
	//Integer containing the number of buttons to add.
	int count=0;
	//Array with the return codes for the buttons.
	int value[3]={0};
	//Array containing the captation for the buttons.
	string button[3]={"","",""};
	switch(buttons){
	case MsgBoxOKCancel:
		count=2;
		button[0]=_("OK");value[0]=MsgBoxOK;
		button[1]=_("Cancel");value[1]=MsgBoxCancel;
		break;
	case MsgBoxAbortRetryIgnore:
		count=3;
		button[0]=_("Abort");value[0]=MsgBoxAbort;
		button[1]=_("Retry");value[1]=MsgBoxRetry;
		button[2]=_("Ignore");value[2]=MsgBoxIgnore;
		break;
	case MsgBoxYesNoCancel:
		count=3;
		button[0]=_("Yes");value[0]=MsgBoxYes;
		button[1]=_("No");value[1]=MsgBoxNo;
		button[2]=_("Cancel");value[2]=MsgBoxCancel;
		break;
	case MsgBoxYesNo:
		count=2;
		button[0]=_("Yes");value[0]=MsgBoxYes;
		button[1]=_("No");value[1]=MsgBoxNo;
		break;
	case MsgBoxRetryCancel:
		count=2;
		button[0]=_("Retry");value[0]=MsgBoxRetry;
		button[1]=_("Cancel");value[1]=MsgBoxCancel;
		break;
	default:
		count=1;
		button[0]=_("OK");value[0]=MsgBoxOK;
		break;
	}
	
	//Now we start making the buttons.
	{
		//Reduce y so that the buttons fit inside the frame.
		y-=40;
		
		double places[3]={0.0};
		if(count==1){
			places[0]=0.5;
		}else if(count==2){
			places[0]=0.4;
			places[1]=0.6;
		}else if(count==3){
			places[0]=0.3;
			places[1]=0.5;
			places[2]=0.7;
		}
		
		//Loop to add the buttons.
		for(int i=0;i<count;i++){
            obj=new GUIButton(imageManager,renderer,root->width*places[i],y,-1,36,button[i].c_str(),value[i],true,true,GUIGravityCenter);
			obj->eventCallback=&objHandler;
			root->addChild(obj);
		}
	}
	
	//Now we dim the screen and keep the GUI rendering/updating.
    GUIOverlay* overlay=new GUIOverlay(renderer,root);
	overlay->keyboardNavigationMode = LeftRightFocus | UpDownFocus | TabFocus | ((count == 1) ? 0 : ReturnControls);
	overlay->enterLoop(imageManager, renderer, true, count == 1);
	
	//And return the result.
	return (msgBoxResult)objHandler.ret;
}

// A helper function to read a character from utf8 string
// s: the string
// p [in,out]: the position
// return value: the character readed, in utf32 format, 0 means end of string, -1 means error
int utf8ReadForward(const char* s, int& p) {
	int ch = (unsigned char)s[p];
	if (ch < 0x80){
		if (ch) p++;
		return ch;
	} else if (ch < 0xC0){
		// skip invalid characters
		while (((unsigned char)s[p] & 0xC0) == 0x80) p++;
		return -1;
	} else if (ch < 0xE0){
		int c2 = (unsigned char)s[++p];
		if ((c2 & 0xC0) != 0x80) return -1;

		ch = ((ch & 0x1F) << 6) | (c2 & 0x3F);
		p++;
		return ch;
	} else if (ch < 0xF0){
		int c2 = (unsigned char)s[++p];
		if ((c2 & 0xC0) != 0x80) return -1;
		int c3 = (unsigned char)s[++p];
		if ((c3 & 0xC0) != 0x80) return -1;

		ch = ((ch & 0xF) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
		p++;
		return ch;
	} else if (ch < 0xF8){
		int c2 = (unsigned char)s[++p];
		if ((c2 & 0xC0) != 0x80) return -1;
		int c3 = (unsigned char)s[++p];
		if ((c3 & 0xC0) != 0x80) return -1;
		int c4 = (unsigned char)s[++p];
		if ((c4 & 0xC0) != 0x80) return -1;

		ch = ((ch & 0x7) << 18) | ((c2 & 0x3F) << 12) | ((c3 & 0x3F) << 6) | (c4 & 0x3F);
		if (ch >= 0x110000) ch = -1;
		p++;
		return ch;
	} else {
		p++;
		return -1;
	}
}

// A helper function to read a character backward from utf8 string (experimental)
// s: the string
// p [in,out]: the position
// return value: the character readed, in utf32 format, 0 means end of string, -1 means error
int utf8ReadBackward(const char* s, int& p) {
	if (p <= 0) return 0;

	do {
		p--;
	} while (p > 0 && ((unsigned char)s[p] & 0xC0) == 0x80);

	int tmp = p;
	return utf8ReadForward(s, tmp);
}

#ifndef WIN32

// ad-hoc function to check if a program is installed
static bool programExists(const std::string& program) {
	std::string p = tfm::format("which \"%s\" 2>&1", program);

	const int BUFSIZE = 128;

	char buf[BUFSIZE];
	FILE *fp;

	if ((fp = popen(p.c_str(), "r")) == NULL) {
		return false;
	}

	while (fgets(buf, BUFSIZE, fp) != NULL) {
		// Drop all outputs since 'which' returns -1 when the program is not found
	}

	if (pclose(fp))  {
		return false;
	}

	return true;
}

#endif

void openWebsite(const std::string& url) {
#ifdef WIN32
	wchar_t ws[4096];
	TO_UTF16(url.c_str(), ws);
	SDL_SysWMinfo info = {};
	SDL_VERSION(&info.version);
	SDL_GetWindowWMInfo(sdlWindow, &info);
	ShellExecuteW(info.info.win.window, L"open", ws, NULL, NULL, SW_SHOW);
#else
	static int method = -1;

	// Some of these methods are copied from https://stackoverflow.com/questions/5116473/

	const char* methods[] = {
		"xdg-open", "xdg-open \"%s\"",
		"gnome-open", "gnome-open \"%s\"",
		"kde-open", "kde-open \"%s\"",
		"open", "open \"%s\"",
		"python", "python -m webbrowser \"%s\"",
		"sensible-browser", "sensible-browser \"%s\"",
		"x-www-browser", "x-www-browser \"%s\"",
		NULL,
	};

	if (method < 0) {
		for (method = 0; methods[method]; method += 2) {
			if (programExists(methods[method])) break;
		}
	}

	if (methods[method]) {
		std::string p = tfm::format(methods[method + 1], url);
		system(p.c_str());
	} else {
		fprintf(stderr, "TODO: openWebsite is not implemented on your system\n");
	}
#endif
}

std::string appendURLToLicense(const std::string& license) {
	// if the license doesn't include url, try to detect it from a predefined list
	if (license.find("://") == std::string::npos) {
		std::string normalized;
		for (char c : license) {
			if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z')) {
				normalized.push_back(c);
			} else if (c >= 'a' && c <= 'z') {
				normalized.push_back(c + ('A' - 'a'));
			}
		}

		const char* licenses[] = {
			// AGPL
			"AGPL1", "AGPLV1", NULL, "http://www.affero.org/oagpl.html",
			"AGPL2", "AGPLV2", NULL, "http://www.affero.org/agpl2.html",
			"AGPL", NULL, "https://gnu.org/licenses/agpl.html",
			// LGPL
			"LGPL21", "LGPLV21", NULL, "https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html",
			"LGPL2", "LGPLV2", NULL, "https://www.gnu.org/licenses/old-licenses/lgpl-2.0.html",
			"LGPL", NULL, "https://www.gnu.org/copyleft/lesser.html",
			// GPL
			"GPL1", "GPLV1", NULL, "https://www.gnu.org/licenses/old-licenses/gpl-1.0.html",
			"GPL2", "GPLV2", NULL, "https://www.gnu.org/licenses/old-licenses/gpl-2.0.html",
			"GPL", NULL, "https://gnu.org/licenses/gpl.html",
			// CC BY-NC-ND
			"CCBYNCND1", "CCBYNDNC1", NULL, "https://creativecommons.org/licenses/by-nd-nc/1.0",
			"CCBYNCND25", "CCBYNDNC25", NULL, "https://creativecommons.org/licenses/by-nc-nd/2.5",
			"CCBYNCND2", "CCBYNDNC2", NULL, "https://creativecommons.org/licenses/by-nc-nd/2.0",
			"CCBYNCND3", "CCBYNDNC3", NULL, "https://creativecommons.org/licenses/by-nc-nd/3.0",
			"CCBYNCND", "CCBYNDNC", NULL, "https://creativecommons.org/licenses/by-nc-nd/4.0",
			// CC BY-NC-SA
			"CCBYNCSA1", NULL, "https://creativecommons.org/licenses/by-nc-sa/1.0",
			"CCBYNCSA25", NULL, "https://creativecommons.org/licenses/by-nc-sa/2.5",
			"CCBYNCSA2", NULL, "https://creativecommons.org/licenses/by-nc-sa/2.0",
			"CCBYNCSA3", NULL, "https://creativecommons.org/licenses/by-nc-sa/3.0",
			"CCBYNCSA", NULL, "https://creativecommons.org/licenses/by-nc-sa/4.0",
			// CC BY-ND
			"CCBYND1", NULL, "https://creativecommons.org/licenses/by-nd/1.0",
			"CCBYND25", NULL, "https://creativecommons.org/licenses/by-nd/2.5",
			"CCBYND2", NULL, "https://creativecommons.org/licenses/by-nd/2.0",
			"CCBYND3", NULL, "https://creativecommons.org/licenses/by-nd/3.0",
			"CCBYND", NULL, "https://creativecommons.org/licenses/by-nd/4.0",
			// CC BY-NC
			"CCBYNC1", NULL, "https://creativecommons.org/licenses/by-nc/1.0",
			"CCBYNC25", NULL, "https://creativecommons.org/licenses/by-nc/2.5",
			"CCBYNC2", NULL, "https://creativecommons.org/licenses/by-nc/2.0",
			"CCBYNC3", NULL, "https://creativecommons.org/licenses/by-nc/3.0",
			"CCBYNC", NULL, "https://creativecommons.org/licenses/by-nc/4.0",
			// CC BY-SA
			"CCBYSA1", NULL, "https://creativecommons.org/licenses/by-sa/1.0",
			"CCBYSA25", NULL, "https://creativecommons.org/licenses/by-sa/2.5",
			"CCBYSA2", NULL, "https://creativecommons.org/licenses/by-sa/2.0",
			"CCBYSA3", NULL, "https://creativecommons.org/licenses/by-sa/3.0",
			"CCBYSA", NULL, "https://creativecommons.org/licenses/by-sa/4.0",
			// CC BY
			"CCBY1", NULL, "https://creativecommons.org/licenses/by/1.0",
			"CCBY25", NULL, "https://creativecommons.org/licenses/by/2.5",
			"CCBY2", NULL, "https://creativecommons.org/licenses/by/2.0",
			"CCBY3", NULL, "https://creativecommons.org/licenses/by/3.0",
			"CCBY", NULL, "https://creativecommons.org/licenses/by/4.0",
			// CC0
			"CC0", NULL, "https://creativecommons.org/publicdomain/zero/1.0",
			// WTFPL
			"WTFPL", NULL, "http://www.wtfpl.net/",
			// end
			NULL,
		};

		for (int i = 0; licenses[i]; i++) {
			bool found = false;
			for (; licenses[i]; i++) {
				if (normalized.find(licenses[i]) != std::string::npos) found = true;
			}
			i++;
			if (found) {
				return license + tfm::format(" <%s>", licenses[i]);
			}
		}
	}

	return license;
}

int getKeyboardRepeatDelay() {
	static int ret = -1;

	if (ret < 0) {
#ifdef WIN32
		int i = 0;
		SystemParametersInfoW(SPI_GETKEYBOARDDELAY, 0, &i, 0);
		// NOTE: these weird numbers are derived from Microsoft's documentation explaining the return value of SystemParametersInfo.
		i = clamp(i, 0, 3);
		ret = (i + 1) * 10;
#else
		// TODO: platform-dependent code
		// Assume it's 250ms, i.e. 10 frames
		ret = 10;
#endif
		// Debug
#ifdef _DEBUG
		printf("getKeyboardRepeatDelay() = %d\n", ret);
#endif
	}

	return ret;
}

int getKeyboardRepeatInterval() {
	static int ret = -1;

	if (ret < 0) {
#ifdef WIN32
		int i = 0;
		SystemParametersInfoW(SPI_GETKEYBOARDSPEED, 0, &i, 0);
		// NOTE: these weird numbers are derived from Microsoft's documentation explaining the return value of SystemParametersInfo.
		i = clamp(i, 0, 31);
		ret = (int)floor(40.0f / (2.5f + 0.887097f * (float)i) + 0.5f);
#else
		// TODO: platform-dependent code
		// Assume it's 25ms, i.e. 1 frame
		ret = 1;
#endif
		// Debug
#ifdef _DEBUG
		printf("getKeyboardRepeatInterval() = %d\n", ret);
#endif
	}

	return ret;
}
