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

#ifndef GAME_OBJECTS_H
#define GAME_OBJECTS_H

#include "Globals.h"
#include "TreeStorageNode.h"
#include <SDL.h>
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
	GameObjectEvent_PlayerLeave, 
	//Event called when the gameObject is created.
	//Only used for scripting purpose.
	GameObjectEvent_OnCreate,
	//Event called every frame.
	//Only used for scripting purpose.
	GameObjectEvent_OnEnterFrame,
	//Event called when the player press DOWN key.
	//Currently this event only fires when the block type is TYPE_SWITCH.
	GameObjectEvent_OnPlayerInteraction,

	//Event called when the block receives "toggle" from a switch/button.
	GameObjectEvent_OnToggle=0x10000,
	//Event called when the block receives "switch on" from a switch/button.
	GameObjectEvent_OnSwitchOn=0x10001,
	//Event called when the block receives "switch off" from a switch/button.
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

	//Copy constuctor (only used in save/load support).
	GameObject(const GameObject& other);

	//Destructor.
	virtual ~GameObject();

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

	//This method sets the size of the object to a given size.
	//w: The new width of the gameObject.
	//h: The new height the gameObject.
	virtual void setSize(int w,int h);
	//This method sets the size of the base of the object to a given size.
	//w: The new width of the gameObject.
	//h: The new height of the gameObject.
	virtual void setBaseSize(int w,int h);
	
	//Method used to draw the GameObject.
    virtual void show(SDL_Renderer& renderer)=0;

	//Play an animation.
	virtual void playAnimation();
	//Invoke an event of the GameObject.
	//eventType: The event type.
	virtual void onEvent(int eventType);
	//Method used to request certain properties of the GameObject.
	//propertyType: The property that is requested.
	//isShadow: If it is shadow.
	virtual int queryProperties(int propertyType, bool isShadow);

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
	virtual std::string getEditorProperty(const std::string& property);
	//Set a single property of the block.
	//property: The property to set.
	//value: The new value for the property.
	virtual void setEditorProperty(const std::string& property, const std::string& value);

	//Method for loading the GameObject from a node.
	//objNode: Pointer to the storage node to load from.
	//Returns: True if it succeeds without errors.
    virtual bool loadFromNode(ImageManager&, SDL_Renderer&, TreeStorageNode*);

	//Update method for GameObjects, used for moving blocks.
	virtual void move();
};

#endif
