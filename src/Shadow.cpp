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

#include "Functions.h"
#include "FileManager.h"
#include "Game.h"
#include "Player.h"
#include "Shadow.h"
#include <vector>
#include <iostream>
using namespace std;

Shadow::Shadow(Game* objParent):Player(objParent){
	//Most of the initialising happens in the Player's constructor.
	//Here we only set some shadow specific options.
	called=false;
	shadow=true;
}

void Shadow::moveLogic(){
	//If we're called and there are still moves left we to that move.
	if(called && state < (signed)playerButton.size()){
		int currentKey=playerButton[state];

		xVel=0;
		//Check if the current move is walking.
		if(currentKey & PlayerButtonRight) xVel=7;
		if(currentKey & PlayerButtonLeft) xVel=-7;

		//Check if the current move is jumping.
		if((currentKey & PlayerButtonJump) && !inAir){
			isJump=true;
		}else{
			isJump=false;
		}

		//Check if the current move is an action (DOWN arrow key).
		if(currentKey & PlayerButtonDown){
			downKeyPressed=true;
		}else{
			downKeyPressed=false;
		}

		//We've done the move so move on to the next one.
		state++;
	}else{
		//We ran out of moves so reset it.
		//FIXME: Every frame when called is false this will be done?
		called=false;
		state=0;
		xVel=0;
	}
}

void Shadow::meCall(){
	called=true;
}

void Shadow::stateReset(){
	state=0;
	called=false;
}

void Shadow::saveState(){
	Player::saveState();
	calledSaved=called;
}

void Shadow::loadState(){
	Player::loadState();
	called=calledSaved;
}
