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

#ifndef SCRIPTEXECUTOR_H
#define SCRIPTEXECUTOR_H

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}
#include <string>

//Method for loading the block library.
int luaopen_block(lua_State* state);

//Class used for executing scripts.
class ScriptExecutor{
public:
	//Constructor.
	ScriptExecutor();
	//Destructor.
	~ScriptExecutor();

	//Add a function for script to use.
	//name: The name used in the lua scripts.
	//function: Pointer to the function.
	void registerFunction(std::string name,lua_CFunction function);
	
	//Method that will execute a given script.
	//script: The script to execute.
	void executeScript(std::string script);
private:
	//The state that will execute the scripts.
	lua_State* state;
};

#endif
