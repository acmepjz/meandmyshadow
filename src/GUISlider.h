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

#ifndef GUISLIDER_H
#define GUISLIDER_H

#include "GUIObject.h"

class GUISlider:public GUIObject{
public:
	//The minimum value of the slider.
	int minValue;
	//The maximum value of the slider.
	int maxValue;
	
	//The step size when a large step is made.
	int largeChange;
private:
	//
	float thumbStart;
	//
	float thumbEnd;
	//Float containing the value per pixel factor.
	float valuePerPixel;
	//Float containing the start position when dragging the scrollbar.
	float startDragPos;
	
	int criticalValue;
	int timer;
	
	//Boolean if the scrollbar position has changed.
	bool changed;
	
	//Method that will calculate the position of the scrollbar.
	void calcPos();
	
	//Method that will render a scrollbar button.
	//index: 
	//x1:
	//y1:
	//x2:
	//y2:
	//srcLeft:
	//srcRight:
	void renderScrollBarButton(int index,int x1,int y1,int x2,int y2,int srcLeft,int srcTop);
public:
	GUISlider(int left=0,int top=0,int width=0,int height=0,
		int value=0,int minValue=0,int maxValue=100,int largeChange=50,
		bool enabled=true,bool visible=true):
		GUIObject(left,top,width,height,NULL,value,enabled,visible),
		minValue(minValue),maxValue(maxValue),largeChange(largeChange),
		thumbStart(0.0f),thumbEnd(0.0f),valuePerPixel(0.0f),startDragPos(0.0f),criticalValue(0),timer(0),changed(false)
	{
		//In the constructor we simply call calcPos().
		calcPos();
	}
	
	
	//Method used to handle mouse and/or key events.
	//x: The x mouse location.
	//y: The y mouse location.
	//enabled: Boolean if the parent is enabled or not.
	//visible: Boolean if the parent is visible or not.
	//processed: Boolean if the event has been processed (by the parent) or not.
	//Returns: Boolean if the event is processed by the child.
	virtual bool handleEvents(int x=0,int y=0,bool enabled=true,bool visible=true,bool processed=false);
	//Method that will render the GUIScrollBar.
	//x: The x location to draw the GUIObject. (x+left)
	//y: The y location to draw the GUIObject. (y+top)
	virtual void render(int x=0,int y=0,bool draw=true);
};

#endif
