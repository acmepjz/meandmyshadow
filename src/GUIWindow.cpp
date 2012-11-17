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

#include "GUIWindow.h"
using namespace std;

GUIWindow::GUIWindow(int left,int top,int width,int height,bool enabled,bool visible,const char* caption):
	GUIObject(left,top,width,height,0,caption,-1,enabled,visible){
}

bool GUIWindow::handleEvents(int x,int y,bool enabled,bool visible,bool processed){
	//Boolean if the event is processed.
	bool b=processed;

	//The GUIObject is only enabled when he and his parent are enabled.
	enabled=enabled && this->enabled;
	//The GUIObject is only enabled when he and his parent are enabled.
	visible=visible && this->visible;

	//Get the absolute position.
	x+=left;
	y+=top;

	//NOTE: We don't reset the state to have a "focus" effect.
	//Only check for events when the object is both enabled and visible.
	if(enabled&&visible){
		//Check if the titlebar is hit.
		if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button==SDL_BUTTON_LEFT){
			//Check if the mouse is inside the window.
			//NOTE: Perhaps only allow dragging when clicking on the 'titlebar'?
			SDL_Rect mouse={event.button.x,event.button.y,0,0};
			SDL_Rect r={x,y,width,height};
			
			if(checkCollision(mouse,r)){
				//Now begin dragging the toolbox
				dragging=true;
			}
		}else if(event.type==SDL_MOUSEBUTTONUP && event.button.button==SDL_BUTTON_LEFT){
			//Stop dragging
			dragging=false;

			SDL_Rect mouse={event.button.x,event.button.y,0,0};

			//Check if close button clicked
			{
				SDL_Rect r={left+width-36,top+12,24,24};
				if(checkCollision(mouse,r)){
					this->visible=false;
					return true;
				}
			}
		}else if(event.type==SDL_MOUSEMOTION){
			if((event.motion.state & SDL_BUTTON_LMASK)==0){
				dragging=false;
			}else if(dragging){
				move(left+event.motion.xrel,top+event.motion.yrel);
			}
		}
	}

	//Process child controls event.
	for(unsigned int i=0;i<childControls.size();i++){
		bool b1=childControls[i]->handleEvents(x,y,enabled,visible,b);

		//The event is processed when either our or the childs is true (or both).
		b=b||b1;
	}
	return b;
}

void GUIWindow::move(int x,int y){
	//Check the horizontal bounds.
	if(x>SCREEN_WIDTH-width)
		x=SCREEN_WIDTH-width;
	else if(x<0)
		x=0;
	//Check the vertical bounds.
	if(y>SCREEN_HEIGHT-height)
		y=SCREEN_HEIGHT-height;
	else if(y<0)
		y=0;

	//And set the new position.
	left=x;
	top=y;
}

void GUIWindow::render(int x,int y,bool draw){
	//Rectangle the size of the GUIObject, used to draw borders.
	SDL_Rect r;
	//There's no need drawing the GUIObject when it's invisible.
	if(!visible||!draw)
		return;

	//Get the absolute x and y location.
	x+=left;
	y+=top;

	//Draw the frame.
	Uint32 color=0xFFFFFFFF;
	drawGUIBox(x,y,width,height,screen,color);

	//Get the mouse position.
	int mouseX,mouseY;
	SDL_GetMouseState(&mouseX,&mouseY);
	SDL_Rect mouse={mouseX,mouseY,0,0};

	//Draw the close button.
	{
		//check highlight
		SDL_Rect r={left+width-36,top+12,24,24};

		if(checkCollision(mouse,r)){
			drawGUIBox(r.x,r.y,r.w,r.h,screen,0x999999FFU);
		}

		SDL_Rect r1={112,0,16,16};
		applySurface(left+width-32,top+16,bmGUI,screen,&r1);
	}

	//Draw the caption.
	{
		SDL_Rect captionRect={6,8,width-16,32};
		//The color black.
		SDL_Color black={0,0,0,0};
		SDL_Surface* bm=TTF_RenderUTF8_Blended(fontGUI,caption.c_str(),black);
		applySurface(x+captionRect.x+(captionRect.w-bm->w)/2,
			y+captionRect.y+(captionRect.h-bm->h)/2,bm,screen,NULL);
		
		SDL_FreeSurface(bm);
	}

	//We now need to draw all the children of the GUIObject.
	for(unsigned int i=0;i<childControls.size();i++){
		childControls[i]->render(x,y,draw);
	}
}
