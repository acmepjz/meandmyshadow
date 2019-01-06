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

#ifndef PLAYER_H
#define PLAYER_H

#include "ThemeManager.h"
#include "Block.h"
#include <vector>
#include <string>
#include <SDL.h>

//Debug the game record file.
//#define RECORD_FILE_DEBUG

class Game;
class PlayerScriptAPI;

//The different player buttons.
//The right arrow.
const int PlayerButtonRight=0x01;
//The left arrow.
const int PlayerButtonLeft=0x02;
//The up arrow for jumping.
const int PlayerButtonJump=0x04;
//The down arrow for actions.
const int PlayerButtonDown=0x08;
//space bar for recording. (Only in recordButton)
const int PlayerButtonSpace=0x10;

class Player{
	friend class PlayerScriptAPI;
protected:
	//Vector used to store the player actions in when recording.
	//These can be given to the shadow so he can execute them.
	std::vector<int> playerButton;
	//Vector used to store the playerButton vector when saving the player's state (checkpoint).
	std::vector<int> playerButtonSaved;

private:
	//Vector used to record the whole game play.
	//And saved record in checkpoint.
	std::vector<int> recordButton,savedRecordButton;

	//record index. -1 means read input from keyboard,
	//otherwise read input from recordings (recordButton[recordIndex]).
	int recordIndex;

	//Vector containing squares along the path the player takes when recording.
	//It will be drawn as a trail of squares.
	std::vector<SDL_Rect> line;
	//Vector that will hold the line vector when saving the player's state (checkpoint).
	std::vector<SDL_Rect> lineSaved;

	//Boolean if the player called the shadow to copy his moves.
	bool shadowCall;
	//Boolean if the player is recording his moves.
	bool record,recordSaved;
	
	//The following variables are to store a state.
	//Rectangle containing the players location.
	SDL_Rect boxSaved;
	//Boolean if the player is in the air.
	bool inAirSaved;
	//Boolean if the player is (going to) jump(ing).
	bool isJumpSaved;
	//Boolean if the player can move.
	bool canMoveSaved;
	//Boolean if the player is holding the other (shadow).
	bool holdingOtherSaved;
	//The x velocity.
	//NOTE: The x velocity is used to indicate that there's a state saved.
	int xVelSaved;
	//The y velocity.
	int yVelSaved;
	//The state.
	int stateSaved;
	
protected:
	//Rectangle containing the player's location.
	SDL_Rect box;
	
	//The x and y velocity.
	int xVel, yVel;
	//The base x and y velocity, used for standing on moving blocks.
	int xVelBase, yVelBase;
	
	//Boolean if the player is in the air.
	bool inAir;
	//Boolean if the player is (going to) jump(ing).
	bool isJump;
	//Boolean if the player can move.
	bool canMove;
	//Boolean if the player is dead.
	bool dead;
	
	//The direction the player is walking, 0=right, 1=left.
	int direction;
	//Integer containing the state of the player.
	int state;
	//The time the player is in the air (jumping).
	int jumpTime;
	//Boolean if the player is in fact the shadow.
	bool shadow;

	//Pointer to the Game state.
	friend class Game;
	Game* objParent;

	//Boolean if the downkey is pressed.
	bool downKeyPressed;
	//Boolean if the space keu is pressed.
	bool spaceKeyPressed;
	//Pointer to the object that is currently been stand on by the player.
	//This is always a valid pointer.
	Block::ObservePointer objCurrentStand;
	//Pointer to the object the player stood last on.
	//NOTE: This is a weak reference only.
	Block::ObservePointer objLastStand;
	//Pointer to the teleporter the player last took.
	//NOTE: This is a weak reference only.
	Block::ObservePointer objLastTeleport;
	//Pointer to the notification block the player is in front of.
	//This is always a valid pointer.
	Block::ObservePointer objNotificationBlock;
	//Pointer to the shadow block the player is in front of.
	//This is always a valid pointer.
	Block::ObservePointer objShadowBlock;

