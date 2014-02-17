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

#ifndef GUIOBJECT_H
#define GUIOBJECT_H

#include "Globals.h"
#include "Functions.h"
#include "FileManager.h"
#include <string>
#include <vector>
#include <list>

//Widget gravity properties
const int GUIGravityLeft=0;
const int GUIGravityCenter=1;
const int GUIGravityRight=2;

//The event id's.
//A click event used for e.g. buttons.
const int GUIEventClick=0;
//A change event used for e.g. textboxes.
const int GUIEventChange=1;


class GUIObject;

//Class that is used as event callback.
class GUIEventCallback{
public:
	//This method is called when an event is fired.
	//name: The name of the event.
	//obj: Pointer to the GUIObject which caused this event.
	//eventType: The type of event as defined above.
	virtual void GUIEventCallback_OnEvent(std::string name,GUIObject* obj,int eventType)=0;
};

//Class containing the 
class GUIObject{
public:
	//The relative x location of the GUIObject.
	int left;
	//The relative y location of the GUIObject.
	int top;
	//The width of the GUIObject.
	int width;
	//The height of the GUIObject.
	int height;
	
	//The type of the GUIObject.
	int type;
	//The value of the GUIObject.
	//It depends on the type of GUIObject what it means.
	int value;

	//The name of the GUIObject.
	std::string name;
	//The caption of the GUIObject.
	//It depends on the type of GUIObject what it is.
	std::string caption;
	
	//Boolean if the GUIObject is enabled.
	bool enabled;
	//Boolean if the GUIObject is visible.
	bool visible;
	
	//Vector containing the children of the GUIObject.
	std::vector<GUIObject*> childControls;
	
	//Event callback used to invoke events.
	GUIEventCallback* eventCallback;
	
	//Widget's gravity to centering
	int gravity;
	int gravityX;
	bool autoWidth;
	
	//Is the parent widget a dialog?
	bool inDialog;
protected:
	//The state of the GUIObject.
	//It depends on the type of GUIObject where it's used for.
	int state;
	
	//Surface containing some gui images.
	SDL_Surface* bmGUI;
	
	//Surface that can be used to cache rendered text.
	SDL_Surface* cache;
	//String containing the old caption to detect if it changed.
	std::string cachedCaption;
	//Boolean containing the previous enabled state.
	bool cachedEnabled;
public:
	//Constructor.
	//left: The relative x location of the GUIObject.
	//top: The relative y location of the GUIObject.
	//witdh: The width of the GUIObject.
	//height: The height of the GUIObject.
	//caption: The text on the GUIObject.
	//value: The value of the GUIObject.
	//enabled: Boolean if the GUIObject is enabled or not.
	//visible: Boolean if the GUIObject is visisble or not.
	//gravity: The way the GUIObject needs to be aligned.
	GUIObject(int left=0,int top=0,int width=0,int height=0,
		const char* caption=NULL,int value=0,
		bool enabled=true,bool visible=true,int gravity=0):
		left(left),top(top),width(width),height(height),
		gravity(gravity),value(value),
		enabled(enabled),visible(visible),
		eventCallback(NULL),state(0),
		cache(NULL),cachedEnabled(enabled),gravityX(0)
	{
		//Make sure that caption isn't NULL before setting it.
		if(caption){
			GUIObject::caption=caption;
			//And set the cached caption.
			cachedCaption=caption;
		}
		
		if(width<=0)
			autoWidth=true;
		else
			autoWidth=false;
		
		inDialog=false;
		
		//Load the gui images.
		bmGUI=loadImage(getDataPath()+"gfx/gui.png");
	}
	//Destructor.
	virtual ~GUIObject();
	
	//Method used to handle mouse and/or key events.
	//x: The x mouse location.
	//y: The y mouse location.
	//enabled: Boolean if the parent is enabled or not.
	//visible: Boolean if the parent is visible or not.
	//processed: Boolean if the event has been processed (by the parent) or not.
	//Returns: Boolean if the event is processed by the child.
	virtual bool handleEvents(int x=0,int y=0,bool enabled=true,bool visible=true,bool processed=false);
	//Method that will render the GUIObject.
	//x: The x location to draw the GUIObject. (x+left)
	//y: The y location to draw the GUIObject. (y+top)
	//draw: Draw widget or just update it without drawing
	virtual void render(int x=0,int y=0,bool draw=true);
	
	void addChild(GUIObject* obj){
		//Add widget add a child
		childControls.push_back(obj);
		
		//Copy inDialog boolean from parent.
		obj->inDialog=inDialog;
	}

	//Method for getting a child from a GUIObject.
	//NOTE: This method doesn't search recursively.
	//name: The name of the child to return.
	//Returns: Pointer to the requested child, NULL otherwise.
	GUIObject* getChild(std::string name){
		//Look for a child with the name.
		for(unsigned int i=0;i<childControls.size();i++)
			if(childControls[i]->name==name)
				return childControls[i];

		//Not found so return NULL.
		return NULL;
	}
};

