/*
 * Copyright (C) 2011-2013 Me and My Shadow
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

#include "Block.h"
#include "Player.h"
#include "Game.h"
#include "Functions.h"
#include "FileManager.h"
#include "Globals.h"
#include "InputManager.h"
#include "SoundManager.h"
#include "StatisticsManager.h"
#include "MD5.h"
#include <stdio.h>
#include <iostream>
#include <SDL.h>
using namespace std;

#ifdef RECORD_FILE_DEBUG
string recordKeyPressLog,recordKeyPressLog_saved;
vector<SDL_Rect> recordPlayerPosition,recordPlayerPosition_saved;
#endif

//static internal array to store time of recent deaths for achievements
static Uint32 recentDeaths[10]={0};
static int loadAndDieTimes=0;

//static internal function to add recent deaths and update achievements
static inline void addRecentDeaths(Uint32 recentLoad){
	//Get current time in ms.
	//We added it by 5 seconds to avoid bug if you choose a level to play
	//and die in 5 seconds after the game has startup.
	Uint32 t=SDL_GetTicks()+5000;

	for(int i=9;i>0;i--){
		recentDeaths[i]=recentDeaths[i-1];
	}
	recentDeaths[0]=t;

	//Update achievements
	if(recentDeaths[4]+5000>t){
		statsMgr.newAchievement("die5in5");
	}
	if(recentDeaths[9]+5000>t){
		statsMgr.newAchievement("die10in5");
	}
	if(recentLoad+1000>t){
		statsMgr.newAchievement("loadAndDie");
	}
}

Player::Player(Game* objParent):xVelBase(0),yVelBase(0),objParent(objParent),recordSaved(false),
inAirSaved(false),isJumpSaved(false),canMoveSaved(false),holdingOtherSaved(false){
	//Set the dimensions of the player.
	//The size of the player is 21x40.
	box.x=0;
	box.y=0;
	box.w=23;
	box.h=40;

	//Set his velocity to zero.
	xVel=0;
	yVel=0;

	//Set the start position.
	fx=0;
	fy=0;

	//Set some default values.
	inAir=true;
	isJump=false;
	shadowCall=false;
	shadow=false;
	canMove=true;
	holdingOther=false;
	dead=false;
	record=false;
	downKeyPressed=false;
	spaceKeyPressed=false;
	
	recordIndex=-1;
#ifdef RECORD_FILE_DEBUG
	recordKeyPressLog.clear();
	recordKeyPressLog_saved.clear();
	recordPlayerPosition.clear();
	recordPlayerPosition_saved.clear();
#endif

	//Some default values for animation variables.
	direction=0;
	jumpTime=0;

	state=stateSaved=0;

	//xVelSaved is used to store if there's a state saved or not.
	xVelSaved=yVelSaved=0x80000000;
	
	objCurrentStand = objLastStand = objLastTeleport = objNotificationBlock = objShadowBlock = NULL;
	objCurrentStandSave = objLastStandSave = objLastTeleportSave = objNotificationBlockSave = objShadowBlockSave = NULL;
}

Player::~Player(){
	//Do nothing here
}

bool Player::isPlayFromRecord(){
	return recordIndex>=0; // && recordIndex<(int)recordButton.size();
}

//get the game record object.
std::vector<int>* Player::getRecord(){
	return &recordButton;
}

#ifdef RECORD_FILE_DEBUG
string& Player::keyPressLog(){
	return recordKeyPressLog;
}
vector<SDL_Rect>& Player::playerPosition(){
	return recordPlayerPosition;
}
#endif

//play the record.
void Player::playRecord(){
	recordIndex=0;
}

void Player::spaceKeyDown(class Shadow* shadow){
	//Start recording or stop, depending on the recording state.
	if(record==false){
		//We start recording.
		if(shadow->called==true){
			//The shadow is still busy so first stop him before we can start recording.
			shadowCall=false;
			shadow->called=false;
			shadow->playerButton.clear();
		}else if(!dead){
			//Check if shadow is dead.
			if(shadow->dead){
				//Show tooltip.
				//Just reset the countdown (the shadow's jumptime).
				shadow->jumpTime=80;

				//Play the error sound.
				getSoundManager()->playSound("error");
			}else{
				//The shadow isn't moving and both player and shadow aren't dead so start recording.
				record=true;

				//We start a recording meaning we need to increase recordings by one.
				objParent->recordings++;

				//Update statistics.
				if(!dead && !objParent->player.isPlayFromRecord() && !objParent->interlevel){
					statsMgr.recordTimes++;

					if(statsMgr.recordTimes==100) statsMgr.newAchievement("record100");
					if(statsMgr.recordTimes==1000) statsMgr.newAchievement("record1k");
				}
			}
		}
	}else{
		//The player is recording so stop recording and call the shadow.
		record=false;
		shadowCall=true;
	}
}

void Player::handleInput(class Shadow* shadow){
	//Check if we should read the input from record file.
	//Actually, we read input from record file in
	//another function shadowSetState.
	bool readFromRecord=false;
	if(recordIndex>=0 && recordIndex<(int)recordButton.size()) readFromRecord=true;

	if(!readFromRecord){
		//Reset horizontal velocity.
		xVel=0;
		if(inputMgr.isKeyDown(INPUTMGR_RIGHT)){
			//Walking to the right.
			xVel+=7;
		}
		if(inputMgr.isKeyDown(INPUTMGR_LEFT)){
			//Walking to the left.
			if(xVel!=0 && !dead && !objParent->player.isPlayFromRecord() && !objParent->interlevel){
				//Horizontal confusion achievement :)
				statsMgr.newAchievement("horizontal");
			}
			xVel-=7;
		}

		//Check if the action key has been released.
		if(!inputMgr.isKeyDown(INPUTMGR_ACTION)){
			//It has so downKeyPressed can't be true.
			downKeyPressed=false;
		}
		/*
		//Don't reset spaceKeyPressed or when you press the space key
		//and release another key then the bug occurs. (ticket #44)
		if(event.type==SDL_KEYUP || !inputMgr.isKeyDown(INPUTMGR_SPACE)){
			spaceKeyPressed=false;
		}*/
	}

	//Check if a key is pressed (down).
	if(inputMgr.isKeyDownEvent(INPUTMGR_JUMP) && !readFromRecord){
		//The up key, if we aren't in the air we start jumping.
		//Fixed a potential bug
		if(!inAir && !isJump){
#ifdef RECORD_FILE_DEBUG
			char c[64];
			sprintf(c,"[%05d] Jump key pressed\n",objParent->time);
			cout<<c;
			recordKeyPressLog+=c;
#endif
			isJump=true;
		}
	}else if(inputMgr.isKeyDownEvent(INPUTMGR_SPACE) && !readFromRecord){
		//Fixed a potential bug
		if(!spaceKeyPressed){
#ifdef RECORD_FILE_DEBUG
			char c[64];
			sprintf(c,"[%05d] Space key pressed\n",objParent->time);
			cout<<c;
			recordKeyPressLog+=c;
#endif
			spaceKeyDown(shadow);
			spaceKeyPressed=true;
		}
	}else if(inputMgr.isKeyUpEvent(INPUTMGR_SPACE) && !readFromRecord){
		if(record && getSettings()->getBoolValue("quickrecord")){
			spaceKeyDown(shadow);
			spaceKeyPressed=true;
		}
	}else if(record && !readFromRecord && inputMgr.isKeyDownEvent(INPUTMGR_CANCELRECORDING)){
		//Cancel current recording

		//Search the recorded button and clear the last space key press
		int i=recordButton.size()-1;
		for(;i>=0;i--){
			if(recordButton[i] & PlayerButtonSpace){
				recordButton[i] &= ~PlayerButtonSpace;
				break;
			}
		}

		if(i>=0){
			//Clear the recording at the player's side.
			playerButton.clear();
			line.clear();

			//reset the record flag
			record=false;

			//decrese the record count
			objParent->recordings--;
		}else{
			cout<<"Failed to find last recording"<<endl;
		}
	}else if(inputMgr.isKeyDownEvent(INPUTMGR_ACTION)){
		//Downkey is pressed.
		//Fixed a potential bug
		if(!downKeyPressed){
#ifdef RECORD_FILE_DEBUG
			char c[64];
			sprintf(c,"[%05d] Action key pressed\n",objParent->time);
			cout<<c;
			recordKeyPressLog+=c;
#endif
			downKeyPressed=true;
		}
	}else if(inputMgr.isKeyDownEvent(INPUTMGR_SAVE)){
		//F2 only works in the level editor.
		if(!(dead || shadow->dead) && stateID==STATE_LEVEL_EDITOR){
			//Save the state. (delayed)
			if (objParent && !objParent->player.isPlayFromRecord() && !objParent->interlevel)
				objParent->saveStateNextTime=true;
		}
	}else if(inputMgr.isKeyDownEvent(INPUTMGR_LOAD) && (!readFromRecord || objParent->interlevel)){
		//F3 is used to load the last state.
		if (objParent && canLoadState()) {
			recordIndex = -1;
			objParent->loadStateNextTime = true;

			//Also delete any gui (most likely the interlevel gui). Only in game mode.
			if (GUIObjectRoot && stateID != STATE_LEVEL_EDITOR){
				delete GUIObjectRoot;
				GUIObjectRoot = NULL;
			}

			//And set interlevel to false.
			objParent->interlevel = false;
		}
	}else if(inputMgr.isKeyDownEvent(INPUTMGR_SWAP)){
		//F4 will swap the player and the shadow, but only in the level editor.
		if(!(dead || shadow->dead) && stateID==STATE_LEVEL_EDITOR){
			swapState(shadow);
		}
	}else if(inputMgr.isKeyDownEvent(INPUTMGR_TELEPORT)){
		//F5 will revive and teleoprt the player to the cursor. Only works in the level editor.
		//Shift+F5 teleports the shadow.
		if(stateID==STATE_LEVEL_EDITOR){
			//get the position of the cursor.
			int x,y;
			SDL_GetMouseState(&x,&y);
			x+=camera.x;
			y+=camera.y;

			if(inputMgr.isKeyDown(INPUTMGR_SHIFT)){
				//teleports the shadow.
				shadow->dead=false;
				shadow->box.x=x;
				shadow->box.y=y;
			}else{
				//teleports the player.
				dead=false;
				box.x=x;
				box.y=y;
			}

			//play sound?
			getSoundManager()->playSound("swap");
		}
	}else if(inputMgr.isKeyDownEvent(INPUTMGR_SUICIDE)){
		//F12 is suicide and only works in the leveleditor.
		if(stateID==STATE_LEVEL_EDITOR){
			die();
			shadow->die();
		}
	}
}

