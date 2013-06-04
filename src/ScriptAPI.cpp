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
#include "Functions.h"
#include "Game.h"
#include <iostream>
using namespace std;

///////////////////////////BLOCK SPECIFIC///////////////////////////
int getBlockById(lua_State* state){
	//Get the number of args, this MUST be one.
	int args=lua_gettop(state);
	if(args!=1){
		lua_pushstring(state,"Incorrect number of arguments for getBlockById, expected 1.");
		lua_error(state);
	}
	//Make sure the given argument is an id (string).
	if(!lua_isstring(state,1)){
		lua_pushstring(state,"Invalid type for argument 1 of getBlockById, should be string.");
		lua_error(state);
	}

	//Check if the currentState is the game state.
	Game* game=dynamic_cast<Game*>(currentState);
	if(game==NULL) return 0;

	//Get the actual game object.
	string id=lua_tostring(state,1);
	std::vector<Block*>& levelObjects=game->levelObjects;
	Block* object=NULL;
	for(unsigned int i=0;i<levelObjects.size();i++){
		if(levelObjects[i]->getEditorProperty("id")==id){
			object=levelObjects[i];
			break;
		}
	}
	if(object==NULL){
		//Unable to find the requested object.
		//Return nothing, will result in a nil in the script. 
		return 0;
	}

	//Create the userdatum.
	object->createUserData(state,"block");

	//We return one object, the userdatum.
	return 1;
}

int getBlocksById(lua_State* state){
	//Get the number of args, this MUST be one.
	int args=lua_gettop(state);
	if(args!=1){
		lua_pushstring(state,"Incorrect number of arguments for getBlocksById, expected 1.");
		lua_error(state);
	}
	//Make sure the given argument is an id (string).
	if(!lua_isstring(state,1)){
		lua_pushstring(state,"Invalid type for argument 1 of getBlocksById, should be string.");
		lua_error(state);
	}

	//Check if the currentState is the game state.
	Game* game=dynamic_cast<Game*>(currentState);
	if(game==NULL) return 0;

	//Get the actual game object.
	string id=lua_tostring(state,1);
	std::vector<Block*>& levelObjects=game->levelObjects;
	std::vector<Block*> result;
	for(unsigned int i=0;i<levelObjects.size();i++){
		if(levelObjects[i]->getEditorProperty("id")==id){
			result.push_back(levelObjects[i]);
		}
	}

	//Create the table that will hold the result.
	lua_createtable(state,result.size(),0);

	//Loop through the results.
	for(unsigned int i=0;i<result.size();i++){
		//Create the userdatum.
		result[i]->createUserData(state,"block");
		//And set the table.
		lua_rawseti(state,-2,i+1);
	}

	//We return one object, the userdatum.
	return 1;
}

int getBlockLocation(lua_State* state){
	//Make sure there's only one argument and that argument is an userdatum.
	int args=lua_gettop(state);
	if(args!=1){
		lua_pushstring(state,"Incorrect number of arguments for getBlockLocation, expected 1.");
		lua_error(state);
	}
	if(!lua_isuserdata(state,1)){
		lua_pushstring(state,"Invalid type for argument 1 of getBlockLocation.");
		lua_error(state);
	}
	Block* object = Block::getObjectFromUserData(state,1);
	if(object==NULL) return 0;
	
	//Get the object.
	lua_pushnumber(state,object->getBox().x);
	lua_pushnumber(state,object->getBox().y);
	return 2;
}

int setBlockLocation(lua_State* state){
	//Check the number of arguments.
	int args=lua_gettop(state);

	//Make sure the number of arguments is correct.
	if(args!=3){
		lua_pushstring(state,"Incorrect number of arguments for setBlockLocation, expected 3.");
		lua_error(state);
	}
	//Check if the arguments are of the right type.
	if(!lua_isuserdata(state,1)){
		lua_pushstring(state,"Invalid type for argument 1 of setBlockLocation.");
		lua_error(state);
	}
	if(!lua_isnumber(state,2)){
		lua_pushstring(state,"Invalid type for argument 2 of setBlockLocation, should be integer.");
		lua_error(state);
	}
	if(!lua_isnumber(state,3)){
		lua_pushstring(state,"Invalid type for argument 3 of setBlockLocation, should be integer.");
		lua_error(state);
	}

	//Now get the pointer to the object.
	Block* object = Block::getObjectFromUserData(state,1);
	if(object==NULL) return 0;

	int x=lua_tonumber(state,2);
	int y=lua_tonumber(state,3);
	object->setLocation(x,y);
	
	return 0;
}

