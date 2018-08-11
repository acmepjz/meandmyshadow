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
#include "GUIObject.h"
#include "ThemeManager.h"
#include "InputManager.h"
#include "GUISpinBox.h"
#include "GUIListBox.h"
#include "GUISlider.h"
#include <algorithm>
#include <iostream>
#include <list>
#include <SDL_ttf.h>

#include "Render.h"

using namespace std;

//Set the GUIObjectRoot to NULL.
GUIObject* GUIObjectRoot=NULL;
//Initialise the event queue.
list<GUIEvent> GUIEventQueue;

//A boolean variable used to skip next mouse up event for GUI (temporary workaround).
bool GUISkipNextMouseUpEvent = false;

void GUIObjectHandleEvents(ImageManager& imageManager, SDL_Renderer& renderer, bool kill){
	//NOTE: This was already not doing anything so commenting it for now.
	/*
	//Check if user resizes the window.
	if(event.type==SDL_VIDEORESIZE){
		//onVideoResize();

		//Don't let other objects process this event (?)
		return;
	}*/

	//Check if we need to reset the skip variable.
	if (event.type == SDL_MOUSEBUTTONDOWN) {
		GUISkipNextMouseUpEvent = false;
	}

	//Check if we need to skip event.
	if (event.type == SDL_MOUSEBUTTONUP && GUISkipNextMouseUpEvent) {
		GUISkipNextMouseUpEvent = false;
	} else {
		//Make sure that GUIObjectRoot isn't null.
		if (GUIObjectRoot)
			GUIObjectRoot->handleEvents(renderer);
	}
	
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
            e.eventCallback->GUIEventCallback_OnEvent(imageManager,renderer,e.name,e.obj,e.eventType);
		}
	}
	//We empty the event queue just to be sure.
	GUIEventQueue.clear();
}

GUIObject::GUIObject(ImageManager& imageManager, SDL_Renderer& renderer, int left, int top, int width, int height,
	const char* caption, int value,
	bool enabled, bool visible, int gravity) :
	left(left), top(top), width(width), height(height),
	gravity(gravity), value(value),
	enabled(enabled), visible(visible),
	eventCallback(NULL), state(0),
	cachedEnabled(enabled), gravityX(0)
{
	//Make sure that caption isn't NULL before setting it.
	if (caption){
		GUIObject::caption = caption;
		//And set the cached caption.
		cachedCaption = caption;
	}

	if (width <= 0)
		autoWidth = true;
	else
		autoWidth = false;

	inDialog = false;

	//Load the gui images.
	bmGuiTex = imageManager.loadTexture(getDataPath() + "gfx/gui.png", renderer);
}

GUIObject::~GUIObject(){
	//We need to delete every child we have.
	for(unsigned int i=0;i<childControls.size();i++){
		delete childControls[i];
	}
	//Deleted the childs now empty the childControls vector.
	childControls.clear();
}

void GUIObject::addChild(GUIObject* obj){
	//Add widget add a child
	childControls.push_back(obj);

	//Copy inDialog boolean from parent.
	obj->inDialog = inDialog;
}

GUIObject* GUIObject::getChild(const std::string& name){
	//Look for a child with the name.
	for (unsigned int i = 0; i<childControls.size(); i++)
		if (childControls[i]->name == name)
			return childControls[i];

	//Not found so return NULL.
	return NULL;
}

bool GUIObject::handleEvents(SDL_Renderer& renderer,int x,int y,bool enabled,bool visible,bool processed){
	//Boolean if the event is processed.
	bool b=processed;
	
	//The GUIObject is only enabled when its parent are enabled.
	enabled=enabled && this->enabled;
	//The GUIObject is only enabled when its parent are enabled.
	visible=visible && this->visible;
	
	//Get the absolute position.
	x+=left-gravityX;
	y+=top;
	
	//Also let the children handle their events.
	for(unsigned int i=0;i<childControls.size();i++){
        bool b1=childControls[i]->handleEvents(renderer,x,y,enabled,visible,b);
		
		//The event is processed when either our or the childs is true (or both).
		b=b||b1;
	}
	return b;
}

void GUIObject::render(SDL_Renderer& renderer, int x,int y,bool draw){
	//There's no need drawing the GUIObject when it's invisible.
	if(!visible)
		return;
	
	//Get the absolute x and y location.
	x+=left;
	y+=top;
	
	//We now need to draw all the children of the GUIObject.
	for(unsigned int i=0;i<childControls.size();i++){
        childControls[i]->render(renderer,x,y,draw);
	}
}

