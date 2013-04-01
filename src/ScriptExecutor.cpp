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
	luaL_requiref(state,"camera",luaopen_camera,1);
}

void ScriptExecutor::registerFunction(std::string name,lua_CFunction function){
	lua_register(state,name.c_str(),function);
}

void ScriptExecutor::executeScript(std::string script,Block* origin){
	//First make sure the stack is empty.
	lua_settop(state,0);

	//If the origin isn't null set it in the global scope.
	if(origin){
		origin->createUserData(state,"block");
		lua_setglobal(state,"this");
	}

	//Now execute the script.
	luaL_dostring(state,script.c_str());

	//If we set an origin set it back to nothing.
	if(origin){
		lua_pushnil(state);
		lua_setglobal(state,"this");
	}

	//Check if there's an error.
	if(lua_gettop(state)!=0){
		cerr<<"LUA ERROR: "<<lua_tostring(state,1)<<endl;
	}
}
