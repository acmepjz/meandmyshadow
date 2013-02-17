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

#ifndef GAME_OBJECTS_H
#define GAME_OBJECTS_H

#include "Globals.h"
#include "ScriptUserData.h"
#include <SDL/SDL.h>
#include <string>
#include <vector>
#include <utility>
#include <map>

class Game;
class Player;

//The different gameObject events.
enum GameObjectEventType{
	//Event called when the player walks on the gameObject.
	GameObjectEvent_PlayerWalkOn=1,
	//Event called when the player is on the gameObject.
	GameObjectEvent_PlayerIsOn,
	//Event called when the player leaves the gameObject.
	//Currently unimplemented.
	GameObjectEvent_PlayerLeave, 
	//Event called when the gameObject is created.
	//Only used for scripting purpose.
	GameObjectEvent_OnCreate,
	//Event called every frame.
	//Only used for scripting purpose.
	GameObjectEvent_OnEnterFrame,
	
	//Event called when the player toggles it. (DOWN key)
	GameObjectEvent_OnToggle=0x10000,
	//Event called when the player switches it on. (DOWN key)
	GameObjectEvent_OnSwitchOn=0x10001,
	//Event called when the player switches it off. (DOWN key)
	GameObjectEvent_OnSwitchOff=0x10002,
};

//The different gameObject properties.
enum GameObjectPropertyType{
	//If the player can walk on the gameObject.
	GameObjectProperty_PlayerCanWalkOn=1,
	//If the object is spiked.
	GameObjectProperty_IsSpikes,
	//If the gameObject has some flags.
	GameObjectProperty_Flags,
};

//The different box types that can be requested using the getBox(int boxType) method.
enum GameObjectBoxType{
	//Box of the current position.
	BoxType_Current=0,
	//Box of the base/start position.
	BoxType_Base,
	//Box of the previous position.
	BoxType_Previous,
	//The movement of the block since last position.
	BoxType_Delta,
	//The velocity for when the player is standing on it.
	BoxType_Velocity,
};

//The GameObject class.
class GameObject{
protected:
	//The box of the gameObject.
	//It's used for the current location of the gameObject and its size.
	SDL_Rect box;
	//The base location of the game object.
	SDL_Rect boxBase;
public:
	//The type of the GameObject.
	int type;
	//Pointer to the Game state.
	Game* parent;

	//Constructor.
	//parent: Pointer to the Game state.
	GameObject(Game* parent);
	//Destructor.
	~GameObject();

	//Method used to retrieve a certain box from the GameObject.
	//boxType: The type of box that is requested. (default=0)
	//Returns: An SDL_Rect.
	virtual SDL_Rect getBox(int boxType=0);
	
	//This method is used to place the location on a given location.
	//x: The x location to place the gameObject.
	//y: The y location to place the gameObject.
	virtual void setLocation(int x,int y);
	//This method is used to set the base of an object to a given location.
	//x: The x location to place the gameObject.
	//y: The y location to place the gameObject.
	virtual void setBaseLocation(int x,int y);

	
	//Method used to draw the GameObject.
	virtual void show()=0;
	//Save the state of the GameObject, used for moving blocks, etc.
	virtual void saveState();
	//Load the state of the GameObject, used for moving blocks, etc.
	virtual void loadState();
	//Reset the state of the GameObject, used for moving blocks, etc.
	//save: Boolean if the saved state should also be reset.
	virtual void reset(bool save);

	//Play an animation.
	//flags: TODO???
	virtual void playAnimation(int flags);
	//Invoke an event of the GameObject.
	//eventType: The event type.
	virtual void onEvent(int eventType);
	//Method used to request certain properties of the GameObject.
	//propertyType: The property that is requested.
	//obj: Pointer to the player.
	virtual int queryProperties(int propertyType,Player* obj);

	//Method used to retrieve the additional editor data for the GameObject.
	//Used for messages, moving positions, etc...
	//obj: Vector containing the editorData pairs. (key, value)
	virtual void getEditorData(std::vector<std::pair<std::string,std::string> >& obj);
	//Set the editorData.
	//obj: Map containing the key/value for the editor data.
	virtual void setEditorData(std::map<std::string,std::string>& obj);

	//Get a single property of the block.
	//property: The property to return.
	//Returns: The value for the requested property.
	virtual std::string getEditorProperty(std::string property);
	//Set a single property of the block.
	//property: The property to set.
	//value: The new value for the property.
	virtual void setEditorProperty(std::string property,std::string value);

	//Method that is called before the move method.
	//It can be used to reset variables like delta movement and velocity.
	virtual void prepareFrame();
	//Update method for GameObjects, used for moving blocks.
	virtual void move();
};

//We include block.h here because it needs some things defined above.
#include "Block.h"
#endif
