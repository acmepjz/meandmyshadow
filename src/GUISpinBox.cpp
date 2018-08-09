/*
 * Copyright (C) 2011-2012 Me and My Shadow
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
#include "GUISpinBox.h"
#include "ThemeManager.h"

#include <algorithm>

bool GUISpinBox::handleEvents(SDL_Renderer& renderer,int x,int y,bool enabled,bool visible,bool processed){
	//Backup the old state.
	int oldState = state;

	//First we call the GUITextBox::handleEvents().
	bool b0 = GUITextBox::handleEvents(renderer, x, y, enabled, visible, processed);

	//Boolean if the event is processed.
	bool b=processed;
	//The GUIObject is only enabled when he and his parent are enabled.
	enabled=enabled && this->enabled;
	//The GUIObject is only enabled when he and his parent are enabled.
	visible=visible && this->visible;
	
	//Get the absolute position.
	x+=left-gravityX;
	y+=top;
	
	//Reset "key" to stop contant update of "number" in render().
	//If the mouse is still on the button, the "key" will be reassigned later.
	key=-1;
	
	//Only check for events when the object is both enabled and visible.
	if (enabled&&visible){
		//Check if there's a key press and the event hasn't been already processed.
		if (state == 2 && event.type == SDL_KEYDOWN && !b) {
			if (event.key.keysym.sym == SDLK_UP) {
				updateValue(true);
			} else if (event.key.keysym.sym == SDLK_DOWN) {
				updateValue(false);
			}
		}

		//Only process mouse event when not in keyboard only mode
		if (!isKeyboardOnly) {
			//The mouse location (x=i, y=j) and the mouse button (k).
			int i, j, k;
			k = SDL_GetMouseState(&i, &j);

			//Check if the mouse is inside the text box.
			if (i >= x && i < x + width && j >= y && j < y + height){
				//Check if the mouse is inside the up/down button.
				if (i >= x + width - 16) {
					//Reset the cursor back to normal.
					currentCursor = CURSOR_POINTER;

					//Check for a mouse button press.
					if (k&SDL_BUTTON(1)){
						if (j < y + 17){
							//Set the key values correct.
							this->key = SDLK_UP;
							keyHoldTime = 0;
							keyTime = getKeyboardRepeatDelay();

							//Update once to prevent a lag.
							updateValue(true);
						} else{
							//Set the key values correct.
							this->key = SDLK_DOWN;
							keyHoldTime = 0;
							keyTime = getKeyboardRepeatDelay();

							//Update once to prevent a lag.
							updateValue(false);
						}
					}
				}

				//Allow mouse wheel to change value.
				if (event.type == SDL_MOUSEWHEEL){
					if (event.wheel.y > 0){
						updateValue(true);
					} else if (event.wheel.y < 0){
						updateValue(false);
					}
				}
			}
		}

		//Validate the input when we lost focus.
		if (oldState == 2 && state == 0){
			update();
		}
	}

	return b || b0;
}

void GUISpinBox::render(SDL_Renderer &renderer, int x, int y, bool draw){
	//Call the GUITextBox::render().
	GUITextBox::render(renderer, x, y, draw);

	//FIXME: Logic in the render method since that is update constant.
	if(key!=-1){
		//Increase the key time.
		keyHoldTime++;
		//Make sure the deletionTime isn't to short.
		if(keyHoldTime>=keyTime){
			keyHoldTime=0;
			keyTime = getKeyboardRepeatInterval();
			
			//Now check the which key it was.
			switch(key){
				case SDLK_UP:
				{
					updateValue(true);
					break;
				}
				case SDLK_DOWN:
				{
					updateValue(false);
					break;
				}
			}
		}
	}
	
	//There's no need drawing when it's invisible.
	if(!visible || !draw)
		return;
	
	//Get the absolute x and y location.
	x+=left;
	y+=top;

	//Draw arrow buttons.
	SDL_Rect srcRect = { 80, 0, 16, 16 };
	SDL_Rect dstRect = { x + width - 18, y + 1, srcRect.w, srcRect.h };
	SDL_RenderCopy(&renderer, bmGuiTex.get(), &srcRect, &dstRect);

	srcRect.x = 96;
	dstRect.y += 16;
	SDL_RenderCopy(&renderer, bmGuiTex.get(), &srcRect, &dstRect);
}

void GUISpinBox::update(){
	//Read number from the caption string.
	float number=(float)atof(caption.c_str());
	
	//Stay in the limits.
	if(number>limitMax){
		number=limitMax;
	}else if(number<limitMin){
		number=limitMin;
	}
	
	//Write the number to the caption string.
	char str[128];
	sprintf(str,format.c_str(),number);
	updateText(str);
}

void GUISpinBox::updateValue(bool positive){
	//Read number from the caption string.
	float number=(float)atof(caption.c_str());
	
	//Apply change.
	if(positive)
		number+=change;
	else
		number-=change;
	
	//Stay in the limits.
	if(number>limitMax){
		number=limitMax;
	}else if(number<limitMin){
		number=limitMin;
	}
	
	//Write the number to the caption string.
	char str[128];
	sprintf(str,format.c_str(),number);
	updateText(str);
}