void GUIObject::refreshCache(bool enabled) {
    //Check if the enabled state changed or the caption, if so we need to clear the (old) cache.
    if(enabled!=cachedEnabled || caption.compare(cachedCaption)!=0 || width<=0){
        //TODO: Only change alpha if only enabled changes.
        //Free the cache.
        cacheTex.reset(nullptr);

        //And cache the new values.
        cachedEnabled=enabled;
        cachedCaption=caption;

        //Finally resize the widget
        if(autoWidth)
            width=-1;
    }
}

int GUIObject::getSelectedControl() {
	for (int i = 0; i < (int)childControls.size(); i++) {
		GUIObject *obj = childControls[i];
		if (obj && obj->visible && obj->enabled && obj->state) {
			if (dynamic_cast<GUIButton*>(obj) || dynamic_cast<GUICheckBox*>(obj)
				|| dynamic_cast<GUITextBox*>(obj) || dynamic_cast<GUISpinBox*>(obj)
				|| dynamic_cast<GUISingleLineListBox*>(obj)
				|| dynamic_cast<GUISlider*>(obj)
				)
			{
				return i;
			}
		}
	}

	return -1;
}

void GUIObject::setSelectedControl(int index) {
	for (int i = 0; i < (int)childControls.size(); i++) {
		GUIObject *obj = childControls[i];
		if (obj && obj->visible && obj->enabled) {
			if (dynamic_cast<GUIButton*>(obj) || dynamic_cast<GUICheckBox*>(obj)) {
				//It's a button.
				obj->state = (i == index) ? 1 : 0;
			} else if (dynamic_cast<GUITextBox*>(obj) || dynamic_cast<GUISpinBox*>(obj)) {
				//It's a text box.
				obj->state = (i == index) ? 2 : 0;
			} else if (dynamic_cast<GUISingleLineListBox*>(obj)) {
				//It's a single line list box.
				obj->state = (i == index) ? 0x100 : 0;
			} else if (dynamic_cast<GUISlider*>(obj)) {
				//It's a slider.
				obj->state = (i == index) ? 0x10000 : 0;
			}
		}
	}
}

int GUIObject::selectNextControl(int direction, int selected) {
	//Get the index of currently selected control.
	if (selected == 0x80000000) {
		selected = getSelectedControl();
	}

	//Find the next control.
	for (int i = 0; i < (int)childControls.size(); i++) {
		if (selected < 0) {
			selected = 0;
		} else {
			selected += direction;
			if (selected >= (int)childControls.size()) {
				selected -= childControls.size();
			} else if (selected < 0) {
				selected += childControls.size();
			}
		}

		GUIObject *obj = childControls[selected];
		if (obj && obj->visible && obj->enabled) {
			if (dynamic_cast<GUIButton*>(obj) || dynamic_cast<GUICheckBox*>(obj)
				|| dynamic_cast<GUITextBox*>(obj) || dynamic_cast<GUISpinBox*>(obj)
				|| dynamic_cast<GUISingleLineListBox*>(obj)
				|| dynamic_cast<GUISlider*>(obj)
				)
			{
				setSelectedControl(selected);
				return selected;
			}
		}
	}

	return -1;
}

