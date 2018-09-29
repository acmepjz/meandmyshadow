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

#include "Functions.h"
#include "Globals.h"
#include "TitleMenu.h"
#include "GUIObject.h"
#include "InputManager.h"
#include "StatisticsManager.h"
#include "Timer.h"
#include <SDL.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>

// the following piece of code fixes the combination of VC2015 with official SDL1.2 binary
#if defined(_MSC_VER) && (_MSC_VER >= 1900)

#pragma comment(lib, "legacy_stdio_definitions.lib")

FILE _iob[] = {*stdin, *stdout, *stderr};

extern "C" FILE * __cdecl __iob_func(void)
{
	return _iob;
}

#endif

static ImageManager *_imageManager = NULL;
static SDL_Renderer *_renderer = NULL;

ImageManager& getImageManager() {
	return *_imageManager;
}

SDL_Renderer& getRenderer() {
	return *_renderer;
}

int main(int argc, char** argv) {
#ifdef _MSC_VER
	//Fix the non-latin file name bug under Visual Studio
	setlocale(LC_ALL,"");
#endif

	//Relocate the standard output for debug purpose (?)
#if defined(ANDROID)
	freopen("stdout.txt","w",stdout);
	freopen("stderr.txt","w",stderr);
#endif
	//First parse the comand line arguments.
	int s=parseArguments(argc,argv);
	if(s==-1){
		printf("Usage: %s [OPTIONS] ...\n",argv[0]);
		printf("%s","Available options:\n");
		printf("    %-5s%-30s  %s\n","","--data-dir <dir>","Specifies the data directory.");
		printf("    %-5s%-30s  %s\n","","--user-dir <dir>","Specifies the user preferences directory.");
		printf("    %-5s%-30s  %s\n","-f,","--fullscreen","Run the game fullscreen.");
		printf("    %-5s%-30s  %s\n","-w,","--windowed","Run the game windowed.");
		printf("    %-5s%-30s  %s\n","-mv,","--music <volume>","Set the music volume.");
		printf("    %-5s%-30s  %s\n","-sv,","--sound <volume>","Set the sound volume.");
		printf("    %-5s%-30s  %s\n","-s,","--set <setting> <value>","Change a setting to a given value.");
		printf("    %-5s%-30s  %s\n","-v,","--version","Display the version and quit.");
		printf("    %-5s%-30s  %s\n","-h,","--help","Display this help message.");
		return 0;
	}else if(s==0){
		return 0;
	}

	//Try to configure the dataPath, userPath, etc...
	if(configurePaths()==false){
		fprintf(stderr,"FATAL ERROR: Failed to configure paths.\n");
		return 1;
	}
	//Load the settings.
	if(loadSettings()==false){
		fprintf(stderr,"ERROR: Unable to load config file, default values will be used.\n");
	}	
    ScreenData screenData = init();
	//Initialise some stuff like SDL, the window, SDL_Mixer.
    if(!screenData) {
		fprintf(stderr,"FATAL ERROR: Failed to initialize game.\n");
		return 1;
	}

    SDL_Renderer& renderer = *screenData.renderer;
	_renderer = screenData.renderer;
    //Initialise the imagemanager.
    //The ImageManager is used to prevent loading images multiple times.
    ImageManager imageManager;
	_imageManager = &imageManager;

	//Load some important files like the background music, default theme.
    if(loadFiles(imageManager,renderer)==false){
		fprintf(stderr,"FATAL ERROR: Failed to load necessary files.\n");
		return 1;
	}
	
	//Seed random.
	srand((unsigned)time(NULL));
	
	//Set the currentState id to the main menu and create it.
	stateID=STATE_MENU;
    currentState=new Menu(imageManager,renderer);
	
	//Set the fadeIn value to zero.
	int fadeIn=255;
	
	//Keep the last resize event, this is to only process one.
    SDL_Event lastResize={};
    Timer timer;
	
	//Start the game loop.
	while(stateID!=STATE_EXIT){
		//We start the timer.
		timer.start();
		
		//Loop the SDL events.
		while(SDL_PollEvent(&event)){
			//Check if user resizes the window.
			if(event.type==SDL_WINDOWEVENT){
                if(event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
				{
					lastResize=event;
				
					//Don't let other objects process this event (?)
					continue;
				}
			}

			//Check if it's mouse event. If it's true then we quit the keyboard-only mode.
			if (event.type == SDL_MOUSEMOTION || event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) {
				isKeyboardOnly = false;
			}
			
			//Check if the fullscreen toggle shortcut is pressed (Alt+Enter).
			if(event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_RETURN && (event.key.keysym.mod & KMOD_ALT)){
				getSettings()->setValue("fullscreen",getSettings()->getBoolValue("fullscreen")?"0":"1");
				
				//We need to create a new screen.
				if(!createScreen()){
					//Screen creation failed so set to safe settings.
					getSettings()->setValue("fullscreen","0");
					getSettings()->setValue("width","800");
					getSettings()->setValue("height","600");
					
					//Try it with the safe settings.
					if(!createScreen()){
						//Everything fails so quit.
						setNextState(STATE_EXIT);
                        std::cerr<<"ERROR: Unable to create screen."<<std::endl;
					}
				}
				
				//The screen is created, now load the (menu) theme.
                if(!loadTheme(imageManager,renderer,"")){
					//Loading the theme failed so quit.
					setNextState(STATE_EXIT);
                    std::cerr<<"ERROR: Unable to load theme after toggling fullscreen."<<std::endl;
				}
				
				//Don't let other objects process this event.
				continue;
			}

			//Set the cursor type to the default one, the GUI can change that if needed.
			currentCursor=CURSOR_POINTER;
			
			//Let the input manager handle the events.
			inputMgr.updateState(true);
			//Let the currentState handle the events.
            currentState->handleEvents(imageManager,renderer);
			//Also pass the events to the GUI.
            GUIObjectHandleEvents(imageManager,renderer);
		}
		
		//Process the resize event.
		if(lastResize.type==SDL_WINDOWEVENT){
			//TODO - used to be SDL_VIDEORESIZE
			// so this may trigger on more events than intended
			event=lastResize;
            onVideoResize(imageManager,renderer);

			//After resize we erase the event type
			//TODO - used to be SDL_NOEVENT
			lastResize.type=SDL_FIRSTEVENT;
		}

		//maybe we should add a check here (??) to fix some bugs (ticket #47)
		if(nextState!=STATE_NULL){
			changeState(imageManager, renderer, fadeIn);

			//Check if fading is enabled.
			if(getSettings()->getBoolValue("fading"))
				fadeIn=17;
			else
				fadeIn=255;
		}
		if(stateID==STATE_EXIT) break;

		//update input state (??)
		inputMgr.updateState(false);
		//Now it's time for the state to do his logic.
        currentState->logic(imageManager,renderer);
		
        currentState->render(imageManager,renderer);
		//TODO: Shouldn't the gamestate take care of rendering the GUI?
        if(GUIObjectRoot) GUIObjectRoot->render(renderer);

		//draw fading effect
        if(fadeIn>0&&fadeIn<255){
            dimScreen(renderer, static_cast<Uint8>(255-fadeIn));
			fadeIn+=17;
		}

		//draw new achievements (if any) as overlay
		statsMgr.render(imageManager, renderer);

		//Before flipping the screen set the cursor.
		SDL_SetCursor(cursors[currentCursor]);
		
		//And draw the screen surface to the actual screen.
        flipScreen(renderer);
		
		//Now calcualte how long we need to wait to keep a constant framerate.
		int t=timer.getTicks();
        t=(1000/FPS)-t;
		if(t>0){
			SDL_Delay(t);
		}
	}

	//The game has ended, save the settings just to be sure.
	if(!saveSettings()){
		fprintf(stderr,"ERROR: Unable to save settings in config file.\n");
	}
	
	//Clean everything up.
	clean();
	
	//End of program.
	return 0;
}