//Method used to handle the GUIEvents from the GUIEventQueue.
//kill: Boolean if an SDL_QUIT event may kill the GUIObjectRoot.
void GUIObjectHandleEvents(bool kill=false);

//A structure containing the needed variables to call an event.
struct GUIEvent{
	//Event callback used to invoke the event.
	GUIEventCallback* eventCallback;
	//The name of the event.
	std::string name;
	//Pointer to the object which invoked the event.
	GUIObject* obj;
	//The type of event.
	int eventType;
};

//List used to queue the gui events.
extern std::list<GUIEvent> GUIEventQueue;

class GUIButton:public GUIObject{
public:
	GUIButton(int left=0,int top=0,int width=0,int height=0,
		const char* caption=NULL,int value=0,
		bool enabled=true,bool visible=true,int gravity=0):
		GUIObject(left,top,width,height,caption,value,enabled,visible,gravity),
		smallFont(false){ };
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
	//draw: Whether displey the widget or not.
	virtual void render(int x=0,int y=0,bool draw=true);
	
	//Boolean if small font is used.
	bool smallFont;
};

class GUICheckBox:public GUIObject{
public:
	GUICheckBox(int left=0,int top=0,int width=0,int height=0,
		const char* caption=NULL,int value=0,
		bool enabled=true,bool visible=true,int gravity=0):
		GUIObject(left,top,width,height,caption,value,enabled,visible,gravity){ };
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
	//draw: Whether displey the widget or not.
	virtual void render(int x=0,int y=0,bool draw=true);
};

class GUILabel:public GUIObject{
public:
	GUILabel(int left=0,int top=0,int width=0,int height=0,
		const char* caption=NULL,int value=0,
		bool enabled=true,bool visible=true,int gravity=0):
		GUIObject(left,top,width,height,caption,value,enabled,visible,gravity){ };
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
	//draw: Whether displey the widget or not.
	virtual void render(int x=0,int y=0,bool draw=true);
};

class GUITextBox:public GUIObject{
public:
	GUITextBox(int left=0,int top=0,int width=0,int height=0,
		const char* caption=NULL,int value=0,
		bool enabled=true,bool visible=true,int gravity=0):
		GUIObject(left,top,width,height,caption,value,enabled,visible,gravity),
		highlightStart(0),highlightEnd(0),tick(15),key(-1),keyHoldTime(0),keyTime(0){ };
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
	//draw: Whether displey the widget or not.
	virtual void render(int x=0,int y=0,bool draw=true);
private:
	//Text highlights.
	int highlightStart;
	int highlightEnd;
	
	int highlightStartX;
	int highlightEndX;
	
	//Carrot ticking.
	int tick;
	
	//Integer containing the key that is holded.
	int key;
	
	//Integer containing the time the key is pressed.
	int keyHoldTime;
	//The time it takes to invoke the key action again.
	int keyTime;
	
	//Functions for modifying the text.
	void backspaceChar();
	void deleteChar();
	
	//Functions for moving the carrot.
	void moveCarrotLeft();
	void moveCarrotRight();
};

class GUIFrame:public GUIObject{
public:
	GUIFrame(int left=0,int top=0,int width=0,int height=0,
		const char* caption=NULL,int value=0,
		bool enabled=true,bool visible=true,int gravity=0):
		GUIObject(left,top,width,height,caption,value,enabled,visible,gravity){
		inDialog=true;
	};
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
	//draw: Whether displey the widget or not.
	virtual void render(int x=0,int y=0,bool draw=true);
};

//A GUIObject that holds an SDL_Surface for rendering.
//NOTE: The image is not freed by the GUIImage.
class GUIImage:public GUIObject{
public:
	GUIImage(int left=0,int top=0,int width=0,int height=0,
		SDL_Surface* image=NULL,SDL_Rect clip=SDL_Rect(),bool managed=false,
		bool enabled=true,bool visible=true):
		GUIObject(left,top,width,height,NULL,0,enabled,visible,0),
		image(image),clip(clip),managed(managed){ };
	//Destructor.
	~GUIImage();
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
	//draw: Whether displey the widget or not.
	virtual void render(int x=0,int y=0,bool draw=true);

	//Method that will change the dimensions of the GUIImage so that the full image is shown.
	//OR in case of a clip rect that the selected section of the image is shown.
	void fitToImage();
	
	//Method for setting the image of the widget.
	//image: SDL_Surface containing the image.
	void setImage(SDL_Surface* surface){
		image=surface;
	}

	//Method for setting the clip rectangle for the GUIImager.
	//rect: The new clip rectangle.
	void setClipRect(SDL_Rect rect){
		clip=rect;
	}
private:
	//Boolean if the image should be managed by the GUIImage.
	//If set to true the image's surface will be freed upon deletion.
	bool managed;
	
	//Pointer to the SDL_Surface to draw.
	SDL_Surface* image;
	//Optional rectangle for defining the section of the surface that should be drawn.
	//NOTE: This doesn't have to correspond with the dimensions of the GUIObject.
	SDL_Rect clip;
};

#endif
