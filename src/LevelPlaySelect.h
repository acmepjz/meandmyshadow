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

#ifndef LEVELPLAYSELECT_H
#define LEVELPLAYSELECT_H

#include "GameState.h"
#include "LevelSelect.h"
#include "GameObjects.h"
#include "Player.h"
#include "GUIObject.h"
#ifdef __APPLE__
#include <SDL_mixer/SDL_mixer.h>
#include <SDL_ttf/SDL_ttf.h>
#else
#include <SDL/SDL_mixer.h>
#include <SDL/SDL_ttf.h>
#endif
#include <vector>
#include <string>

//This is the LevelSelect state, here you can select levelpacks and levels.
class LevelPlaySelect : public LevelSelect{
private:
	//Pointer to the play button, it is only shown when a level is selected.
	GUIObject* play;
	
	//Image of a play icon used as button to start replays.
	SDL_Surface* playButtonImage;
	
	//Image containing the time icon.
	SDL_Surface* timeIcon;
	//Image containing the recordings icon.
	SDL_Surface* recordingsIcon;
	
	//Method that will create the GUI elements.
	//initial: Boolean if it is the first time the gui is created.
	void createGUI(bool initial);
	
	//display level info.
	void displayLevelInfo(int number);

	//Check where and if the mouse clicked on a number.
	//If so it will start the level.
	void checkMouse();
public:
	//Constructor.
	LevelPlaySelect();
	//Destructor.
	~LevelPlaySelect();

	//Inherited from LevelSelect.
	void refresh(bool change=true);
	void selectNumber(unsigned int number,bool selected);
	
	//Inherited from GameState.
	void render();
	
	//Inherited from GameState.
	void resize();

	//Inherited from LevelSelect.
	void renderTooltip(unsigned int number,int dy);
	
	//GUI events will be handled here.
	void GUIEventCallback_OnEvent(std::string name,GUIObject* obj,int eventType);
};

#endif