void Player::setLocation(int x,int y){
	box.x=x;
	box.y=y;
}

void Player::move(vector<Block*> &levelObjects,int lastX,int lastY){
	//Only move when the player isn't dead.
	//Fixed the bug that player/shadow can teleport or pull the switch even if died.
	//FIXME: Don't know if there will be any side-effects.
	if(dead) return;

	//Pointer to a checkpoint.
	Block* objCheckPoint=NULL;
	//Pointer to a swap.
	Block* objSwap=NULL;

	//Set the objShadowBlock to NULL.
	//Only for swapping to prevent the shadow from swapping in a shadow block.
	objShadowBlock=NULL;

	//Set the objNotificationBlock to NULL.
	objNotificationBlock=NULL;

	//NOTE: to fix bugs regarding player/shadow swap, we should first process collision of player/shadow
	//then move them. The code is moved to Game::logic().

	/*//Store the location.
	int lastX=box.x;
	int lastY=box.y;

	collision(levelObjects);*/

	bool canTeleport=true;
	bool isTraveling=true;

	// for checking the achievenemt that player and shadow come to exit simultaneously.
	bool weWon = false;

	//Now check the functional blocks.
	for(unsigned int o=0;o<levelObjects.size();o++){
		//Skip block which is not visible.
		if (levelObjects[o]->queryProperties(GameObjectProperty_Flags, this) & 0x80000000) continue;

		//Check for collision.
		if(checkCollision(box,levelObjects[o]->getBox())){
			//Now switch the type.
			switch(levelObjects[o]->type){
				case TYPE_CHECKPOINT:
				{
					//If we're not the shadow set the gameTip to Checkpoint.
					if(!shadow && objParent!=NULL)
						objParent->gameTipIndex=TYPE_CHECKPOINT;

					//And let objCheckPoint point to this object.
					objCheckPoint=levelObjects[o];
					break;
				}
				case TYPE_SWAP:
				{
					//If we're not the shadow set the gameTip to swap.
					if(!shadow && objParent!=NULL)
						objParent->gameTipIndex=TYPE_SWAP;

					//And let objSwap point to this object.
					objSwap=levelObjects[o];
					break;
				}
				case TYPE_EXIT:
				{
					//Make sure we're not in the leveleditor.
					if(stateID==STATE_LEVEL_EDITOR)
						break;

					//Check to see if we have enough keys to finish the level
					if(objParent->currentCollectables>=objParent->totalCollectables){
						//Update achievements
						if(!objParent->player.isPlayFromRecord() && !objParent->interlevel){
							if(objParent->player.dead || objParent->shadow.dead){
								//Finish the level with player or shadow died.
								statsMgr.newAchievement("forget");
							}
							if(objParent->won && !weWon){ // This checks if somebody already hit the exit but we haven't hit the exit yet.
								//Player and shadow come to exit simultaneously.
								statsMgr.newAchievement("jit");
							}
						}

						//We can't just handle the winning here (in the middle of the update cycle)/
						//So set won in Game true.
						objParent->won=true;

						//We hit the exit.
						weWon = true;
					}
					break;
				}
				case TYPE_PORTAL:
				{
					//Check if the teleport id isn't empty.
					if(levelObjects[o]->id.empty()){
                        std::cerr<<"WARNING: Invalid teleport id!"<<std::endl;
						canTeleport=false;
					}

					//If we're not the shadow set the gameTip to portal.
					if(!shadow && objParent!=NULL)
						objParent->gameTipIndex=TYPE_PORTAL;

					//Check if we can teleport and should (downkey -or- auto).
					if(canTeleport && (downKeyPressed || (levelObjects[o]->queryProperties(GameObjectProperty_Flags,this)&1))){
						canTeleport=false;
						if(downKeyPressed || levelObjects[o]!=objLastTeleport.get()){
							//Loop the levelobjects again to find the destination.
							for(unsigned int oo=o+1;;){
								//We started at our index+1.
								//Meaning that if we reach the end of the vector then we need to start at the beginning.
								if(oo>=levelObjects.size())
									oo-=(int)levelObjects.size();
								//It also means that if we reach the same index we need to stop.
								//If the for loop breaks this way then we have no succes.
								if(oo==o){
									//Couldn't teleport. We play the error sound only when the down key pressed.
									if (downKeyPressed) {
										getSoundManager()->playSound("error");
									}
									break;
								}

								//Check if the second (oo) object is a portal and is visible.
								if (levelObjects[oo]->type == TYPE_PORTAL && (levelObjects[oo]->queryProperties(GameObjectProperty_Flags, this) & 0x80000000) == 0){
									//Check the id against the destination of the first portal.
									if(levelObjects[o]->destination==levelObjects[oo]->id){
										//Get the destination location.
										SDL_Rect r = levelObjects[oo]->getBox();
										r.x += 5;
										r.y += 2;
										r.w = box.w;
										r.h = box.h;

										//Check if the destination location is blocked.
										bool blocked = false;
										for (auto ooo : levelObjects){
											//Make sure to only check visible blocks.
											if (ooo->queryProperties(GameObjectProperty_Flags, this) & 0x80000000)
												continue;
											//Make sure the object is solid for the player.
											if (!ooo->queryProperties(GameObjectProperty_PlayerCanWalkOn, this))
												continue;

											//Check for collision.
											if (checkCollision(r, ooo->getBox())) {
												blocked = true;
												break;
											}
										}

										//Teleport only if the destination is not blocked.
										if (!blocked) {
											//Call the event.
											objParent->broadcastObjectEvent(GameObjectEvent_OnToggle, -1, NULL, levelObjects[o]);
											objLastTeleport = levelObjects[oo];

											//Teleport the player.
											box.x = r.x;
											box.y = r.y;

											//We don't count it to traveling distance.
											isTraveling = false;

											//Play the swap sound.
											getSoundManager()->playSound("swap");
											break;
										}
									}
								}

								//Increase oo.
								oo++;
							}

							//Reset the down key pressed.
							downKeyPressed = false;
						}
					}
					break;
				}
				case TYPE_SWITCH:
				{
					//If we're not the shadow set the gameTip to switch.
					if(!shadow && objParent!=NULL)
						objParent->gameTipIndex=TYPE_SWITCH;

					//If the down key is pressed then invoke an event.
					if(downKeyPressed){
						//Play the animation.
						levelObjects[o]->playAnimation();
						
						//Play the toggle sound.
						getSoundManager()->playSound("toggle");
						
						//Update statistics.
						if(!dead && !objParent->player.isPlayFromRecord() && !objParent->interlevel){
							statsMgr.switchTimes++;

							//Update achievements
							switch(statsMgr.switchTimes){
							case 100:
								statsMgr.newAchievement("switch100");
								break;
							case 1000:
								statsMgr.newAchievement("switch1k");
								break;
							}
						}
						
						levelObjects[o]->onEvent(GameObjectEvent_OnPlayerInteraction);
					}
					break;
				}
				case TYPE_SHADOW_BLOCK:
				case TYPE_MOVING_SHADOW_BLOCK:
				{
					//This only applies to the player.
					if(!shadow)
						objShadowBlock=levelObjects[o];
					break;
				}
				case TYPE_NOTIFICATION_BLOCK:
				{
					//This only applies to the player.
					if(!shadow)
						objNotificationBlock=levelObjects[o];
					break;
				}
				case TYPE_COLLECTABLE:
				{
					//Check if collectable is active (if it's not it's equal to 1(inactive))
					if((levelObjects[o]->queryProperties(GameObjectProperty_Flags, this) & 0x1) == 0) {
						//Toggle an event
						objParent->broadcastObjectEvent(GameObjectEvent_OnToggle,-1,NULL,levelObjects[o]);
						//Increase the current number of collectables
						objParent->currentCollectables++;
						getSoundManager()->playSound("collect");
						//Open exit(s)
						if(objParent->currentCollectables>=objParent->totalCollectables){
							for(unsigned int i=0;i<levelObjects.size();i++){
								if(levelObjects[i]->type==TYPE_EXIT){
									objParent->broadcastObjectEvent(GameObjectEvent_OnSwitchOn,-1,NULL,levelObjects[i]);
								}
							}
						}
					}
					break;
				}
			}

			//Now check for the spike property.
			if(levelObjects[o]->queryProperties(GameObjectProperty_IsSpikes,this)){
				//It is so get the collision box.
				SDL_Rect r=levelObjects[o]->getBox();

				//TODO: pixel-accuracy hit test.
				//For now we shrink the box.
				r.x+=2;
				r.y+=2;
				r.w-=4;
				r.h-=4;
				
				//Check collision, if the player collides then let him die.
				if(checkCollision(box,r)){
						die();
				}
			}
		}
	}

	//Check if the player can teleport.
	if(canTeleport)
		objLastTeleport=NULL;

	//Check the checkpoint pointer only if the downkey is pressed.
	//new: don't save the game if playing game record
	if (objParent != NULL && downKeyPressed && objCheckPoint != NULL && !objParent->player.isPlayFromRecord() && !objParent->interlevel){
		//Checkpoint thus save the state.
		if(objParent->canSaveState()){
			objParent->saveStateNextTime=true;
			objParent->objLastCheckPoint=objCheckPoint;
		}
	}
	//Check the swap pointer only if the down key is pressed.
	if(objSwap!=NULL && downKeyPressed && objParent!=NULL){
		//Now check if the shadow we're the shadow or not.
		if(shadow){
			if(!(dead || objParent->player.dead)){
				//Check if the player isn't in front of a shadow block.
				if(!objParent->player.objShadowBlock.get()){
					objParent->player.swapState(this);
					objSwap->playAnimation();
					//We don't count it to traveling distance.
					isTraveling=false;
					//NOTE: Statistics updated in swapState() function.
				}else{
					//We can't swap so play the error sound.
					getSoundManager()->playSound("error");
				}
			}
		}else{
			if(!(dead || objParent->shadow.dead)){
				//Check if the player isn't in front of a shadow block.
				if(!objShadowBlock.get()){
					swapState(&objParent->shadow);
					objSwap->playAnimation();
					//We don't count it to traveling distance.
					isTraveling=false;
					//NOTE: Statistics updated in swapState() function.
				}else{
					//We can't swap so play the error sound.
					getSoundManager()->playSound("error");
				}
			}
		}
	}

	//Determine the correct theme state.
	if(!dead){
		//Set the direction depending on the velocity.
		if(xVel>0)
			direction=0;
		else if(xVel<0)
			direction=1;

		//Check if the player is in the air.
		if(!inAir){
			//On the ground so check the direction and movement.
			if(xVel>0){
				appearance.changeState("walkright",true,true);
			}else if(xVel<0){
				appearance.changeState("walkleft",true,true);
			}else if(xVel==0){
				if(direction==1){
					appearance.changeState("standleft",true,true);
				}else{
					appearance.changeState("standright",true,true);
				}
			}
		}else{
			//Check for jump appearance (inAir).
			if(direction==1){
				if(yVel>0){
					appearance.changeState("fallleft",true,true);
				}else{
					appearance.changeState("jumpleft",true,true);
				}
			}else{
				if(yVel>0){
					appearance.changeState("fallright",true,true);
				}else{
					appearance.changeState("jumpright",true,true);
				}
			}
		}
	}


	//Update traveling distance statistics.
	if(isTraveling && (lastX!=box.x || lastY!=box.y) && !objParent->player.isPlayFromRecord() && !objParent->interlevel){
		float dx=float(lastX-box.x),dy=float(lastY-box.y);
		float d0=statsMgr.playerTravelingDistance+statsMgr.shadowTravelingDistance,
			d=sqrtf(dx*dx+dy*dy)/50.0f;
		if(shadow) statsMgr.shadowTravelingDistance+=d;
		else statsMgr.playerTravelingDistance+=d;

		//Update achievement
		d+=d0;
		if(d0<=100.0f && d>=100.0f) statsMgr.newAchievement("travel100");
		if(d0<=1000.0f && d>=1000.0f) statsMgr.newAchievement("travel1k");
		if(d0<=10000.0f && d>=10000.0f) statsMgr.newAchievement("travel10k");
		if(d0<=42195.0f && d>=42195.0f) statsMgr.newAchievement("travel42k");
	}

	//Reset the downKeyPressed flag.
	downKeyPressed=false;
}

