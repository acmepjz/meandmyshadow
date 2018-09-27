/*
 * Copyright (C) 2012-2013 Me and My Shadow
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

#include "ScriptAPI.h"
#include "ScriptExecutor.h"
#include "SoundManager.h"
#include "Functions.h"
#include "Block.h"
#include "Game.h"
#include "MusicManager.h"
#include "ScriptDelayExecution.h"
#include <iostream>
#include <algorithm>
using namespace std;

/////////////////////////// HELPER MACRO ///////////////////////////

#define HELPER_GET_AND_CHECK_ARGS(ARGS) \
	const int args = lua_gettop(state); \
	if(args != ARGS) { \
		return luaL_error(state, "Incorrect number of arguments for %s, expected %d.", __FUNCTION__, ARGS); \
	}

#define HELPER_GET_AND_CHECK_ARGS_RANGE(ARGS1, ARGS2) \
	const int args = lua_gettop(state); \
	if(args < ARGS1 || args > ARGS2) { \
		return luaL_error(state, "Incorrect number of arguments for %s, expected %d-%d.", __FUNCTION__, ARGS1, ARGS2); \
	}

#define HELPER_GET_AND_CHECK_ARGS_2(ARGS1, ARGS2) \
	const int args = lua_gettop(state); \
	if(args != ARGS1 && args != ARGS2) { \
		return luaL_error(state, "Incorrect number of arguments for %s, expected %d or %d.", __FUNCTION__, ARGS1, ARGS2); \
	}

#define HELPER_GET_AND_CHECK_ARGS_AT_LEAST(ARGS) \
	const int args = lua_gettop(state); \
	if(args < ARGS) { \
		return luaL_error(state, "Incorrect number of arguments for %s, expected at least %d.", __FUNCTION__, ARGS); \
	}

#define HELPER_GET_AND_CHECK_ARGS_AT_MOST(ARGS) \
	const int args = lua_gettop(state); \
	if(args > ARGS) { \
		return luaL_error(state, "Incorrect number of arguments for %s, expected at most %d.", __FUNCTION__, ARGS); \
	}

//================================================================

#define HELPER_CHECK_ARGS_TYPE(INDEX, TYPE) \
	if(!lua_is##TYPE(state,INDEX)) { \
		return luaL_error(state, "Invalid type for argument %d of %s, should be %s.", INDEX, __FUNCTION__, #TYPE); \
	}

#define HELPER_CHECK_ARGS_TYPE_NO_HINT(INDEX, TYPE) \
	if(!lua_is##TYPE(state,INDEX)) { \
		return luaL_error(state, "Invalid type for argument %d of %s.", INDEX, __FUNCTION__); \
	}

#define HELPER_CHECK_ARGS_TYPE_2(INDEX, TYPE1, TYPE2) \
	if(!lua_is##TYPE1(state,INDEX) && !lua_is##TYPE2(state,INDEX)) { \
		return luaL_error(state, "Invalid type for argument %d of %s, should be %s or %s.", INDEX, __FUNCTION__, #TYPE1, #TYPE2); \
	}

#define HELPER_CHECK_ARGS_TYPE_2_NO_HINT(INDEX, TYPE1, TYPE2) \
	if(!lua_is##TYPE1(state,INDEX) && !lua_is##TYPE2(state,INDEX)) { \
		return luaL_error(state, "Invalid type for argument %d of %s.", INDEX, __FUNCTION__); \
	}

#define HELPER_CHECK_ARGS_TYPE_OR_NIL(INDEX, TYPE) \
	HELPER_CHECK_ARGS_TYPE_2(INDEX, TYPE, nil)

#define HELPER_CHECK_ARGS_TYPE_OR_NIL_NO_HINT(INDEX, TYPE) \
	HELPER_CHECK_ARGS_TYPE_2_NO_HINT(INDEX, TYPE, nil)

//================================================================

#define HELPER_CHECK_OPTIONAL_ARGS_TYPE(INDEX, TYPE) \
	if(args>=INDEX && !lua_is##TYPE(state,INDEX)) { \
		return luaL_error(state, "Invalid type for argument %d of %s, should be %s.", INDEX, __FUNCTION__, #TYPE); \
	}

#define HELPER_CHECK_OPTIONAL_ARGS_TYPE_NO_HINT(INDEX, TYPE) \
	if(args>=INDEX && !lua_is##TYPE(state,INDEX)) { \
		return luaL_error(state, "Invalid type for argument %d of %s.", INDEX, __FUNCTION__); \
	}

#define HELPER_CHECK_OPTIONAL_ARGS_TYPE_2(INDEX, TYPE1, TYPE2) \
	if(args>=INDEX && !lua_is##TYPE1(state,INDEX) && !lua_is##TYPE2(state,INDEX)) { \
		return luaL_error(state, "Invalid type for argument %d of %s, should be %s or %s.", INDEX, __FUNCTION__, #TYPE1, #TYPE2); \
	}

#define HELPER_CHECK_OPTIONAL_ARGS_TYPE_2_NO_HINT(INDEX, TYPE1, TYPE2) \
	if(args>=INDEX && !lua_is##TYPE1(state,INDEX) && !lua_is##TYPE2(state,INDEX)) { \
		return luaL_error(state, "Invalid type for argument %d of %s.", INDEX, __FUNCTION__); \
	}

#define HELPER_CHECK_OPTIONAL_ARGS_TYPE_OR_NIL(INDEX, TYPE) \
	HELPER_CHECK_OPTIONAL_ARGS_TYPE_2(INDEX, TYPE, nil)

#define HELPER_CHECK_OPTIONAL_ARGS_TYPE_OR_NIL_NO_HINT(INDEX, TYPE) \
	HELPER_CHECK_OPTIONAL_ARGS_TYPE_2_NO_HINT(INDEX, TYPE, nil)

//================================================================

#define HELPER_REGISTER_IS_VALID_FUNCTION(CLASS) \
int isValid(lua_State* state){ \
	HELPER_GET_AND_CHECK_ARGS(1); \
	HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata); \
	CLASS* object = CLASS::getObjectFromUserData(state, 1); \
	lua_pushboolean(state, object ? 1 : 0); \
	return 1; \
}

//================================================================

#define _F(FUNC) { #FUNC, _L::FUNC }
#define _FG(FUNC) _F(get##FUNC)
#define _FI(FUNC) _F(is##FUNC)
#define _FS(FUNC) _F(set##FUNC)
#define _FGS(FUNC) _F(get##FUNC), _F(set##FUNC)
#define _FIS(FUNC) _F(is##FUNC), _F(set##FUNC)

///////////////////////////BLOCK SPECIFIC///////////////////////////

class BlockScriptAPI {
public:
	static int getFlags(const Block* block) {
		return block->flags;
	}
	static void setFlags(Block* block, int flags) {
		block->flags = flags;
	}
	static void fragileUpdateState(Block* block, int state) {
		state &= 0x3;
		block->flags = (block->flags & ~0x3) | state;
		const char* s = (state == 0) ? "default" : ((state == 1) ? "fragile1" : ((state == 2) ? "fragile2" : "fragile3"));
		block->appearance.changeState(s);
	}
	static int getTemp(const Block* block) {
		return block->temp;
	}
	static void setTemp(Block* block, int value) {
		block->temp = value;
	}
	static int getSpeed(const Block* block) {
		return block->speed;
	}
	static void setSpeed(Block* block, int value) {
		block->speed = value;
	}
	static void invalidatePathMaxTime(Block* block) {
		block->movingPosTime = -1;
	}
	static std::vector<SDL_Rect>& getMovingPos(Block* block) {
		return block->movingPos;
	}
};

namespace block {

	HELPER_REGISTER_IS_VALID_FUNCTION(Block);

	int getBlockById(lua_State* state){
		//Get the number of args, this MUST be one.
		HELPER_GET_AND_CHECK_ARGS(1);

		//Make sure the given argument is an id (string).
		HELPER_CHECK_ARGS_TYPE(1, string);

		//Check if the currentState is the game state.
		Game* game = dynamic_cast<Game*>(currentState);
		if (game == NULL) return 0;

		//Get the actual game object.
		string id = lua_tostring(state, 1);
		std::vector<Block*>& levelObjects = game->levelObjects;
		Block* object = NULL;
		for (unsigned int i = 0; i < levelObjects.size(); i++){
			if (levelObjects[i]->getEditorProperty("id") == id){
				object = levelObjects[i];
				break;
			}
		}
		if (object == NULL){
			//Unable to find the requested object.
			//Return nothing, will result in a nil in the script. 
			return 0;
		}

		//Create the userdatum.
		object->createUserData(state, "block");

		//We return one object, the userdatum.
		return 1;
	}

	int getBlocksById(lua_State* state){
		//Get the number of args, this MUST be one.
		HELPER_GET_AND_CHECK_ARGS(1);

		//Make sure the given argument is an id (string).
		HELPER_CHECK_ARGS_TYPE(1, string);

		//Check if the currentState is the game state.
		Game* game = dynamic_cast<Game*>(currentState);
		if (game == NULL) return 0;

		//Get the actual game object.
		string id = lua_tostring(state, 1);
		std::vector<Block*>& levelObjects = game->levelObjects;
		std::vector<Block*> result;
		for (unsigned int i = 0; i < levelObjects.size(); i++){
			if (levelObjects[i]->getEditorProperty("id") == id){
				result.push_back(levelObjects[i]);
			}
		}

		//Create the table that will hold the result.
		lua_createtable(state, result.size(), 0);

		//Loop through the results.
		for (unsigned int i = 0; i < result.size(); i++){
			//Create the userdatum.
			result[i]->createUserData(state, "block");
			//And set the table.
			lua_rawseti(state, -2, i + 1);
		}

		//We return one object, the userdatum.
		return 1;
	}

	int moveTo(lua_State* state){
		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS(3);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);
		HELPER_CHECK_ARGS_TYPE(2, integer);
		HELPER_CHECK_ARGS_TYPE(3, integer);

		//Now get the pointer to the object.
		Block* object = Block::getObjectFromUserData(state, 1);
		if (object == NULL) return 0;

		int x = lua_tointeger(state, 2);
		int y = lua_tointeger(state, 3);
		object->moveTo(x, y);

		return 0;
	}

	int getLocation(lua_State* state){
		//Make sure there's only one argument and that argument is an userdatum.
		HELPER_GET_AND_CHECK_ARGS(1);

		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);

		Block* object = Block::getObjectFromUserData(state, 1);
		if (object == NULL) return 0;

		//Get the object.
		SDL_Rect r = object->getBox();
		lua_pushinteger(state, r.x);
		lua_pushinteger(state, r.y);
		return 2;
	}

	int getBaseLocation(lua_State* state){
		//Make sure there's only one argument and that argument is an userdatum.
		HELPER_GET_AND_CHECK_ARGS(1);

		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);

		Block* object = Block::getObjectFromUserData(state, 1);
		if (object == NULL) return 0;

		//Get the object.
		SDL_Rect r = object->getBox(BoxType_Base);
		lua_pushinteger(state, r.x);
		lua_pushinteger(state, r.y);
		return 2;
	}

	int setLocation(lua_State* state){
		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS(3);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);
		HELPER_CHECK_ARGS_TYPE(2, integer);
		HELPER_CHECK_ARGS_TYPE(3, integer);

		//Now get the pointer to the object.
		Block* object = Block::getObjectFromUserData(state, 1);
		if (object == NULL) return 0;

		int x = lua_tointeger(state, 2);
		int y = lua_tointeger(state, 3);
		object->setLocation(x, y);

		return 0;
	}

	int growTo(lua_State* state){
		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS(3);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);
		HELPER_CHECK_ARGS_TYPE(2, integer);
		HELPER_CHECK_ARGS_TYPE(3, integer);

		//Now get the pointer to the object.
		Block* object = Block::getObjectFromUserData(state, 1);
		if (object == NULL) return 0;

		int w = lua_tointeger(state, 2);
		int h = lua_tointeger(state, 3);
		object->growTo(w, h);

		return 0;
	}

	int getSize(lua_State* state){
		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS(1);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);

		Block* object = Block::getObjectFromUserData(state, 1);
		if (object == NULL) return 0;

		//Get the object.
		lua_pushinteger(state, object->getBox().w);
		lua_pushinteger(state, object->getBox().h);
		return 2;
	}

	int setSize(lua_State* state){
		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS(3);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);
		HELPER_CHECK_ARGS_TYPE(2, integer);
		HELPER_CHECK_ARGS_TYPE(3, integer);

		//Now get the pointer to the object.
		Block* object = Block::getObjectFromUserData(state, 1);
		if (object == NULL) return 0;

		int w = lua_tointeger(state, 2);
		int h = lua_tointeger(state, 3);
		object->setSize(w, h);

		return 0;
	}

	int getType(lua_State* state){
		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS(1);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);

		Block* object = Block::getObjectFromUserData(state, 1);
		if (object == NULL || object->type < 0 || object->type >= TYPE_MAX) return 0;

		lua_pushstring(state, Game::blockName[object->type]);
		return 1;
	}

	int changeThemeState(lua_State* state){
		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS(2);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);
		HELPER_CHECK_ARGS_TYPE(2, string);

		Block* object = Block::getObjectFromUserData(state, 1);
		object->appearance.changeState(lua_tostring(state, 2));

		return 0;
	}

	int setVisible(lua_State* state){
		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS(2);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);
		HELPER_CHECK_ARGS_TYPE(2, boolean);

		Block* object = Block::getObjectFromUserData(state, 1);
		if (object == NULL)
			return 0;

		BlockScriptAPI::setFlags(object,
			(BlockScriptAPI::getFlags(object) & ~0x80000000) | (lua_toboolean(state, 2) ? 0 : 0x80000000)
			);

		return 0;
	}

	int isVisible(lua_State* state){
		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS(1);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);

		Block* object = Block::getObjectFromUserData(state, 1);
		if (object == NULL)
			return 0;

		lua_pushboolean(state, (BlockScriptAPI::getFlags(object) & 0x80000000) ? 0 : 1);
		return 1;
	}

	int getEventHandler(lua_State* state){
		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS(2);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);
		HELPER_CHECK_ARGS_TYPE(2, string);

		Block* object = Block::getObjectFromUserData(state, 1);
		if (object == NULL) return 0;

		//Check event type
		string eventType = lua_tostring(state, 2);
		map<string, int>::iterator it = Game::gameObjectEventNameMap.find(eventType);
		if (it == Game::gameObjectEventNameMap.end()) return 0;

		//Check compiled script
		map<int, int>::iterator script = object->compiledScripts.find(it->second);
		if (script == object->compiledScripts.end()) return 0;

		//Get event handler
		lua_rawgeti(state, LUA_REGISTRYINDEX, script->second);
		return 1;
	}

	//It will return old event handler.
	int setEventHandler(lua_State* state){
		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS(3);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);
		HELPER_CHECK_ARGS_TYPE(2, string);
		HELPER_CHECK_ARGS_TYPE_OR_NIL(3, function);

		Block* object = Block::getObjectFromUserData(state, 1);
		if (object == NULL) return 0;

		//Check event type
		string eventType = lua_tostring(state, 2);
		map<string, int>::const_iterator it = Game::gameObjectEventNameMap.find(eventType);
		if (it == Game::gameObjectEventNameMap.end()){
			lua_pushfstring(state, "Unknown block event type: '%s'.", eventType.c_str());
			return lua_error(state);
		}

		//Check compiled script
		int scriptIndex = LUA_REFNIL;
		{
			map<int, int>::iterator script = object->compiledScripts.find(it->second);
			if (script != object->compiledScripts.end()) scriptIndex = script->second;
		}

		//Set new event handler
		object->compiledScripts[it->second] = luaL_ref(state, LUA_REGISTRYINDEX);

		if (scriptIndex == LUA_REFNIL) return 0;

		//Get old event handler and unreference it
		lua_rawgeti(state, LUA_REGISTRYINDEX, scriptIndex);
		luaL_unref(state, LUA_REGISTRYINDEX, scriptIndex);
		return 1;
	}

	int onEvent(lua_State* state) {
		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS(2);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);
		HELPER_CHECK_ARGS_TYPE(2, string);

		Block* object = Block::getObjectFromUserData(state, 1);
		if (object == NULL) return 0;

		//Check event type
		string eventType = lua_tostring(state, 2);
		map<string, int>::const_iterator it = Game::gameObjectEventNameMap.find(eventType);
		if (it == Game::gameObjectEventNameMap.end()){
			lua_pushfstring(state, "Unknown block event type: '%s'.", eventType.c_str());
			return lua_error(state);
		}

		object->onEvent(it->second);

		return 0;
	}

	int isActivated(lua_State* state) {
		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS(1);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);

		Block* object = Block::getObjectFromUserData(state, 1);
		if (object == NULL) return 0;

		switch (object->type) {
		case TYPE_MOVING_BLOCK:
		case TYPE_MOVING_SHADOW_BLOCK:
		case TYPE_MOVING_SPIKES:
		case TYPE_CONVEYOR_BELT:
		case TYPE_SHADOW_CONVEYOR_BELT:
			lua_pushboolean(state, (BlockScriptAPI::getFlags(object) & 0x1) ? 0 : 1);
			return 1;
		default:
			return 0;
		}
	}

	int setActivated(lua_State* state) {
		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS(2);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);
		HELPER_CHECK_ARGS_TYPE(2, boolean);

		Block* object = Block::getObjectFromUserData(state, 1);
		if (object == NULL) return 0;

		switch (object->type) {
		case TYPE_MOVING_BLOCK:
		case TYPE_MOVING_SHADOW_BLOCK:
		case TYPE_MOVING_SPIKES:
		case TYPE_CONVEYOR_BELT:
		case TYPE_SHADOW_CONVEYOR_BELT:
			BlockScriptAPI::setFlags(object,
				(BlockScriptAPI::getFlags(object) & ~1) | (lua_toboolean(state, 2) ? 0 : 1)
				);
			break;
		}

		return 0;
	}

	int isAutomatic(lua_State* state) {
		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS(1);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);

		Block* object = Block::getObjectFromUserData(state, 1);
		if (object == NULL) return 0;

		switch (object->type) {
		case TYPE_PORTAL:
			lua_pushboolean(state, (BlockScriptAPI::getFlags(object) & 0x1) ? 1 : 0);
			return 1;
		default:
			return 0;
		}
	}

	int setAutomatic(lua_State* state) {
		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS(2);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);
		HELPER_CHECK_ARGS_TYPE(2, boolean);

		Block* object = Block::getObjectFromUserData(state, 1);
		if (object == NULL) return 0;

		switch (object->type) {
		case TYPE_PORTAL:
			BlockScriptAPI::setFlags(object,
				(BlockScriptAPI::getFlags(object) & ~1) | (lua_toboolean(state, 2) ? 1 : 0)
				);
			break;
		}

		return 0;
	}

	int getBehavior(lua_State* state) {
		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS(1);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);

		Block* object = Block::getObjectFromUserData(state, 1);
		if (object == NULL) return 0;

		switch (object->type) {
		case TYPE_BUTTON:
		case TYPE_SWITCH:
			switch (BlockScriptAPI::getFlags(object) & 0x3) {
			default:
				lua_pushstring(state, "toggle");
				break;
			case 1:
				lua_pushstring(state, "on");
				break;
			case 2:
				lua_pushstring(state, "off");
				break;
			}
			return 1;
		default:
			return 0;
		}
	}

	int setBehavior(lua_State* state) {
		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS(2);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);
		HELPER_CHECK_ARGS_TYPE(2, string);

		Block* object = Block::getObjectFromUserData(state, 1);
		if (object == NULL) return 0;

		switch (object->type) {
		case TYPE_BUTTON:
		case TYPE_SWITCH:
			{
				int newFlags = BlockScriptAPI::getFlags(object) & ~3;
				std::string s = lua_tostring(state, 2);
				if (s == "on") newFlags |= 1;
				else if (s == "off") newFlags |= 2;
				BlockScriptAPI::setFlags(object, newFlags);
			}
			break;
		}

		return 0;
	}

	int getState(lua_State* state) {
		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS(1);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);

		Block* object = Block::getObjectFromUserData(state, 1);
		if (object == NULL) return 0;

		switch (object->type) {
		case TYPE_FRAGILE:
			lua_pushinteger(state, BlockScriptAPI::getFlags(object) & 0x3);
			return 1;
		default:
			return 0;
		}
	}

	int setState(lua_State* state) {
		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS(2);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);
		HELPER_CHECK_ARGS_TYPE(2, integer);

		Block* object = Block::getObjectFromUserData(state, 1);
		if (object == NULL) return 0;

		switch (object->type) {
		case TYPE_FRAGILE:
			{
				int oldState = BlockScriptAPI::getFlags(object) & 0x3;
				int newState = (int)lua_tointeger(state, 2);
				if (newState < 0) newState = 0;
				else if (newState > 3) newState = 3;
				if (newState != oldState) {
					BlockScriptAPI::fragileUpdateState(object, newState);
				}
			}
			break;
		}

		return 0;
	}

	int isPlayerOn(lua_State* state) {
		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS(1);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);

		Block* object = Block::getObjectFromUserData(state, 1);
		if (object == NULL) return 0;

		switch (object->type) {
		case TYPE_BUTTON:
			lua_pushboolean(state, (BlockScriptAPI::getFlags(object) & 0x4) ? 1 : 0);
			return 1;
		default:
			return 0;
		}
	}

	int getPathMaxTime(lua_State* state) {
		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS(1);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);

		Block* object = Block::getObjectFromUserData(state, 1);
		if (object == NULL) return 0;

		switch (object->type) {
		case TYPE_MOVING_BLOCK:
		case TYPE_MOVING_SHADOW_BLOCK:
		case TYPE_MOVING_SPIKES:
			lua_pushinteger(state, object->getPathMaxTime());
			return 1;
		default:
			return 0;
		}
	}

	int getPathTime(lua_State* state) {
		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS(1);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);

		Block* object = Block::getObjectFromUserData(state, 1);
		if (object == NULL) return 0;

		switch (object->type) {
		case TYPE_MOVING_BLOCK:
		case TYPE_MOVING_SHADOW_BLOCK:
		case TYPE_MOVING_SPIKES:
			lua_pushinteger(state, BlockScriptAPI::getTemp(object));
			return 1;
		default:
			return 0;
		}
	}

	int setPathTime(lua_State* state) {
		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS(2);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);
		HELPER_CHECK_ARGS_TYPE(2, integer);

		Block* object = Block::getObjectFromUserData(state, 1);
		if (object == NULL) return 0;

		switch (object->type) {
		case TYPE_MOVING_BLOCK:
		case TYPE_MOVING_SHADOW_BLOCK:
		case TYPE_MOVING_SPIKES:
			BlockScriptAPI::setTemp(object, (int)lua_tointeger(state, 2));
			break;
		}

		return 0;
	}

	int isLooping(lua_State* state) {
		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS(1);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);

		Block* object = Block::getObjectFromUserData(state, 1);
		if (object == NULL) return 0;

		switch (object->type) {
		case TYPE_MOVING_BLOCK:
		case TYPE_MOVING_SHADOW_BLOCK:
		case TYPE_MOVING_SPIKES:
			lua_pushboolean(state, (BlockScriptAPI::getFlags(object) & 0x2) ? 0 : 1);
			return 1;
		default:
			return 0;
		}
	}

	int setLooping(lua_State* state) {
		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS(2);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);
		HELPER_CHECK_ARGS_TYPE(2, boolean);

		Block* object = Block::getObjectFromUserData(state, 1);
		if (object == NULL) return 0;

		switch (object->type) {
		case TYPE_MOVING_BLOCK:
		case TYPE_MOVING_SHADOW_BLOCK:
		case TYPE_MOVING_SPIKES:
			BlockScriptAPI::setFlags(object,
				(BlockScriptAPI::getFlags(object) & ~2) | (lua_toboolean(state, 2) ? 0 : 2)
				);
			break;
		}

		return 0;
	}

	int getSpeed(lua_State* state) {
		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS(1);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);

		Block* object = Block::getObjectFromUserData(state, 1);
		if (object == NULL) return 0;

		switch (object->type) {
		case TYPE_CONVEYOR_BELT:
		case TYPE_SHADOW_CONVEYOR_BELT:
			lua_pushinteger(state, BlockScriptAPI::getSpeed(object));
			return 1;
		default:
			return 0;
		}
	}

	int setSpeed(lua_State* state) {
		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS(2);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);
		HELPER_CHECK_ARGS_TYPE(2, integer);

		Block* object = Block::getObjectFromUserData(state, 1);
		if (object == NULL) return 0;

		switch (object->type) {
		case TYPE_CONVEYOR_BELT:
		case TYPE_SHADOW_CONVEYOR_BELT:
			BlockScriptAPI::setSpeed(object, (int)lua_tointeger(state, 2));
			break;
		}

		return 0;
	}

	int getAppearance(lua_State* state) {
		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS(1);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);

		Block* object = Block::getObjectFromUserData(state, 1);
		if (object == NULL) return 0;

		lua_pushstring(state, object->customAppearanceName.c_str());

		return 1;
	}

	int setAppearance(lua_State* state) {
		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS(2);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);
		HELPER_CHECK_ARGS_TYPE_OR_NIL(2, string);

		Block* object = Block::getObjectFromUserData(state, 1);
		if (object == NULL) return 0;

		if (lua_isnil(state, 2)) {
			object->setEditorProperty("appearance", "");
		} else {
			object->setEditorProperty("appearance", lua_tostring(state, 2));
		}

		return 0;
	}

	int getId(lua_State* state) {
		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS(1);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);

		Block* object = Block::getObjectFromUserData(state, 1);
		if (object == NULL) return 0;

		lua_pushstring(state, object->id.c_str());

		return 1;
	}

	int setId(lua_State* state) {
		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS(2);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);
		HELPER_CHECK_ARGS_TYPE_OR_NIL(2, string);

		Block* object = Block::getObjectFromUserData(state, 1);
		if (object == NULL) return 0;

		if (lua_isnil(state, 2)) {
			object->id.clear();
		} else {
			object->id = lua_tostring(state, 2);
		}

		return 0;
	}

	int getDestination(lua_State* state) {
		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS(1);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);

		Block* object = Block::getObjectFromUserData(state, 1);
		if (object == NULL) return 0;

		switch (object->type) {
		case TYPE_PORTAL:
			lua_pushstring(state, object->destination.c_str());
			return 1;
		default:
			return 0;
		}

		return 1;
	}

	int setDestination(lua_State* state) {
		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS(2);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);
		HELPER_CHECK_ARGS_TYPE_OR_NIL(2, string);

		Block* object = Block::getObjectFromUserData(state, 1);
		if (object == NULL) return 0;

		switch (object->type) {
		case TYPE_PORTAL:
			if (lua_isnil(state, 2)) {
				object->destination.clear();
			} else {
				object->destination = lua_tostring(state, 2);
			}
			break;
		}

		return 0;
	}

	int getMessage(lua_State* state) {
		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS(1);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);

		Block* object = Block::getObjectFromUserData(state, 1);
		if (object == NULL) return 0;

		switch (object->type) {
		case TYPE_NOTIFICATION_BLOCK:
			lua_pushstring(state, object->message.c_str());
			return 1;
		default:
			return 0;
		}

		return 1;
	}

	int setMessage(lua_State* state) {
		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS(2);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);
		HELPER_CHECK_ARGS_TYPE_OR_NIL(2, string);

		Block* object = Block::getObjectFromUserData(state, 1);
		if (object == NULL) return 0;

		std::string newMessage;

		switch (object->type) {
		case TYPE_NOTIFICATION_BLOCK:
			if (!lua_isnil(state, 2)) {
				newMessage = lua_tostring(state, 2);
			}
			if (newMessage != object->message) {
				object->message = newMessage;

				//Invalidate the notification texture
				if (Game* game = dynamic_cast<Game*>(currentState)) {
					game->invalidateNotificationTexture(object);
				}
			}
			break;
		}

		return 0;
	}

	int getMovingPosCount(lua_State* state) {
		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS(1);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);

		Block* object = Block::getObjectFromUserData(state, 1);
		if (object == NULL) return 0;

		switch (object->type) {
		case TYPE_MOVING_BLOCK:
		case TYPE_MOVING_SHADOW_BLOCK:
		case TYPE_MOVING_SPIKES:
			lua_pushinteger(state, BlockScriptAPI::getMovingPos(object).size());
			return 1;
		default:
			return 0;
		}
	}

	void _pushAMovingPos(lua_State* state, const SDL_Rect& r) {
		lua_createtable(state, 3, 0);

		lua_pushinteger(state, r.x);
		lua_rawseti(state, -2, 1);
		lua_pushinteger(state, r.y);
		lua_rawseti(state, -2, 2);
		lua_pushinteger(state, r.w);
		lua_rawseti(state, -2, 3);
	}

	int getMovingPos(lua_State* state) {
		//Available overloads:
		//getMovingPos()
		//getMovingPos(index)
		//getMovingPos(start, length)

		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS_RANGE(1, 3);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);
		HELPER_CHECK_OPTIONAL_ARGS_TYPE(2, integer);
		HELPER_CHECK_OPTIONAL_ARGS_TYPE(3, integer);

		Block* object = Block::getObjectFromUserData(state, 1);
		if (object == NULL) return 0;

		switch (object->type) {
		case TYPE_MOVING_BLOCK:
		case TYPE_MOVING_SHADOW_BLOCK:
		case TYPE_MOVING_SPIKES:
			break;
		default:
			return 0;
		}

		const std::vector<SDL_Rect> &movingPos = BlockScriptAPI::getMovingPos(object);
		const int m = movingPos.size();
		int start = 0, length = -1;

		if (args >= 2) start = lua_tointeger(state, 2) - 1;
		if (args >= 3) length = lua_tointeger(state, 3);

		//Length<0 means get all of remaining points
		if (length < 0) length = m - start;

		//Some sanity check
		if (start < 0) return 0;
		if (start + length > m) length = m - start;
		if (length < 0) length = 0;

		if (args == 2) {
			//Get single point

			//Sanity check
			if (start >= m) return 0;

			_pushAMovingPos(state, movingPos[start]);
		} else {
			//Get array of points

			lua_createtable(state, length, 0);

			for (int i = 0; i < length; i++) {
				_pushAMovingPos(state, movingPos[start + i]);
				lua_rawseti(state, -2, i + 1);
			}
		}

		return 1;
	}

	SDL_Rect _getAMovingPos(lua_State* state, int index) {
		SDL_Rect ret = { 0, 0, 0, 0 };

		if (lua_istable(state, index) && lua_rawlen(state, index) >= 3) {
			lua_rawgeti(state, index, 1);
			ret.x = lua_tointeger(state, -1);
			lua_pop(state, 1);
			lua_rawgeti(state, index, 2);
			ret.y = lua_tointeger(state, -1);
			lua_pop(state, 1);
			lua_rawgeti(state, index, 3);
			ret.w = lua_tointeger(state, -1);
			lua_pop(state, 1);
		}

		return ret;
	}

	void _getArrayOfMovingPos(lua_State* state, int index, std::vector<SDL_Rect>& ret, int maxLength = -1) {
		if (lua_istable(state, index)) {
			int m = lua_rawlen(state, index);
			if (maxLength >= 0 && m > maxLength) m = maxLength;
			for (int i = 0; i < m; i++) {
				lua_rawgeti(state, index, i + 1);
				ret.push_back(_getAMovingPos(state, -1));
				lua_pop(state, 1);
			}
		}
	}

	int setMovingPos(lua_State* state) {
		//Available overloads:
		//setMovingPos(array)
		//setMovingPos(index, point)
		//setMovingPos(start, length, array)

		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS_RANGE(2, 4);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);
		for (int i = 2; i < args; i++) {
			HELPER_CHECK_ARGS_TYPE(i, integer);
		}
		HELPER_CHECK_ARGS_TYPE(args, table);

		Block* object = Block::getObjectFromUserData(state, 1);
		if (object == NULL) return 0;

		switch (object->type) {
		case TYPE_MOVING_BLOCK:
		case TYPE_MOVING_SHADOW_BLOCK:
		case TYPE_MOVING_SPIKES:
			break;
		default:
			return 0;
		}

		std::vector<SDL_Rect> &movingPos = BlockScriptAPI::getMovingPos(object);

		if (args == 2) {
			//Overwrite the whole array

			movingPos.clear();
			_getArrayOfMovingPos(state, args, movingPos);
			BlockScriptAPI::invalidatePathMaxTime(object);
			return 0;
		}

		const int m = movingPos.size();
		int start = 0, length = -1;

		if (args >= 3) start = lua_tointeger(state, 2) - 1;
		if (args >= 4) length = lua_tointeger(state, 3);

		//Length<0 means set all of remaining points
		if (length < 0) length = m - start;

		//Some sanity check
		if (start < 0) return 0;
		if (start + length > m) length = m - start;
		if (length < 0) length = 0;

		if (args == 3) {
			//Set single point

			//Sanity check
			if (start >= m) return 0;

			movingPos[start] = _getAMovingPos(state, args);
			BlockScriptAPI::invalidatePathMaxTime(object);
		} else if (length > 0) {
			//Set array of points

			std::vector<SDL_Rect> newPos;
			_getArrayOfMovingPos(state, args, newPos, length);

			length = newPos.size();

			for (int i = 0; i < length; i++) {
				movingPos[start + i] = newPos[i];
			}

			if (length > 0) {
				BlockScriptAPI::invalidatePathMaxTime(object);
			}
		}

		return 0;
	}

}

#define _L block
//Array with the methods for the block library.
static const luaL_Reg blocklib_m[]={
	_FI(Valid),
	_FG(BlockById),
	_FG(BlocksById),
	_F(moveTo),
	_FGS(Location),
	_FG(BaseLocation),
	_F(growTo),
	_FGS(Size),
	_FG(Type),
	_F(changeThemeState),
	_FIS(Visible),
	_FGS(EventHandler),
	_F(onEvent),
	_FIS(Activated),
	_FIS(Automatic),
	_FGS(Behavior),
	_FGS(State),
	_FI(PlayerOn),
	_FG(PathMaxTime),
	_FGS(PathTime),
	_FIS(Looping),
	_FGS(Speed),
	_FGS(Appearance),
	_FGS(Id),
	_FGS(Destination),
	_FGS(Message),
	_FG(MovingPosCount),
	_FGS(MovingPos),
	{ NULL, NULL }
};
#undef _L

int luaopen_block(lua_State* state){
	luaL_newlib(state,blocklib_m);
	
	//Create the metatable for the block userdata.
	luaL_newmetatable(state,"block");

	lua_pushstring(state,"__index");
	lua_pushvalue(state,-2);
	lua_settable(state,-3);

	Block::registerMetatableFunctions(state,-3);

	//Register the functions and methods.
	luaL_setfuncs(state,blocklib_m,0);
	return 1;
}

//////////////////////////PLAYER SPECIFIC///////////////////////////

class PlayerScriptAPI {
public:
	static bool isInAir(Player* player) {
		return player->inAir;
	}
	static bool canMode(Player* player) {
		return player->canMove;
	}
	static bool isDead(Player* player) {
		return player->dead;
	}
	static bool isHoldingOther(Player* player) {
		return player->holdingOther;
	}
};

struct PlayerUserDatum{
	char sig1,sig2,sig3,sig4;
};

Player* getPlayerFromUserData(lua_State* state,int idx){
	PlayerUserDatum* ud=(PlayerUserDatum*)lua_touserdata(state,1);
	//Make sure the user datum isn't null.
	if(!ud) return NULL;

	//Get the game state.
	Game* game=dynamic_cast<Game*>(currentState);
	if(game==NULL) return NULL;

	Player* player=NULL;

	//Check the signature to see if it's the player or the shadow.
	if(ud->sig1=='P' && ud->sig2=='L' && ud->sig3=='Y' && ud->sig4=='R')
		player=&game->player;
	else if(ud->sig1=='S' && ud->sig2=='H' && ud->sig3=='D' && ud->sig4=='W')
		player=&game->shadow;
	
	return player;
}

namespace playershadow {

	int getLocation(lua_State* state){
		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS(1);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);

		Player* player = getPlayerFromUserData(state, 1);
		if (player == NULL) return 0;

		//Get the object.
		lua_pushinteger(state, player->getBox().x);
		lua_pushinteger(state, player->getBox().y);
		return 2;
	}

	int setLocation(lua_State* state){
		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS(3);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);
		HELPER_CHECK_ARGS_TYPE(2, integer);
		HELPER_CHECK_ARGS_TYPE(3, integer);

		//Get the player.
		Player* player = getPlayerFromUserData(state, 1);
		if (player == NULL) return 0;

		//Get the new location.
		int x = lua_tointeger(state, 2);
		int y = lua_tointeger(state, 3);
		player->setLocation(x, y);

		return 0;
	}

	int jump(lua_State* state){
		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS_2(1, 2);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);
		HELPER_CHECK_OPTIONAL_ARGS_TYPE(2, integer);

		//Get the player.
		Player* player = getPlayerFromUserData(state, 1);
		if (player == NULL) return 0;

		//Get the new location.
		if (args == 2){
			int yVel = lua_tointeger(state, 2);
			player->jump(yVel);
		} else{
			//Use default jump strength.
			player->jump();
		}

		return 0;
	}

	int isShadow(lua_State* state){
		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS(1);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);

		Player* player = getPlayerFromUserData(state, 1);
		if (player == NULL) return 0;

		lua_pushboolean(state, player->isShadow());
		return 1;
	}

	int getCurrentStand(lua_State* state){
		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS(1);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);

		Player* player = getPlayerFromUserData(state, 1);
		if (player == NULL) return 0;

		//Get the actual game object.
		Block* object = player->getObjCurrentStand();
		if (object == NULL){
			return 0;
		}

		//Create the userdatum.
		object->createUserData(state, "block");

		//We return one object, the userdatum.
		return 1;
	}

	int isInAir(lua_State* state){
		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS(1);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);

		Player* player = getPlayerFromUserData(state, 1);
		if (player == NULL) return 0;

		lua_pushboolean(state, PlayerScriptAPI::isInAir(player));
		return 1;
	}

	int canMove(lua_State* state){
		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS(1);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);

		Player* player = getPlayerFromUserData(state, 1);
		if (player == NULL) return 0;

		lua_pushboolean(state, PlayerScriptAPI::canMode(player));
		return 1;
	}

	int isDead(lua_State* state){
		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS(1);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);

		Player* player = getPlayerFromUserData(state, 1);
		if (player == NULL) return 0;

		lua_pushboolean(state, PlayerScriptAPI::isDead(player));
		return 1;
	}

	int isHoldingOther(lua_State* state){
		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS(1);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);

		Player* player = getPlayerFromUserData(state, 1);
		if (player == NULL) return 0;

		lua_pushboolean(state, PlayerScriptAPI::isHoldingOther(player));
		return 1;
	}
}

#define _L playershadow
//Array with the methods for the player and shadow library.
static const luaL_Reg playerlib_m[]={
	_FGS(Location),
	_F(jump),
	_FI(Shadow),
	_FG(CurrentStand),
	_FI(InAir),
	_F(canMove),
	_FI(Dead),
	_FI(HoldingOther),
	{ NULL, NULL }
};
#undef _L

int luaopen_player(lua_State* state){
	luaL_newlib(state,playerlib_m);

	//Create the metatable for the player userdata.
	luaL_newmetatable(state,"player");

	lua_pushstring(state,"__index");
	lua_pushvalue(state,-2);
	lua_settable(state,-3);

	//Now create two default player user data, one for the player and one for the shadow.
	PlayerUserDatum* ud=(PlayerUserDatum*)lua_newuserdata(state,sizeof(PlayerUserDatum));
	ud->sig1='P';ud->sig2='L';ud->sig3='Y';ud->sig4='R';
	luaL_getmetatable(state,"player");
	lua_setmetatable(state,-2);
	lua_setglobal(state,"player");

	ud=(PlayerUserDatum*)lua_newuserdata(state,sizeof(PlayerUserDatum));
	ud->sig1='S';ud->sig2='H';ud->sig3='D';ud->sig4='W';
	luaL_getmetatable(state,"player");
	lua_setmetatable(state,-2);
	lua_setglobal(state,"shadow");

	//Register the functions and methods.
	luaL_setfuncs(state,playerlib_m,0);
	return 1;
}

//////////////////////////LEVEL SPECIFIC///////////////////////////

namespace level {

	int getSize(lua_State* state){
		//NOTE: this function accepts 0 arguments, but we ignore the argument count.

		//Returns level size.
		lua_pushinteger(state, LEVEL_WIDTH);
		lua_pushinteger(state, LEVEL_HEIGHT);
		return 2;
	}

	int getWidth(lua_State* state){
		//NOTE: this function accepts 0 arguments, but we ignore the argument count.

		//Returns level size.
		lua_pushinteger(state, LEVEL_WIDTH);
		return 1;
	}

	int getHeight(lua_State* state){
		//NOTE: this function accepts 0 arguments, but we ignore the argument count.

		//Returns level size.
		lua_pushinteger(state, LEVEL_HEIGHT);
		return 1;
	}

	int getName(lua_State* state){
		//NOTE: this function accepts 0 arguments, but we ignore the argument count.

		//Check if the currentState is the game state.
		Game* game = dynamic_cast<Game*>(currentState);
		if (game == NULL) return 0;

		//Returns level name.
		lua_pushstring(state, game->getLevelName().c_str());
		return 1;
	}

	int getEventHandler(lua_State* state){
		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS(1);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE(1, string);

		//Check if the currentState is the game state.
		Game* game = dynamic_cast<Game*>(currentState);
		if (game == NULL) return 0;

		//Check event type
		string eventType = lua_tostring(state, 1);
		map<string, int>::iterator it = Game::levelEventNameMap.find(eventType);
		if (it == Game::levelEventNameMap.end()) return 0;

		//Check compiled script
		map<int, int>::iterator script = game->compiledScripts.find(it->second);
		if (script == game->compiledScripts.end()) return 0;

		//Get event handler
		lua_rawgeti(state, LUA_REGISTRYINDEX, script->second);
		return 1;
	}

	//It will return old event handler.
	int setEventHandler(lua_State* state){
		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS(2);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE(1, string);
		HELPER_CHECK_ARGS_TYPE_OR_NIL(2, function);

		//Check if the currentState is the game state.
		Game* game = dynamic_cast<Game*>(currentState);
		if (game == NULL) return 0;

		//Check event type
		string eventType = lua_tostring(state, 1);
		map<string, int>::const_iterator it = Game::levelEventNameMap.find(eventType);
		if (it == Game::levelEventNameMap.end()){
			lua_pushfstring(state, "Unknown level event type: '%s'.", eventType.c_str());
			return lua_error(state);
		}

		//Check compiled script
		int scriptIndex = LUA_REFNIL;
		{
			map<int, int>::iterator script = game->compiledScripts.find(it->second);
			if (script != game->compiledScripts.end()) scriptIndex = script->second;
		}

		//Set new event handler
		game->compiledScripts[it->second] = luaL_ref(state, LUA_REGISTRYINDEX);

		if (scriptIndex == LUA_REFNIL) return 0;

		//Get old event handler and unreference it
		lua_rawgeti(state, LUA_REGISTRYINDEX, scriptIndex);
		luaL_unref(state, LUA_REGISTRYINDEX, scriptIndex);
		return 1;
	}

	int win(lua_State* state){
		//NOTE: this function accepts 0 arguments, but we ignore the argument count.

		//Check if the currentState is the game state.
		if (stateID == STATE_LEVEL_EDITOR)
			return 0;
		Game* game = dynamic_cast<Game*>(currentState);
		if (game == NULL) return 0;

		game->won = true;
		return 0;
	}

	int getTime(lua_State* state){
		//NOTE: this function accepts 0 arguments, but we ignore the argument count.

		//Check if the currentState is the game state.
		Game* game = dynamic_cast<Game*>(currentState);
		if (game == NULL) return 0;

		//Returns level size.
		lua_pushinteger(state, game->time);
		return 1;
	}

	int getRecordings(lua_State* state){
		//NOTE: this function accepts 0 arguments, but we ignore the argument count.

		//Check if the currentState is the game state.
		Game* game = dynamic_cast<Game*>(currentState);
		if (game == NULL) return 0;

		//Returns level size.
		lua_pushinteger(state, game->recordings);
		return 1;
	}

	int broadcastObjectEvent(lua_State* state) {
		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS_RANGE(1, 4);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE(1, string);
		HELPER_CHECK_OPTIONAL_ARGS_TYPE_OR_NIL(2, string);
		HELPER_CHECK_OPTIONAL_ARGS_TYPE_OR_NIL(3, string);
		HELPER_CHECK_OPTIONAL_ARGS_TYPE_OR_NIL_NO_HINT(4, userdata);

		//Check event type
		int eventType = 0;
		{
			string s = lua_tostring(state, 1);
			auto it = Game::gameObjectEventNameMap.find(s);
			if (it == Game::gameObjectEventNameMap.end()){
				lua_pushfstring(state, "Unknown block event type: '%s'.", s.c_str());
				return lua_error(state);
			} else {
				eventType = it->second;
			}
		}

		//Check object type
		int objType = -1;
		if (args >= 2 && lua_isstring(state, 2)) {
			string s = lua_tostring(state, 2);
			auto it = Game::blockNameMap.find(s);
			if (it == Game::blockNameMap.end()){
				lua_pushfstring(state, "Unknown object type: '%s'.", s.c_str());
				return lua_error(state);
			} else {
				objType = it->second;
			}
		}

		//Check id
		const char* id = NULL;
		if (args >= 3 && lua_isstring(state, 3)) {
			id = lua_tostring(state, 3);
		}

		//Check target
		Block *target = NULL;
		if (args >= 4) {
			target = Block::getObjectFromUserData(state, 4);
		}

		//Check if the currentState is the game state.
		Game* game = dynamic_cast<Game*>(currentState);
		if (game == NULL) return 0;

		game->broadcastObjectEvent(eventType, objType, id, target);

		return 0;
	}

}

#define _L level
//Array with the methods for the level library.
static const luaL_Reg levellib_m[]={
	_FG(Size),
	_FG(Width),
	_FG(Height),
	_FG(Name),
	_FGS(EventHandler),
	_F(win),
	_FG(Time),
	_FG(Recordings),
	_F(broadcastObjectEvent),
	{NULL,NULL}
};
#undef _L

int luaopen_level(lua_State* state){
	luaL_newlib(state,levellib_m);
	
	//Register the functions and methods.
	luaL_setfuncs(state,levellib_m,0);
	return 1;
}

/////////////////////////CAMERA SPECIFIC///////////////////////////

//FIXME: I can't define namespace camera since there is already a global variable named camera.
//Therefore I use struct camera for a workaround.

struct camera {

	static int setMode(lua_State* state){
		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS(1);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE(1, string);

		string mode = lua_tostring(state, 1);

		//Get the game for setting the camera.
		Game* game = dynamic_cast<Game*>(currentState);
		if (game == NULL) return 0;
		//Check which mode.
		if (mode == "player"){
			game->cameraMode = Game::CAMERA_PLAYER;
		} else if (mode == "shadow"){
			game->cameraMode = Game::CAMERA_SHADOW;
		} else{
			//Unknown OR invalid camera mode.
			return luaL_error(state, "Unknown or invalid camera mode for %s: '%s'.", __FUNCTION__, mode.c_str());
		}

		//Returns nothing.
		return 0;
	}

	static int lookAt(lua_State* state){
		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS(2);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE(1, integer);
		HELPER_CHECK_ARGS_TYPE(2, integer);

		//Get the point.
		int x = lua_tointeger(state, 1);
		int y = lua_tointeger(state, 2);

		//Get the game for setting the camera.
		Game* game = dynamic_cast<Game*>(currentState);
		if (game == NULL) return 0;
		game->cameraMode = Game::CAMERA_CUSTOM;
		game->cameraTarget.x = x;
		game->cameraTarget.y = y;

		return 0;
	}

};

#define _L camera
//Array with the methods for the camera library.
static const luaL_Reg cameralib_m[]={
	_FS(Mode),
	_F(lookAt),
	{NULL,NULL}
};
#undef _L

int luaopen_camera(lua_State* state){
	luaL_newlib(state,cameralib_m);

	//Register the functions and methods.
	luaL_setfuncs(state,cameralib_m,0);
	return 1;
}

/////////////////////////AUDIO SPECIFIC///////////////////////////

namespace audio {

	int playSound(lua_State* state){
		//Get the number of args, this can be anything from one to three.
		HELPER_GET_AND_CHECK_ARGS_RANGE(1, 4);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE(1, string);
		HELPER_CHECK_OPTIONAL_ARGS_TYPE(2, integer);
		HELPER_CHECK_OPTIONAL_ARGS_TYPE(3, boolean);
		HELPER_CHECK_OPTIONAL_ARGS_TYPE(4, integer);

		//Default values for concurrent and force.
		//See SoundManager.h
		int concurrent = -1;
		bool force = false;
		int fadeMusic = -1;

		//If there's a second one it should be an integer.
		if (args > 1){
			concurrent = lua_tointeger(state, 2);
		}
		//If there's a third one it should be a boolean.
		if (args > 2){
			force = lua_toboolean(state, 3);
		}

		if (args > 3){
			fadeMusic = lua_tointeger(state, 4);
		}

		//Get the name of the sound.
		string sound = lua_tostring(state, 1);
		//Try to play the sound.
		int channel = getSoundManager()->playSound(sound, concurrent, force, fadeMusic);

		//Returns whether the operation is successful.
		lua_pushboolean(state, channel >= 0 ? 1 : 0);
		return 1;
	}

	int playMusic(lua_State* state){
		//Get the number of args, this can be either one or two.
		HELPER_GET_AND_CHECK_ARGS_2(1, 2);

		//Make sure the first argument is a string.
		HELPER_CHECK_ARGS_TYPE(1, string);
		HELPER_CHECK_OPTIONAL_ARGS_TYPE(2, boolean);

		//Default value of fade for playMusic.
		//See MusicManager.h.
		bool fade = true;

		//If there's a second one it should be a boolean.
		if (args > 1){
			fade = lua_toboolean(state, 2);
		}

		//Get the name of the music.
		string music = lua_tostring(state, 1);
		//Try to switch to the new music.
		getMusicManager()->playMusic(music, fade);

		//Returns nothing.
		return 0;
	}

	int pickMusic(lua_State* state){
		//NOTE: this function accepts 0 arguments, but we ignore the argument count.

		//Let the music manager pick a song from the current music list.
		getMusicManager()->pickMusic();
		return 0;
	}

	int setMusicList(lua_State* state){
		//Get the number of args, this MUST be one.
		HELPER_GET_AND_CHECK_ARGS(1);

		//Make sure the given argument is a string.
		HELPER_CHECK_ARGS_TYPE(1, string);

		//And set the music list in the music manager.
		string list = lua_tostring(state, 1);
		getMusicManager()->setMusicList(list);
		return 0;
	}

	int getMusicList(lua_State* state){
		//NOTE: this function accepts 0 arguments, but we ignore the argument count.

		//Return the name of the song (contains list prefix).
		lua_pushstring(state, getMusicManager()->getCurrentMusicList().c_str());
		return 1;
	}


	int currentMusic(lua_State* state){
		//NOTE: this function accepts 0 arguments, but we ignore the argument count.

		//Return the name of the song (contains list prefix).
		lua_pushstring(state, getMusicManager()->getCurrentMusic().c_str());
		return 1;
	}

}

#define _L audio
//Array with the methods for the audio library.
static const luaL_Reg audiolib_m[]={
	_F(playSound),
	_F(playMusic),
	_F(pickMusic),
	_FGS(MusicList),
	_F(currentMusic),
	{NULL,NULL}
};
#undef _L

int luaopen_audio(lua_State* state){
	luaL_newlib(state,audiolib_m);

	//Register the functions and methods.
	luaL_setfuncs(state,audiolib_m,0);
	return 1;
}

/////////////////////////DELAY EXECUTION SPECIFIC///////////////////////////

namespace delayExecution {

	HELPER_REGISTER_IS_VALID_FUNCTION(ScriptDelayExecution);

	int schedule(lua_State* state) {
		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS_AT_LEAST(2);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE_OR_NIL(1, function);
		HELPER_CHECK_ARGS_TYPE(2, integer);
		HELPER_CHECK_OPTIONAL_ARGS_TYPE_OR_NIL(3, integer);
		HELPER_CHECK_OPTIONAL_ARGS_TYPE_OR_NIL(4, integer);
		HELPER_CHECK_OPTIONAL_ARGS_TYPE_OR_NIL(5, boolean);

		//Check if the currentState is the game state.
		Game* game = dynamic_cast<Game*>(currentState);
		if (game == NULL) return 0;

		//Create the delay execution object.
		ScriptDelayExecution *obj = new ScriptDelayExecution(game->getScriptExecutor()->getDelayExecutionList());
		obj->setActive();

		obj->time = (int)lua_tointeger(state, 2);
		obj->repeatCount = (args >= 3 && lua_isnumber(state, 3)) ? (int)lua_tointeger(state, 3) : 1;
		obj->repeatInterval = (args >= 4 && lua_isnumber(state, 4)) ? (int)lua_tointeger(state, 4) : obj->time;
		obj->enabled = ((args >= 5 && lua_isboolean(state, 5)) ? lua_toboolean(state, 5) : 1) != 0;

		//Get arguments.
		for (int i = 6; i <= args; i++) {
			obj->arguments.push_back(luaL_ref(state, LUA_REGISTRYINDEX));
		}
		std::reverse(obj->arguments.begin(), obj->arguments.end());

		//Get the function.
		lua_settop(state, 1);
		obj->func = luaL_ref(state, LUA_REGISTRYINDEX);

		//Create the userdatum.
		obj->createUserData(state, "delayExecution");

		//We return one object, the userdatum.
		return 1;
	}

	int cancel(lua_State* state){
		HELPER_GET_AND_CHECK_ARGS(1);

		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);

		auto object = ScriptDelayExecution::getObjectFromUserData(state, 1);
		if (object == NULL) return 0;

		//Delete the object.
		delete object;
		return 0;
	}

	int isEnabled(lua_State* state){
		HELPER_GET_AND_CHECK_ARGS(1);

		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);

		auto object = ScriptDelayExecution::getObjectFromUserData(state, 1);
		if (object == NULL) return 0;

		lua_pushboolean(state, object->enabled ? 1 : 0);
		return 1;
	}

	int setEnabled(lua_State* state) {
		HELPER_GET_AND_CHECK_ARGS(2);

		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);
		HELPER_CHECK_ARGS_TYPE(2, boolean);

		auto object = ScriptDelayExecution::getObjectFromUserData(state, 1);
		if (object == NULL) return 0;

		object->enabled = lua_toboolean(state, 2) != 0;
		return 0;
	}

	int getTime(lua_State* state){
		HELPER_GET_AND_CHECK_ARGS(1);

		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);

		auto object = ScriptDelayExecution::getObjectFromUserData(state, 1);
		if (object == NULL) return 0;

		lua_pushinteger(state, object->time);
		return 1;
	}

	int setTime(lua_State* state) {
		HELPER_GET_AND_CHECK_ARGS(2);

		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);
		HELPER_CHECK_ARGS_TYPE(2, integer);

		auto object = ScriptDelayExecution::getObjectFromUserData(state, 1);
		if (object == NULL) return 0;

		object->time = (int)lua_tointeger(state, 2);
		return 0;
	}

	int getRepeatCount(lua_State* state){
		HELPER_GET_AND_CHECK_ARGS(1);

		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);

		auto object = ScriptDelayExecution::getObjectFromUserData(state, 1);
		if (object == NULL) return 0;

		lua_pushinteger(state, object->repeatCount);
		return 1;
	}

	int setRepeatCount(lua_State* state) {
		HELPER_GET_AND_CHECK_ARGS(2);

		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);
		HELPER_CHECK_ARGS_TYPE(2, integer);

		auto object = ScriptDelayExecution::getObjectFromUserData(state, 1);
		if (object == NULL) return 0;

		object->repeatCount = (int)lua_tointeger(state, 2);
		return 0;
	}

	int getRepeatInterval(lua_State* state){
		HELPER_GET_AND_CHECK_ARGS(1);

		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);

		auto object = ScriptDelayExecution::getObjectFromUserData(state, 1);
		if (object == NULL) return 0;

		lua_pushinteger(state, object->repeatInterval);
		return 1;
	}

	int setRepeatInterval(lua_State* state) {
		HELPER_GET_AND_CHECK_ARGS(2);

		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);
		HELPER_CHECK_ARGS_TYPE(2, integer);

		auto object = ScriptDelayExecution::getObjectFromUserData(state, 1);
		if (object == NULL) return 0;

		//Set the repeat interval (should >=1).
		int i = (int)lua_tointeger(state, 2);
		if (i > 0) object->repeatInterval = i;
		return 0;
	}

	int getFunc(lua_State* state){
		HELPER_GET_AND_CHECK_ARGS(1);

		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);

		auto object = ScriptDelayExecution::getObjectFromUserData(state, 1);
		if (object == NULL) return 0;
		if (object->func == LUA_REFNIL) return 0;

		lua_rawgeti(state, LUA_REGISTRYINDEX, object->func);
		return 1;
	}

	int setFunc(lua_State* state) {
		HELPER_GET_AND_CHECK_ARGS(2);

		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);
		HELPER_CHECK_ARGS_TYPE_OR_NIL(2, function);

		auto object = ScriptDelayExecution::getObjectFromUserData(state, 1);
		if (object == NULL) return 0;

		int oldFunc = object->func;
		object->func = luaL_ref(state, LUA_REGISTRYINDEX);

		if (oldFunc == LUA_REFNIL) return 0;

		lua_rawgeti(state, LUA_REGISTRYINDEX, oldFunc);
		luaL_unref(state, LUA_REGISTRYINDEX, oldFunc);
		return 1;
	}

	int getArguments(lua_State* state){
		HELPER_GET_AND_CHECK_ARGS(1);

		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);

		auto object = ScriptDelayExecution::getObjectFromUserData(state, 1);
		if (object == NULL) return 0;

		for (int a : object->arguments) {
			lua_rawgeti(state, LUA_REGISTRYINDEX, a);
		}

		return object->arguments.size();
	}

	int setArguments(lua_State* state) {
		HELPER_GET_AND_CHECK_ARGS_AT_LEAST(1);

		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);

		auto object = ScriptDelayExecution::getObjectFromUserData(state, 1);
		if (object == NULL) return 0;

		//Remove old arguments.
		for (int a : object->arguments) {
			luaL_unref(state, LUA_REGISTRYINDEX, a);
		}
		object->arguments.clear();

		//Get arguments.
		for (int i = 2; i <= args; i++) {
			object->arguments.push_back(luaL_ref(state, LUA_REGISTRYINDEX));
		}
		std::reverse(object->arguments.begin(), object->arguments.end());

		return 0;
	}

	int getExecutionTime(lua_State* state){
		HELPER_GET_AND_CHECK_ARGS(1);

		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);

		auto object = ScriptDelayExecution::getObjectFromUserData(state, 1);
		if (object == NULL) return 0;

		lua_pushinteger(state, object->executionTime);
		return 1;
	}

	int setExecutionTime(lua_State* state) {
		HELPER_GET_AND_CHECK_ARGS(2);

		HELPER_CHECK_ARGS_TYPE_NO_HINT(1, userdata);
		HELPER_CHECK_ARGS_TYPE(2, integer);

		auto object = ScriptDelayExecution::getObjectFromUserData(state, 1);
		if (object == NULL) return 0;

		object->executionTime = (int)lua_tointeger(state, 2);
		return 0;
	}
}


#define _L delayExecution
//Array with the methods for the block library.
static const luaL_Reg delayExecutionLib_m[] = {
	_FI(Valid),
	_F(schedule),
	_F(cancel),
	_FIS(Enabled),
	_FGS(Time),
	_FGS(RepeatCount),
	_FGS(RepeatInterval),
	_FGS(Func),
	_FGS(Arguments),
	_FGS(ExecutionTime),
	{ NULL, NULL }
};
#undef _L

int luaopen_delayExecution(lua_State* state){
	luaL_newlib(state, delayExecutionLib_m);

	//Create the metatable for the delay execution userdata.
	luaL_newmetatable(state, "delayExecution");

	lua_pushstring(state, "__index");
	lua_pushvalue(state, -2);
	lua_settable(state, -3);

	ScriptDelayExecution::registerMetatableFunctions(state, -3);

	//Register the functions and methods.
	luaL_setfuncs(state, delayExecutionLib_m, 0);
	return 1;
}

/////////////////////////GETTEXT SPECIFIC///////////////////////////

namespace gettext {

	int gettext(lua_State* state){
		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS(1);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE(1, string); //msgid

		if (levels) {
			auto dm = levels->getDictionaryManager();
			if (dm) {
				lua_pushstring(state, dm->get_dictionary().translate(lua_tostring(state, 1)).c_str());
				return 1;
			}
		}

		//If we failed to find dictionay manager, we just return the original string.
		lua_pushvalue(state, 1);
		return 1;
	}

	int pgettext(lua_State* state){
		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS(2);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE(1, string); //msgctxt
		HELPER_CHECK_ARGS_TYPE(2, string); //msgid

		if (levels) {
			auto dm = levels->getDictionaryManager();
			if (dm) {
				lua_pushstring(state, dm->get_dictionary().translate_ctxt(lua_tostring(state, 1), lua_tostring(state, 2)).c_str());
				return 1;
			}
		}

		//If we failed to find dictionay manager, we just return the original string.
		lua_pushvalue(state, 2);
		return 1;
	}

	int ngettext(lua_State* state){
		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS(3);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE(1, string); //msgid
		HELPER_CHECK_ARGS_TYPE(2, string); //msgid_plural
		HELPER_CHECK_ARGS_TYPE(3, integer);

		if (levels) {
			auto dm = levels->getDictionaryManager();
			if (dm) {
				lua_pushstring(state, dm->get_dictionary().translate_plural(
					lua_tostring(state, 1),
					lua_tostring(state, 2),
					lua_tointeger(state, 3)
					).c_str());
				return 1;
			}
		}

		//If we failed to find dictionay manager, we just return the original string.
		if (lua_tointeger(state, 3) == 1) {
			lua_pushvalue(state, 1);
		} else {
			lua_pushvalue(state, 2);
		}
		return 1;
	}

	int npgettext(lua_State* state){
		//Check the number of arguments.
		HELPER_GET_AND_CHECK_ARGS(4);

		//Check if the arguments are of the right type.
		HELPER_CHECK_ARGS_TYPE(1, string); //msgctxt
		HELPER_CHECK_ARGS_TYPE(2, string); //msgid
		HELPER_CHECK_ARGS_TYPE(3, string); //msgid_plural
		HELPER_CHECK_ARGS_TYPE(4, integer);

		if (levels) {
			auto dm = levels->getDictionaryManager();
			if (dm) {
				lua_pushstring(state, dm->get_dictionary().translate_ctxt_plural(
					lua_tostring(state, 1),
					lua_tostring(state, 2),
					lua_tostring(state, 3),
					lua_tointeger(state, 4)
					).c_str());
				return 1;
			}
		}

		//If we failed to find dictionay manager, we just return the original string.
		if (lua_tointeger(state, 4) == 1) {
			lua_pushvalue(state, 2);
		} else {
			lua_pushvalue(state, 3);
		}
		return 1;
	}

}

#define _L gettext
static const luaL_Reg gettextlib_m[] = {
	_F(gettext),
	_F(pgettext),
	_F(ngettext),
	_F(npgettext),
	{ NULL, NULL }
};
#undef _L

int luaopen_gettext(lua_State* state){
	//Register the global shortcut function _() and __().
	luaL_loadstring(state,
		"function _(s)\n"
		"  return gettext.gettext(s)\n"
		"end\n"
		"function __(s)\n"
		"  return s\n"
		"end\n"
		);
	lua_pcall(state, 0, 0, 0);

	luaL_newlib(state, gettextlib_m);

	//Register the functions and methods.
	luaL_setfuncs(state, gettextlib_m, 0);
	return 1;
}
