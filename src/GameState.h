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

#ifndef GAMESTATE_H
#define GAMESTATE_H


class GameState{
public:
	//Destructor.
	virtual ~GameState(){};
  
	//In this method all the key and mouse events should be handled.
	//NOTE: The GUIEvents won't be handled here.
	virtual void handleEvents()=0;
	
	//All the logic that needs to be done should go in this method.
	virtual void logic()=0;
	
	//This method handles all the rendering.
	virtual void render()=0;
	
	//Method that will be called when the screen size has been changed in runtime.
	virtual void resize()=0;
};

#endif
