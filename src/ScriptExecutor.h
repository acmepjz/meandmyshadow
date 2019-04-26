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
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}
#include <string>
#include <vector>
class Block;
class ScriptDelayExecutionList;

struct ScriptExecutorSaveState {
	//The saved delay execution objects.
	//WARNING: Should be initialized to NULL!
	ScriptDelayExecutionList *savedDelayExecutionObjects;
};

//Class used for executing scripts.
class ScriptExecutor {
public:
	static bool enableDebugSupport;

public:
	//Constructor.
	ScriptExecutor();
	//Destructor.
	~ScriptExecutor();

	//Destroy lua state.
	void destroy();

	//Resets the lua state back to how it orignally was.
	//save: Boolean if the saved state should also be deleted. This also means recreate the Lua context.
	void reset(bool save);

	//Add a function for script to use.
	//name: The name used in the lua scripts.
	//function: Pointer to the function.
	void registerFunction(const std::string& name,lua_CFunction function);

	//Method that will compile a given script and save it to LUA_REGISTRYINDEX.
	//script: The script to compile.
	//Return value: The index in LUA_REGISTRYINDEX.
	int compileScript(const std::string& script);

	//Method that will compile a given file (only accept lua source, not bytecode) and save it to LUA_REGISTRYINDEX.
	//script: The file to compile.
	//Return value: The index in LUA_REGISTRYINDEX.
	int compileFile(const std::string& fileName);

	//Method that will execute a given script.
	//script: The script to execute.
	//origin: Pointer to the block that the script originated from.
	//Return value: The return value of script code.
	int executeScript(const std::string& script,Block* origin=NULL);
	
	//Method that will execute a script in LUA_REGISTRYINDEX.
	//scriptIndex: The script index in LUA_REGISTRYINDEX to execute.
	//origin: Pointer to the block that the script originated from.
	//Return value: The return value of script code.
	int executeScript(int scriptIndex,Block* origin=NULL);

	//Process delay execution objects.
	void processDelayExecution();

	//Get the delay execution list;
	ScriptDelayExecutionList* getDelayExecutionList();

	//Save state (for delay execution objects, etc.)
	void saveState(ScriptExecutorSaveState* o);

	//Load state (for delay execution objects, etc.)
	void loadState(ScriptExecutorSaveState* o);

	//Get the lua state.
	lua_State* getLuaState() {
		return state;
	}

protected:
	//The state that will execute the scripts.
	lua_State* state;

	//The delay execution objects.
	ScriptDelayExecutionList *delayExecutionObjects;

	//Internal function to execute a script on the top of Lua stack.
	//origin: Pointer to the block that the script originated from.
	//Return value: The return value of script code.
	int executeScriptInternal(Block* origin);
};

#endif