void Player::collision(vector<Block*> &levelObjects, Player* other){
	//Only move when the player isn't dead.
	if(dead)
		return;

	//First sort out the velocity.

	//NOTE: This is the temporary xVel which takes canMove into consideration.
	//This shadows Player::xVel.
	const int xVel = canMove ? this->xVel : 0;

	//Add gravity acceleration to the vertical velocity.
	if(isJump)
		jump();
	if(inAir==true){
		yVel+=1;

		//Cap fall speed to 13.
		if(yVel>13)
			yVel=13;
	}

	Block* baseBlock=NULL;
	if(auto tmp = objCurrentStand.get()) {
		baseBlock=tmp;
	} else if(other && other->holdingOther) {
		//NOTE: this actually CAN happen, e.g. when player is holding shadow and the player is going to jump
		//assert(other->objCurrentStand != NULL);
		baseBlock=other->objCurrentStand.get();
	}
	if(baseBlock!=NULL){
		//Now get the velocity and delta of the object the player is standing on.
		SDL_Rect v=baseBlock->getBox(BoxType_Velocity);
		SDL_Rect delta=baseBlock->getBox(BoxType_Delta);

		switch(baseBlock->type){
		//For conveyor belts the velocity is transfered.
		case TYPE_CONVEYOR_BELT:
		case TYPE_SHADOW_CONVEYOR_BELT:
			xVelBase=v.x;
			break;
		//In other cases, such as player on shadow, player on crate. the change in x position must be considered.
		default:
			{
				if(delta.x != 0)
					xVelBase+=delta.x;
			}
			break;
		}
		//NOTE: Only copy the velocity of the block when moving down.
		//Upwards is automatically resolved before the player is moved.
		if(delta.y>0){
			//Fixes the jitters when the player is on a pushable block on a downward moving box.
			//NEW FIX: the squash bug. The following line of code is commented and change 'v' to 'delta'.
			//box.y+=delta.y;
			yVelBase=delta.y;
		}
		else
			yVelBase=0;
	}

	//Set the object the player is currently standing to NULL.
	objCurrentStand=NULL;

	//Store the location of the player.
	int lastX=box.x;
	int lastY=box.y;

	//An array that will hold all the GameObjects that are involved in the collision/movement.
	vector<Block*> objects;
	//All the blocks have moved so if there's collision with the player, the block moved into him.
	for(unsigned int o=0;o<levelObjects.size();o++){
		//Make sure to only check visible blocks.
		if (levelObjects[o]->queryProperties(GameObjectProperty_Flags, this) & 0x80000000)
			continue;
		//Make sure the object is solid for the player.
		if(!levelObjects[o]->queryProperties(GameObjectProperty_PlayerCanWalkOn,this))
			continue;

		//Check for collision.
		if(checkCollision(box,levelObjects[o]->getBox()))
			objects.push_back(levelObjects[o]);
	}
	//There was collision so try to resolve it.
	if(!objects.empty()){
		//FIXME: When multiple moving blocks are overlapping the player can be "bounced" off depending on the block order.
		for(unsigned int o=0;o<objects.size();o++){
			SDL_Rect r=objects[o]->getBox();
			SDL_Rect delta=objects[o]->getBox(BoxType_Delta);

			//Check on which side of the box the player is.
			if(delta.x!=0){
				if(delta.x>0){
					//Move the player right if necessary.
					if((r.x+r.w)-box.x<=delta.x && box.x<r.x+r.w)
						box.x=r.x+r.w;
				}else{
					//Move the player left if necessary.
					if((box.x+box.w)-r.x<=-delta.x && box.x>r.x-box.w)
						box.x=r.x-box.w;
				}
			}
			if(delta.y!=0){
				if(delta.y>0){
					//Move the player down if necessary.
					if((r.y+r.h)-box.y<=delta.y && box.y<r.y+r.h)
						box.y=r.y+r.h;
				}else{
					//Move the player up if necessary.
					if((box.y+box.h)-r.y<=-delta.y && box.y>r.y-box.h)
						box.y=r.y-box.h;
				}
			}
		}

		//Check if the player is squashed.
		for(unsigned int o=0;o<levelObjects.size();o++){
			//Make sure the object is visible.
			if (levelObjects[o]->queryProperties(GameObjectProperty_Flags, this) & 0x80000000)
				continue;
			//Make sure the object is solid for the player.
			if(!levelObjects[o]->queryProperties(GameObjectProperty_PlayerCanWalkOn,this))
				continue;
				
			if(checkCollision(box,levelObjects[o]->getBox())){
				//The player is squashed so first move him back.
				box.x=lastX;
				box.y=lastY;
				
				//Update statistics.
				if(!dead && !objParent->player.isPlayFromRecord() && !objParent->interlevel){
					if(shadow) statsMgr.shadowSquashed++;
					else statsMgr.playerSquashed++;
				
					switch(statsMgr.playerSquashed+statsMgr.shadowSquashed){
						case 1:
							statsMgr.newAchievement("squash1");
							break;
						case 50:
							statsMgr.newAchievement("squash50");
							break;
						}
				}
				
				//Now call the die method.
				die();
				return;
			}
		}
	}
	
	//Reuse the objects array, this time for blocks the player walks into.
	objects.clear();
	//Determine the collision frame.
	SDL_Rect frame={box.x,box.y,box.w,box.h};
	//Keep the horizontal movement of the player in mind.
	if(xVel+xVelBase>=0){
		frame.w+=(xVel+xVelBase);
	}else{
		frame.x+=(xVel+xVelBase);
		frame.w-=(xVel+xVelBase);
	}
	//And the vertical movement.
	if(yVel+yVelBase>=0){
		frame.h+=(yVel+yVelBase);
	}else{
		frame.y+=(yVel+yVelBase);
		frame.h-=(yVel+yVelBase);
	}
	//Loop through the game objects.
	for(unsigned int o=0; o<levelObjects.size(); o++){
		//Make sure the block is visible.
		if (levelObjects[o]->queryProperties(GameObjectProperty_Flags, this) & 0x80000000)
			continue;
		//Check if the player can collide with this game object.
		if(!levelObjects[o]->queryProperties(GameObjectProperty_PlayerCanWalkOn,this))
			continue;

		//Check if the block is inside the frame.
		if(checkCollision(frame,levelObjects[o]->getBox()))
			objects.push_back(levelObjects[o]);
	}
	//Horizontal pass.
	if(xVel+xVelBase!=0){
		box.x+=xVel+xVelBase;
		for(unsigned int o=0;o<objects.size();o++){
			SDL_Rect r=objects[o]->getBox();
			if(!checkCollision(box,r))
				continue;

			//In case of a pushable block we give it velocity.
			if(objects[o]->type==TYPE_PUSHABLE){
				objects[o]->xVel+=(xVel+xVelBase)/2;
			}

			if(xVel+xVelBase>0){
				//We came from the left so the right edge of the player must be less or equal than xVel+xVelBase.
				if((box.x+box.w)-r.x<=xVel+xVelBase)
					box.x=r.x-box.w;
			}else{
				//We came from the right so the left edge of the player must be greater or equal than xVel+xVelBase.
				if(box.x-(r.x+r.w)>=xVel+xVelBase)
					box.x=r.x+r.w;
			}
		}
	}
	//Some variables that are used in vertical movement.
	Block* lastStand=NULL;
	inAir=true;

	//Vertical pass.
	if(yVel+yVelBase!=0){
		box.y+=yVel+yVelBase;

		//Value containing the previous 'depth' of the collision.
		int prevDepth=0;
		
		for(unsigned int o=0;o<objects.size();o++){
			SDL_Rect r=objects[o]->getBox();
			if(!checkCollision(box,r))
				continue;
		
			//Now check how we entered the block (vertically or horizontally).
			if(yVel+yVelBase>0){
				//Calculate the number of pixels the player is in the block (vertically).
				int depth=(box.y+box.h)-r.y;
				
				//We came from the top so the bottom edge of the player must be less or equal than yVel+yVelBase.
				if(depth<=yVel+yVelBase){
					//NOTE: lastStand is handled later since the player can stand on only one block at the time.

					//Check if there's already a lastStand.
					if(lastStand){
						//Since the player fell he will stand on the highest block, meaning the highest 'depth'.
						if(depth>prevDepth){
							lastStand=objects[o];
							prevDepth=depth;
						}else if(depth==prevDepth){
							//Both blocks are at the same height so determine the block by the amount the player is standing on them.
							SDL_Rect r=objects[o]->getBox();
							int w=0;
							if(box.x+box.w>r.x+r.w)
								w=(r.x+r.w)-box.x;
							else
								w=(box.x+box.w)-r.x;
							
							//Do the same for the other box.
							r=lastStand->getBox();
							int w2=0;
							if(box.x+box.w>r.x+r.w)
								w2=(r.x+r.w)-box.x;
							else
								w2=(box.x+box.w)-r.x;
						
							//NOTE: It doesn't matter which block the player is on if they are both stationary.
							SDL_Rect v=objects[o]->getBox(BoxType_Velocity);
							SDL_Rect v2=lastStand->getBox(BoxType_Velocity);

							//Either the have the same (vertical) velocity so most pixel standing on is the lastStand...
							// ... OR one is moving slower down/faster up and that's the one the player is standing on.
							if((v.y==v2.y && w>w2) || v.y<v2.y){
								lastStand=objects[o];
								prevDepth=depth;
							}
						}
					}else{
						//There isn't one so assume the current block for now.
						lastStand=objects[o];
						prevDepth=depth;
					}
				}
			}else{
				//We came from the bottom so the upper edge of the player must be greater or equal than yVel+yVelBase.
				if(box.y-(r.y+r.h)>=yVel+yVelBase){
					box.y=r.y+r.h;
					yVel=0;
				}
			}
		}
	}
	if(lastStand){
		inAir=false;
		yVel=1;
		SDL_Rect r=lastStand->getBox();
		box.y=r.y-box.h;
	}

	//Check if the player fell of the level, if so let him die but without animation.
	if(box.y>objParent->levelRect.y+objParent->levelRect.h)
		die(false);

	//Check if the player changed blocks, meaning stepped onto a block.
	objCurrentStand=lastStand;
	auto ols = objLastStand.get();
	if(lastStand!=ols){
		//The player has changed block so call the playerleave event.
		if(ols)
			objParent->broadcastObjectEvent(GameObjectEvent_PlayerLeave,-1,NULL,ols);

		//Set the new lastStand.
		objLastStand=lastStand;
		if(lastStand){
			//NOTE: We partially revert this piece of code to that in commit 0072762,
			//i.e. change the event GameObjectEvent_PlayerWalkOn from asynchronous back to synchronous,
			//to fix the fragile block hit test bug when it is breaking.
			//Hopefully it will not introduce bugs (e.g. bugs regarding dynamic add/delete of objects).

			if (lastStand->type == TYPE_FRAGILE) {
				//Call the walk on event of the laststand in a synchronous way.
				lastStand->onEvent(GameObjectEvent_PlayerWalkOn);

				//Bugfix for Fragile blocks.
				if (!lastStand->queryProperties(GameObjectProperty_PlayerCanWalkOn, this)) {
					inAir = true;
					isJump = false;
				}
			} else {
				//Call the walk on event of the laststand in an asynchronous way.
				objParent->broadcastObjectEvent(GameObjectEvent_PlayerWalkOn, -1, NULL, lastStand);
			}
		}
	}
	//NOTE: The PlayerIsOn event must be handled here so that the script can change the location of a block without interfering with the collision detection.
	//Handlingin it here also guarantees that this event will only be called once for one block per update.
	if(lastStand)
		objParent->broadcastObjectEvent(GameObjectEvent_PlayerIsOn,-1,NULL,lastStand);

	//Reset the base velocity.
	xVelBase=yVelBase=0;
	canMove=true;
} 

