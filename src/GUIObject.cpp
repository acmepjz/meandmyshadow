/****************************************************************************
** Copyright (C) 2011 Luka Horvat <redreaper132 at gmail.com>
** Copyright (C) 2011 Edward Lii <edward_iii at myway.com>
** Copyright (C) 2011 O. Bahri Gordebak <gordebak at gmail.com>
**
**
** This file may be used under the terms of the GNU General Public
** License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
**  You should have received a copy of the GNU General Public License
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/
#include "GUIObject.h"
#include <iostream>
#include <list>
using namespace std;

//Set the GUIObjectRoot to NULL.
GUIObject* GUIObjectRoot=NULL;
//Initialise the event queue.
list<GUIEvent> GUIEventQueue;


void GUIObjectHandleEvents(bool kill){
	//Make sure that GUIObjectRoot isn't null.
	if(GUIObjectRoot)
		GUIObjectRoot->handleEvents();
	
	//Check for SDL_QUIT.
	if(event.type==SDL_QUIT && kill){
		//We get a quit event so enter the exit state.
		setNextState(STATE_EXIT);
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
		return;
	}
	
	//Keep calling events until there are none left.
	while(!GUIEventQueue.empty()){
		//Get one event and remove it from the queue.
		GUIEvent e=GUIEventQueue.front();
		GUIEventQueue.pop_front();
		
		//If an eventCallback exist call it.
		if(e.eventCallback){
			e.eventCallback->GUIEventCallback_OnEvent(e.name,e.obj,e.eventType);
		}
	}
	//We empty the event queue just to be sure.
	GUIEventQueue.clear();
}

GUIObject::~GUIObject(){
	//We need to delete every child we have.
	for(unsigned int i=0;i<childControls.size();i++){
		delete childControls[i];
	}
	//Deleted the childs now empty the childControls vector.
	childControls.clear();
}

bool GUIObject::handleEvents(int x,int y,bool enabled,bool visible,bool processed){
	//Boolean if the event is processed.
	bool b=processed;
	
	//The GUIObject is only enabled when he and his parent are enabled.
	enabled=enabled && this->enabled;
	//The GUIObject is only enabled when he and his parent are enabled.
	visible=visible && this->visible;
	
	//Get the absolute position.
	x+=left;
	y+=top;
	
	//Type specific event handling.
	switch(type){
	case GUIObjectButton:
		//Set state to 0.
		state=0;
		
		//Only check for events when the object is both enabled and visible.
		if(enabled&&visible){
			//The mouse location (x=i, y=j) and the mouse button (k).
			int i,j,k;
			k=SDL_GetMouseState(&i,&j);
			
			//Check if the mouse is inside the GUIObject.
			if(i>=x&&i<x+width&&j>=y&&j<y+height){
				//We have hover so set state to one.
				state=1;
				//Check for a mouse button press.
				if(k&SDL_BUTTON(1))
					state=2;
				
				//Check if there's a mouse press and the event hasn't been already processed.
				if(event.type==SDL_MOUSEBUTTONUP && event.button.button==SDL_BUTTON_LEFT && !b){
					//If event callback is configured then add an event to the queue.
					if(eventCallback){
						GUIEvent e={eventCallback,name,this,GUIEventClick};
						GUIEventQueue.push_back(e);
					}
					
					//Event has been processed.
					b=true;
				}
			}
		}
		break;
	case GUIObjectCheckBox:
		//Set state to 0.
		state=0;
		
		//Only check for events when the object is both enabled and visible.
		if(enabled&&visible){
			//The mouse location (x=i, y=j) and the mouse button (k).
			int i,j,k;
			k=SDL_GetMouseState(&i,&j);
			
			//Check if the mouse is inside the GUIObject.
			if(i>=x&&i<x+width&&j>=y&&j<y+height){
				//We have hover so set state to one.
				state=1;
				//Check for a mouse button press.
				if(k&SDL_BUTTON(1))
					state=2;
				
				//Check if there's a mouse press and the event hasn't been already processed.
				if(event.type==SDL_MOUSEBUTTONUP && event.button.button==SDL_BUTTON_LEFT && !b){
					//It's a checkbox so toggle the value.
					value=value?0:1;
					
					//If event callback is configured then add an event to the queue.
					if(eventCallback){
						GUIEvent e={eventCallback,name,this,GUIEventClick};
						GUIEventQueue.push_back(e);
					}
					
					//Event has been processed.
					b=true;
				}
			}
		}
		break;
	case GUIObjectTextBox:
		//NOTE: We don't reset the state to have a "focus" effect.
		  
		//Only check for events when the object is both enabled and visible.
		if(enabled&&visible){
			//Check if there's a key press and the event hasn't been already processed.
			if(state==2 && event.type==SDL_KEYDOWN && !b){
				//Get the keycode.
				int key=(int)event.key.keysym.unicode;
				
				//Check if the key is supported.
				if(key>=32&&key<=126){
					//Add the key to the text.
					caption+=char(key);
					
					//If there is an event callback then call it.
					if(eventCallback){
						GUIEvent e={eventCallback,name,this,GUIEventChange};
						GUIEventQueue.push_back(e);
					}
				}else if(event.key.keysym.sym==SDLK_BACKSPACE||event.key.keysym.sym==SDLK_DELETE){
					//We need to remove a character so first make sure that there is text.
					if(caption.length()>0){
						//Remove the last character from the text.
						caption=caption.substr(0,caption.length()-1);
						
						//If there is an event callback then call it.
						if(eventCallback){
							GUIEvent e={eventCallback,name,this,GUIEventChange};
							GUIEventQueue.push_back(e);
						}
					}
				}
					
				//The event has been processed.
				b=true;
			}
			
			//The mouse location (x=i, y=j) and the mouse button (k).
			int i,j,k;
			k=SDL_GetMouseState(&i,&j);
			
			//Check if the mouse is inside the GUIObject.
			if(i>=x&&i<x+width&&j>=y&&j<y+height){
				//We can only increase our state. (nothing->hover->focus).
				if(state!=2){
					state=1;
				}
				
				//Check for a mouse button press.
				if(k&SDL_BUTTON(1)){
					//We have focus.
					state=2;
				}
			}else{
				//The mouse is outside the TextBox.
				//If we don't have focus but only hover we lose it.
				if(state==1){
					state=0;
				}
				
				//If it's a click event outside the textbox then we blur.
				if(event.type==SDL_MOUSEBUTTONUP && event.button.button==SDL_BUTTON_LEFT){
					//Set state to 0.
					state=0;
				}
			}
		}
		break;
	}
	
	//Also let the children handle their events.
	for(unsigned int i=0;i<childControls.size();i++){
		bool b1=childControls[i]->handleEvents(x,y,enabled,visible,b);
		
		//The event is processed when either our or the childs is true (or both).
		b=b||b1;
	}
	return b;
}

void GUIObject::render(int x,int y){
	//Rectangle the size of the GUIObject, used to draw borders.
	SDL_Rect r;
	
	//There's no need drawing the GUIObject when it's invisible.
	if(!visible)
		return;
	
	//Get the absolute x and y location.
	x+=left;
	y+=top;
	
	//Now do the type specific rendering.
	switch(type){
	case GUIObjectLabel:
		{
			//The rectangle is simple.
			r.x=x;
			r.y=y;
			r.w=width;
			r.h=height;
			
			//We don't draw a background and/or border since that label is transparent.
			//Get the caption and make sure it isn't empty.
			const char* lp=caption.c_str();
			if(lp!=NULL && lp[0]){
				//Color the text will be: black.
				SDL_Color black={0,0,0,0};
				
				//Render the text using the small font.
				SDL_Surface* bm=TTF_RenderText_Blended(fontSmall,lp,black);

				//Center the text vertically and draw it to the screen.
				r.y=y+(height - bm->h)/2;
				SDL_BlitSurface(bm,NULL,screen,&r);
				SDL_FreeSurface(bm);
			}
		}
		break;
	case GUIObjectCheckBox:
		{
			//The rectangle is simple.
			r.x=x;
			r.y=y;
			r.w=width;
			r.h=height;
			
			//The background color.
			int clr=-1;
			
			//If hover we draw lightgray.
			if(state==1)
				clr=SDL_MapRGB(screen->format,192,192,192);
			//Else we draw gray.
			else if(state==2)
				clr=SDL_MapRGB(screen->format,128,128,128);
			
			//Fill the checkbox with gray.
			SDL_FillRect(screen,&r,clr);
			
			//Get the text.
			const char* lp=caption.c_str();
			//Make sure it isn't empty.
			if(lp!=NULL && lp[0]){
				//We render black text.
				SDL_Color black={0,0,0,0};
				SDL_Surface* bm=TTF_RenderText_Blended(fontSmall,lp,black);
				
				//Calculate the location, center it vertically.
				r.x=x+20;
				r.y=y+(height - bm->h)/2;
				
				//Draw the text and free the surface.
				SDL_BlitSurface(bm,NULL,screen,&r);
				SDL_FreeSurface(bm);
			}
			
			//Draw the check (or not).
			SDL_Rect r1={0,0,16,16};
			if(value==1||value==2)
				r1.x=value*16;
			r.x=x+2;
			r.y=y+(height-16)/2;
			SDL_BlitSurface(bmGUI,&r1,screen,&r);
		}
		break;
	case GUIObjectButton:
		{
			//The background color.
			int clr=-1;
			
			//If hover we draw lightgray.
			if(state==1)
				clr=SDL_MapRGB(screen->format,192,192,192);
			//Else we draw gray.
			else if(state==2)
				clr=SDL_MapRGB(screen->format,128,128,128);
			
			//Create a rectangle the size of the button and fill it.
			r.x=x;
			r.y=y;
			r.w=width;
			r.h=height;
			SDL_FillRect(screen,&r,0);
			//Shrink the rectangle by one pixel and fill with white leaving an one pixel border.
			r.x=x+1;
			r.y=y+1;
			r.w=width-2;
			r.h=height-2;
			SDL_FillRect(screen,&r,clr);
			
			//Get the text.
			const char* lp=caption.c_str();
			//Make sure the text isn't empty.
			if(lp!=NULL && lp[0]){
				//Draw black text.
				SDL_Color black={0,0,0,0};
				SDL_Surface* bm=TTF_RenderText_Blended(fontSmall,lp,black);
				
				//Center the text both vertically as horizontally.
				r.x=x+(width - bm->w)/2;
				r.y=y+(height - bm->h)/2;
				
				//Draw the text and free the surface.
				SDL_BlitSurface(bm,NULL,screen,&r);
				SDL_FreeSurface(bm);
			}
		}
		break;
	case GUIObjectTextBox:
		{
			//The background color.
			int clr=-1;
			//If hovering choose a lightgray background color.
			if(state==1) 
			clr=SDL_MapRGB(screen->format,192,192,192);
			
			//Create a rectangle the size of the button and fill it.
			r.x=x;
			r.y=y;
			r.w=width;
			r.h=height;
			SDL_FillRect(screen,&r,0);
			//Shrink the rectangle by one pixel and fill with white leaving an one pixel border.
			r.x=x+1;
			r.y=y+1;
			r.w=width-2;
			r.h=height-2;
			SDL_FillRect(screen,&r,clr);
			
			//Get the text.
			const char* lp=caption.c_str();
			//Make sure it isn't empty.
			if(lp!=NULL && lp[0]){
				//Draw the black text.
				SDL_Color black={0,0,0,0};
				SDL_Surface* bm=TTF_RenderText_Blended(fontSmall,lp,black);
				
				//Calculate the location, center it vertically.
				r.x=x+2;
				r.y=y+(height - bm->h)/2;
				
				//Draw the text.
				SDL_Rect tmp={0,0,width-2,25};
				SDL_BlitSurface(bm,&tmp,screen,&r);
				//Only draw the carrot when focus.
				if(state==2){
					r.x=x+4+bm->w;
					r.y=y+4;
					r.w=2;
					r.h=height-8;
					
					//Make sure that the carrot is inside the textbox.
					if(r.x<x+width)
						SDL_FillRect(screen,&r,0);
				}
				//And free the surface.
				SDL_FreeSurface(bm);
			}else{
				//Only draw the carrot when focus.
				if(state==2){
					r.x=x+4;
					r.y=y+4;
					r.w=2;
					r.h=height-8;
					SDL_FillRect(screen,&r,0);
				}
			}
		}
		break;
	case GUIObjectFrame:
		{
			//Create a rectangle the size of the button and fill it.
			r.x=x;
			r.y=y;
			r.w=width;
			r.h=height;
			SDL_FillRect(screen,&r,0);
			//Shrink the rectangle by one pixel and fill with white leaving an one pixel border.
			r.x=x+1;
			r.y=y+1;
			r.w=width-2;
			r.h=height-2;
			SDL_FillRect(screen,&r,-1);
			
			//Get the title text.
			const char* lp=caption.c_str();
			//Make sure it isn't empty.
			if(lp!=NULL && lp[0]){
				//The colors black and white used to render the title with white background.
				SDL_Color black={0,0,0,0};
				SDL_Color white={255,255,255,255};
				SDL_Surface* bm=TTF_RenderText_Shaded(font,lp,black,white);
				
				//Calculate the location, center horizontally and vertically relative to the top.
				r.x=x+(width-bm->w)/2;
				r.y=y-(int(bm->h))/2;
				
				//Create second rectangle for slightly wider text.
				SDL_Rect r2=r;
				r2.x-=10;
				r2.w=bm->w+20;
				r2.h=bm->h;
				SDL_FillRect(screen,&r2,-1);
				
				//Draw the text and free the surface.
				SDL_BlitSurface(bm,NULL,screen,&r);
				
				//Now draw a black border.
				drawLine(r2.x,r2.y,r2.x+r2.w,r2.y,screen,0);
				drawLine(r2.x,r2.y,r2.x,r2.y+r2.h/2,screen,0);
				drawLine(r2.x+r2.w,r2.y,r2.x+r2.w,r2.y+r2.h/2,screen,0);
				
				//And free the surface.
				SDL_FreeSurface(bm);
				
			}
		}
		break;
	}
	
	//We now need to draw all the children of the GUIObject.
	for(unsigned int i=0;i<childControls.size();i++){
		childControls[i]->render(x,y);
	}
}