bool GUIObject::handleKeyboardNavigationEvents(ImageManager& imageManager, SDL_Renderer& renderer, int keyboardNavigationMode) {
	if (keyboardNavigationMode == 0) return false;

	//Check operation on focused control. These have higher priority.
	if (isKeyboardOnly) {
		//Check enter key.
		if ((keyboardNavigationMode & 8) != 0 && inputMgr.isKeyDownEvent(INPUTMGR_SELECT)) {
			int index = getSelectedControl();
			if (index >= 0) {
				GUIObject *obj = childControls[index];

				if (dynamic_cast<GUIButton*>(obj)) {
					//It's a button.
					if (obj->eventCallback) {
						obj->eventCallback->GUIEventCallback_OnEvent(imageManager, renderer, obj->name, obj, GUIEventClick);
					}
					return true;
				}
				if (dynamic_cast<GUICheckBox*>(obj)) {
					//It's a check box.
					obj->value = obj->value ? 0 : 1;
					if (obj->eventCallback) {
						obj->eventCallback->GUIEventCallback_OnEvent(imageManager, renderer, obj->name, obj, GUIEventClick);
					}
					return true;
				}
			}
		}

		//Check left/right key.
		if ((keyboardNavigationMode & 16) != 0 && (inputMgr.isKeyDownEvent(INPUTMGR_LEFT) || inputMgr.isKeyDownEvent(INPUTMGR_RIGHT))) {
			int index = getSelectedControl();
			if (index >= 0) {
				GUIObject *obj = childControls[index];

				auto sllb = dynamic_cast<GUISingleLineListBox*>(obj);
				if (sllb) {
					//It's a single line list box.
					int newValue = sllb->value + (inputMgr.isKeyDownEvent(INPUTMGR_RIGHT) ? 1 : -1);
					if (newValue >= (int)sllb->item.size()) {
						newValue -= sllb->item.size();
					} else if (newValue < 0) {
						newValue += sllb->item.size();
					}

					if (sllb->value != newValue) {
						sllb->value = newValue;
						if (obj->eventCallback) {
							obj->eventCallback->GUIEventCallback_OnEvent(imageManager, renderer, obj->name, obj, GUIEventClick);
						}
					}
					return true;
				}

				auto slider = dynamic_cast<GUISlider*>(obj);
				if (slider) {
					//It's a slider.
					int newValue = slider->value + (inputMgr.isKeyDownEvent(INPUTMGR_RIGHT) ? slider->largeChange : -slider->largeChange);
					newValue = clamp(newValue, slider->minValue, slider->maxValue);

					if (slider->value != newValue) {
						slider->value = newValue;
						if (obj->eventCallback) {
							obj->eventCallback->GUIEventCallback_OnEvent(imageManager, renderer, obj->name, obj, GUIEventChange);
						}
					}
					return true;
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
		return true;
	} else if (((keyboardNavigationMode & 1) != 0 && inputMgr.isKeyDownEvent(INPUTMGR_LEFT))
		|| ((keyboardNavigationMode & 2) != 0 && inputMgr.isKeyDownEvent(INPUTMGR_UP))
		|| ((keyboardNavigationMode & 4) != 0 && inputMgr.isKeyDownEvent(INPUTMGR_TAB) && (m & KMOD_SHIFT) != 0)
		)
	{
		isKeyboardOnly = true;
		selectNextControl(-1);
		return true;
	}

	return false;
}

//////////////GUIButton///////////////////////////////////////////////////////////////////

bool GUIButton::handleEvents(SDL_Renderer& renderer,int x,int y,bool enabled,bool visible,bool processed){
	//Boolean if the event is processed.
	bool b=processed;
	
	//The widget is only enabled when its parent are enabled.
	enabled=enabled && this->enabled;
	//The widget is only enabled when its parent are enabled.
	visible=visible && this->visible;
	
	//Get the absolute position.
	x+=left-gravityX;
	y+=top;
	
	//We don't update button state under keyboard only mode.
	if (!isKeyboardOnly) {
		//Set state to 0.
		state = 0;

		//Only check for events when the object is both enabled and visible.
		if (enabled && visible) {
			//The mouse location (x=i, y=j) and the mouse button (k).
			int i, j, k;
			k = SDL_GetMouseState(&i, &j);

			//Check if the mouse is inside the widget.
			if (i >= x && i < x + width && j >= y && j < y + height) {
				//We have hover so set state to one.
				state = 1;
				//Check for a mouse button press.
				if (k&SDL_BUTTON(1))
					state = 2;

				//Check if there's a mouse press and the event hasn't been already processed.
				if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT && !b) {
					//If event callback is configured then add an event to the queue.
					if (eventCallback) {
						GUIEvent e = { eventCallback, name, this, GUIEventClick };
						GUIEventQueue.push_back(e);
					}

					//Event has been processed.
					b = true;
				}
			}
		}
	}
	
	//Also let the children handle their events.
	for(unsigned int i=0;i<childControls.size();i++){
        bool b1=childControls[i]->handleEvents(renderer,x,y,enabled,visible,b);
		
		//The event is processed when either our or the childs is true (or both).
		b=b||b1;
	}
	return b;
}

void GUIButton::render(SDL_Renderer& renderer, int x,int y,bool draw){
	//There's no need drawing the widget when it's invisible.
	if(!visible)
		return;
	
	//Get the absolute x and y location.
	x+=left;
	y+=top;
	
    refreshCache(enabled);
	
	//Get the text and make sure it isn't empty.
	const char* lp=caption.c_str();
	if(lp!=NULL && lp[0]){
		//Update cache if needed.
        if(!cacheTex){
			SDL_Color color = objThemes.getTextColor(inDialog);
					
            if(!smallFont) {
                cacheTex = textureFromText(renderer, *fontGUI, lp, color);
            } else {
                cacheTex = textureFromText(renderer, *fontGUISmall, lp, color);
            }
			//Make the widget transparent if it's disabled.
            if(!enabled) {
                SDL_SetTextureAlphaMod(cacheTex.get(), 128);
            }
			//Calculate proper size for the widget.
			if(width<=0){
                width=textureWidth(*cacheTex)+50;
				if(gravity==GUIGravityCenter){
					gravityX=int(width/2);
				}else if(gravity==GUIGravityRight){
					gravityX=width;
				}else{
					gravityX=0;
				}
			}
		}
		
		if(draw){
			//Center the text both vertically as horizontally.
            const SDL_Rect size = rectFromTexture(*cacheTex);
            const int drawX=x-gravityX+(width-size.w)/2;
            const int drawY=y+(height-size.h)/2-GUI_FONT_RAISE;
		
			//Check if the arrows don't fall of.
            if(size.w+32<=width){
				if(state==1){
					if(inDialog){
                        applyTexture(x-gravityX+(width-size.w)/2+4+size.w+5,y+2,*arrowLeft2,renderer);
                        applyTexture(x-gravityX+(width-size.w)/2-25,y+2,*arrowRight2,renderer);
					}else{
                        applyTexture(x-gravityX+(width-size.w)/2+4+size.w+5,y+2,*arrowLeft1,renderer);
                        applyTexture(x-gravityX+(width-size.w)/2-25,y+2,*arrowRight1,renderer);
					}
				}else if(state==2){
					if(inDialog){
                        applyTexture(x-gravityX+(width-size.w)/2+4+size.w,y+2,*arrowLeft2,renderer);
                        applyTexture(x-gravityX+(width-size.w)/2-20,y+2,*arrowRight2,renderer);
					}else{
                        applyTexture(x-gravityX+(width-size.w)/2+4+size.w,y+2,*arrowLeft1,renderer);
                        applyTexture(x-gravityX+(width-size.w)/2-20,y+2,arrowRight1,renderer);
					}
				}
			}
			
            //Draw the text.
            applyTexture(drawX, drawY, *cacheTex, renderer);
		}
	}
}

//////////////GUICheckBox///////////////////////////////////////////////////////////////////

bool GUICheckBox::handleEvents(SDL_Renderer&,int x,int y,bool enabled,bool visible,bool processed){
	//Boolean if the event is processed.
	bool b=processed;
	
	//The widget is only enabled when its parent are enabled.
	enabled=enabled && this->enabled;
	//The widget is only enabled when its parent are enabled.
	visible=visible && this->visible;
	
	//Get the absolute position.
	x+=left-gravityX;
	y+=top;

	//We don't update state under keyboard only mode.
	if (!isKeyboardOnly) {
		//Set state to 0.
		state = 0;

		//Only check for events when the object is both enabled and visible.
		if (enabled&&visible){
			//The mouse location (x=i, y=j) and the mouse button (k).
			int i, j, k;
			k = SDL_GetMouseState(&i, &j);

			//Check if the mouse is inside the widget.
			if (i >= x && i < x + width && j >= y && j < y + height){
				//We have hover so set state to one.
				state = 1;
				//Check for a mouse button press.
				if (k&SDL_BUTTON(1))
					state = 2;

				//Check if there's a mouse press and the event hasn't been already processed.
				if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT && !b){
					//It's a checkbox so toggle the value.
					value = value ? 0 : 1;

					//If event callback is configured then add an event to the queue.
					if (eventCallback){
						GUIEvent e = { eventCallback, name, this, GUIEventClick };
						GUIEventQueue.push_back(e);
					}

					//Event has been processed.
					b = true;
				}
			}
		}
	}
	
	return b;
}

void GUICheckBox::render(SDL_Renderer& renderer, int x,int y,bool draw){
	//There's no need drawing the widget when it's invisible.
	if(!visible)
		return;
	
	//Get the absolute x and y location.
	x+=left;
	y+=top;

    refreshCache(enabled);
	
	//Draw the highlight in keyboard only mode.
	if (isKeyboardOnly && state && draw) {
		drawGUIBox(x, y, width, height, renderer, 0xFFFFFF40);
	}

	//Get the text.
	const char* lp=caption.c_str();
	//Make sure it isn't empty.
	if(lp!=NULL && lp[0]){
		//Update the cache if needed.
        if(!cacheTex){
			SDL_Color color = objThemes.getTextColor(inDialog);

            cacheTex=textureFromText(renderer,*fontText,lp,color);
		}
		
		if(draw){
			//Calculate the location, center it vertically.
            const int drawX=x;
            const int drawY=y+(height - textureHeight(*cacheTex))/2;
		
            //Draw the text
            applyTexture(drawX, drawY, *cacheTex, renderer);
		}
	}
	
	if(draw){
		//Draw the check (or not).
        //value*16 determines where in the gui textures we draw from.
        //if(value==1||value==2)
        //	r1.x=value*16;
        const SDL_Rect srcRect={value*16,0,16,16};
        const SDL_Rect dstRect={x+width-20, y+(height-16)/2, 16, 16};
        //Get the right image depending on the state of the object.
        SDL_RenderCopy(&renderer, bmGuiTex.get(), &srcRect, &dstRect);
	}
}

//////////////GUILabel///////////////////////////////////////////////////////////////////

bool GUILabel::handleEvents(SDL_Renderer&,int ,int ,bool ,bool ,bool processed){
	return processed;
}

void GUILabel::render(SDL_Renderer& renderer, int x,int y,bool draw){
	//There's no need drawing the widget when it's invisible.
	if(!visible)
		return;
	
	//Check if the enabled state changed or the caption, if so we need to clear the (old) cache.
    refreshCache(enabled);
	
	//Get the absolute x and y location.
	x+=left;
	y+=top;
	
	//Rectangle the size of the widget.
	SDL_Rect r;
	r.x=x;
	r.y=y;
	r.w=width;
	r.h=height;
	
	//Get the caption and make sure it isn't empty.
	const char* lp=caption.c_str();
	if(lp!=NULL && lp[0]){
		//Update cache if needed.
        if(!cacheTex){
			SDL_Color color = objThemes.getTextColor(inDialog);

            cacheTex=textureFromText(renderer, *fontText, lp, color);
			if(width<=0)
                width=textureWidth(*cacheTex);
		}
		
		//Align the text properly and draw it.
		if(draw){
            const SDL_Rect size = rectFromTexture(*cacheTex);
			if(gravity==GUIGravityCenter)
                gravityX=(width-size.w)/2;
			else if(gravity==GUIGravityRight)
                gravityX=width-size.w;
			else
				gravityX=0;
			
            r.y=y+(height - size.h)/2;
			r.x+=gravityX;
            applyTexture(r.x, r.y, cacheTex, renderer);
		}
	}
}

//////////////GUITextBox///////////////////////////////////////////////////////////////////

void GUITextBox::backspaceChar(){
	//We need to remove a character so first make sure that there is text.
	if(caption.length()>0){
		if(highlightStart==highlightEnd&&highlightStart>0){
			int advance = 0;

			// this is proper UTF-8 support
			int ch = utf8ReadBackward(caption.c_str(), highlightStart); // we obtain new highlightStart from this
			if (ch > 0) TTF_GlyphMetrics(fontText, ch, NULL, NULL, NULL, NULL, &advance);
			highlightEndX = highlightStartX = highlightEndX - advance;
			
			caption.erase(highlightStart, highlightEnd - highlightStart);
			highlightEnd = highlightStart;
		} else if (highlightStart<highlightEnd){
			caption.erase(highlightStart,highlightEnd-highlightStart);
			highlightEnd=highlightStart;
			highlightEndX=highlightStartX;
		}else{
			caption.erase(highlightEnd,highlightStart-highlightEnd);
			highlightStart=highlightEnd;
			highlightStartX=highlightEndX;
		}
		
		//If there is an event callback then call it.
		if(eventCallback){
			GUIEvent e={eventCallback,name,this,GUIEventChange};
			GUIEventQueue.push_back(e);
		}
	}
}

void GUITextBox::deleteChar(){
	//We need to remove a character so first make sure that there is text.
	if(caption.length()>0){
		if(highlightStart==highlightEnd){
			// this is proper utf8 support
			int i = highlightEnd;
			utf8ReadForward(caption.c_str(), i);
			if (i > highlightEnd) caption.erase(highlightEnd, i - highlightEnd);
			
			highlightStart=highlightEnd;
			highlightStartX=highlightEndX;
		}else if(highlightStart<highlightEnd){
			caption.erase(highlightStart,highlightEnd-highlightStart);
			
			highlightEnd=highlightStart;
			highlightEndX=highlightStartX;
		}else{
			caption.erase(highlightEnd,highlightStart-highlightEnd);
			
			highlightStart=highlightEnd;
			highlightStartX=highlightEndX;
		}
			
		//If there is an event callback then call it.
		if(eventCallback){
			GUIEvent e={eventCallback,name,this,GUIEventChange};
			GUIEventQueue.push_back(e);
		}
	}
}

void GUITextBox::moveCarrotLeft(){
	if(highlightEnd>0){
		int advance = 0;

		// this is proper UTF-8 support
		int ch = utf8ReadBackward(caption.c_str(), highlightEnd); // we obtain new highlightEnd from this
		if (ch > 0) TTF_GlyphMetrics(fontText, ch, NULL, NULL, NULL, NULL, &advance);

		if(SDL_GetModState() & KMOD_SHIFT){
			highlightEndX-=advance;
		}else{
			highlightStart=highlightEnd;
			highlightStartX=highlightEndX=highlightEndX-advance;
		}
	}else{
		if((SDL_GetModState() & KMOD_SHIFT)==0){
			highlightStart=highlightEnd;
			highlightStartX=highlightEndX;
		}
	}
	tick=15;
}

void GUITextBox::moveCarrotRight(){
	if(highlightEnd<caption.length()){
		int advance = 0;

		// this is proper UTF-8 support
		int ch = utf8ReadForward(caption.c_str(), highlightEnd); // we obtain new highlightEnd from this
		if (ch > 0) TTF_GlyphMetrics(fontText, ch, NULL, NULL, NULL, NULL, &advance);

		if(SDL_GetModState() & KMOD_SHIFT){
			highlightEndX+=advance;
		}else{
			highlightStartX=highlightEndX=highlightEndX+advance;
			highlightStart=highlightEnd;
		}
	}else{
		if((SDL_GetModState() & KMOD_SHIFT)==0){
			highlightStart=highlightEnd;
			highlightStartX=highlightEndX;
		}
	}
	tick=15;
}

void GUITextBox::updateText(const std::string& text) {
	caption = text;
	updateSelection(0, 0);
}

void GUITextBox::updateSelection(int start, int end) {
	start = clamp(start, 0, caption.size());
	end = clamp(end, 0, caption.size());

	highlightStart = start;
	highlightStartX = 0;
	highlightEnd = end;
	highlightEndX = 0;

	if (start > 0) {
		TTF_SizeUTF8(fontText, caption.substr(0, start).c_str(), &highlightStartX, NULL);
	}
	if (end > 0) {
		TTF_SizeUTF8(fontText, caption.substr(0, end).c_str(), &highlightEndX, NULL);
	}
}

void GUITextBox::inputText(const char* s) {
	int m = strlen(s);

	if (m > 0){
		if (highlightStart == highlightEnd) {
			caption.insert((size_t)highlightStart, s);
			highlightStart += m;
			highlightEnd = highlightStart;
		} else if (highlightStart < highlightEnd) {
			caption.erase(highlightStart, highlightEnd - highlightStart);
			caption.insert((size_t)highlightStart, s);
			highlightStart += m;
			highlightEnd = highlightStart;
			highlightEndX = highlightStartX;
		} else {
			caption.erase(highlightEnd, highlightStart - highlightEnd);
			caption.insert((size_t)highlightEnd, s);
			highlightEnd += m;
			highlightStart = highlightEnd;
			highlightStartX = highlightEndX;
		}
		int advance = 0;
		for (int i = 0;;) {
			int a = 0;
			int ch = utf8ReadForward(s, i);
			if (ch <= 0) break;
			TTF_GlyphMetrics(fontText, ch, NULL, NULL, NULL, NULL, &a);
			advance += a;
		}
		highlightStartX = highlightEndX = highlightStartX + advance;

		//If there is an event callback then call it.
		if (eventCallback){
			GUIEvent e = { eventCallback, name, this, GUIEventChange };
			GUIEventQueue.push_back(e);
		}
	}
}

bool GUITextBox::handleEvents(SDL_Renderer&,int x,int y,bool enabled,bool visible,bool processed){
	//Boolean if the event is processed.
	bool b=processed;
	
	//The widget is only enabled when its parent are enabled.
	enabled=enabled && this->enabled;
	//The widget is only enabled when its parent are enabled.
	visible=visible && this->visible;
	
	//Get the absolute position.
	x+=left-gravityX;
	y+=top;

	//NOTE: We don't reset the state to have a "focus" effect.
	
	//Only check for events when the object is both enabled and visible.
	if(enabled&&visible){
		//Check if there's a key press and the event hasn't been already processed.
		if(state==2 && event.type==SDL_KEYDOWN && !b){
			//Get the keycode.
			SDL_Keycode key=event.key.keysym.sym;
			
			if ((event.key.keysym.mod & KMOD_CTRL) == 0) {
				//Check if the key is supported.
				if (event.key.keysym.sym == SDLK_BACKSPACE){
					backspaceChar();
				} else if (event.key.keysym.sym == SDLK_DELETE){
					deleteChar();
				} else if (event.key.keysym.sym == SDLK_RIGHT){
					moveCarrotRight();
				} else if (event.key.keysym.sym == SDLK_LEFT){
					moveCarrotLeft();
				}
			} else {
				//Check hotkey.
				if (event.key.keysym.sym == SDLK_a) {
					//Select all.
					highlightStart = 0;
					highlightStartX = 0;
					highlightEnd = caption.size();
					highlightEndX = 0;
					if (highlightEnd > 0) {
						TTF_SizeUTF8(fontText, caption.c_str(), &highlightEndX, NULL);
					}
				} else if (event.key.keysym.sym == SDLK_x || event.key.keysym.sym == SDLK_c) {
					//Cut or copy.
					int start = highlightStart, end = highlightEnd;
					if (start > end) std::swap(start, end);
					if (start < end) {
						SDL_SetClipboardText(caption.substr(start, end - start).c_str());
						if (event.key.keysym.sym == SDLK_x) {
							//Cut.
							backspaceChar();
						}
					}
				} else if (event.key.keysym.sym == SDLK_v) {
					//Paste.
					if (SDL_HasClipboardText()) {
						char *s = SDL_GetClipboardText();
						inputText(s);
						SDL_free(s);
					}
				}
			}

			//The event has been processed.
			b = true;
		} else if (state == 2 && event.type == SDL_TEXTINPUT && !b){
			inputText(event.text.text);

			//The event has been processed.
			b = true;
		} else if (state == 2 && event.type == SDL_TEXTEDITING && !b){
			// TODO: process SDL_TEXTEDITING event
		}

		//Only process mouse event when not in keyboard only mode
		if (!isKeyboardOnly) {
			//The mouse location (x=i, y=j) and the mouse button (k).
			int i, j, k;
			k = SDL_GetMouseState(&i, &j);

			//Check if the mouse is inside the widget.
			if (i >= x && i < x + width && j >= y && j < y + height){
				//We can only increase our state. (nothing->hover->focus).
				if (state != 2){
					state = 1;
				}

				//Also update the cursor type.
				currentCursor = CURSOR_CARROT;

				//Move carrot and highlightning according to mouse input.
				int clickX = i - x - 2;

				int finalPos = 0;
				int finalX = 0;

				if (cacheTex&&!caption.empty()){
					finalPos = caption.length();
					for (int i = 0;;){
						int advance = 0;

						// this is proper UTF-8 support
						int i0 = i;
						int ch = utf8ReadForward(caption.c_str(), i);
						if (ch <= 0) break;
						TTF_GlyphMetrics(fontText, ch, NULL, NULL, NULL, NULL, &advance);
						finalX += advance;

						if (clickX < finalX - advance / 2){
							finalPos = i0;
							finalX -= advance;
							break;
						}
					}
				}

				if (event.type == SDL_MOUSEBUTTONUP){
					state = 2;
					highlightEnd = finalPos;
					highlightEndX = finalX;
				} else if (event.type == SDL_MOUSEBUTTONDOWN){
					state = 2;
					highlightStart = highlightEnd = finalPos;
					highlightStartX = highlightEndX = finalX;
				} else if (event.type == SDL_MOUSEMOTION && (k&SDL_BUTTON(1))){
					state = 2;
					highlightEnd = finalPos;
					highlightEndX = finalX;
				}
			} else{
				//The mouse is outside the TextBox.
				//If we don't have focus but only hover we lose it.
				if (state == 1){
					state = 0;
				}

				//If it's a click event outside the textbox then we blur.
				if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT){
					//Set state to 0.
					state = 0;
				}
			}
		}
	}
	
	return b;
}

