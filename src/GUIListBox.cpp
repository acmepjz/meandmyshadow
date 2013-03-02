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

#include "GUIListBox.h"
using namespace std;

GUIListBox::GUIListBox(int left,int top,int width,int height,bool enabled,bool visible,int gravity):
GUIObject(left,top,width,height,0,NULL,-1,enabled,visible,gravity),itemHeight(24),selectable(true){
	//Set the state -1.
	state=-1;
	
	//Create the scrollbar and add it to the children.
	scrollBar=new GUIScrollBar(width-16,0,16,height,1,0,0,0,1,1,true,true);
	childControls.push_back(scrollBar);
}

GUIListBox::~GUIListBox(){
	//Remove all items and cache.
	clearItems();
	//We need to delete every child we have.
	for(unsigned int i=0;i<childControls.size();i++){
		delete childControls[i];
	}
	//Deleted the childs now empty the childControls vector.
	childControls.clear();
}

bool GUIListBox::handleEvents(int x,int y,bool enabled,bool visible,bool processed){
	//Boolean if the event is processed.
	bool b=processed;
	
	//The GUIObject is only enabled when he and his parent are enabled.
	enabled=enabled && this->enabled;
	//The GUIObject is only enabled when he and his parent are enabled.
	visible=visible && this->visible;
	
	//Get the absolute position.
	x+=left;
	y+=top;
	
	//Update the scrollbar.
	if(scrollBar->visible)
		b=b||scrollBar->handleEvents(x,y,enabled,visible,b);
	
	//Set state negative.
	state=-1;
	
	//Check if the GUIListBox is visible, enabled and no event has been processed before.
	if(enabled&&visible&&!b){
		//The mouse location (x=i, y=j) and the mouse button (k).
		int i,j;
		SDL_GetMouseState(&i,&j);
		
		//Convert the mouse location to a relative location.
		i-=x+2;
		j-=y+2;
		
		//Check if the mouse is inside the GUIListBox.
		if(i>=0&&i<width-4&&j>=0&&j<height-4){
			//Calculate the y location with the scrollbar position.
			int idx=j/itemHeight+scrollBar->value;
			
			//If the entry isn't above the max we have an event.
			if(idx>=0&&idx<(int)item.size()){
				state=idx;
				
				//Check if the left mouse button is pressed.
				if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button==SDL_BUTTON_LEFT && value!=idx){
					value=idx;
					
					//If event callback is configured then add an event to the queue.
					if(eventCallback){
						GUIEvent e={eventCallback,name,this,GUIEventClick};
						GUIEventQueue.push_back(e);
					}
				}
			}
			
			//Check for mouse wheel scrolling.
			if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button==SDL_BUTTON_WHEELDOWN && scrollBar->enabled){
				scrollBar->value++;
				if(scrollBar->value>scrollBar->maxValue)
					scrollBar->value=scrollBar->maxValue;
			}else if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button==SDL_BUTTON_WHEELUP && scrollBar->enabled){
				scrollBar->value--;
				if(scrollBar->value<0)
					scrollBar->value=0;
			}
		}
	}
	
	//Process child controls event except for the scrollbar.
	//That's why i starts at one.
	for(unsigned int i=1;i<childControls.size();i++){
		bool b1=childControls[i]->handleEvents(x,y,enabled,visible,b);
		
		//The event is processed when either our or the childs is true (or both).
		b=b||b1;
	}
	return b;
}

