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

#include "Functions.h"
#include "GameState.h"
#include "Globals.h"
#include "Objects.h"
#include "GUIOverlay.h"

using namespace std;

GUIOverlay::GUIOverlay(GUIObject* root,bool dim):root(root),dim(dim){
	//First keep the pointer to the current GUIObjectRoot and currentState.
	parentState=currentState;
	tempGUIObjectRoot=GUIObjectRoot;

	//Now set the GUIObject root to the new root.
	currentState=this;
	GUIObjectRoot=root;
	
	//Dim the background.
	if(dim){
		SDL_FillRect(tempSurface,NULL,0);
		SDL_SetAlpha(tempSurface,SDL_SRCALPHA,155);
		SDL_BlitSurface(tempSurface,NULL,screen,NULL);
	}
}

GUIOverlay::~GUIOverlay(){
	//We need to place everything back.
	currentState=parentState;
	parentState=NULL;

	//Delete the GUI if present.
	if(GUIObjectRoot)
		delete GUIObjectRoot;

	//Now put back the parent gui.
	GUIObjectRoot=tempGUIObjectRoot;
	tempGUIObjectRoot=NULL;
}

void GUIOverlay::enterLoop(bool skip){
	while(GUIObjectRoot){
		while(SDL_PollEvent(&event)){
			//Check for a resize event.
			if(event.type==SDL_VIDEORESIZE){
				onVideoResize();
				continue;
			}
			GUIObjectHandleEvents(true);
			
			//Also check for the return, escape or backspace button.
			//escape = KEYUP.
			//backspace and return = KEYDOWN.
			if(skip && ((event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_ESCAPE) ||
				(event.type==SDL_KEYDOWN && (event.key.keysym.sym==SDLK_RETURN || event.key.keysym.sym==SDLK_BACKSPACE)))){
				delete GUIObjectRoot;
				GUIObjectRoot=NULL;
			}
		}
		//Render the gui.
		if(GUIObjectRoot)
			GUIObjectRoot->render();
		flipScreen();
		SDL_Delay(30);
	}

	//We broke out so clean up.
	delete this;
}

void GUIOverlay::handleEvents(){
	//Check if we need to quit, if so we enter the exit state.
	if(event.type==SDL_QUIT){
		setNextState(STATE_EXIT);
	}
}

//Nothing to do here
void GUIOverlay::logic(){
	//Check if the GUIObjectRoot (of the overlay) is deleted.
	if(!GUIObjectRoot)
		delete this;
}
void GUIOverlay::render(){}


void GUIOverlay::resize(){
	//We recenter the GUI.
	GUIObjectRoot->left=(SCREEN_WIDTH-GUIObjectRoot->width)/2;
	GUIObjectRoot->top=(SCREEN_HEIGHT-GUIObjectRoot->height)/2;

	//Now let the parent state resize.
	GUIObjectRoot=tempGUIObjectRoot;
	parentState->resize();
	//NOTE: After the resize it's likely that the GUIObjectRoot is new so we need to update our tempGUIObjectRoot pointer.
	tempGUIObjectRoot=GUIObjectRoot;

	//Now render the parentState.
	parentState->render();
	if(GUIObjectRoot)
		GUIObjectRoot->render();

	//And set the GUIObjectRoot back to the overlay gui.
	GUIObjectRoot=root;

	//Dim the background.
	if(dim){
		SDL_FillRect(tempSurface,NULL,0);
		SDL_SetAlpha(tempSurface,SDL_SRCALPHA,155);
		SDL_BlitSurface(tempSurface,NULL,screen,NULL);
	}
}