void GUITextBox::render(SDL_Renderer& renderer, int x,int y,bool draw){
	//There's no need drawing the widget when it's invisible.
	if(!visible)
		return;
	
	//Get the absolute x and y location.
	x+=left;
	y+=top;

    //Check if the enabled state changed or the caption, if so we need to clear the (old) cache.
    refreshCache(enabled);
	
	if(draw){
		//Default background opacity
		int clr=50;
		//If hovering or focused make background more visible.
		if(state==1)
			clr=128;
		else if (state==2)
			clr=100;
	
		//Draw the box.
		Uint32 color=0xFFFFFF00|clr;
        drawGUIBox(x,y,width,height,renderer,color);
	}
	
	//Rectangle used for drawing.
    SDL_Rect r{0,0,0,0};
	
	//Get the text and make sure it isn't empty.
	const char* lp=caption.c_str();
	if(lp!=NULL && lp[0]){
        if(!cacheTex) {
            //Draw the text.
            cacheTex=textureFromText(renderer,*fontText,lp,objThemes.getTextColor(true));
        }
				
		if(draw){
			//Only draw the carrot and highlight when focus.
			if(state==2){
				//Place the highlighted area.
				r.x=x+4;
				r.y=y+3;
				r.h=height-6;
				
				if(highlightStart<highlightEnd){
					r.x+=highlightStartX;
					r.w=highlightEndX-highlightStartX;
				}else{
					r.x+=highlightEndX;
					r.w=highlightStartX-highlightEndX;
				}
				
				//Draw the area.
                //SDL_FillRect(screen,&r,SDL_MapRGB(screen->format,128,128,128));
                SDL_SetRenderDrawColor(&renderer, 128,128,128,255);
                SDL_RenderFillRect(&renderer, &r);

				//Ticking carrot.
				if(tick<16){
					//Show carrot: 15->0.
					r.x=x+highlightEndX+2;
					r.y=y+3;
					r.h=height-6;
					r.w=2;
                    //SDL_FillRect(screen,&r,SDL_MapRGB(screen->format,0,0,0));
                    SDL_SetRenderDrawColor(&renderer,0,0,0,255);
                    SDL_RenderFillRect(&renderer, &r);

					//Reset: 32 or count down.
					if(tick<=0)
						tick=32;
					else
						tick--;
				}else{
					//Hide carrot: 32->16.
                    tick--;
				}
			}
			
			//Calculate the location, center it vertically.
            SDL_Rect dstRect=rectFromTexture(*cacheTex);
            dstRect.x=x+4;
            dstRect.y=y+(height-dstRect.h)/2;
            dstRect.w=std::min(width-2, dstRect.w);
			//Draw the text.
            const SDL_Rect srcRect={0,0,width-2,25};
            SDL_RenderCopy(&renderer, cacheTex.get(), &srcRect, &dstRect);
		}
	}else{
		//Only draw the carrot when focus.
		if(state==2&&draw){
			//Ticking carrot.
			if (tick<16){
				//Show carrot: 15->0.
				r.x = x + 4;
				r.y = y + 4;
				r.w = 2;
				r.h = height - 8;
				//SDL_FillRect(screen,&r,SDL_MapRGB(screen->format,0,0,0));
				SDL_SetRenderDrawColor(&renderer, 0, 0, 0, 255);
				SDL_RenderFillRect(&renderer, &r);

				//Reset: 32 or count down.
				if (tick <= 0)
					tick = 32;
				else
					tick--;
			} else{
				//Hide carrot: 32->16.
				tick--;
			}
		}
	}
}