void Player::jump(int strength){
	//Check if the player can jump.
	if(inAir==false){
		//Set the jump velocity.
		yVel=-strength;
		inAir=true;
		isJump=false;
		jumpTime++;

		//Update statistics
		if(!objParent->player.isPlayFromRecord() && !objParent->interlevel){
			if(shadow) statsMgr.shadowJumps++;
			else statsMgr.playerJumps++;

			int tmp = statsMgr.playerJumps + statsMgr.shadowJumps;

			switch (tmp) {
			case 100:
				statsMgr.newAchievement("jump100");
				break;
			case 1000:
				statsMgr.newAchievement("jump1k");
				break;
			}
		}

		//Check if sound is enabled, if so play the jump sound.
		getSoundManager()->playSound("jump");
	}
}

void Player::show(SDL_Renderer& renderer){
	//Check if we should render the recorded line.
	//Only do this when we're recording and we're not the shadow.
	if(shadow==false && record==true){
		//FIXME: Adding an entry not in update but in render?
		line.push_back(SDL_Rect());
		line[line.size()-1].x=box.x+11;
		line[line.size()-1].y=box.y+20;

		//Loop through the line dots and draw them.
		for(int l=0; l<(signed)line.size(); l++){
            appearance.drawState("line",renderer,line[l].x-camera.x,line[l].y-camera.y);
		}
	}

	//NOTE: We do logic here, because it's only needed by the appearance.
	appearance.updateAnimation();
    appearance.draw(renderer, box.x-camera.x, box.y-camera.y);
}

