/*
 * Copyright (C) 2012-2013 Me and My Shadow
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
#include "GUIOverlay.h"
#include "InputManager.h"
//#include "StatisticsManager.h"

using namespace std;

GUIOverlay::GUIOverlay(SDL_Renderer& renderer, GUIObject* root,bool dim)
	: root(root), dim(dim), keyboardNavigationMode(0)
{
	//First keep the pointer to the current GUIObjectRoot and currentState.
	parentState=currentState;
	tempGUIObjectRoot=GUIObjectRoot;

	//Now set the GUIObject root to the new root.
	currentState=this;
	GUIObjectRoot=root;
	
	//Dim the background.
	if(dim){
        dimScreen(renderer);
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

void GUIOverlay::enterLoop(ImageManager& imageManager, SDL_Renderer& renderer, bool skipByEscape, bool skipByReturn){
	//Keep the last resize event, this is to only process one.
	SDL_Event lastResize = {};

	while (GUIObjectRoot){
		while(SDL_PollEvent(&event)){
			//Check for a resize event.
			if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
				lastResize = event;
				continue;
			}

			//Check if it's mouse event. If it's true then we quit the keyboard-only mode.
			if (event.type == SDL_MOUSEMOTION || event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) {
				isKeyboardOnly = false;
			}

			//Set the cursor type to the default one, the GUI can change that if needed.
			currentCursor = CURSOR_POINTER;

			//Let the input manager handle the events.
			inputMgr.updateState(true);

			//Let the currentState handle the events. (???)
			currentState->handleEvents(imageManager, renderer);

			//Also pass the events to the GUI.
			GUIObjectHandleEvents(imageManager, renderer, true);
			
			//Also check for the escape/return button.
			if ((skipByEscape && inputMgr.isKeyDownEvent(INPUTMGR_ESCAPE)) || (skipByReturn && inputMgr.isKeyDownEvent(INPUTMGR_SELECT))) {
				delete GUIObjectRoot;
				GUIObjectRoot=NULL;
			}
		}

		//Process the resize event.
		if (lastResize.type == SDL_WINDOWEVENT){
			//TODO - used to be SDL_VIDEORESIZE
			// so this may trigger on more events than intended
			event = lastResize;
			onVideoResize(imageManager, renderer);

			//After resize we erase the event type
			//TODO - used to be SDL_NOEVENT
			lastResize.type = SDL_FIRSTEVENT;
		}

		//update input state (??)
		inputMgr.updateState(false);

		//Render the gui.
		if(GUIObjectRoot)
            GUIObjectRoot->render(renderer);

		/*//draw new achievements (if any)
		statsMgr.render();*/

		//display it
        flipScreen(renderer);
		SDL_Delay(30);
	}

	//We broke out so clean up.
	delete this;
}

// internal function which is used in keyboard only mode
static GUIButton* getSelectedButton() {
	if (GUIObjectRoot == NULL) return NULL;

	for (GUIObject *obj : GUIObjectRoot->childControls) {
		GUIButton *btn = dynamic_cast<GUIButton*>(obj);
		if (btn && btn->visible && btn->enabled && btn->state) {
			return btn;
		}
	}

	return NULL;
}

