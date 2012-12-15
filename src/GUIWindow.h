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

#ifndef GUIWINDOW_H
#define GUIWINDOW_H

#include "GUIObject.h"

//Resize directions.
const int GUIResizeTop=0;
const int GUIResizeTopRight=1;
const int GUIResizeRight=2;
const int GUIResizeBottomRight=3;
const int GUIResizeBottom=4;
const int GUIResizeBottomLeft=5;
const int GUIResizeLeft=6;
const int GUIResizeTopLeft=7;

//It extends GUIObject because it's a special GUIObject.
class GUIWindow:public GUIObject{
private:
	//Boolean if the window is being dragged.
	bool dragging;

	//Boolean if the window is being resized.
	bool resizing;
	//The direction the resizing occurs.
	int resizeDirection;
public:
	//The minimum and maximum size of the window.
	int minWidth,minHeight;
	int maxWidth,maxHeight;
	
	//Constructor.
	//left: The relative x location of the GUIWindow.
	//top: The relative y location of the GUIWindow.
	//witdh: The width of the GUIWindow.
	//height: The height of the GUIWindow.
	//enabled: Boolean if the GUIWindow is enabled or not.
	//visible: Boolean if the GUIWindow is visisble or not.
	//caption: The title of the Window.
	GUIWindow(int left=0,int top=0,int width=0,int height=0,bool enabled=true,bool visible=true,const char* caption=NULL);

	//Method used for moving the window around, also used internally by handleEvents.
	//x: The desired x location to move the window to.
	//y: The desired y location to move the window to.
	void move(int x,int y);

	//Method that will resize the window.
	//x: The new x location of the window.
	//y: The new y location of the window.
	//width: The new width of the resized window.
	//height: The new height of the resized window.
	void resize(int x,int y,int width,int height);
	
	//Method used to handle mouse and/or key events.
	//x: The x mouse location.
	//y: The y mouse location.
	//enabled: Boolean if the parent is enabled or not.
	//visible: Boolean if the parent is visible or not.
	//processed: Boolean if the event has been processed (by the parent) or not.
	//Returns: Boolean if the event is processed by the child.
	virtual bool handleEvents(int x=0,int y=0,bool enabled=true,bool visible=true,bool processed=false);
	//Method that will render the GUITextArea.
	//x: The x location to draw the GUITextArea. (x+left)
	//y: The y location to draw the GUITextArea. (y+top)
	virtual void render(int x=0,int y=0,bool draw=true);
};

#endif