void Player::shadowSetState(){
	int currentKey=0;

	/*//debug
	extern int block_test_count;
	extern bool block_test_only;
	if(SDL_GetKeyState(NULL)[SDLK_p]){
		block_test_count=recordButton.size();
	}
	if(block_test_count==(int)recordButton.size()){
		block_test_only=true;
	}*/

	//Check if we should read the input from record file.
	if(recordIndex>=0){ // && recordIndex<(int)recordButton.size()){
		//read the input from record file
		if(recordIndex<(int)recordButton.size()){
			currentKey=recordButton[recordIndex];
			recordIndex++;
		}

		//Reset horizontal velocity.
		xVel=0;
		if(currentKey&PlayerButtonRight){
			//Walking to the right.
			xVel+=7;
		}
		if(currentKey&PlayerButtonLeft){
			//Walking to the left.
			xVel-=7;
		}

		if(currentKey&PlayerButtonJump){
			//The up key, if we aren't in the air we start jumping.
			if(inAir==false){
				isJump=true;
			}else{
				//Shouldn't go here
				cout<<"Replay BUG"<<endl;
			}
		}

		//check the down key
		downKeyPressed=(currentKey&PlayerButtonDown)!=0;

		//check the space key
		if(currentKey&PlayerButtonSpace){
			spaceKeyDown(&objParent->shadow);
		}
	}else{
		//read the input from keyboard.
		recordIndex=-1;

		//Check for xvelocity.
		if(xVel>0)
			currentKey|=PlayerButtonRight;
		if(xVel<0)
			currentKey|=PlayerButtonLeft;

		//Check for jumping.
		if(isJump){
#ifdef RECORD_FILE_DEBUG
			char c[64];
			sprintf(c,"[%05d] Jump key recorded\n",objParent->time-1);
			cout<<c;
			recordKeyPressLog+=c;
#endif
			currentKey|=PlayerButtonJump;
		}

		//Check if the downbutton is pressed.
		if(downKeyPressed){
#ifdef RECORD_FILE_DEBUG
			char c[64];
			sprintf(c,"[%05d] Action key recorded\n",objParent->time-1);
			cout<<c;
			recordKeyPressLog+=c;
#endif
			currentKey|=PlayerButtonDown;
		}

		if(spaceKeyPressed){
#ifdef RECORD_FILE_DEBUG
			char c[64];
			sprintf(c,"[%05d] Space key recorded\n",objParent->time-1);
			cout<<c;
			recordKeyPressLog+=c;
#endif
			currentKey|=PlayerButtonSpace;
		}

		//Record it.
		recordButton.push_back(currentKey);
	}

#ifdef RECORD_FILE_DEBUG
	if(recordIndex>=0){
		if(recordIndex>0 && recordIndex<=int(recordPlayerPosition.size())/2){
			SDL_Rect &r1=recordPlayerPosition[recordIndex*2-2];
			SDL_Rect &r2=recordPlayerPosition[recordIndex*2-1];
			if(r1.x!=box.x || r1.y!=box.y || r2.x!=objParent->shadow.box.x || r2.y!=objParent->shadow.box.y){
				char c[192];
				sprintf(c,"Replay ERROR [%05d] %d %d %d %d Expected: %d %d %d %d\n",
					objParent->time-1,box.x,box.y,objParent->shadow.box.x,objParent->shadow.box.y,r1.x,r1.y,r2.x,r2.y);
				cout<<c;
			}
		}
	}else{
		recordPlayerPosition.push_back(box);
		recordPlayerPosition.push_back(objParent->shadow.box);
	}
#endif

	//reset spaceKeyPressed.
	spaceKeyPressed=false;

	//Only add an entry if the player is recording.
	if(record){
		//Add the action.
		if(!dead && !objParent->shadow.dead){
			playerButton.push_back(currentKey);

			//Change the state.
			state++;
		}else{
			//Either player or shadow is dead, stop recording.
			playerButton.clear();
			state=0;
			record=false;
		}
	}
}