void GUIOverlay::handleEvents(ImageManager& imageManager, SDL_Renderer& renderer){
	//Check if we need to quit, if so we enter the exit state.
	if(event.type==SDL_QUIT){
		setNextState(STATE_EXIT);
	}

	//Experimental code for keyboard navigation.
	if (keyboardNavigationMode) {
		int m = SDL_GetModState();
		if (((keyboardNavigationMode & 1) != 0 && inputMgr.isKeyDownEvent(INPUTMGR_RIGHT))
			|| ((keyboardNavigationMode & 2) != 0 && inputMgr.isKeyDownEvent(INPUTMGR_DOWN))
			|| ((keyboardNavigationMode & 4) != 0 && inputMgr.isKeyDownEvent(INPUTMGR_TAB) && (m & KMOD_SHIFT) == 0)
			)
		{
			isKeyboardOnly = true;

			if (GUIObjectRoot) {
				int selected = 0;

				//Get the index of currently selected button.
				for (int i = 0; i < (int)GUIObjectRoot->childControls.size(); i++) {
					GUIButton *btn = dynamic_cast<GUIButton*>(GUIObjectRoot->childControls[i]);
					if (btn && btn->visible && btn->enabled && btn->state) {
						btn->state = 0;
						selected = i;
						break;
					}
				}

				//Find the next button.
				for (int i = 0; i < (int)GUIObjectRoot->childControls.size(); i++) {
					selected++;
					if (selected >= (int)GUIObjectRoot->childControls.size()) {
						selected = 0;
					}
					GUIButton *btn = dynamic_cast<GUIButton*>(GUIObjectRoot->childControls[selected]);
					if (btn && btn->visible && btn->enabled) {
						btn->state = 1;
						break;
					}
				}
			}
		} else if (((keyboardNavigationMode & 1) != 0 && inputMgr.isKeyDownEvent(INPUTMGR_LEFT))
			|| ((keyboardNavigationMode & 2) != 0 && inputMgr.isKeyDownEvent(INPUTMGR_UP))
			|| ((keyboardNavigationMode & 4) != 0 && inputMgr.isKeyDownEvent(INPUTMGR_TAB) && (m & KMOD_SHIFT) != 0)
			)
		{
			isKeyboardOnly = true;

			if (GUIObjectRoot) {
				int selected = 0;

				//Get the index of currently selected button.
				for (int i = 0; i < (int)GUIObjectRoot->childControls.size(); i++) {
					GUIButton *btn = dynamic_cast<GUIButton*>(GUIObjectRoot->childControls[i]);
					if (btn && btn->visible && btn->enabled && btn->state) {
						btn->state = 0;
						selected = i;
						break;
					}
				}

				//Find the previous button.
				for (int i = 0; i < (int)GUIObjectRoot->childControls.size(); i++) {
					selected--;
					if (selected < 0) {
						selected += GUIObjectRoot->childControls.size();
					}
					GUIButton *btn = dynamic_cast<GUIButton*>(GUIObjectRoot->childControls[selected]);
					if (btn && btn->visible && btn->enabled) {
						btn->state = 1;
						break;
					}
				}
			}
		} else if (isKeyboardOnly && (keyboardNavigationMode & 8) != 0 && inputMgr.isKeyDownEvent(INPUTMGR_SELECT)) {
			GUIButton *btn = getSelectedButton();
			if (btn && btn->eventCallback) {
				btn->eventCallback->GUIEventCallback_OnEvent(imageManager, renderer, btn->name, btn, GUIEventClick);
			}
		}
	}
}

//Nothing to do here
void GUIOverlay::logic(ImageManager&, SDL_Renderer&){
	//Check if the GUIObjectRoot (of the overlay) is deleted.
	if(!GUIObjectRoot)
		delete this;
}

void GUIOverlay::render(ImageManager&, SDL_Renderer&){}

void GUIOverlay::resize(ImageManager& imageManager, SDL_Renderer& renderer){
	//We recenter the GUI.
	GUIObjectRoot->left=(SCREEN_WIDTH-GUIObjectRoot->width)/2;
	GUIObjectRoot->top=(SCREEN_HEIGHT-GUIObjectRoot->height)/2;

	//Now let the parent state resize.
	GUIObjectRoot=tempGUIObjectRoot;
    parentState->resize(imageManager, renderer);
	//NOTE: After the resize it's likely that the GUIObjectRoot is new so we need to update our tempGUIObjectRoot pointer.
	tempGUIObjectRoot=GUIObjectRoot;

	//Now render the parentState.
    parentState->render(imageManager,renderer);
	if(GUIObjectRoot)
        GUIObjectRoot->render(renderer);

	//And set the GUIObjectRoot back to the overlay gui.
	GUIObjectRoot=root;

	//Dim the background.
	if(dim){
        dimScreen(renderer);
	}
}