int getBlockType(lua_State* state){
	int args=lua_gettop(state);
	if(args!=1){
		lua_pushstring(state,"Incorrect number of arguments for getBlockType, expected 1.");
		lua_error(state);
	}
	if(!lua_isuserdata(state,1)){
		lua_pushstring(state,"Invalid type for argument 1 of getBlockType.");
		lua_error(state);
	}
	Block* object = Block::getObjectFromUserData(state,1);
	if(object==NULL || object->type<0 || object->type>=TYPE_MAX) return 0;

	lua_pushstring(state,Game::blockName[object->type]);
	return 1;
}

int changeBlockThemeState(lua_State* state){
	int args=lua_gettop(state);
	if(args!=2){
		lua_pushstring(state,"Incorrect number of arguments for changeBlockThemeState, expected 2.");
		lua_error(state);
	}
	if(!lua_isuserdata(state,1)){
		lua_pushstring(state,"Invalid type for argument 1 of changeBlockThemeState.");
		lua_error(state);
	}
	if(!lua_isstring(state,2)){
		lua_pushstring(state,"Invalid type for argument 2 of changeBlockThemeState, should be string.");
		lua_error(state);
	}
	Block* object = Block::getObjectFromUserData(state,1);
	object->appearance.changeState(lua_tostring(state,2));
	
	return 0;
}

int setBlockEnabled(lua_State* state){
	int args=lua_gettop(state);
	if(args!=2){
		lua_pushstring(state,"Incorrect number of arguments for setBlockEnabled, expected 2.");
		lua_error(state);
	}
	if(!lua_isuserdata(state,1)){
		lua_pushstring(state,"Invalid type for argument 1 of setBlockEnabled.");
		lua_error(state);
	}
	if(!lua_isboolean(state,2)){
		lua_pushstring(state,"Invalid type for argument 2 of setBlockEnabled, should be boolean.");
		lua_error(state);
	}
	
	Block* object = Block::getObjectFromUserData(state,1);
	if(object==NULL)
		return 0;

	bool enabled=lua_toboolean(state,2);
	object->enabled=enabled;
	
	return 0;
}

int isBlockEnabled(lua_State* state){
	int args=lua_gettop(state);
	if(args!=1){
		lua_pushstring(state,"Incorrect number of arguments for isBlockEnabled, expected 1.");
		lua_error(state);
	}
	if(!lua_isuserdata(state,1)){
		lua_pushstring(state,"Invalid type for argument 1 of isBlockEnabled.");
		lua_error(state);
	}
	
	Block* object = Block::getObjectFromUserData(state,1);
	if(object==NULL)
		return 0;

	lua_pushboolean(state,object->enabled);
	return 1;
}

int getBlockEventHandler(lua_State* state){
	int args=lua_gettop(state);
	if(args!=2){
		lua_pushstring(state,"Incorrect number of arguments for getBlockEventHandler, expected 2.");
		lua_error(state);
	}
	if(!lua_isuserdata(state,1)){
		lua_pushstring(state,"Invalid type for argument 1 of getBlockEventHandler.");
		lua_error(state);
	}
	if(!lua_isstring(state,2)){
		lua_pushstring(state,"Invalid type for argument 2 of getBlockEventHandler, should be string.");
		lua_error(state);
	}

	Block* object = Block::getObjectFromUserData(state,1);
	if(object==NULL) return 0;

	//Check event type
	string eventType=lua_tostring(state,2);
	map<string,int>::iterator it=Game::gameObjectEventNameMap.find(eventType);
	if(it==Game::gameObjectEventNameMap.end()) return 0;

	//Check compiled script
	map<int,int>::iterator script=object->compiledScripts.find(it->second);
	if(script==object->compiledScripts.end()) return 0;

	//Get event handler
	lua_rawgeti(state,LUA_REGISTRYINDEX,script->second);
	return 1;
}