void Player::shadowGiveState(Shadow* shadow){
	//Check if the player calls the shadow.
	if(shadowCall==true){
		//Clear any recording still with the shadow.
		shadow->playerButton.clear();

		//Loop the recorded moves and add them to the one of the shadow.
		for(unsigned int s=0;s<playerButton.size();s++){
			shadow->playerButton.push_back(playerButton[s]);
		}

		//Reset the state of both the player and the shadow.
		stateReset();
		shadow->stateReset();

		//Clear the recording at the player's side.
		playerButton.clear();
		line.clear();

		//Set shadowCall false
		shadowCall=false;
		//And let the shadow know that the player called him.
		shadow->meCall();
	}
}

void Player::stateReset(){
	//Reset the state by setting it to 0.
	state=0;
}

void Player::otherCheck(class Player* other){
	//Now check if the player is on the shadow.
	//First make sure they are both alive.
	if(!dead && !other->dead){
		//Get the box of the shadow.
		SDL_Rect boxShadow=other->getBox();

		//Check if the player is on top of the shadow.
		if(checkCollision(box,boxShadow)==true){
			//We have collision now check if the other is standing on top of you.
			if(box.y+box.h<=boxShadow.y+13 && !other->inAir){
				//Player is on shadow.
				int yVelocity=yVel-1;
				if(yVelocity>0){
					//If the player is going to stand on the shadow for the first time, check if there are enough spaces for it.
					if (!other->holdingOther) {
						const SDL_Rect r = { box.x, boxShadow.y - box.h, box.w, box.h };
						for (auto ooo : objParent->levelObjects){
							//Make sure to only check visible blocks.
							if (ooo->queryProperties(GameObjectProperty_Flags, this) & 0x80000000)
								continue;
							//Make sure the object is solid for the player.
							if (!ooo->queryProperties(GameObjectProperty_PlayerCanWalkOn, this))
								continue;

							//Check for collision.
							if (checkCollision(r, ooo->getBox())) {
								//We are blocked hence we can't stand on it.
								return;
							}
						}
					}

					box.y=boxShadow.y-box.h;
					inAir=false;
					canMove=false;
					//Reset the vertical velocity.
					yVel=2;
					other->holdingOther=true;
					other->appearance.changeState("holding");

					//Change our own appearance to standing.
					if(direction==1){
						appearance.changeState("standleft");
					}else{
						appearance.changeState("standright");
					}

					//Set the velocity things.
					objCurrentStand=NULL;
				}
			}else if(boxShadow.y+boxShadow.h<=box.y+13 && !inAir){
				//Shadow is on player.
				int yVelocity=other->yVel-1;
				if(yVelocity>0){
					//If the shadow is going to stand on the player for the first time, check if there are enough spaces for it.
					if (!holdingOther) {
						const SDL_Rect r = { boxShadow.x, box.y - boxShadow.h, boxShadow.w, boxShadow.h };
						for (auto ooo : objParent->levelObjects){
							//Make sure to only check visible blocks.
							if (ooo->queryProperties(GameObjectProperty_Flags, other) & 0x80000000)
								continue;
							//Make sure the object is solid for the shadow.
							if (!ooo->queryProperties(GameObjectProperty_PlayerCanWalkOn, other))
								continue;

							//Check for collision.
							if (checkCollision(r, ooo->getBox())) {
								//We are blocked hence we can't stand on it.
								return;
							}
						}
					}

					other->box.y=box.y-boxShadow.h;
					other->inAir=false;
					other->canMove=false;
					//Reset the vertical velocity of the other.
					other->yVel=2;
					holdingOther=true;
					appearance.changeState("holding");

					//Change our own appearance to standing.
					if(other->direction==1){
						other->appearance.changeState("standleft");
					}else{
						other->appearance.changeState("standright");
					}

					//Set the velocity things.
					other->objCurrentStand=NULL;
				}
			}
		}else{
			holdingOther=false;
			other->holdingOther=false;
		}
	}
}

