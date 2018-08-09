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

#ifndef GUISPINBOX_H
#define GUISPINBOX_H

#include "GUIObject.h"

class GUISpinBox:public GUITextBox{
public:
    GUISpinBox(ImageManager& imageManager,SDL_Renderer& renderer,int left=0,int top=0,int width=0,int height=0,
		bool enabled=true,bool visible=true):
        GUITextBox(imageManager,renderer,left,top,width,height,NULL,0,enabled,visible),
		change(1.0f),limitMax(100),limitMin(-100),format("%g"),
        key(-1),keyHoldTime(0),keyTime(0){}
	
	//Amount of single change.
	float change;
	//Widget's value stays between these values.
	float limitMax,limitMin;
	
	//Standard C printf format used for displaying the number.
	std::string format;
	
	//Method to update widget's value.
	void update();
	
	//Method to change widget's value.
	//positive: Boolean if add or remove change.
	void updateValue(bool positive);
	
	//Method used to handle mouse and/or key events.
	//x: The x mouse location.
	//y: The y mouse location.
	//enabled: Boolean if the parent is enabled or not.
	//visible: Boolean if the parent is visible or not.
	//processed: Boolean if the event has been processed (by the parent) or not.
	//Returns: Boolean if the event is processed by the child.
	virtual bool handleEvents(SDL_Renderer&,int x=0,int y=0,bool enabled=true,bool visible=true,bool processed=false) override;
	//Method that will render the GUIScrollBar.
	//x: The x location to draw the GUIObject. (x+left)
	//y: The y location to draw the GUIObject. (y+top)
    virtual void render(SDL_Renderer& renderer, int x=0,int y=0,bool draw=true) override;
private:
	//Integer containing the key that is holded.
	int key;
	
	//Integer containing the time the key is pressed.
	int keyHoldTime;
	//The time it takes to invoke the key action again.
	int keyTime;
};

#endif
