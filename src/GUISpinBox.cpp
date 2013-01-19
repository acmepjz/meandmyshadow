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

#include "GUISpinBox.h"

bool GUISpinBox::handleEvents(int x,int y,bool enabled,bool visible,bool processed){
	//Boolean if the event is processed.
	bool b=processed;
	//The GUIObject is only enabled when he and his parent are enabled.
	enabled=enabled && this->enabled;
	//The GUIObject is only enabled when he and his parent are enabled.
	visible=visible && this->visible;
	
	//Get the absolute position.
	x+=left-gravityX;
	y+=top;
	
	if(enabled&&visible){
		//The mouse location (x=i, y=j) and the mouse button (k).
		int i,j,k;
		k=SDL_GetMouseState(&i,&j);
		
		//Check if the mouse is inside the GUIObject.
		if(i>=x&&i<x+width&&j>=y&&j<y+height){
			//We can only increase our state. (nothing->hover->focus).
			if(state!=2){
				state=1;
			}
			
			if(k&SDL_BUTTON(1)){
				//We have focus.
				state=2;
			}
			
			//Check if there's a mouse press and the event hasn't been already processed.
			if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button==SDL_BUTTON_LEFT){			
				//Handle buttons.
				if((i>x+width-16)){
					if(j<y+17){
						number+=change;
					}else{
						number-=change;
					}
					//Clear cache so new surface is genereted for updated number.
					if(cache){
						SDL_FreeSurface(cache);
						cache=NULL;
					}
				}
			}
			
			//Allow mouse wheel to change value.
			if(event.type==SDL_MOUSEBUTTONUP){
				if(event.button.button==SDL_BUTTON_WHEELUP){
					number+=change;
				}else if(event.button.button==SDL_BUTTON_WHEELDOWN){
					number-=change;
				}
				//Clear cache so new surface is genereted for updated number.
				if(cache){
					SDL_FreeSurface(cache);
					cache=NULL;
				}
			}
			
			//TODO: handle keyboard input.
		}else{
			//The mouse is outside the GUISpinBox.
			//If we don't have focus but only hover we lose it.
			if(state==1){
				state=0;
			}
			
			//If it's a click event outside the GUISpinBox then we blur.
			if(event.type==SDL_MOUSEBUTTONUP && event.button.button==SDL_BUTTON_LEFT){
				//Set state to 0.
				state=0;
			}
		}
	}
	
	//Clamp number between defined limits.
	if(number>limitMax){
		number=limitMax;
	}else if(number<limitMin){
		number=limitMin;
	}
	
	//Also let the children handle their events.
	for(unsigned int i=0;i<childControls.size();i++){
		bool b1=childControls[i]->handleEvents(x,y,enabled,visible,b);
		
		//The event is processed when either our or the childs is true (or both).
		b=b||b1;
	}
	return b;
}

void GUISpinBox::render(int x,int y,bool draw){
	//Rectangle.
	SDL_Rect r;
	
	//There's no need drawing when it's invisible.
	if(!visible)
		return;
	
	//Get the absolute x and y location.
	x+=left;
	y+=top;
	
	//Update graphic cache if empty.
	if(!cache){
		//Draw a black text with current number.
		char str[32];
		sprintf(str,format,number);
		
		SDL_Color black={0,0,0,0};
		cache=TTF_RenderUTF8_Blended(fontText,str,black);
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
		Uint32 color=0xFFFFFF00|clr;
		drawGUIBox(x,y,width,height,screen,color);
		
		r.x=x+4;
		r.y=y+(height - cache->h)/2;
		
		//Draw the text.
		SDL_Rect tmp={0,0,width-2,25};
		SDL_BlitSurface(cache,&tmp,screen,&r);
		
		//Draw arrow buttons.
		r.y=y+1;
		r.x=x+width-18;
		SDL_Rect r1={80,0,16,16};
		SDL_BlitSurface(bmGUI,&r1,screen,&r);
		
		r.y+=16;
		r1.x=96;
		SDL_BlitSurface(bmGUI,&r1,screen,&r);
	}
	
	//We now need to draw all the children.
	for(unsigned int i=0;i<childControls.size();i++){
		childControls[i]->render(x,y,draw);
	}
}
