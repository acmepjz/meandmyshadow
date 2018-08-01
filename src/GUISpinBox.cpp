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
#include <SDL_ttf.h>

bool GUISpinBox::handleEvents(SDL_Renderer&,int x,int y,bool enabled,bool visible,bool processed){
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
	if(enabled&&visible){
		//Check if there's a key press and the event hasn't been already processed.
		if(state==2 && event.type==SDL_KEYDOWN && !b){
			//Get the keycode.
			SDL_Keycode key=event.key.keysym.sym;
			
			//Check if the key is supported.
			if(key>=32&&key<=126){
				//Add the key to the text after the carrot. 
				caption.insert((size_t)value,1,char(key)); 
				value=clamp(value+1,0,caption.length()); 
				
				//If there is an event callback then call it.
				if(eventCallback){
					GUIEvent e={eventCallback,name,this,GUIEventChange};
					GUIEventQueue.push_back(e);
				}
			}else if(event.key.keysym.sym==SDLK_BACKSPACE){
				//We need to remove a character so first make sure that there is text.
				if(caption.length()>0&&value>0){
					//Remove the character before the carrot. 
					value=clamp(value-1,0,caption.length()); 
					caption.erase((size_t)value,1);
					
					//If there is an event callback then call it.
					if(eventCallback){
						GUIEvent e={eventCallback,name,this,GUIEventChange};
						GUIEventQueue.push_back(e);
					}
				}
			}else if(event.key.keysym.sym==SDLK_DELETE){
				//We need to remove a character so first make sure that there is text.
				if(caption.length()>0){
					//Remove the character after the carrot.
					value=clamp(value,0,caption.length());
					caption.erase((size_t)value,1);
					
					//If there is an event callback then call it.
					if(eventCallback){
						GUIEvent e={eventCallback,name,this,GUIEventChange};
						GUIEventQueue.push_back(e);
					}
				}
			}else if(event.key.keysym.sym==SDLK_RIGHT){
				value=clamp(value+1,0,caption.length());
			}else if(event.key.keysym.sym==SDLK_LEFT){
				value=clamp(value-1,0,caption.length());
			}		
			
			//The event has been processed.
			b=true;
		}
		
		//Only process mouse event when not in keyboard only mode
		if (!isKeyboardOnly) {
			//The mouse location (x=i, y=j) and the mouse button (k).
			int i, j, k;
			k = SDL_GetMouseState(&i, &j);

			//Check if the mouse is inside the GUIObject.
			if (i >= x&&i < x + width&&j >= y&&j < y + height){
				//We can only increase our state. (nothing->hover->focus).
				if (state != 2){
					state = 1;
				}

				//Also update the cursor type.
				if (i < x + width - 16)
					currentCursor = CURSOR_CARROT;

				//Check for a mouse button press.
				if (k&SDL_BUTTON(1)){
					//We have focus.
					state = 2;

					//Handle buttons.
					if (i > x + width - 16){
						if (j < y + 17){
							//Set the key values correct.
							this->key = SDLK_UP;
							keyHoldTime = 0;
							keyTime = 5;

							//Update once to prevent a lag.
							updateValue(true);
						} else{
							//Set the key values correct.
							this->key = SDLK_DOWN;
							keyHoldTime = 0;
							keyTime = 5;

							//Update once to prevent a lag.
							updateValue(false);
						}
					} else{
						//Move caret to the place clicked
						int click = i - x;

						if (!cacheTex){
							value = 0;
						} else if (click > textureWidth(*cacheTex)){
							value = caption.length();
						} else{
							unsigned int wid = 0;
							for (unsigned int i = 0; i < caption.length(); i++){
								int advance;
								TTF_GlyphMetrics(fontText, caption[i], NULL, NULL, NULL, NULL, &advance);
								wid += advance;

								if (click < (int)wid - (int)advance / 2){
									value = i;
									break;
								}
							}
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
			} else{
				//The mouse is outside the TextBox.
				//If we don't have focus but only hover we lose it.
				if (state == 1){
					state = 0;
					update();
				}

				//If it's a click event outside the textbox then we blur.
				if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT){
					//Set state to 0.
					state = 0;
					update();
				}
			}
		}
	}
	
	return b;
}

void GUISpinBox::render(SDL_Renderer &renderer, int x, int y, bool draw){
	//FIXME: Logic in the render method since that is update constant.
	if(key!=-1){
		//Increase the key time.
		keyHoldTime++;
		//Make sure the deletionTime isn't to short.
		if(keyHoldTime>=keyTime){
			keyHoldTime=0;
			keyTime--;
			if(keyTime<1)
				keyTime=1;
			
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
	if(!visible)
		return;
	
	//Get the absolute x and y location.
	x+=left;
	y+=top;
	
	//Check if the enabled state changed or the caption, if so we need to clear the (old) cache.
	if(enabled!=cachedEnabled || caption.compare(cachedCaption)!=0 || width<=0){
		//Free the cache.
        cacheTex.reset(nullptr);

		//And cache the new values.
		cachedEnabled=enabled;
		cachedCaption=caption;
		
		//Finally resize the widget
		if(autoWidth)
			width=-1;
	}
	
	if(draw){
		//Default background opacity.
		int clr=50;
		//If hovering or focused make background more visible.
		if(state==1) 
			clr=128;
		else if (state==2)
			clr=100;
	
		//Draw a background box.
        const Uint32 color=0xFFFFFF00|clr;
        drawGUIBox(x,y,width,height,renderer,color);
		
        SDL_Rect srcRect={80,0,16,16};
		//Draw arrow buttons.
        SDL_Rect dstRect = {x+width-18, y+1, srcRect.w, srcRect.h};
        SDL_RenderCopy(&renderer, bmGuiTex.get(), &srcRect, &dstRect);

        srcRect.x=96;
        dstRect.y+=16;
        SDL_RenderCopy(&renderer, bmGuiTex.get(), &srcRect, &dstRect);
	}
	
	if(!caption.empty()){
		//Update graphic cache if empty.
        if(!cacheTex){
            cacheTex=textureFromText(renderer,*fontText,caption.c_str(),objThemes.getTextColor(true));
		}

        //Cache the render color
        Uint8 r,g,b,a;
        SDL_GetRenderDrawColor(&renderer, &r,&g,&b,&a);
        //Set it to black for drawing the caret.
        SDL_SetRenderDrawColor(&renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);

		if(draw){
            const SDL_Rect srcRect={0,0,width-2,25};
            SDL_Rect dstRect = rectFromTexture(x+2,
                                               y+(height-textureHeight(*cacheTex))/2,
                                               *cacheTex);
            dstRect.w = std::min(dstRect.w, width - 4);
            SDL_RenderCopy(&renderer, cacheTex.get(), &srcRect, &dstRect);

            //Only draw the caret when focus.
			if(state==2){
                SDL_Rect caretRect{x, y+4, 2, height-8};
				
				int advance; 
				for(int n=0;n<value;n++){ 
					TTF_GlyphMetrics(fontText,caption[n],NULL,NULL,NULL,NULL,&advance); 
                    caretRect.x+=advance;
				}
				
				//Make sure that the carrot is inside the textbox.
                if(caretRect.x<x+width) {
                    //Draw a box representing the caret.
                    SDL_RenderFillRect(&renderer, &caretRect);
                }
			}

		}else{
            //Only draw the caret when focus.
			if(state==2&&draw){
                const SDL_Rect caretRect{x+4, y+4, 2, height-8};
                SDL_RenderFillRect(&renderer, &caretRect);
			}
		}
        //Reset the render color.
        SDL_SetRenderDrawColor(&renderer, r, g, b, a);
	}
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
	char str[32];
	sprintf(str,format,number);
	caption=str;
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
	char str[32];
	sprintf(str,format,number);
	caption=str;

	// restrict the caret position
	value = clamp(value, 0, caption.length());
}