SDL_Rect Player::getBox(){
	return box;
}

void Player::setMyCamera(){
	//Only change the camera when the player isn't dead.
	if(dead)
		return;

	//Check if the level fit's horizontally inside the camera.
	if(camera.w>objParent->levelRect.w){
		camera.x=objParent->levelRect.x-(camera.w-objParent->levelRect.w)/2;
	}else{
		//Check if the player is halfway pass the halfright of the screen.
		if(box.x>camera.x+(SCREEN_WIDTH/2+50)){
			//It is so ease the camera to the right.
			camera.x+=(box.x-camera.x-(SCREEN_WIDTH/2))>>4;

			//Check if the camera isn't going too far.
			if(box.x<camera.x+(SCREEN_WIDTH/2+50)){
				camera.x=box.x-(SCREEN_WIDTH/2+50);
			}
		}

		//Check if the player is halfway pass the halfleft of the screen.
		if(box.x<camera.x+(SCREEN_WIDTH/2-50)){
			//It is so ease the camera to the left.
			camera.x+=(box.x-camera.x-(SCREEN_WIDTH/2))>>4;

			//Check if the camera isn't going too far.
			if(box.x>camera.x+(SCREEN_WIDTH/2-50)){
				camera.x=box.x-(SCREEN_WIDTH/2-50);
			}
		}

		//If the camera is too far to the left we set it to 0.
		if(camera.x<objParent->levelRect.x){
			camera.x=objParent->levelRect.x;
		}
		//If the camera is too far to the right we set it to the max right.
		if(camera.x+camera.w>objParent->levelRect.x+objParent->levelRect.w){
			camera.x=objParent->levelRect.x+objParent->levelRect.w-camera.w;
		}
	}

	//Check if the level fit's vertically inside the camera.
	if(camera.h>objParent->levelRect.h){
		//We don't centre vertical because the bottom line of the level (deadly) will be mid air.
		camera.y=objParent->levelRect.y-(camera.h-objParent->levelRect.h);
	}else{
		//Check if the player is halfway pass the lower half of the screen.
		if(box.y>camera.y+(SCREEN_HEIGHT/2+50)){
			//If is so ease the camera down.
			camera.y+=(box.y-camera.y-(SCREEN_HEIGHT/2))>>4;

			//Check if the camera isn't going too far.
			if(box.y<camera.y+(SCREEN_HEIGHT/2+50)){
				camera.y=box.y-(SCREEN_HEIGHT/2+50);
			}
		}

		//Check if the player is halfway pass the upper half of the screen.
		if(box.y<camera.y+(SCREEN_HEIGHT/2-50)){
			//It is so ease the camera up.
			camera.y+=(box.y-camera.y-(SCREEN_HEIGHT/2))>>4;

			//Check if the camera isn't going too far.
			if(box.y>camera.y+(SCREEN_HEIGHT/2-50)){
				camera.y=box.y-(SCREEN_HEIGHT/2-50);
			}
		}

		//If the camera is too far up we set it to 0.
		if(camera.y<objParent->levelRect.y){
			camera.y=objParent->levelRect.y;
		}
		//If the camera is too far down we set it to the max down.
		if(camera.y+camera.h>objParent->levelRect.y+objParent->levelRect.h){
			camera.y=objParent->levelRect.y+objParent->levelRect.h-camera.h;
		}
	}
}