void GUIListBox::render(int x,int y,bool draw){
	if(updateScrollbar){
		//Calculate the height of the content.
		int maxY;
		for(int i=0;i<images.size();i++){
			maxY+=images.at(i)->h;
		}
		
		//Check if we need to show the scrollbar for many entries.
		if(maxY<height){
			scrollBar->maxValue=0;
			scrollBar->value=0;
			scrollBar->visible=false;
		}else{
			scrollBar->visible=true;
			scrollBar->maxValue=item.size();
			int yy=0;
			for(int i=images.size()-1;i>0;i--){
				yy+=images.at(i)->h;
				if(yy>height)
					break;
				else
					scrollBar->maxValue--;
			}
			scrollBar->largeChange=item.size()/4;
		}
		updateScrollbar=false;
	}
	
	//Rectangle the size of the GUIObject, used to draw borders.
	//SDL_Rect r; //Unused local variable :/
	//There's no need drawing the GUIObject when it's invisible.
	if(!visible||!draw) 
		return;
	
	//Get the absolute x and y location.
	x+=left;
	y+=top;
	
	//Draw the background box.
	SDL_Rect r={x,y,width,height};
	SDL_FillRect(screen,&r,0xFFFFFFFF);
	
	//Loop through the entries and draw them.
	for(int i=scrollBar->value,j=y+1;j>height,i<item.size();i++){
		//Check if the current item is out side of the widget.
		int yOver=images[i]->h;
		if(j+images[i]->h>y+height)
			yOver=y+height-j;
		
		if(yOver>0){
			if(selectable){
				//Check if the mouse is hovering on current entry. If so draw borders around it.
				if(state==i)
					drawGUIBox(x,j-1,width,yOver+1,screen,0x00000000);
				
				//Check if the current entry is selected. If so draw a gray background.
				if(value==i)
					drawGUIBox(x,j-1,width,yOver+1,screen,0xDDDDDDFF);
			}
			
			//Draw the image.
			SDL_Rect clip;
			clip.x=0;
			clip.y=0;
			clip.w=images[i]->w;
			clip.h=yOver;
			applySurface(x,j,images[i],screen,&clip);
		}else{
			break;
		}
		j+=images[i]->h;
	}
	
	//Draw borders around the whole thing.
	drawGUIBox(x,y,width,height,screen,0x00000000);
	
	//We now need to draw all the children of the GUIObject.
	for(unsigned int i=0;i<childControls.size();i++){
		childControls[i]->render(x,y,draw);
	}
}

void GUIListBox::clearItems(){
	item.clear();
	for(unsigned int i=0;i<images.size();i++){
		SDL_FreeSurface(images[i]);
	}
	images.clear();
}

void GUIListBox::addItem(std::string name, SDL_Surface* image){
	item.push_back(name);
	
	if(image){
		itemHeight=image->h;
		images.push_back(image);
	}else if(!image&&!name.empty()){
		SDL_Color black={0,0,0,0};
		SDL_Surface* tmp=TTF_RenderUTF8_Blended(fontText,name.c_str(),black);
		images.push_back(tmp);
	}
	
	updateScrollbar=true;
}

void GUIListBox::updateItem(int index, std::string newText, SDL_Surface* newImage){
	item.at(index)=newText;
	
	if(newImage){
		itemHeight=newImage->h;
		SDL_FreeSurface(images.at(index));
		images.at(index)=newImage;
	}else if(!newImage&&!newText.empty()){
		SDL_FreeSurface(images.at(index));
		SDL_Color black={0,0,0,0};
		SDL_Surface* tmp=TTF_RenderUTF8_Blended(fontText,newText.c_str(),black);
		images.at(index)=tmp;
	}
	
	updateScrollbar=true;
}

std::string GUIListBox::getItem(int index){
	return item.at(index);
}

GUISingleLineListBox::GUISingleLineListBox(int left,int top,int width,int height,bool enabled,bool visible,int gravity):
GUIObject(left,top,width,height,0,NULL,-1,enabled,visible,gravity),animation(0){}

bool GUISingleLineListBox::handleEvents(int x,int y,bool enabled,bool visible,bool processed){
	//Boolean if the event is processed.
	bool b=processed;
	
	//The GUIObject is only enabled when he and his parent are enabled.
	enabled=enabled && this->enabled;
	//The GUIObject is only enabled when he and his parent are enabled.
	visible=visible && this->visible;
	
	//Get the absolute position.
	x+=left-gravityX;
	y+=top;
	
	state&=~0xF;
	if(enabled&&visible){
		//The mouse location (x=i, y=j) and the mouse button (k).
		int i,j,k;
		k=SDL_GetMouseState(&i,&j);
		
		//Convert the mouse location to a relative location.
		i-=x;
		j-=y;
		
		//The selected button.
		//0=nothing 1=left 2=right.
		int idx=0;

		//Check which button the mouse is above.
		if(i>=0&&i<width&&j>=0&&j<height){
			if(i<26 && i<width/2){
				//The left arrow.
				idx=1;
			}else if(i>=width-26){
				//The right arrow.
				idx=2;
			}
		}
		
		//If idx is 0 it means the mous doesn't hover any arrow so reset animation.
		if(idx==0)
			animation=0;
		
		//Check if there's a mouse button press or not.
		if(k&SDL_BUTTON(1)){
			if(((state>>4)&0xF)==idx) 
				state|=idx;
		}else{
			state|=idx;
		}
		
		//Check if there's a mouse press.
		if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button==SDL_BUTTON_LEFT && idx){
			state=idx|(idx<<4);
		}else if(event.type==SDL_MOUSEBUTTONUP && event.button.button==SDL_BUTTON_LEFT && idx && ((state>>4)&0xF)==idx){
			int m=(int)item.size();
			if(m>0){
				if(idx==2){
					idx=value+1;
					if(idx<0||idx>=m) idx=0;
					if(idx!=value){
						value=idx;
						
						//If there is an event callback then call it.
						if(eventCallback){
							GUIEvent e={eventCallback,name,this,GUIEventClick};
							GUIEventQueue.push_back(e);
						}
					}
				}else if(idx==1){
					idx=value-1;
					if(idx<0||idx>=m) idx=m-1;
					if(idx!=value){
						value=idx;
						
						//If there is an event callback then call it.
						if(eventCallback){
							GUIEvent e={eventCallback,name,this,GUIEventClick};
							GUIEventQueue.push_back(e);
						}
					}
				}
			}
		}
		if(event.type==SDL_MOUSEBUTTONUP) state&=0xF;
	}else{
		//Set state zero.
		state=0;
	}
	
	//Also let the children handle their events.
	for(unsigned int i=0;i<childControls.size();i++){
		bool b1=childControls[i]->handleEvents(x,y,enabled,visible,b);
		
		//The event is processed when either our or the childs is true (or both).
		b=b||b1;
	}
	return b;
}

