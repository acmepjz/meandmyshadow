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
#include "GUIListBox.h"
using namespace std;

namespace {
    inline int tHeight(const SharedTexture& t) {
        if(t) {
            return rectFromTexture(*t).h;
        } else {
            return 0;
        }
    }
    inline int tWidth(const SharedTexture& t) {
        if(t) {
            return rectFromTexture(*t).w;
        } else {
            return 0;
        }
    }
}

GUIListBox::GUIListBox(ImageManager& imageManager, SDL_Renderer& renderer,int left,int top,int width,int height,bool enabled,bool visible,int gravity):
GUIObject(imageManager,renderer,left,top,width,height,NULL,-1,enabled,visible,gravity),selectable(true),clickEvents(false){
	//Set the state -1.
	state=-1;
	
	//Create the scrollbar and add it to the children.
    scrollBar=new GUIScrollBar(imageManager,renderer,width-16,0,16,height,1,0,0,0,1,1,true,true);
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

bool GUIListBox::handleEvents(SDL_Renderer& renderer,int x,int y,bool enabled,bool visible,bool processed){
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
        b=b||scrollBar->handleEvents(renderer,x,y,enabled,visible,b);
	
	//Set state negative.
	state=-1;
	
	//Check if the GUIListBox is visible, enabled and no event has been processed before.
	if(enabled&&visible&&!b){
		//The mouse location (x=i, y=j) and the mouse button (k).
		int i,j;
		SDL_GetMouseState(&i,&j);
		
		//Convert the mouse location to a relative location.
		i-=x;
		j-=y;
		
		//Check if the mouse is inside the GUIListBox.
		if(i>=0&&i<width-4&&j>=0&&j<height-4){
			//Calculate selected item.
			int idx=-1;
			int yPos=-firstItemY;
			int i=scrollBar->value;
			if(yPos!=0) i--;
			for(;i<images.size();++i){
                const SharedTexture& tex = images.at(i);
                if(tex){
                    yPos+=tHeight(tex);
                }
                if(j<yPos){
                    idx=i;
                    break;
                }
			}
			
			//If the entry isn't above the max we have an event.
			if(idx>=0&&idx<(int)item.size()&&selectable){
				state=idx;
				
				//Check if the left mouse button is pressed.
				if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button==SDL_BUTTON_LEFT){
					//Check if the slected item changed.
					if(value!=idx){
						value=idx;
						
						//If event callback is configured then add an event to the queue.
						if(eventCallback){
							GUIEvent e={eventCallback,name,this,GUIEventChange};
							GUIEventQueue.push_back(e);
						}
					}

					//After possibly a change event, there will always be a click event.
					if(eventCallback && clickEvents){
						GUIEvent e={eventCallback,name,this,GUIEventClick};
						GUIEventQueue.push_back(e);
					}
				}
			}
			
			//Check for mouse wheel scrolling.
            if(event.type==SDL_MOUSEWHEEL && event.wheel.y < 0 && scrollBar->enabled){
				scrollBar->value++;
				if(scrollBar->value>scrollBar->maxValue)
					scrollBar->value=scrollBar->maxValue;
            }else if(event.type==SDL_MOUSEWHEEL && event.wheel.y > 0 && scrollBar->enabled){
				scrollBar->value--;
				if(scrollBar->value<0)
					scrollBar->value=0;
			}
		}
	}
	
	//Process child controls event except for the scrollbar.
	//That's why i starts at one.
	for(unsigned int i=1;i<childControls.size();i++){
        bool b1=childControls[i]->handleEvents(renderer,x,y,enabled,visible,b);
		
		//The event is processed when either our or the childs is true (or both).
		b=b||b1;
	}
	return b;
}

