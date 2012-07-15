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

#ifndef TITLE_MENU_H
#define TITLE_MENU_H

#include <SDL/SDL.h>
#include "GameState.h"

//Included for the Options menu.
#include "GUIObject.h"
#include "GUIListBox.h"
#include "GUISlider.h"


//The Main menu.
class Menu : public GameState{
private:
	//The title of the main menu.
	SDL_Surface* title;
	
	//Array containg pointers to the five main menu entries.
	//The last two are the '>' and '<' characters.
	SDL_Surface* entries[7];
	
	//Integer used for animations.
	int animation;
public:
	//Constructor.
	Menu();
	//Destructor.
	~Menu();

	//Inherited from GameState.
	void handleEvents();
	void logic();
	void render();
	void resize();
};

//The Options menu.
class Options : public GameState, private GUIEventCallback{
private:
	//The title of the options menu.
	SDL_Surface* title;

	//Slider used to set the music volume
	GUISlider* musicSlider;
	//Slider used to set the sound volume
	GUISlider* soundSlider;
	
	//The jump sound used as reference to configure sound volume.
	Mix_Chunk* jumpSound;
	//Integer to keep track of the time passed since last playing the test sound.
	int lastJumpSound;
	
	//ListBox containing the themes the user can choose out.
	GUISingleLineListBox* theme;
	
	//Map containing the locations the themes are stored.
	//The key is the name of the theme and the value the path.
	std::map<std::string,std::string> themeLocations;
	
	//Available languages
	GUISingleLineListBox* langs;
	std::vector<std::string> langValues;
	
	//Resolution list
	GUISingleLineListBox* resolutions;
	
	//GUI events are handled here.
	//name: The name of the element that invoked the event.
	//obj: Pointer to the object that invoked the event.
	//eventType: Integer containing the type of event.
	void GUIEventCallback_OnEvent(std::string name,GUIObject* obj,int eventType);

public:
	//Constructor.
	Options();
	//Destructor.
	~Options();
	
	//Method that will create the GUI for the options menu.
	void createGUI();
	
	//Inherited from GameState.
	void handleEvents();
	void logic();
	void render();
	void resize();
};

//A very simple structure for resolutions
struct _res{
	int w,h;
};

#endif