//////////////GUIFrame///////////////////////////////////////////////////////////////////

bool GUIFrame::handleEvents(SDL_Renderer& renderer,int x,int y,bool enabled,bool visible,bool processed){
	//Boolean if the event is processed.
	bool b=processed;
	
	//The widget is only enabled when its parent are enabled.
	enabled=enabled && this->enabled;
	//The widget is only enabled when its parent are enabled.
	visible=visible && this->visible;
	
	//Get the absolute position.
	x+=left;
	y+=top;
	
	//Also let the children handle their events.
	for(unsigned int i=0;i<childControls.size();i++){
        bool b1=childControls[i]->handleEvents(renderer,x,y,enabled,visible,b);
		
		//The event is processed when either our or the childs is true (or both).
		b=b||b1;
	}
	return b;
}

void GUIFrame::render(SDL_Renderer& renderer, int x,int y,bool draw){
	//There's no need drawing this widget when it's invisible.
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
		
		//Finally resize the widget.
		if(autoWidth)
			width=-1;
	}
	
	//Draw fill and borders.
	if(draw){
		Uint32 color=0xDDDDDDFF;
        drawGUIBox(x,y,width,height,renderer,color);
	}
	
	//Get the title text and make sure it isn't empty.
	const char* lp=caption.c_str();
	if(lp!=NULL && lp[0]){
		//Update cache if needed.
        if(!cacheTex) {
            cacheTex = textureFromText(renderer, *fontGUI, lp, objThemes.getTextColor(true));
        }
		//Draw the text.
        if(draw) {
            applyTexture(x+(width-textureWidth(*cacheTex))/2, y+6-GUI_FONT_RAISE, *cacheTex, renderer);
        }
    }
	//We now need to draw all the children.
	for(unsigned int i=0;i<childControls.size();i++){
        childControls[i]->render(renderer,x,y,draw);
	}
}

