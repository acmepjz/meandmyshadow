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

ScriptExecutor::ScriptExecutor(){
	//Initialize the state.
	state=lua_open();
	
	//Load the lua libraries.
	luaL_openlibs(state);
}

ScriptExecutor::~ScriptExecutor(){
	lua_close(state);
}

void ScriptExecutor::registerFunction(std::string name,lua_CFunction function){
	lua_register(state,name.c_str(),function);
}

void ScriptExecutor::executeScript(Script script){
	luaL_dostring(state,script.script);
}