void Player::reset(bool save){
	//Set the location of the player to it's initial state.
	box.x=fx;
	box.y=fy;

	//Reset back to default value.
	inAir=true;
	isJump=false;
	shadowCall=false;
	canMove=true;
	holdingOther=false;
	dead=false;
	record=false;
	downKeyPressed=false;
	spaceKeyPressed=false;

	//Some animation variables.
	appearance = appearanceInitial;
	if (save) appearanceSave = appearanceInitial;
	direction=0;

	state=0;
	xVel=0; //??? fixed a strange bug in game replay
	yVel=0;

	//Reset the gameObject pointers.
	objCurrentStand=objLastStand=objLastTeleport=objNotificationBlock=objShadowBlock=NULL;
	if(save)
		objCurrentStandSave=objLastStandSave=objLastTeleportSave=objNotificationBlockSave=objShadowBlockSave=NULL;

	//Clear the recording.
	line.clear();
	playerButton.clear();
	recordButton.clear();
	recordIndex=-1;
#ifdef RECORD_FILE_DEBUG
	recordKeyPressLog.clear();
	recordPlayerPosition.clear();
#endif

	if(save){
		//xVelSaved is used to indicate if there's a state saved or not.
		xVelSaved=0x80000000;

		loadAndDieTimes=0;
	}
}

void Player::saveState(){
	//We can only save the state when the player isn't dead.
	if(!dead){
		boxSaved.x=box.x;
		boxSaved.y=box.y;
		xVelSaved=xVel;
		yVelSaved=yVel;
		inAirSaved=inAir;
		isJumpSaved=isJump;
		canMoveSaved=canMove;
		holdingOtherSaved=holdingOther;
		stateSaved=state;

		//Let the appearance save.
		appearanceSave = appearance;

		//Save the lastStand and currentStand pointers.
		objCurrentStandSave=objCurrentStand;
		objLastStandSave=objLastStand;
		objLastTeleportSave = objLastTeleport;
		objNotificationBlockSave = objNotificationBlock;
		objShadowBlockSave = objShadowBlock;

		//Save any recording stuff.
		recordSaved=record;
		playerButtonSaved=playerButton;
		lineSaved=line;

		//Save the record
		savedRecordButton=recordButton;
#ifdef RECORD_FILE_DEBUG
		recordKeyPressLog_saved=recordKeyPressLog;
		recordPlayerPosition_saved=recordPlayerPosition;
#endif

		//To prevent playing the sound twice, only the player can cause the sound.
		if(!shadow)
			getSoundManager()->playSound("checkpoint");

		//We saved a new state so reset the counter
		loadAndDieTimes=0;
	}
}

void Player::loadState(){
	//Check with xVelSaved if there's a saved state.
	if(xVelSaved==int(0x80000000)){
		//There isn't so reset the game to load the first initial state.
		//NOTE: There's no need in removing the saved state since there is none.
		reset(false);
		return;
	}

	//Restore the saved values.
	box.x=boxSaved.x;
	box.y=boxSaved.y;
	//xVel is set to 0 since it's saved counterpart is used to indicate a saved state.
	xVel=0;
	yVel=yVelSaved;

	//Restore the saved values.
	inAir=inAirSaved;
	isJump=isJumpSaved;
	canMove=canMoveSaved;
	holdingOther=holdingOtherSaved;
	dead=false;
	record=false;
	shadowCall=false;
	state=stateSaved;

	objCurrentStand=objCurrentStandSave;
	objLastStand=objLastStandSave;
	objLastTeleport = objLastTeleportSave;
	objNotificationBlock = objNotificationBlockSave;
	objShadowBlock = objShadowBlockSave;

	//Restore the appearance.
	appearance = appearanceSave;

	//Restore any recorded stuff.
	record=recordSaved;
	playerButton=playerButtonSaved;
	line=lineSaved;
	
	//Load the previously saved record
	recordButton=savedRecordButton;
#ifdef RECORD_FILE_DEBUG
	recordKeyPressLog=recordKeyPressLog_saved;
	recordPlayerPosition=recordPlayerPosition_saved;
#endif
}

void Player::swapState(Player* other){
	//We need to swap the values of the player with the ones of the given player.
	swap(box.x,other->box.x);
	swap(box.y,other->box.y);
	swap(xVelBase, other->yVelBase);
	swap(yVelBase, other->yVelBase);
	objCurrentStand.swap(other->objCurrentStand);
	//NOTE: xVel isn't there since it's used for something else.
	swap(yVel,other->yVel);
	swap(inAir,other->inAir);
	swap(isJump,other->isJump);
	swap(canMove,other->canMove);
	swap(holdingOther,other->holdingOther);
	swap(dead, other->dead);

	//Also reset the state of the other.
	other->stateReset();

	//Play the swap sound.
	getSoundManager()->playSound("swap");

	//Update statistics.
	if(!dead && !objParent->player.isPlayFromRecord() && !objParent->interlevel){
		if(objParent->time < objParent->recentSwap + FPS){
			//Swap player and shadow twice in 1 senond.
			statsMgr.newAchievement("quickswap");
		}
		objParent->recentSwap=objParent->time;

		statsMgr.swapTimes++;

		//Update achievements
		switch(statsMgr.swapTimes){
		case 100:
			statsMgr.newAchievement("swap100");
			break;
		}
	}
}

bool Player::canSaveState(){
	//We can only save the state if the player isn't dead.
	return !dead;
}

bool Player::canLoadState(){
	//We use xVelSaved to indicate if a state is saved or not.
	return xVelSaved != int(0x80000000);
}

void Player::die(bool animation){
	//Make sure the player isn't already dead.
	if(!dead){
		dead=true;

		//In the arcade mode, the game finishes when the player (not the shadow) dies
		if (objParent->arcade && !shadow && stateID != STATE_LEVEL_EDITOR) {
			objParent->won = true;
		}

		//If sound is enabled run the hit sound.
		getSoundManager()->playSound("hit");

		//Change the apearance to die (if animation is true).
		if(animation){
			if(direction==1){
				appearance.changeState("dieleft");
			}else{
				appearance.changeState("dieright");
			}
		} else {
			appearance.changeState("dead");
		}

		//Update statistics
		if(!objParent->player.isPlayFromRecord() && !objParent->interlevel){
			addRecentDeaths(objParent->recentLoad);

			if(shadow) statsMgr.shadowDies++;
			else statsMgr.playerDies++;

			switch(statsMgr.playerDies+statsMgr.shadowDies){
			case 1:
				statsMgr.newAchievement("die1");
				break;
			case 50:
				statsMgr.newAchievement("die50");
				break;
			case 1000:
				statsMgr.newAchievement("die1000");
				break;
			}

			if(canLoadState() && (++loadAndDieTimes)==100){
				statsMgr.newAchievement("loadAndDie100");
			}

			if(objParent->player.dead && objParent->shadow.dead) statsMgr.newAchievement("doubleKill");
		}
	}

	//We set the jumpTime to 80 when this is the shadow.
	//That's the countdown for the "Your shadow has died." message.
	if(shadow){
		jumpTime=80;
	}
}