	//The save variable for the GameObject pointers.
	//FIXME: Also save the other game object pointers?
	Block::ObservePointer objCurrentStandSave;
	Block::ObservePointer objLastStandSave;
	Block::ObservePointer objLastTeleportSave;
	Block::ObservePointer objNotificationBlockSave;
	Block::ObservePointer objShadowBlockSave;

public:

	//X and y location where the player starts and gets when reseted.
	int fx, fy;
	//The appearance of the player.
	ThemeBlockInstance appearance, appearanceSave, appearanceInitial;
	//Boolean if the player is holding the other.
	bool holdingOther;

	//Constructor.
	//objParent: Pointer to the Game state.
	Player(Game* objParent);
	//Destructor.
	~Player();

	//Method used to set the position of the player.
	//x: The new x location of the player.
	//y: The new y location of the player.
	void setLocation(int x,int y);

	//Method used to handle (key) input.
	//shadow: Pointer to the shadow used for recording/calling.
	void handleInput(class Shadow* shadow);
	//Method used to do the movement of the player.
	//NOTE: should call collision() for both player/shadow before call move().
	//levelObjects: Array containing the levelObjects, used to check collision.
	//lastX, lastY: the position of player before calling collision().
	void move(std::vector<Block*> &levelObjects, int lastX, int lastY);
	//Method used to check if the player can jump and executes the jump.
	//strength: The strength of the jump.
	void jump(int strength=13);
	
	//This method will render the player to the screen.
    void show(SDL_Renderer &renderer);
	//Method that stores the actions if the player is recording.
	void shadowSetState();
	
	//Method that will reset the state to 0.
	virtual void stateReset();
	
	//This method checks the player against the other to see if they stand on eachother.
	//other: The shadow or the player.
	void otherCheck(class Player* other);
	
	//Method that will ease the camera so that the player is in the center.
	void setMyCamera();
	//This method will reset the player to it's initial position.
	//save: Boolean if the saved state should also be deleted.
	void reset(bool save);
	//Method used to retrieve the current location of the player.
	//Returns: SDL_Rect containing the player's location.
	SDL_Rect getBox();

	//This method will 
	void shadowGiveState(class Shadow* shadow);
	
	//Method that will save the current state.
	//NOTE: The special <name>Saved variables will be used.
	virtual void saveState();
	//Method that will retrieve the last saved state.
	//If there is none it will reset the player.
	virtual void loadState();
	//Method that checks if the player can save the state.
	//Returns: True if the player can save his state.
	virtual bool canSaveState();
	//Method that checks if the player can load a state.
	//Returns: True if the player can load a state.
	virtual bool canLoadState();
	//Method that will swap the state of the player with the other.
	//other: The player or the shadow.
	void swapState(Player* other);
	
	//Check if this player is in fact the shadow.
	//Returns: True if this is the shadow.
	inline bool isShadow(){
		return shadow;
	}

	//Method for returning the objCurrentStand pointer.
	//Returns: Pointer to the gameobject the player is standing on.
	inline Block* getObjCurrentStand(){
		return objCurrentStand.get();
	}
	
	//Let the player die when he falls of or hits spikes.
	//animation: Boolean if the death animation should be played, default is true.
	void die(bool animation=true);

	//Check if currently it's play from record file.
	bool isPlayFromRecord();

	//get the game record object.
	std::vector<int>* getRecord();

#ifdef RECORD_FILE_DEBUG
	std::string& keyPressLog();
	std::vector<SDL_Rect>& playerPosition();
#endif

	//play the record.
	void playRecord();

private:
	//The space key is down. call this function from handleInput and another function.
	void spaceKeyDown(class Shadow* shadow);

public:
	//Method that will handle the actual movement.
	//NOTE: partially internal function. Should call collision() for both player/shadow before call move().
	//levelObjects: Array containing the levelObjects, used to check collision.
	void collision(std::vector<Block*> &levelObjects, Player* other);
	
};

#endif