//It will return old event handler.
int setBlockEventHandler(lua_State* state){
	int args=lua_gettop(state);
	if(args!=3){
		lua_pushstring(state,"Incorrect number of arguments for setBlockEventHandler, expected 3.");
		lua_error(state);
	}
	if(!lua_isuserdata(state,1)){
		lua_pushstring(state,"Invalid type for argument 1 of setBlockEventHandler.");
		lua_error(state);
	}
	if(!lua_isstring(state,2)){
		lua_pushstring(state,"Invalid type for argument 2 of setBlockEventHandler, should be string.");
		lua_error(state);
	}
	if(!lua_isfunction(state,3) && !lua_isnil(state,3)){
		lua_pushstring(state,"Invalid type for argument 3 of setBlockEventHandler, should be function.");
		lua_error(state);
	}

	Block* object = Block::getObjectFromUserData(state,1);
	if(object==NULL) return 0;

	//Check event type
	string eventType=lua_tostring(state,2);
	map<string,int>::const_iterator it=Game::gameObjectEventNameMap.find(eventType);
	if(it==Game::gameObjectEventNameMap.end()){
		lua_pushfstring(state,"Unknown block event type: '%s'.",eventType.c_str());
		lua_error(state);
	}

	//Check compiled script
	int scriptIndex=LUA_REFNIL;
	{
		map<int,int>::iterator script=object->compiledScripts.find(it->second);
		if(script!=object->compiledScripts.end()) scriptIndex=script->second;
	}

	//Set new event handler
	object->compiledScripts[it->second]=luaL_ref(state,LUA_REGISTRYINDEX);

	//Get old event handler and unreference it
	lua_rawgeti(state,LUA_REGISTRYINDEX,scriptIndex);
	luaL_unref(state,LUA_REGISTRYINDEX,scriptIndex);
	return 1;
}

//Array with the methods for the block library.
static const struct luaL_Reg blocklib_m[]={
	{"getBlockById",getBlockById},
	{"getBlocksById",getBlocksById},
	{"getLocation",getBlockLocation},
	{"setLocation",setBlockLocation},
	{"getType",getBlockType},
	{"changeThemeState",changeBlockThemeState},
	{"setEnabled",setBlockEnabled},
	{"isEnabled",isBlockEnabled},
	{"getEventHandler",getBlockEventHandler},
	{"setEventHandler",setBlockEventHandler},
	{NULL,NULL}
};

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


int getPlayerLocation(lua_State* state){
	//Make sure there's only one argument and that argument is an userdatum.
	int args=lua_gettop(state);
	if(args!=1){
		lua_pushstring(state,"Incorrect number of arguments for getPlayerLocation, expected 1.");
		lua_error(state);
	}
	if(!lua_isuserdata(state,1)){
		lua_pushstring(state,"Invalid type for argument 1 of getPlayerLocation.");
		lua_error(state);
	}
	
	Player* player=getPlayerFromUserData(state,1);
	if(player==NULL) return 0;

	//Get the object.
	lua_pushnumber(state,player->getBox().x);
	lua_pushnumber(state,player->getBox().y);
	return 2;
}

