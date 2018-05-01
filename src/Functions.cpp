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

#ifdef HARDWARE_ACCELERATION
#include <GL/gl.h>
#include <GL/glu.h>

//fix some Windows header bug
#ifndef GL_BGR
#define GL_BGR GL_BGR_EXT
#endif
#ifndef GL_BGRA
#define GL_BGRA GL_BGRA_EXT
#endif

#endif
using namespace std;

#ifdef WIN32
#include <windows.h>
#include <shlobj.h>
#else
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#endif

//Workaround for the resizing below 800x600 for X systems.
#if defined(__linux__) && !defined(ANDROID)
#include<X11/Xlib.h>
#include<X11/Xutil.h>
#define __X11_INCLUDED__
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

//The scriptExecutor used for executing scripts.
ScriptExecutor scriptExecutor;

//Map containing changed settings using command line arguments.
map<string,string> tmpSettings;
//Pointer to the settings object.
//It is used to load and save the settings file and change the settings.
Settings* settings=nullptr;

SDL_Renderer* sdlRenderer=nullptr;

#ifdef HARDWARE_ACCELERATION
GLuint screenTexture;
#endif

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
	
	//Check if we should use gl or software rendering.
	if(settings->getBoolValue("gl")){
#ifdef HARDWARE_ACCELERATION
		SDL_GL_SetAttribute(SDL_GL_RED_SIZE,8);
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,8);
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,8);
		SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE,8);
		
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,16);
		SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE,32);
		
		SDL_GL_SetAttribute(SDL_GL_ACCUM_RED_SIZE,8);
		SDL_GL_SetAttribute(SDL_GL_ACCUM_GREEN_SIZE,8);
		SDL_GL_SetAttribute(SDL_GL_ACCUM_BLUE_SIZE,8);
		SDL_GL_SetAttribute(SDL_GL_ACCUM_ALPHA_SIZE,8);
		
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS,0);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES,0);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);
		
		//Set the video mode.
		Uint32 flags=SDL_HWSURFACE | SDL_OPENGL;
		if(settings->getBoolValue("fullscreen"))
			flags|=SDL_FULLSCREEN;
		else if(settings->getBoolValue("resizable"))
			flags|=SDL_RESIZABLE;
		if(SDL_SetVideoMode(SCREEN_WIDTH,SCREEN_HEIGHT,SCREEN_BPP,flags)==NULL){
			fprintf(stderr,"FATAL ERROR: SDL_SetVideoMode failed\n");
			return false;
		}
		
		//Delete the old screen.
		//Warning: only if previous mode is OpenGL mode.
		//NOTE: The previous mode can't switch during runtime.
		if(screen){
			SDL_FreeSurface(screen);
			screen=NULL;
		}

		//Create a screen 
		screen=SDL_CreateRGBSurface(SDL_HWSURFACE,SCREEN_WIDTH,SCREEN_HEIGHT,32,0x00FF0000,0x0000FF00,0x000000FF,0);
		
		//Create a texture.
		glDeleteTextures(1,&screenTexture);
		glGenTextures(1,&screenTexture);
		
		//And set up gl correctly.
		glClearColor(0, 0, 0, 0);
		glClearDepth(1.0f);
		glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 1, -1);
		glMatrixMode(GL_MODELVIEW);
		glEnable(GL_TEXTURE_2D);
		glLoadIdentity();
#else
		//NOTE: Hardware accelerated rendering requested but compiled without.
		fprintf(stderr,"FATAL ERROR: Unable to use hardware acceleration (compiled without).\n");
        return creationFailed();
