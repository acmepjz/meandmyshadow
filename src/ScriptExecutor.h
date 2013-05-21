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
#include "Block.h"
class Block;

//Class used for executing scripts.
class ScriptExecutor{
public:
	//Constructor.
	ScriptExecutor();
	//Destructor.
	~ScriptExecutor();

	//Resets the lua state back to how it orignally was.
	void reset();

	//Add a function for script to use.
	//name: The name used in the lua scripts.
	//function: Pointer to the function.
	void registerFunction(std::string name,lua_CFunction function);

	//Method that will compile a given script and save it to LUA_REGISTRYINDEX.
	//script: The script to compile.
	//Return value: The index in LUA_REGISTRYINDEX.
	int compileScript(std::string script);
	
	//Method that will execute a given script.
	//script: The script to execute.
	//origin: Pointer to the block that the script originated from.
	//Return value: The return value of script code.
	int executeScript(std::string script,Block* origin=NULL);
	
	//Method that will execute a script in LUA_REGISTRYINDEX.
	//scriptIndex: The script index in LUA_REGISTRYINDEX to execute.
	//origin: Pointer to the block that the script originated from.
	//Return value: The return value of script code.
	int executeScript(int scriptIndex,Block* origin=NULL);
private:
	//The state that will execute the scripts.
	lua_State* state;

	//Internal function to execute a script on the top of Lua stack.
	//origin: Pointer to the block that the script originated from.
	//Return value: The return value of script code.
	int executeScriptInternal(Block* origin);
};

#endif
