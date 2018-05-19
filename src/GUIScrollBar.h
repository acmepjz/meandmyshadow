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

#ifndef GUISCROLLBAR_H
#define GUISCROLLBAR_H

#include "GUIObject.h"

//Constant integers containing the two possible orientations of the scrollbar.
const int ScrollBarHorizontal=0;
const int ScrollBarVertical=1;

class GUIScrollBar:public GUIObject{
public:
	//The minimum value of the scrollbar.
	int minValue;
	//The maximum value of the scrollbar.
	int maxValue;
	
	//The step size when a small step is made.
	int smallChange;
	//The step size when a large step is made.
	int largeChange;
	
	//The orientation of the scrollbar.
	int orientation;
private:
	//
	float thumbStart;
	//
	float thumbEnd;
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
    void renderScrollBarButton(SDL_Renderer &renderer, int index, int x1, int y1, int x2, int y2, int srcLeft, int srcTop);
public:
    GUIScrollBar(ImageManager& imageManager, SDL_Renderer& renderer,int left=0,int top=0,int width=0,int height=0,int orientation=0,
		int value=0,int minValue=0,int maxValue=100,int smallChange=10,int largeChange=50,
		bool enabled=true,bool visible=true):
        GUIObject(imageManager,renderer,left,top,width,height,NULL,value,enabled,visible),
		minValue(minValue),maxValue(maxValue),smallChange(smallChange),largeChange(largeChange),orientation(orientation),
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
	virtual bool handleEvents(SDL_Renderer&,int x=0,int y=0,bool enabled=true,bool visible=true,bool processed=false);
	//Method that will render the GUIScrollBar.
	//x: The x location to draw the GUIObject. (x+left)
	//y: The y location to draw the GUIObject. (y+top)
    virtual void render(SDL_Renderer& renderer, int x=0,int y=0,bool draw=true);
};

#endif
