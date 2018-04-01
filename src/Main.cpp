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
#include <SDL.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>

#ifdef HARDWARE_ACCELERATION
#include <GL/gl.h>
#include <GL/glu.h>
#endif

//Variables for recording.
//To enable picture recording uncomment the next line:
//#define RECORD_PICUTRE_SEQUENCE
#ifdef RECORD_PICUTRE_SEQUENCE
bool recordPictureSequence=false;
int recordPictureIndex=0;
#endif
#ifdef __APPLE__
int SDL_main(int argc, char** argv) {
#else
int main(int argc, char** argv) {
#endif
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
	//Initialise some stuff like SDL, the window, SDL_Mixer.
	if(init()==false) {
		fprintf(stderr,"FATAL ERROR: Failed to initialize game.\n");
		return 1;
	}
	//Load some important files like the background music, default theme.
	if(loadFiles()==false){
		fprintf(stderr,"FATAL ERROR: Failed to load necessary files.\n");
		return 1;
	}
	
	//Seed random.
	srand((unsigned)time(NULL));
	
	//Set the currentState id to the main menu and create it.
	stateID=STATE_MENU;
	currentState=new Menu();
	
	//Set the fadeIn value to zero.
	int fadeIn=0;
	
	//Keep the last resize event, this is to only process one.
	SDL_Event lastResize={};
	
	//Start the game loop.
	while(stateID!=STATE_EXIT){
		//We start the timer.
		timer.start();
		
		//Loop the SDL events.
		while(SDL_PollEvent(&event)){
			//Check if user resizes the window.
			if(event.type==SDL_VIDEORESIZE){
				lastResize=event;
				
				//Don't let other objects process this event (?)
				continue;
			}
			
			//Check if the fullscreen toggle shortcut is pressed (Alt+Enter).
			if(event.type==SDL_KEYUP && event.key.keysym.sym==SDLK_RETURN && (event.key.keysym.mod & KMOD_ALT)){
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
						cerr<<"ERROR: Unable to create screen."<<endl;
					}
				}
				
				//The screen is created, now load the (menu) theme.
				if(!loadTheme("")){
					//Loading the theme failed so quit.
					setNextState(STATE_EXIT);
					cerr<<"ERROR: Unable to load theme after toggling fullscreen."<<endl;
				}
				
				//Don't let other objects process this event.
				continue;
			}

#ifdef RECORD_PICUTRE_SEQUENCE
			if(event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_F10){
				recordPictureSequence=!recordPictureSequence;
				printf("Record Picture Sequence %s\n",recordPictureSequence?"ON":"OFF");
			}
#endif
			//Set the cursor type to the default one, the GUI can change that if needed.
			currentCursor=CURSOR_POINTER;
			
			//Let the input manager handle the events.
			inputMgr.updateState(true);
			//Let the currentState handle the events.
			currentState->handleEvents();
			//Also pass the events to the GUI.
			GUIObjectHandleEvents();
		}
		
		//Process the resize event.
		if(lastResize.type==SDL_VIDEORESIZE){
			event=lastResize;
			onVideoResize();

			//After resize we erase the event type
			lastResize.type=SDL_NOEVENT;
		}

		//maybe we should add a check here (??) to fix some bugs (ticket #47)
		if(nextState!=STATE_NULL){
			//Check if fading is enabled.
			if(getSettings()->getBoolValue("fading"))
				fadeIn=17;
			else
				fadeIn=255;
			changeState();
		}
		if(stateID==STATE_EXIT) break;

		//update input state (??)
		inputMgr.updateState(false);
		//Now it's time for the state to do his logic.
		currentState->logic();
		
		currentState->render();
		//TODO: Shouldn't the gamestate take care of rendering the GUI?
		if(GUIObjectRoot) GUIObjectRoot->render();

		//draw new achievements (if any)
		statsMgr.render();

		//draw fading effect
		if(fadeIn>0&&fadeIn<255){
			SDL_BlitSurface(screen,NULL,tempSurface,NULL);
			SDL_FillRect(screen,NULL,0);
			SDL_SetAlpha(tempSurface, SDL_SRCALPHA, fadeIn);
			SDL_BlitSurface(tempSurface,NULL,screen,NULL);
			fadeIn+=17;
		}

#ifdef RECORD_PICUTRE_SEQUENCE
		if(recordPictureSequence){
			char s[64];
			recordPictureIndex++;
			sprintf(s,"pic%08d.bmp",recordPictureIndex);
			printf("Save screen to %s\n",s);
			SDL_SaveBMP(screen,(getUserPath(USER_CACHE)+s).c_str());
		}
#endif
		//Before flipping the screen set the cursor.
		SDL_SetCursor(cursors[currentCursor]);
		
		//And draw the screen surface to the actual screen.
		flipScreen();
		
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
