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

	//Set some default values.
	dragging=false;
	resizing=false;
	minWidth=minHeight=0;
	maxWidth=maxHeight=0;
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
		bool clicked=(event.type==SDL_MOUSEBUTTONDOWN && event.button.button==SDL_BUTTON_LEFT);
		
		//Check if the mouse is inside the window.
		SDL_Rect mouse={event.button.x,event.button.y,0,0};
		SDL_Rect titlebar={x,y+5,width,43}; //We have a resize edge at the top five pixels.

		//FIXME: Only set the cursor to POINTER when moving away from the GUIWindow?
		if(clicked && checkCollision(mouse,titlebar)){
			//Mouse pressed inside the window,so assume dragging
			dragging=true;
		}

		//Check for resizing.
		SDL_Rect edge={x,y,width,5};
		//Check each edge only if not resizing.
		//NOTE: This is done to preserve the resize cursor type when off the edge.
		bool topEdge=resizing?(resizeDirection==GUIResizeTop || resizeDirection==GUIResizeTopLeft || resizeDirection==GUIResizeTopRight):checkCollision(mouse,edge);
		edge.x=x+width-5;
		edge.w=5;
		edge.h=height;
		bool rightEdge=resizing?(resizeDirection==GUIResizeRight || resizeDirection==GUIResizeTopRight || resizeDirection==GUIResizeBottomRight):checkCollision(mouse,edge);
		edge.x=x;
		edge.y=y+height-5;
		edge.w=width;
		edge.h=5;
		bool bottomEdge=resizing?(resizeDirection==GUIResizeBottom || resizeDirection==GUIResizeBottomLeft || resizeDirection==GUIResizeBottomRight):checkCollision(mouse,edge);
		edge.y=y;
		edge.w=5;
		edge.h=height;
		bool leftEdge=resizing?(resizeDirection==GUIResizeLeft || resizeDirection==GUIResizeTopLeft || resizeDirection==GUIResizeBottomLeft):checkCollision(mouse,edge);

		//Set resizing true when resizing previously of clicking on a edge.
		if(topEdge || rightEdge || bottomEdge || leftEdge)
			resizing=resizing?true:clicked;
		//Determine the resize direction.
		if(topEdge){
			resizeDirection=GUIResizeTop;
			currentCursor=CURSOR_SIZE_VER;

			//Check if there's an additional horizontal edge (corner).
			if(leftEdge){
				currentCursor=CURSOR_SIZE_FDIAG;
				resizeDirection=GUIResizeTopLeft;
			}else if(rightEdge){
				currentCursor=CURSOR_SIZE_BDIAG;
				resizeDirection=GUIResizeTopRight;
			}
		}else if(bottomEdge){
			resizeDirection=GUIResizeBottom;
			currentCursor=CURSOR_SIZE_VER;

			//Check if there's an additional horizontal edge (corner).
			if(leftEdge){
				currentCursor=CURSOR_SIZE_BDIAG;
				resizeDirection=GUIResizeBottomLeft;
			}else if(rightEdge){
				currentCursor=CURSOR_SIZE_FDIAG;
				resizeDirection=GUIResizeBottomRight;
			}
		}else if(leftEdge){
			resizeDirection=GUIResizeLeft;
			currentCursor=CURSOR_SIZE_HOR;
		}else if(rightEdge){
			resizeDirection=GUIResizeRight;
			currentCursor=CURSOR_SIZE_HOR;
		}
		
		if(event.type==SDL_MOUSEBUTTONUP && event.button.button==SDL_BUTTON_LEFT){
			//Stop dragging
			dragging=false;

			SDL_Rect mouse={event.button.x,event.button.y,0,0};

			//Check if close button clicked
			{
				SDL_Rect r={left+width-36,top+12,24,24};
				if(checkCollision(mouse,r)){
					this->visible=false;
					//And we add a close event to the queue.
					GUIEvent e={eventCallback,name,this,GUIEventClick};
					GUIEventQueue.push_back(e);
				}
			}
		}else if(event.type==SDL_MOUSEMOTION){
			if((event.motion.state & SDL_BUTTON_LMASK)==0){
				//Stop dragging or resizing.
				dragging=false;
				resizing=false;
			}else if(dragging){
				move(left+event.motion.xrel,top+event.motion.yrel);
			}else if(resizing){
				//Check what the resize direction is.
				switch(resizeDirection){
					case GUIResizeTop:
						resize(left,top+event.motion.yrel,width,height-event.motion.yrel);
						break;
					case GUIResizeTopRight:
						resize(left,top+event.motion.yrel,width+event.motion.xrel,height-event.motion.yrel);
						break;
					case GUIResizeRight:
						resize(left,top,width+event.motion.xrel,height);
						break;
					case GUIResizeBottomRight:
						resize(left,top,width+event.motion.xrel,height+event.motion.yrel);
						break;
					case GUIResizeBottom:
						resize(left,top,width,height+event.motion.yrel);
						break;
					case GUIResizeBottomLeft:
						resize(left+event.motion.xrel,top,width-event.motion.xrel,height+event.motion.yrel);
						break;
					case GUIResizeLeft:
						resize(left+event.motion.xrel,top,width-event.motion.xrel,height);
						break;
					case GUIResizeTopLeft:
						resize(left+event.motion.xrel,top+event.motion.yrel,width-event.motion.xrel,height-event.motion.yrel);
						break;
				}
			}
		}

		//Also update the cursor type accordingly.
		if(dragging)
			currentCursor=CURSOR_DRAG;
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

void GUIWindow::resize(int x,int y,int width,int height){
	//FIXME: In case of resizing to the left or top the window moves when the maximum size has been reached.
	//Check for the minimum width.
	if(minWidth){
		if(width<minWidth)
			width=minWidth;
	}
	//Check for the minimum height.
	if(minHeight){
		if(height<minHeight)
			height=minHeight;
	}
	//Check for maximum width.
	if(maxWidth){
		if(width>maxWidth)
			width=maxWidth;
	}
	//Check for maximum height.
	if(maxHeight){
		if(height>maxHeight)
			height=maxHeight;
	}

	//Now set the values.
	this->left=x;
	this->top=y;
	this->width=width;
	this->height=height;

	//And we add a resize event to the queue.
	GUIEvent e={eventCallback,name,this,GUIEventChange};
	GUIEventQueue.push_back(e);
}

void GUIWindow::render(int x,int y,bool draw){
	//Rectangle the size of the GUIObject, used to draw borders.
	//SDL_Rect r; //Unused local variable :/
	//There's no need drawing the GUIObject when it's invisible.
	if(!visible||!draw)
		return;

	//Get the absolute x and y location.
	x+=left;
	y+=top;

	//Draw the frame.
	Uint32 color=0xFFFFFFFF;
	drawGUIBox(x,y,width,height,screen,color);
	//Draw the titlebar.
	color=0x00000033;
	drawGUIBox(x,y,width,48,screen,color);

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

void GUIWindow::GUIEventCallback_OnEvent(string name,GUIObject* obj,int eventType){
	//Check if we have a eventCallback.
	if(eventCallback){
		//We call the onEvent method of the callback, but change the GUIObject pointer to ourself.
		eventCallback->GUIEventCallback_OnEvent(name,this,eventType);
	}
}
