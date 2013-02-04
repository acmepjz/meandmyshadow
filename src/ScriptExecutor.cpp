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
#include <iostream>
using namespace std;

ScriptExecutor::ScriptExecutor(){
	//Initialize the state.
	state=luaL_newstate();
	
	//Load the lua libraries.
	//FIXME: Only allow safe libraries/functions.
	luaopen_base(state);
	luaopen_table(state);
	luaopen_string(state);
	luaopen_math(state);

	//Load our own libraries.
	luaopen_block(state);
}

ScriptExecutor::~ScriptExecutor(){
	lua_close(state);
}

void ScriptExecutor::registerFunction(std::string name,lua_CFunction function){
	lua_register(state,name.c_str(),function);
}

void ScriptExecutor::executeScript(std::string script){
	//First make sure the stack is empty.
	lua_settop(state,0);

	//Now execute the script.
	luaL_dostring(state,script.c_str());

	//Check if there's an error.
	if(lua_gettop(state)!=0){
		cerr<<"LUA ERROR: "<<lua_tostring(state,1)<<endl;
	}
}
