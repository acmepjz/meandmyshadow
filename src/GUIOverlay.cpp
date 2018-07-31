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
#include "GUIObject.h"
#include "GUITextArea.h"
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
static int getSelectedControl() {
	if (GUIObjectRoot == NULL) return -1;

	for (int i = 0; i < (int)GUIObjectRoot->childControls.size(); i++) {
		GUIObject *obj = GUIObjectRoot->childControls[i];
		if (obj && obj->visible && obj->enabled && obj->state) {
			if (dynamic_cast<GUIButton*>(obj)
				|| dynamic_cast<GUITextBox*>(obj)
				)
			{
				return i;
			}
		}
	}

	return -1;
}

// internal function which is used in keyboard only mode
static void selectNextControl(int direction) {
	if (GUIObjectRoot == NULL) return;

	//Get the index of currently selected control.
	int selected = getSelectedControl();
	if (selected >= 0) GUIObjectRoot->childControls[selected]->state = 0;

	//Find the next control.
	for (int i = 0; i < (int)GUIObjectRoot->childControls.size(); i++) {
		if (selected < 0) {
			selected = 0;
		} else {
			selected += direction;
			if (selected >= (int)GUIObjectRoot->childControls.size()) {
				selected -= GUIObjectRoot->childControls.size();
			} else if (selected < 0) {
				selected += GUIObjectRoot->childControls.size();
			}
		}

		GUIObject *obj = GUIObjectRoot->childControls[selected];
		if (obj && obj->visible && obj->enabled) {
			if (dynamic_cast<GUIButton*>(obj)) {
				//It's a button.
				obj->state = 1;
				return;
			} else if (dynamic_cast<GUITextBox*>(obj)) {
				//It's a button.
				obj->state = 2;
				return;
			}
		}
	}
}

void GUIOverlay::handleEvents(ImageManager& imageManager, SDL_Renderer& renderer){
	//Check if we need to quit, if so we enter the exit state.
	if(event.type==SDL_QUIT){
		setNextState(STATE_EXIT);
	}

	//Experimental code for keyboard navigation.
	if (keyboardNavigationMode) {
		//Check operation on focused control. These have higher priority.
		if (isKeyboardOnly) {
			//Check enter key.
			if ((keyboardNavigationMode & 8) != 0 && inputMgr.isKeyDownEvent(INPUTMGR_SELECT)) {
				int index = getSelectedControl();
				if (index >= 0) {
					GUIObject *obj = GUIObjectRoot->childControls[index];
					if (obj->eventCallback) {
						if (dynamic_cast<GUIButton*>(obj)) {
							//It's a button.
							obj->eventCallback->GUIEventCallback_OnEvent(imageManager, renderer, obj->name, obj, GUIEventClick);
							return;
						}
					}
				}
			}
		}

		//Check focus movement
		int m = SDL_GetModState();
		if (((keyboardNavigationMode & 1) != 0 && inputMgr.isKeyDownEvent(INPUTMGR_RIGHT))
			|| ((keyboardNavigationMode & 2) != 0 && inputMgr.isKeyDownEvent(INPUTMGR_DOWN))
			|| ((keyboardNavigationMode & 4) != 0 && inputMgr.isKeyDownEvent(INPUTMGR_TAB) && (m & KMOD_SHIFT) == 0)
			)
		{
			isKeyboardOnly = true;
			selectNextControl(1);
		} else if (((keyboardNavigationMode & 1) != 0 && inputMgr.isKeyDownEvent(INPUTMGR_LEFT))
			|| ((keyboardNavigationMode & 2) != 0 && inputMgr.isKeyDownEvent(INPUTMGR_UP))
			|| ((keyboardNavigationMode & 4) != 0 && inputMgr.isKeyDownEvent(INPUTMGR_TAB) && (m & KMOD_SHIFT) != 0)
			)
		{
			isKeyboardOnly = true;
			selectNextControl(-1);
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

AddonOverlay::AddonOverlay(SDL_Renderer &renderer, GUIObject* root, GUIButton *cancelButton, GUITextArea *textArea)
	: GUIOverlay(renderer, root), cancelButton(cancelButton), textArea(textArea)
{
	keyboardNavigationMode = 4 | 8 | 16;
}

void AddonOverlay::handleEvents(ImageManager& imageManager, SDL_Renderer& renderer) {
	GUIOverlay::handleEvents(imageManager, renderer);

	//Do our own stuff.

	//Scroll the text area.
	if (textArea) {
		if (inputMgr.isKeyDownEvent(INPUTMGR_RIGHT)){
			isKeyboardOnly = true;
			textArea->scrollScrollbar(20, 0);
		} else if (inputMgr.isKeyDownEvent(INPUTMGR_LEFT)){
			isKeyboardOnly = true;
			textArea->scrollScrollbar(-20, 0);
		} else if (inputMgr.isKeyDownEvent(INPUTMGR_UP)){
			isKeyboardOnly = true;
			textArea->scrollScrollbar(0, -1);
		} else if (inputMgr.isKeyDownEvent(INPUTMGR_DOWN)){
			isKeyboardOnly = true;
			textArea->scrollScrollbar(0, 1);
		}
	}

	//Check escape key.
	if (cancelButton && cancelButton->eventCallback && inputMgr.isKeyDownEvent(INPUTMGR_ESCAPE)){
		cancelButton->eventCallback->GUIEventCallback_OnEvent(imageManager, renderer, cancelButton->name, cancelButton, GUIEventClick);
		return;
	}
}