void GUIListBox::render(SDL_Renderer& renderer, int x,int y,bool draw){
	if(updateScrollbar){
		//Calculate the height of the content.
		int maxY=0;
        for(const SharedTexture& t: images){
            if(t){
                maxY+=textureHeight(*t);
            } else {
                std::cerr << "WARNING: Null texture in GUIListBox!" << std::endl;
            }
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
                yy+=textureHeight(*images.at(i));
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
    const SDL_Rect r={x,y,width,height};
    SDL_SetRenderDrawColor(&renderer,255,255,255,220);
    SDL_RenderFillRect(&renderer, &r);

	firstItemY=0;
	
	//Loop through the entries and draw them.
    if(scrollBar->value==scrollBar->maxValue&&scrollBar->visible){
		int lowNumber=height;
		int currentItem=images.size()-1;
		while(lowNumber>=0&&currentItem>=0){
            const SharedTexture& currentTexture = images.at(currentItem);
            lowNumber-=tHeight(currentTexture);//->h;
			
			if(lowNumber>0){
				if(selectable){
					//Check if the mouse is hovering on current entry. If so draw borders around it.
					if(state==currentItem)
                        drawGUIBox(x,y+lowNumber-1,width,tHeight(currentTexture)+1,renderer,0x00000000);
					
					//Check if the current entry is selected. If so draw a gray background.
					if(value==currentItem)
                        drawGUIBox(x,y+lowNumber-1,width,tHeight(currentTexture)+1,renderer,0xDDDDDDFF);
				}
				
                applyTexture(x,y+lowNumber,*currentTexture,renderer);
			}else{
                // This is the top item that is partially obscured.
				if(selectable){
					//Check if the mouse is hovering on current entry. If so draw borders around it.
					if(state==currentItem)
                        drawGUIBox(x,y,width,tHeight(currentTexture)+lowNumber+1,renderer,0x00000000);
					
					//Check if the current entry is selected. If so draw a gray background.
					if(value==currentItem)
                        drawGUIBox(x,y,width,tHeight(currentTexture)+lowNumber+1,renderer,0xDDDDDDFF);
				}

				firstItemY=-lowNumber;
				
                const SDL_Rect clip = rectFromTexture(0, -lowNumber, *currentTexture);

                const SDL_Rect dstRect{x, y, clip.w, clip.h};
                SDL_RenderCopy(&renderer, currentTexture.get(), &clip, &dstRect);
				break;
			}
			
			currentItem--;
		}
	}else{
		for(int i=scrollBar->value,j=y+1;j<=height&&i<(int)item.size();i++){
			//Check if the current item is out side of the widget.
            int yOver=tHeight(images[i]);
            if(j+yOver>y+height)
				yOver=y+height-j;
			
			if(yOver>0){
				if(selectable){
					//Check if the mouse is hovering on current entry. If so draw borders around it.
					if(state==i)
                        drawGUIBox(x,j-1,width,yOver+1,renderer,0x00000000);
					
					//Check if the current entry is selected. If so draw a gray background.
					if(value==i)
                        drawGUIBox(x,j-1,width,yOver+1,renderer,0xDDDDDDFF);
				}
				
				//Draw the image.
                const SDL_Rect clip{0, 0, tWidth(images[i]), yOver};
                const SDL_Rect dstRect{x, j, clip.w, clip.h};
                SDL_RenderCopy(&renderer, images[i].get(), &clip, &dstRect);
			}else{
				break;
			}
            j+=tHeight(images[i]);
		}
	}
	
	//Draw borders around the whole thing.
    drawGUIBox(x,y,width,height,renderer,0x00000000);
	
	//We now need to draw all the children of the GUIObject.
	for(unsigned int i=0;i<childControls.size();i++){
        childControls[i]->render(renderer,x,y,draw);
	}
}

void GUIListBox::clearItems(){
	item.clear();
	images.clear();
}

void GUIListBox::addItem(SDL_Renderer &renderer, std::string name, SharedTexture texture) {


    if(texture){
        images.push_back(texture);
    }else if(!texture&&!name.empty()){
        SDL_Color black={0,0,0,0};
        auto tex=SharedTexture(textureFromText(renderer, *fontText, name.c_str(), black));
        // Make sure we don't create any empty textures.
        if(!tex) {
            std::cerr << "WARNING: Failed to create texture from text: \"" << name << "\"" << std::endl;
            return;
        }
        images.push_back(tex);
    } else {
        // If nothing was added, ignore it.
        return;
    }
    item.push_back(name);
    updateScrollbar=true;
}

void GUIListBox::updateItem(SDL_Renderer &renderer, int index, string newText, SharedTexture newTexture) {
    if(newTexture) {
        images.at(index) = newTexture;
    } else if (!newTexture&&!newText.empty()) {
        SDL_Color black={0,0,0,0};
        auto tex=SharedTexture(textureFromText(renderer, *fontText, newText.c_str(), black));
        // Make sure we don't create any empty textures.
        if(!tex) {
            std::cerr << "WARNING: Failed to update texture at index" << index << " \"" << newText << "\"" << std::endl;
            return;
        }
        images.at(index)=tex;
    } else {
        return;
    }
    item.at(index)=newText;
    updateScrollbar=true;
}

std::string GUIListBox::getItem(int index){
	return item.at(index);
}

GUISingleLineListBox::GUISingleLineListBox(ImageManager& imageManager, SDL_Renderer& renderer, int left, int top, int width, int height, bool enabled, bool visible, int gravity):
GUIObject(imageManager,renderer,left,top,width,height,NULL,-1,enabled,visible,gravity),animation(0){}

void GUISingleLineListBox::addItem(string name,string label){
	//Check if the label is set, if not use the name.
	if(label.size()==0)
		label=name;

	item.push_back(pair<string,string>(name,label));
}

void GUISingleLineListBox::addItems(vector<pair<string,string> > items){
	vector<pair<string,string> >::iterator it;
	for(it=items.begin();it!=items.end();++it){
		addItem(it->first,it->second);
	}
}

void GUISingleLineListBox::addItems(vector<string> items){
	vector<string>::iterator it;
	for(it=items.begin();it!=items.end();++it){
		addItem(*it);
	}
}

string GUISingleLineListBox::getName(unsigned int index){
	if(index==-1)
		index=value;
	if(index<0||index>item.size())
		return "";

	return item[index].first;
}

bool GUISingleLineListBox::handleEvents(SDL_Renderer&,int x,int y,bool enabled,bool visible,bool processed){
	//Boolean if the event is processed.
	bool b=processed;
	
	//The GUIObject is only enabled when he and his parent(s) are enabled.
	enabled=enabled && this->enabled;
	//The GUIObject is only enabled when he and his parent(s) are enabled.
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
	
	return b;
}

void GUISingleLineListBox::render(SDL_Renderer& renderer, int x,int y,bool draw){
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
	if(enabled!=cachedEnabled || item[value].second.compare(cachedCaption)!=0){
		//Free the cache.
        cacheTex.reset(nullptr);

		//And cache the new values.
		cachedEnabled=enabled;
		cachedCaption=item[value].second;
	}
	
	//Draw the text.
	if(value>=0 && value<(int)item.size()){
		//Get the text.
        const std::string& lp=item[value].second;
		
		//Check if the text is empty or not.
        if(!lp.empty()){
            if(!cacheTex){
				SDL_Color color;
				if(inDialog)
					color=themeTextColorDialog;
				else
					color=themeTextColor;
				
                cacheTex=textureFromText(renderer, *fontGUI, lp.c_str(), color);

				//If the text is too wide then we change to smaller font (?)
				//NOTE: The width is 32px smaller (2x16px for the arrows).
                if(rectFromTexture(*cacheTex).w>width-32){
                    cacheTex=textureFromText(renderer, *fontGUISmall,lp.c_str(),color);
				}
			}
			
			if(draw){
				//Center the text both vertically as horizontally.
                const SDL_Rect textureSize = rectFromTexture(*cacheTex);
                r.x=x+(width-textureSize.w)/2;
                r.y=y+(height-textureSize.h)/2-GUI_FONT_RAISE;
			
                //Draw the text.
                applyTexture(r.x, r.y, cacheTex, renderer);
			}
		}
	}
	
	if(draw){
		//Draw the arrows.
		r.x=x;
		if((state&0xF)==0x1)
			r.x+=abs(animation/2);
		r.y=y+4;
		if(inDialog)
            applyTexture(r.x,r.y,*arrowLeft2,renderer);
		else
            applyTexture(r.x,r.y,*arrowLeft1,renderer);

		r.x=x+width-16;
		if((state&0xF)==0x2)
			r.x-=abs(animation/2);
		if(inDialog)
            applyTexture(r.x,r.y,*arrowRight2,renderer);
		else
            applyTexture(r.x,r.y,*arrowRight1,renderer);
	}
}