int setPlayerLocation(lua_State* state){
	//Make sure there are three arguments, userdatum and two integers.
	int args=lua_gettop(state);
	if(args!=3){
		lua_pushstring(state,"Incorrect number of arguments for setPlayerLocation, expected 3.");
		lua_error(state);
	}
	//Check if the arguments are of the right type.
	if(!lua_isuserdata(state,1)){
		lua_pushstring(state,"Invalid type for argument 1 of setPlayerLocation.");
		lua_error(state);
	}
	if(!lua_isnumber(state,2)){
		lua_pushstring(state,"Invalid type for argument 2 of setPlayerLocation, should be integer.");
		lua_error(state);
	}
	if(!lua_isnumber(state,3)){
		lua_pushstring(state,"Invalid type for argument 3 of setPlayerLocation, should be integer.");
		lua_error(state);
	}

	//Get the player.
	Player* player=getPlayerFromUserData(state,1);
	if(player==NULL) return 0;

	//Get the new location.
	int x=lua_tonumber(state,2);
	int y=lua_tonumber(state,3);
	player->setLocation(x,y);
	
	return 0;
}

int setPlayerJump(lua_State* state){
	//Make sure there are three arguments, userdatum and one integers.
	int args=lua_gettop(state);
	if(args!=1 && args!=2){
		lua_pushstring(state,"Incorrect number of arguments for setPlayerJump, expected 1 or 2.");
		lua_error(state);
	}
	//Check if the arguments are of the right type.
	if(!lua_isuserdata(state,1)){
		lua_pushstring(state,"Invalid type for argument 1 of setPlayerJump.");
		lua_error(state);
	}
	if(args==2 && !lua_isnumber(state,2)){
		lua_pushstring(state,"Invalid type for argument 2 of setPlayerJump, should be integer.");
		lua_error(state);
	}

	//Get the player.
	Player* player=getPlayerFromUserData(state,1);
	if(player==NULL) return 0;

	//Get the new location.
	if(args==2){
		int yVel=lua_tonumber(state,2);
		player->jump(yVel);
	}else{
		//Use default jump strength.
		player->jump();
	}

	return 0;
}

int isPlayerShadow(lua_State* state){
	//Make sure there's only one argument and that argument is an userdatum.
	int args=lua_gettop(state);
	if(args!=1){
		lua_pushstring(state,"Incorrect number of arguments for isPlayerShadow, expected 1.");
		lua_error(state);
	}
	if(!lua_isuserdata(state,1)){
		lua_pushstring(state,"Invalid type for argument 1 of isPlayerShadow.");
		lua_error(state);
	}

	Player* player=getPlayerFromUserData(state,1);
	if(player==NULL) return 0;

	lua_pushboolean(state,player->isShadow());
	return 1;
}

int getPlayerCurrentStand(lua_State* state){
	//Get the number of args, this MUST be one.
	int args=lua_gettop(state);
	if(args!=1){
		lua_pushstring(state,"Incorrect number of arguments for getPlayerCurrentStand, expected 1.");
		lua_error(state);
	}
	//Make sure the given argument is a player userdatum.
	if(!lua_isuserdata(state,1)){
		lua_pushstring(state,"Invalid type for argument 1 of getPlayerCurrentStand.");
		lua_error(state);
	}

	Player* player=getPlayerFromUserData(state,1);
	if(player==NULL) return 0;

	//Get the actual game object.
	Block* object=player->getObjCurrentStand();
	if(object==NULL){
		return 0;
	}

	//Create the userdatum.
	object->createUserData(state,"block");

	//We return one object, the userdatum.
	return 1;
}

//Array with the methods for the player and shadow library.
static const struct luaL_Reg playerlib_m[]={
	{"getLocation",getPlayerLocation},
	{"setLocation",setPlayerLocation},
	{"jump",setPlayerJump},
	{"isShadow",isPlayerShadow},
	{"getCurrentStand",getPlayerCurrentStand},
	{NULL,NULL}
};

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

int getLevelSize(lua_State* state){
	//NOTE: this function accepts 0 arguments, but we ignore the argument count.

	//Returns level size.
	lua_pushinteger(state,LEVEL_WIDTH);
	lua_pushinteger(state,LEVEL_HEIGHT);
	return 2;
}

int getLevelWidth(lua_State* state){
	//NOTE: this function accepts 0 arguments, but we ignore the argument count.

	//Returns level size.
	lua_pushinteger(state,LEVEL_WIDTH);
	return 1;
}