#endif
	}else{
		//Set the flags.
//		Uint32 flags=SCREEN_FLAGS;
        //Flags are not used in SDL2.
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
    //		screen=SDL_SetVideoMode(SCREEN_WIDTH,SCREEN_HEIGHT,SCREEN_BPP,flags);
            SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, flags, &sdlWindow, &sdlRenderer);
            if(!sdlWindow || !sdlRenderer){
                std::cerr <<  "FATAL ERROR: SDL_CreateWindowAndRenderer failed.\nError: " << SDL_GetError() << std::endl;
                return creationFailed();
            }

            SDL_SetRenderDrawBlendMode(sdlRenderer, SDL_BlendMode::SDL_BLENDMODE_BLEND);

            // White background so we see the menu on failure.
            SDL_SetRenderDrawColor(sdlRenderer, 255, 255, 255, 255);
        } else if (sdlWindow) {
            if(SDL_SetWindowFullscreen(sdlWindow, flags & SDL_WINDOW_FULLSCREEN) != 0) {
                std::cerr << "WARNING: Failed to switch to fullscreen: " << SDL_GetError() << std::endl;
            };
            currentFlags = SDL_GetWindowFlags(sdlWindow);

            if((flags & SDL_WINDOW_FULLSCREEN ) || (currentFlags & SDL_WINDOW_FULLSCREEN_DESKTOP)) {
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

//Workaround for the resizing below 800x600 for Windows.
#ifdef WIN32

static WNDPROC m_OldWindowProc=NULL;

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
	if(msg==WM_GETMINMAXINFO){
		if(m_OldWindowProc){
			CallWindowProc(m_OldWindowProc,hwnd,msg,wParam,lParam);
		}else{
			DefWindowProc(hwnd,msg,wParam,lParam);
		}

		RECT r={0,0,800,600};
		AdjustWindowRect(&r,GetWindowLong(hwnd,GWL_STYLE),FALSE);

		MINMAXINFO *info=(MINMAXINFO*)lParam;
		info->ptMinTrackSize.x=r.right-r.left;
		info->ptMinTrackSize.y=r.bottom-r.top;

		return 0;
	}else{
		if(m_OldWindowProc){
			return CallWindowProc(m_OldWindowProc,hwnd,msg,wParam,lParam);
		}else{
			return DefWindowProc(hwnd,msg,wParam,lParam);
		}
	}
}

#endif

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

#ifdef __X11_INCLUDED__
int handleXError(Display* disp,XErrorEvent* event){
	//NOTE: This is UNTESTED code, there are still some things that should be tested/changed.
	//NOTE: It checks against hardcoded opcodes, this should be based on included defines from the xf86vid headers instead.
	//NOTE: This code assumes Xlib is in use, just like the resize restriction code for Linux.
	
	//Print out the error message as normal.
	char output[256];
	XGetErrorText(disp,event->error_code,output,256);
	cerr<<output<<endl;
	
	//Check if the game is fullscreen.
	if(getSettings()->getBoolValue("fullscreen")){
		//Check for the exact error we want to handle differently.
		if(event->error_code==BadValue && event->minor_code==10/*X_XF86VidModeSwitchToMode*/){
			//The cause of this problem has likely something to do with fullscreen mode, so fallback to windowed.
			cerr<<"ERROR: Xlib error code "<<event->error_code<<", request code "<<event->request_code<<"."<<endl;
			cerr<<"ERROR: Falling back to windowed mode!"<<endl;
	
			getSettings()->setValue("fullscreen","false");
			createScreen();
			return 0;
		}
	}

	//Do the normal Xlib behaviour.
	exit(1);
	return 0;
}
#endif

void configureWindow(){
	//We only need to configure the window if it's resizable.
	if(!getSettings()->getBoolValue("resizable"))
		return;
	
	//Retrieve the WM info from SDL containing the window handle.
	struct SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	SDL_GetWindowWMInfo(sdlWindow, &wmInfo);
	
#ifdef __X11_INCLUDED__
	//We assume that a linux system running meandmyshadow is also running an Xorg server.
	if(wmInfo.subsystem==SDL_SYSWM_X11){
		//Create the size hints to give to the window.
		XSizeHints* sizeHints;
		if(!(sizeHints=XAllocSizeHints())){
            std::cerr<<"ERROR: Unable to allocate memory for XSizeHints."<<endl;
			return;
		}
		
		//Configure the size hint.
		sizeHints->flags=PMinSize;
		sizeHints->min_width=800;
		sizeHints->min_height=600;
		
		//Set the normal hints of the window.
        //TODO: Lock func no longer exists in SDL2, is using
        //XLockDisplay is correct?
        //(void)wmInfo.info.x11.lock_func;
        XLockDisplay(wmInfo.info.x11.display);
        XSetWMNormalHints(wmInfo.info.x11.display,wmInfo.info.x11.window,sizeHints);
        XUnlockDisplay(wmInfo.info.x11.display);
        //(void)wmInfo.info.x11.unlock_func;
		
		//Free size hint structure
		XFree(sizeHints);
	}else{
		//No X11 so an unsupported window manager.
        std::cerr<<"WARNING: Untested windowing system!"<<endl;
	}
#elif defined(WIN32)
	//We overwrite the window proc of SDL
    WNDPROC wndproc=(WNDPROC)GetWindowLong(wmInfo.info.win.window,GWL_WNDPROC);
	if(wndproc!=NULL && wndproc!=(WNDPROC)WindowProc){
		m_OldWindowProc=wndproc;
        SetWindowLong(wmInfo.info.win.window,GWL_WNDPROC,(LONG)(WNDPROC)WindowProc);
	}
#endif
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
    snprintf(s,32,"%d",event.window.data1);
	getSettings()->setValue("width",s);
    snprintf(s,32,"%d",event.window.data2);
	getSettings()->setValue("height",s);
    //FIXME: THIS doesn't work properly.
	//Do resizing.
	if(!createScreen())
		return;
	
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
    if(Mix_OpenAudio(44100,MIX_DEFAULT_FORMAT,2,512)==-1){
        std::cerr << "FATAL ERROR: Mix_OpenAudio failed\nError: " << Mix_GetError() << std::endl;
        return creationFailed();
	}
	//Set the volume.
	Mix_Volume(-1,atoi(settings->getValue("sound").c_str()));

	//Initialze SDL_ttf (fonts).
	if(TTF_Init()==-1){
        std::cerr << "FATAL ERROR: TTF_Init failed\nError: " << TTF_GetError() << std::endl;
        return creationFailed();
	}

#ifdef __X11_INCLUDED__
	//Before creating the screen set the XErrorHandler in case of X11.
	XSetErrorHandler(handleXError);
#endif
	
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
			{LevelEvent_OnReset,"onReset"},
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
	
    arrowLeft1=textureFromText(renderer,*fontArrow,"<",themeTextColor);
    arrowRight1=textureFromText(renderer,*fontArrow,">",themeTextColor);
    arrowLeft2=textureFromText(renderer,*fontArrow,"<",themeTextColorDialog);
    arrowRight2=textureFromText(renderer,*fontArrow,">",themeTextColorDialog);
	
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
        TexturePtr loadingTexture = textureFromText(renderer, *fontTitle, _("Loading..."),fg);
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
	//NOTE: Set the type of 'levels' to main so it won't be added to the custom packs, even though it contains non-main levels.
	levelsPack->type=COLLECTION;
	LevelPack* customLevelsPack=new LevelPack;
	customLevelsPack->levelpackName="Custom Levels";
	customLevelsPack->levelpackPath="Custom Levels/";
	customLevelsPack->type=COLLECTION;

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
	settings=new Settings(getUserPath(USER_CONFIG)+"meandmyshadow.cfg");
	settings->parseFile();
	
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

ScriptExecutor* getScriptExecutor(){
	return &scriptExecutor;
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

void changeState(ImageManager& imageManager, SDL_Renderer& renderer){

	//Check if there's a nextState.
	if(nextState!=STATE_NULL){
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
		
		//Fade out, if fading is enabled.
		int fade=0;
		if(settings->getBoolValue("fading"))
			fade=255;
        while(fade>0){
            // FIXME: Disabled for now
            fade-=17;
			if(fade<0)
				fade=0;
            dimScreen(renderer, static_cast<Uint8>(255-fade));
            flipScreen(renderer);
            SDL_Delay(25);
		}
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

msgBoxResult msgBox(ImageManager& imageManager,SDL_Renderer& renderer, string prompt,msgBoxButtons buttons,const string& title){
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
		//Pointer to the string.
		char* lps=(char*)prompt.c_str();
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
    overlay->enterLoop(imageManager,renderer, true);
	
	//And return the result.
	return (msgBoxResult)objHandler.ret;
}

struct fileDialogHandler:public GUIEventCallback{
public:
	//The ret(urn) value, true=ok and false=cancel
	bool ret;
	//Boolean if it's a save dialog.
	bool isSave;
	//Boolean if the file should be verified.
	bool verifyFile;
	//Boolean if files should be listed instead of directories.
	bool files;
	
	//Pointer to the textfield containing the filename.
	GUIObject* txtName;
	//Pointer to the listbox containing the different files.
	GUIListBox* lstFile;
	
	//The extension the files listed should have.
	const char* extension;
	//The current filename.
	string fileName;
	//The current search path.
	string path;
	
	//Vector containing the search paths.
	vector<string> searchPath;
public:
	//Constructor.
	fileDialogHandler(bool isSave=false,bool verifyFile=false, bool files=true):ret(false),
		isSave(isSave),verifyFile(verifyFile),
		files(files),txtName(NULL),lstFile(NULL),extension(NULL){}
	
    void GUIEventCallback_OnEvent(ImageManager& imageManager, SDL_Renderer& renderer,std::string name,GUIObject* obj,int /*eventType*/) override {
		//Check for the ok event.
		if(name=="cmdOK"){
			//Get the entered fileName from the text field.
			std::string s=txtName->caption;

			//If it doesn't contain a slash we need to add the path to the fileName.
			if(s.find_first_of("/")==string::npos)
				s=path+s;
			
			//If the string empty we return.
			if(s.empty() || s.find_first_of("*?")!=string::npos)
				return;
			
			//We only need to check for extensions if it isn't a folder dialog.
			if(files){
				//If there isn't right extension add it.
				size_t found=s.find_first_of(".");
				if(found!=string::npos)
					s.replace(s.begin()+found+1,s.end(),extension);
				else if (s.substr(found+1)!=extension)
					s.append(string(".")+extension);
			}
			
			//Check if we should save or load the file.
			//
			if(isSave){
				//Open the file with read permission to check if it already exists.
				FILE* f;
				f=fopen(processFileName(s).c_str(),"rb");
				
				//Check if it exists.
				if(f){
					//Close the file.
					fclose(f);
					
					//Let the currentState render once to prevent multiple GUI overlapping and prevent the screen from going black.
                    currentState->render(imageManager,renderer);
					
					//Prompt the user with a Yes or No question.
					/// TRANSLATORS: Filename is coming before this text
                    if(msgBox(imageManager,renderer, tfm::format(_("%s already exists.\nDo you want to overwrite it?"),s),MsgBoxYesNo,_("Overwrite Prompt"))!=MsgBoxYes){
						//He answered no, so we return.
						return;
					}
				}
				
				//Check if we should verify the file.
				//Verifying only applies to files not to directories.
				if(verifyFile && files){
					//Open the file with write permission.
					f=fopen(processFileName(s).c_str(),"wb");
					
					//Check if their aren't problems.
					if(f){
						//Close the file.
						fclose(f);
					}else{
						//Let the currentState render once to prevent multiple GUI overlapping and prevent the screen from going black.
                        currentState->render(imageManager,renderer);
						
						//The file can't be opened so tell the user.
                        msgBox(imageManager,renderer, tfm::format(_("Can't open file %s."),s),MsgBoxOKOnly,_("Error"));
						return;
					}
				}
			}else if(verifyFile && files){
				//We need to verify a file for opening.
				FILE *f;
				f=fopen(processFileName(s).c_str(),"rb");
				
				//Check if it didn't fail.
				if(f){
					//Succes, so close the file.
					fclose(f);
				}else{
					//Let the currentState render once to prevent multiple GUI overlapping and prevent the screen from going black.
                    currentState->render(imageManager,renderer);
					
					//Unable to open file so tell the user.
                    msgBox(imageManager,renderer, tfm::format(_("Can't open file %s."),s),MsgBoxOKOnly,_("Error"));
					return;
				}
			}
			
			//If we haven't returned then it's fine.
			//Set the fileName to the chosen file.
			fileName=s;
			
			//Delete the GUI.
			if(GUIObjectRoot){
				delete GUIObjectRoot;
				GUIObjectRoot=NULL;
			}
			
			//Set return to true.
			ret=true;
		}else if(name=="cmdCancel"){
			//Cancel means we can kill the gui.
			if(GUIObjectRoot){
				delete GUIObjectRoot;
				GUIObjectRoot=NULL;
			}
		}else if(name=="lstFile"){
			//Get a pointer to the listbox.
			GUIListBox* obj1=lstFile;
			
			//Make sure the option exist and change textfield to it.
			if(obj1!=NULL && txtName!=NULL && obj1->value>=0 && obj1->value<(int)obj1->item.size()){
				txtName->caption=obj1->item[obj1->value];
			}
		}else if(name=="lstSearchIn"){
			//Get the searchpath listbox.
			GUISingleLineListBox *obj1=dynamic_cast<GUISingleLineListBox*>(obj);
			
			//Check if the entry exists.
			if(obj1!=NULL && lstFile!=NULL && obj1->value>=0 && obj1->value<(int)searchPath.size()){
				//Temp string.
				string s;
				
				//Get the new search path.
				path=searchPath[obj1->value];
				
				//Make sure it isn't empty.
				if(!path.empty()){
					//Process the filename.
					s=processFileName(path);
				}else{
					//It's empty so we give the userpath.
					s=getUserPath();
				}
				
				//Fill the list with files or directories.
				if(files) {
					lstFile->item=enumAllFiles(s,extension);
				}else
					lstFile->item=enumAllDirs(s);
				
				//Remove any selection from the list.
				lstFile->value=-1;
			}
		}
	}
};

//SDL2 port note: Commented this out since it was unused.
/*
bool fileDialog(ImageManager& imageManager,SDL_Renderer& renderer, string& fileName,const char* title,const char* extension,const char* path,bool isSave,bool verifyFile,bool files){
	//Pointer to GUIObject to make the GUI with.
	GUIObject* obj;
	
	//Create the fileDialogHandler, used for event handling.
	fileDialogHandler objHandler(isSave,verifyFile,files);
	//Vector containing the pathNames.
	vector<string> pathNames;

	//Set the extension of the objHandler.
	objHandler.extension=extension;
	
	//We now need to splits the given path into multiple path names.
	if(path && path[0]){
		//The string isn't empty.
		//Pointer to the paths string.
		char* lp=(char*)path;
		//Pointer to the first newline.
		char* lps=strchr(lp,'\n');
		//Pointer used for checking if their's another newline.
		//It will indicate if it's the last set or not.
		char* lpe;
		
		//Check for a newline.
		if(lps){
			//We have newline(s) so loop forever.
			//We can only break out of the loop when the string ends.
			for(;;){
				//Add the first searchpath.
				//This is the beginning of the string (lp) to the first newline. (lps)
				objHandler.searchPath.push_back(string(lp,lps-lp));
				
				//We should have another newline so search for it.
				lpe=strchr(lps+1,'\n');
				if(lpe){
					//We found it so we add that to the pathname.
					pathNames.push_back(string(lps+1,lpe-lps-1));
					//And start over again by setting lp to the start of a new set of searchPath/pathName.
					lp=lpe+1;
				}else{
					//There is no newline anymore, meaning the last entry, the rest of the string must be the pathName.
					pathNames.push_back(string(lps+1));
					//And break out of the loop.
					break;
				}
				
				//We haven't broken out so search for a newline.
				lps=strchr(lp,'\n');
				//If there isn't a newline break.
				if(!lps) 
					break;
			}
		}else{
			//There is no newline thus the whole string is the searchPath.
			objHandler.searchPath.push_back(path);
		}
	}else{
		//Empty so put an empty string as searchPath.
		objHandler.searchPath.push_back(string());
	}
	
	//It's time to create the GUI.
	//If there are more than one pathNames we need to add a GUISingleLineListBox.
	int base_y=pathNames.empty()?20:60;
	
	//Create the frame.
	GUIObject* root=new GUIFrame(100,100-base_y/2,600,400+base_y,title?title:(isSave?_("Save File"):_("Load File")));
	
	//Create the search path list box if needed.
	if(!pathNames.empty()){
		root->addChild(new GUILabel(8,40,184,36,_("Search In")));
		GUISingleLineListBox* obj1=new GUISingleLineListBox(160,40,432,36);
		obj1->addItems(pathNames);
		obj1->value=0;
		obj1->name="lstSearchIn";
		obj1->eventCallback=&objHandler;
		root->addChild(obj1);
	}
	
	//Add the FileName label and textfield.
	root->addChild(new GUILabel(8,20+base_y,184,36,_("File Name")));
	{
		//Fill the textbox with the given fileName.
		string s=fileName;
		
		if(!isSave){
			//But only if it isn't empty.
			if(s.empty() && extension && extension[0])
				s=string("*.")+string(extension);
		}
		
		//Create the textbox and add it to the GUI.
		objHandler.txtName=new GUITextBox(160,20+base_y,432,36,s.c_str());
		root->addChild(objHandler.txtName);
	}
	
	//Now we add the ListBox containing the files or directories.
	{
		GUIListBox* obj1=new GUIListBox(8,60+base_y,584,292);
		
		//Get the searchPath.
		string s=objHandler.searchPath[0];
		//Make sure it isn't empty.
		if(!s.empty()){
			objHandler.path=s;
			s=processFileName(s);
		}else{
			s=getUserPath();
		}
		
		//Check if we should list files or directories.
		if(files){
			//Fill the list with files.
			obj1->item=enumAllFiles(s,extension);
		}else{
			//Fill the list with directories.
			obj1->item=enumAllDirs(s);
		}
		obj1->name="lstFile";
		obj1->eventCallback=&objHandler;
		root->addChild(obj1);
		objHandler.lstFile=obj1;
	}
	
	//Now create the OK and Cancel buttons.
	obj=new GUIButton(200,360+base_y,192,36,_("OK"));
	obj->name="cmdOK";
	obj->eventCallback=&objHandler;
	root->addChild(obj);
	obj=new GUIButton(400,360+base_y,192,36,_("Cancel"));
	obj->name="cmdCancel";
	obj->eventCallback=&objHandler;
	root->addChild(obj);

	//Create the gui overlay.
    GUIOverlay* overlay=new GUIOverlay(renderer,root);
    overlay->enterLoop(imageManager,renderer);
	
	//Now determine what the return value is (and if there is one).
	if(objHandler.ret) 
		fileName=objHandler.fileName;
	return objHandler.ret;
}
