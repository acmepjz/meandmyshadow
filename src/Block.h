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

#ifndef BLOCK_H
#define BLOCK_H

#include "GameObjects.h"
#include "ThemeManager.h"
#include "ScriptUserData.h"
#include "ScriptExecutor.h"
#include <vector>
#include <SDL.h>

class Game;
class LevelEditor;
class AddRemoveGameObjectCommand;
class AddRemovePathCommand;
class BlockScriptAPI;

class Block: public GameObject, public ScriptProxyUserClass<'B','L','O','K',Block>{
	friend class Game;
	friend class LevelEditor;
	friend class AddRemoveGameObjectCommand;
	friend class AddRemovePathCommand;
	friend class BlockScriptAPI;
private:
	//Integer that a block can use for all animation or visual related things.
	int animation;
	
	//flags:
	//all: 0x80000000=invisible (If it's not visible it will not collide with anything or execute any scripts except for 'onCreate'.)
	//moving object: 0x1=disabled 0x2=NOT loop
	//button: bit0-1=behavior 0x4=pressed
	//switch: bit0-1=behavior
	//portal: 0x1=automatic
	//fragile: bit0-1 state
	//collectible: 0x1=collected
	int flags;

	//Temp variables used to keep track of time/state.
	int temp;

	//Delta variables, if the block moves these must be set to the delta movement.
	int dx,dy;
	
	//Vector containing the poisitions of the moving block.
	std::vector<SDL_Rect> movingPos;

	//Cached variable for total time ot moving positions. -1 means uninitialized
	//NOTE: should reset it when editing movingPos (except for edit mode since it will reset when level starts)
	int movingPosTime;

	//Integer containing the speed for conveyorbelts.
	//NOTE: in V0.5 the speed 1 means 0.1 pixel/frame = 0.08 block/s
	//which is 1/10 of the old speed, and named "speed10" in the level file to keep compatibility
	int speed;

	//Following is for the pushable block.
	Block::ObservePointer objCurrentStand;
public:
	// The custom appearance name, whose meaning is the same as Scenery::sceneryName_. "" means using default one
	std::string customAppearanceName;

	//The Appearance of the block.
	ThemeBlockInstance appearance;
	
	//Velocity variables for the block, if the block moves these must be set for collision/movement of the player.
	int xVel,yVel;
	
	//Follwing is for pushable block.
	bool inAir;
	int xVelBase,yVelBase;
	
	//The id of the block.
	std::string id;
	//String containing the id of the destination for portals.
	std::string destination;
	//String containing the message of the notification block.
	std::string message;

	//The map that holds a script for every event.
	//NOTE: This will NOT get copy constructed since the copy constructor is only used for save/load support!!!
	map<int,std::string> scripts;

	//Compiled scripts. Use lua_rawgeti(L, LUA_REGISTRYINDEX, r) to get the function.
	std::map<int, int> compiledScripts;

	//The boolean indicating if we will be deleted in next (or current) frame.
	//Used in dynamic delete of blocks in scripting.
	bool isDelete;
	
	//Constructor.
	//objParent: Pointer to the Game object.
	//x: The x location of the block.
	//y: The y location of the block.
	//w: The width of the block.
	//h: The height of the block.
	//type: The block type.
	Block(Game* objParent,int x=0,int y=0,int w=50,int h=50,int type=-1);

	//Copy constructor (which is only used in save/load support).
	Block(const Block& other);

	Block& operator=(const Block& other) = delete;

	//Desturctor.
	~Block();

	//Method for initializing the block.
	//x: The x location of the block.
	//y: The y location of the block.
	//w: The width of the block.
	//h: The height of the block.
	//type: The block type.
	void init(int x,int y,int w,int h,int type);

	//Method used to draw the block.
    void show(SDL_Renderer &renderer) override;

	//Returns the box of a given type.
	//boxType: The type of box that should be returned.
	//See GameObjects.h for the types.
	//Returns: The box.
	virtual SDL_Rect getBox(int boxType=BoxType_Current) override;

	//Method for setting the block to a given location as if it moved there.
	//x: The new x location.
	//y: The new y location.
	void moveTo(int x,int y);

	//Method for setting a new size as if the block grew,
	//w: The new width of the block.
	//h: The new height of the block.
	void growTo(int w,int h);
	
	//Play an animation.
	virtual void playAnimation() override;
	
	//Method called when there's an event.
	//eventType: The type of event.
	//See GameObjects.h for the eventtypes.
	virtual void onEvent(int eventType) override;
	
	//Method used to retrieve a property from the block.
	//propertyType: The type of property requested.
	//See GameObjects.h for the properties.
	//obj: Pointer to the player.
	//Returns: Integer containing the value of the property.
	virtual int queryProperties(int propertyType,Player* obj) override;
	
	//Get the editor data of the block.
	//obj: The vector that will be filled with the editorData.
	virtual void getEditorData(std::vector<std::pair<std::string,std::string> >& obj) override;
	//Set the editor data of the block.
	//obj: The new editor data.
	virtual void setEditorData(std::map<std::string,std::string>& obj) override;

	//Get a single property of the block.
	//property: The property to return.
	//Returns: The value for the requested property.
	virtual std::string getEditorProperty(const std::string& property) override;
	//Set a single property of the block.
	//property: The property to set.
	//value: The new value for the property.
	virtual void setEditorProperty(const std::string& property, const std::string& value) override;

	//Method for loading the Block from a node.
	//objNode: Pointer to the storage node to load from.
	//Returns: True if it succeeds without errors.
    virtual bool loadFromNode(ImageManager&, SDL_Renderer&, TreeStorageNode* objNode) override;

	//Method used for updating moving blocks or elements of blocks.
	virtual void move() override;

	//Get total time ot moving positions.
	int getPathMaxTime();

	//Mark this block to be deleted in next frame. Also hide this block and invalidate references to it.
	void deleteMe();
};

#endif