int getLevelHeight(lua_State* state){
	//NOTE: this function accepts 0 arguments, but we ignore the argument count.

	//Returns level size.
	lua_pushinteger(state,LEVEL_HEIGHT);
	return 1;
}

int getLevelName(lua_State* state){
	//NOTE: this function accepts 0 arguments, but we ignore the argument count.

	//Check if the currentState is the game state.
	Game* game=dynamic_cast<Game*>(currentState);
	if(game==NULL) return 0;

	//Returns level name.
	lua_pushstring(state,game->getLevelName().c_str());
	return 1;
}

int getLevelEventHandler(lua_State* state){
	int args=lua_gettop(state);
	if(args!=1){
		lua_pushstring(state,"Incorrect number of arguments for getLevelEventHandler, expected 1.");
		lua_error(state);
	}
	if(!lua_isstring(state,1)){
		lua_pushstring(state,"Invalid type for argument 1 of getLevelEventHandler, should be string.");
		lua_error(state);
	}

	//Check if the currentState is the game state.
	Game* game=dynamic_cast<Game*>(currentState);
	if(game==NULL) return 0;

	//Check event type
	string eventType=lua_tostring(state,1);
	map<string,int>::iterator it=Game::levelEventNameMap.find(eventType);
	if(it==Game::levelEventNameMap.end()) return 0;

	//Check compiled script
	map<int,int>::iterator script=game->compiledScripts.find(it->second);
	if(script==game->compiledScripts.end()) return 0;

	//Get event handler
	lua_rawgeti(state,LUA_REGISTRYINDEX,script->second);
	return 1;
}

//It will return old event handler.
int setLevelEventHandler(lua_State* state){
	int args=lua_gettop(state);
	if(args!=2){
		lua_pushstring(state,"Incorrect number of arguments for setLevelEventHandler, expected 2.");
		lua_error(state);
	}
	if(!lua_isstring(state,1)){
		lua_pushstring(state,"Invalid type for argument 1 of setLevelEventHandler, should be string.");
		lua_error(state);
	}
	if(!lua_isfunction(state,2) && !lua_isnil(state,2)){
		lua_pushstring(state,"Invalid type for argument 2 of setLevelEventHandler, should be function.");
		lua_error(state);
	}

	//Check if the currentState is the game state.
	Game* game=dynamic_cast<Game*>(currentState);
	if(game==NULL) return 0;

	//Check event type
	string eventType=lua_tostring(state,1);
	map<string,int>::const_iterator it=Game::levelEventNameMap.find(eventType);
	if(it==Game::levelEventNameMap.end()){
		lua_pushfstring(state,"Unknown level event type: '%s'.",eventType.c_str());
		lua_error(state);
	}

	//Check compiled script
	int scriptIndex=LUA_REFNIL;
	{
		map<int,int>::iterator script=game->compiledScripts.find(it->second);
		if(script!=game->compiledScripts.end()) scriptIndex=script->second;
	}

	//Set new event handler
	game->compiledScripts[it->second]=luaL_ref(state,LUA_REGISTRYINDEX);

	//Get old event handler and unreference it
	lua_rawgeti(state,LUA_REGISTRYINDEX,scriptIndex);
	luaL_unref(state,LUA_REGISTRYINDEX,scriptIndex);
	return 1;
}

//Array with the methods for the level library.
static const struct luaL_Reg levellib_m[]={
	{"getSize",getLevelSize},
	{"getWidth",getLevelWidth},
	{"getHeight",getLevelHeight},
	{"getName",getLevelName},
	{"getEventHandler",getLevelEventHandler},
	{"setEventHandler",setLevelEventHandler},
	{NULL,NULL}
};

int luaopen_level(lua_State* state){
	luaL_newlib(state,levellib_m);
	
	//Register the functions and methods.
	luaL_setfuncs(state,levellib_m,0);
	return 1;
}

//////////////////////////GAME SPECIFIC///////////////////////////

