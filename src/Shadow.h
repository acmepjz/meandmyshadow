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

#ifndef SHADOW_H
#define SHADOW_H

#include "Player.h"

//The shadow class, it extends the player class since their almost the same.
class Shadow : public Player{
protected:
	//Boolean if the shadow is called by the player.
	//If so the shadow will copy the moves the player made.
	bool called;

	friend class Player;
public:
	//Constructor, it sets a few variables and calls the Player's constructor.
	//objParent: Pointer to the game instance.
	Shadow(Game* objParent);

	//Method that's called before the move function.
	//It's used to let the shadow do his logic, moving and jumping.
	void moveLogic();
	
	//Method used to notify the shadow that he is called.
	//He then must copy the moves that are given to him.
	void meCall();
	
	//Method used to reset the state.
	virtual void stateReset();
	//Method used to load the state.
	virtual void loadState();
};
#endif