//////////////GUIImage///////////////////////////////////////////////////////////////////

GUIImage::~GUIImage(){
}

bool GUIImage::handleEvents(SDL_Renderer&,int ,int ,bool ,bool ,bool processed){
	return processed;
}

void GUIImage::fitToImage(){
    const SDL_Rect imageSize = rectFromTexture(*image);

    //Increase or decrease the width and height to fully show the image.
    if(clip.w!=0) {
		width=clip.w;
    } else {
        width=imageSize.w;
    }
    if(clip.h!=0) {
		height=clip.h;
    } else {
        height=imageSize.h;
    }
}

void GUIImage::render(SDL_Renderer& renderer, int x,int y,bool draw){
	//There's no need drawing the widget when it's invisible.
    //Also make sure the image isn't null.
    if(!visible || !image)
		return;
	
	//Get the absolute x and y location.
	x+=left;
	y+=top;

	//Create a clip rectangle.
    SDL_Rect r=clip;
	//The width and height are capped by the GUIImage itself.
    if(r.w>width || r.w==0) {
		r.w=width;
    }
    if(r.h>height || r.h==0) {
		r.h=height;
    }

    const SDL_Rect dstRect={x,y,r.w,r.h};
    SDL_RenderCopy(&renderer, image.get(), &r, &dstRect);
}