int winGame(lua_State* state){
	//NOTE: this function accepts 0 arguments, but we ignore the argument count.

	//Check if the currentState is the game state.
	if(stateID==STATE_LEVEL_EDITOR)
		return 0;
	Game* game=dynamic_cast<Game*>(currentState);
	if(game==NULL) return 0;

	game->won=true;
	return 0;
}

int getGameTime(lua_State* state){
	//NOTE: this function accepts 0 arguments, but we ignore the argument count.

	//Check if the currentState is the game state.
	Game* game=dynamic_cast<Game*>(currentState);
	if(game==NULL) return 0;

	//Returns level size.
	lua_pushinteger(state,game->time);
	return 1;
}

int getGameRecordings(lua_State* state){
	//NOTE: this function accepts 0 arguments, but we ignore the argument count.

	//Check if the currentState is the game state.
	Game* game=dynamic_cast<Game*>(currentState);
	if(game==NULL) return 0;

	//Returns level size.
	lua_pushinteger(state,game->recordings);
	return 1;
}


//Array with the methods for the game library.
static const struct luaL_Reg gamelib_m[]={
	{"win",winGame},
	{"getTime",getGameTime},
	{"getRecordings",getGameRecordings},
	{NULL,NULL}
};

int luaopen_game(lua_State* state){
	luaL_newlib(state,gamelib_m);

	//Register the functions and methods.
	luaL_setfuncs(state,gamelib_m,0);
	return 1;
}

/////////////////////////CAMERA SPECIFIC///////////////////////////

int setCameraMode(lua_State* state){
	//Get the number of args, this MUST be one.
	int args=lua_gettop(state);
	if(args!=1){
		lua_pushstring(state,"Incorrect number of arguments for setCameraMode, expected 1.");
		lua_error(state);
	}
	//Make sure the given argument is a string.
	if(!lua_isstring(state,1)){
		lua_pushstring(state,"Invalid type for argument 1 of setCameraMode, should be string.");
		lua_error(state);
	}

	string mode=lua_tostring(state,1);
	
	//Get the game for setting the camera.
	Game* game=dynamic_cast<Game*>(currentState);
	if(game==NULL) return 0;
	//Check which mode.
	if(mode=="player"){
		game->cameraMode=Game::CAMERA_PLAYER;
	}else if(mode=="shadow"){
		game->cameraMode=Game::CAMERA_SHADOW;
	}else{
		//Unkown OR invalid camera mode.
		lua_pushfstring(state,"Unkown or invalid camera mode for setCameraMode: '%s'.",mode.c_str());
		lua_error(state);
	}

	//Returns nothing.
	return 0;
}

int cameraLookAt(lua_State* state){
	//Get the number of args, this MUST be two (x and y).
	int args=lua_gettop(state);
	if(args!=2){
		lua_pushstring(state,"Incorrect number of arguments for cameraLookAt, expected 2.");
		lua_error(state);
	}
	//Make sure the given arguments are integers.
	if(!lua_isnumber(state,1)){
		lua_pushstring(state,"Invalid type for argument 1 of cameraLookAt, should be integer.");
		lua_error(state);
	}
	if(!lua_isnumber(state,2)){
		lua_pushstring(state,"Invalid type for argument 2 of cameraLookAt, should be integer.");
		lua_error(state);
	}

	//Get the point.
	int x=lua_tonumber(state,1);
	int y=lua_tonumber(state,2);

	//Get the game for setting the camera.
	Game* game=dynamic_cast<Game*>(currentState);
	if(game==NULL) return 0;
	game->cameraMode=Game::CAMERA_CUSTOM;
	game->cameraTarget.x=x;
	game->cameraTarget.y=y;
	
	return 0;
}

//Array with the methods for the camera library.
static const struct luaL_Reg cameralib_m[]={
	{"setMode",setCameraMode},
	{"lookAt",cameraLookAt},
	{NULL,NULL}
};

int luaopen_camera(lua_State* state){
	luaL_newlib(state,cameralib_m);

	//Register the functions and methods.
	luaL_setfuncs(state,cameralib_m,0);
	return 1;
}

/////////////////////////AUDIO SPECIFIC///////////////////////////

