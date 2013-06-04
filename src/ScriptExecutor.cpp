/*
 * Copyright (C) 2012 Me and My Shadow
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

#include "ScriptExecutor.h"
#include "ScriptAPI.h"
#include <iostream>
using namespace std;

ScriptExecutor::ScriptExecutor():state(NULL){
	//NOTE: If a state is going to use the scriptExecutor it is his task to reset it.
}

ScriptExecutor::~ScriptExecutor(){
	//Make sure there is a state to close.
	if(state)
		lua_close(state);
}

void ScriptExecutor::reset(){
	//Close the lua_state, if any.
	if(state)
		lua_close(state);

	//Create a new state.
	state=luaL_newstate();

	//Now load the lua libraries.
	//FIXME: Only allow safe libraries/functions.
	luaopen_base(state);
	luaL_requiref(state,"table",luaopen_table,1);
	luaL_requiref(state,"bit32",luaopen_bit32,1);
	luaL_requiref(state,"string",luaopen_string,1);
	luaL_requiref(state,"math",luaopen_math,1);

	//Load our own libraries.
	luaL_requiref(state,"block",luaopen_block,1);
	luaL_requiref(state,"playershadow",luaopen_player,1);
	luaL_requiref(state,"level",luaopen_level,1);
	luaL_requiref(state,"game",luaopen_game,1);
	luaL_requiref(state,"camera",luaopen_camera,1);
	luaL_requiref(state,"audio",luaopen_audio,1);
}

void ScriptExecutor::registerFunction(std::string name,lua_CFunction function){
	lua_register(state,name.c_str(),function);
}

int ScriptExecutor::compileScript(std::string script){
	//First make sure the stack is empty.
	lua_settop(state,0);

	//Compile the script.
	if(luaL_loadstring(state,script.c_str())!=LUA_OK){
		cerr<<"LUA ERROR: "<<lua_tostring(state,-1)<<endl;
		return LUA_REFNIL;
	}

	//Save it to LUA_REGISTRYINDEX and return values.
	return luaL_ref(state,LUA_REGISTRYINDEX);
}

int ScriptExecutor::executeScript(std::string script,Block* origin){
	//First make sure the stack is empty.
	lua_settop(state,0);

	//Compile the script.
	if(luaL_loadstring(state,script.c_str())!=LUA_OK){
		cerr<<"LUA ERROR: "<<lua_tostring(state,-1)<<endl;
		return 0;
	}

	//Now execute the script.
	return executeScriptInternal(origin);
}

int ScriptExecutor::executeScript(int scriptIndex,Block* origin){
	//Check if reference is empty.
	if(scriptIndex==LUA_REFNIL) return 0;

	//Make sure the stack is empty.
	lua_settop(state,0);

	//Get the function
	lua_rawgeti(state,LUA_REGISTRYINDEX,scriptIndex);

	//Check if it's function and run.
	if(lua_isfunction(state,-1)){
		return executeScriptInternal(origin);
	}else{
		cerr<<"LUA ERROR: Not a function"<<endl;
		return 0;
	}
}

int ScriptExecutor::executeScriptInternal(Block* origin){
	//If the origin isn't null set it in the global scope.
	if(origin){
		origin->createUserData(state,"block");
		lua_setglobal(state,"this");
	}

	//Now execute the script on the top of Lua stack.
	int ret=lua_pcall(state,0,1,0);

	//If we set an origin set it back to nothing.
	if(origin){
		lua_pushnil(state);
		lua_setglobal(state,"this");
	}

	//Check if there's an error.
	if(ret!=LUA_OK){
		cerr<<"LUA ERROR: "<<lua_tostring(state,-1)<<endl;
		return 0;
	}

	//Get the return value.
	return lua_tonumber(state,-1);
}
