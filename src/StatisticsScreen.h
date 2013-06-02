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

#ifndef STATISTICSSCREEN_H
#define STATISTICSSCREEN_H

#include <SDL/SDL.h>
#include "GameState.h"
#include "GUIObject.h"
#include "GUIListBox.h"

class StatisticsScreen:public GameState, private GUIEventCallback{
private:
	//Contains title.
	SDL_Surface* title;

	//The list box used to switch between statistics and achievements.
	GUISingleLineListBox* listBox;

	//The list widgets used for achievements and statistics.
	std::vector<GUIListBox*> lists;

	//GUI events are handled here.
	//name: The name of the element that invoked the event.
	//obj: Pointer to the object that invoked the event.
	//eventType: Integer containing the type of event.
	void GUIEventCallback_OnEvent(std::string name,GUIObject* obj,int eventType);
public:
	//Constructor.
	StatisticsScreen();
	//Destructor.
	virtual ~StatisticsScreen();

	//Method that will create the GUI for the options menu.
	void createGUI();

	//In this method all the key and mouse events should be handled.
	//NOTE: The GUIEvents won't be handled here.
	virtual void handleEvents();
	
	//All the logic that needs to be done should go in this method.
	virtual void logic();
	
	//This method handles all the rendering.
	virtual void render();
	
	//Method that will be called when the screen size has been changed in runtime.
	virtual void resize();
};

#endif