int playSound(lua_State* state){
	//Get the number of args, this can be anything from one to three.
	int args=lua_gettop(state);
	if(args<1 || args>3){
		lua_pushstring(state,"Incorrect number of arguments for playSound, expected 1-3.");
		lua_error(state);
	}
	//Make sure the first argument is a string.
	if(!lua_isstring(state,1)){
		lua_pushstring(state,"Invalid type for argument 1 of playSound, should be string.");
		lua_error(state);
	}

	//Default values for concurrent and force.
	//See SoundManager.h
	int concurrent=1;
	bool force=false;
	
	//If there's a second one it should be an integer.
	if(args>1){
		if(!lua_isnumber(state,2)){
			lua_pushstring(state,"Invalid type for argument 2 of playSound, should be integer.");
			lua_error(state);
		}else{
			concurrent=lua_tonumber(state,2);
		}
	}
	//If there's a third one it should be a boolean.
	if(args>2){
		if(!lua_isboolean(state,3)){
			lua_pushstring(state,"Invalid type for argument 3 of playSound, should be boolean.");
			lua_error(state);
		}else{
			force=lua_toboolean(state,3);
		}
	}

	//Get the name of the sound.
	string sound=lua_tostring(state,1);
	//Try to play the sound.
	getSoundManager()->playSound(sound,concurrent,force);

	//Returns nothing.
	return 0;
}

int playMusic(lua_State* state){
	//Get the number of args, this can be either one or two.
	int args=lua_gettop(state);
	if(args<1 || args>2){
		lua_pushstring(state,"Incorrect number of arguments for playMusic, expected 1 or 2.");
		lua_error(state);
	}
	//Make sure the first argument is a string.
	if(!lua_isstring(state,1)){
		lua_pushstring(state,"Invalid type for argument 1 of playMusic, should be string.");
		lua_error(state);
	}

	//Default value of fade for playMusic.
	//See MusicManager.h.
	bool fade=true;
	
	//If there's a second one it should be a boolean.
	if(args>1){
		if(!lua_isboolean(state,2)){
			lua_pushstring(state,"Invalid type for argument 2 of playMusic, should be boolean.");
			lua_error(state);
		}else{
			fade=lua_toboolean(state,2);
		}
	}

	//Get the name of the music.
	string music=lua_tostring(state,1);
	//Try to switch to the new music.
	getMusicManager()->playMusic(music,fade);

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
	int args=lua_gettop(state);
	if(args!=1){
		lua_pushstring(state,"Incorrect number of arguments for setMusicList, expected 1.");
		lua_error(state);
	}
	//Make sure the given argument is a string.
	if(!lua_isstring(state,1)){
		lua_pushstring(state,"Invalid type for argument 1 of setMusicList, should be string.");
		lua_error(state);
	}

	//And set the music list in the music manager.
	string list=lua_tostring(state,1);
	getMusicManager()->setMusicList(list);
	return 0;
}

int getMusicList(lua_State* state){
	//NOTE: this function accepts 0 arguments, but we ignore the argument count.

	//Return the name of the song (contains list prefix).
	lua_pushstring(state,getMusicManager()->getCurrentMusicList().c_str());
	return 1;
}


int currentMusicPlaying(lua_State* state){
	//NOTE: this function accepts 0 arguments, but we ignore the argument count.
	
	//Return the name of the song (contains list prefix).
	lua_pushstring(state,getMusicManager()->getCurrentMusic().c_str());
	return 1;
}

//Array with the methods for the audio library.
static const struct luaL_Reg audiolib_m[]={
	{"playSound",playSound},
	{"playMusic",playMusic},
	{"pickMusic",pickMusic},
	{"setMusicList",setMusicList},
	{"getMusicList",getMusicList},
	{"currentMusic",currentMusicPlaying},
	{NULL,NULL}
};

int luaopen_audio(lua_State* state){
	luaL_newlib(state,audiolib_m);

	//Register the functions and methods.
	luaL_setfuncs(state,audiolib_m,0);
	return 1;
}