void GUISingleLineListBox::render(int x,int y,bool draw){
	//Rectangle the size of the GUIObject, used to draw borders.
	SDL_Rect r;
	
	//There's no need drawing the GUIObject when it's invisible.
	if(!visible) 
		return;
	
	//NOTE: logic in the render method since it's the only part that gets called every frame.
	if((state&0xF)==0x1 || (state&0xF)==0x2){
		animation++;
		if(animation>20)
			animation=-20;
	}
	
	//Get the absolute x and y location.
	x+=left;
	y+=top;
	
	if(gravity==GUIGravityCenter)
		gravityX=int(width/2);
	else if(gravity==GUIGravityRight)
		gravityX=width;
	
	x-=gravityX;
	
	//Check if the enabled state changed or the caption, if so we need to clear the (old) cache.
	if(enabled!=cachedEnabled || item[value].compare(cachedCaption)!=0){
		//Free the cache.
		SDL_FreeSurface(cache);
		cache=NULL;
		
		//And cache the new values.
		cachedEnabled=enabled;
		cachedCaption=item[value];
	}
	
	//Draw the text.
	if(value>=0 && value<(int)item.size()){
		//Get the text.
		const char* lp=item[value].c_str();
		
		//Check if the text is empty or not.
		if(lp!=NULL && lp[0]){
			if(!cache){
				SDL_Color color;
				if(inDialog)
					color=themeTextColorDialog;
				else
					color=themeTextColor;
				
				cache=TTF_RenderUTF8_Blended(fontGUI,lp,color);
				
				//If the text is too wide then we change to smaller font (?)
				//NOTE: The width is 32px smaller (2x16px for the arrows).
				if(cache->w>width-32){
					SDL_FreeSurface(cache);
					cache=TTF_RenderUTF8_Blended(fontGUISmall,lp,color);
				}
			}
			
			if(draw){
				//Center the text both vertically as horizontally.
				r.x=x+(width-cache->w)/2;
				r.y=y+(height-cache->h)/2-GUI_FONT_RAISE;
			
				//Draw the text and free the surface afterwards.
				SDL_BlitSurface(cache,NULL,screen,&r);
			}
		}
	}
	
	if(draw){
		//Draw the arrows.
		SDL_Rect r2={48,0,16,16};
		r.x=x;
		if((state&0xF)==0x1)
			r.x+=abs(animation/2);
		r.y=y+4;
		if(inDialog)
			applySurface(r.x,r.y,arrowLeft2,screen,NULL);
		else
			applySurface(r.x,r.y,arrowLeft1,screen,NULL);
		
		r.x=x+width-16;
		if((state&0xF)==0x2)
			r.x-=abs(animation/2);
		if(inDialog)
			applySurface(r.x,r.y,arrowRight2,screen,NULL);
		else
			applySurface(r.x,r.y,arrowRight1,screen,NULL);
	}
	
	//We now need to draw all the children of the GUIObject.
	for(unsigned int i=0;i<childControls.size();i++){
		childControls[i]->render(x,y,draw);
	}
}
