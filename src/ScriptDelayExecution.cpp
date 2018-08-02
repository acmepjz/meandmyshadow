/*
 * Copyright (C) 2018 Me and My Shadow
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

#include "ScriptDelayExecution.h"

#include <assert.h>
#include <iostream>

ScriptDelayExecution::ScriptDelayExecution(ScriptDelayExecutionList *parent)
	: parent(parent)
	, func(LUA_REFNIL), time(0), repeatCount(0), repeatInterval(0)
	, executionTime(0)
	, enabled(true)
{
	assert(parent != NULL);

	//Link ourself to the parent.
	parent->delayExecutionObjects.push_back(this);
	index = parent->delayExecutionObjects.size() - 1;
}

ScriptDelayExecution::ScriptDelayExecution(ScriptDelayExecutionList *parent, const ScriptDelayExecution& other)
	: parent(parent)
	, func(LUA_REFNIL), time(other.time), repeatCount(other.repeatCount), repeatInterval(other.repeatInterval)
	, executionTime(other.executionTime)
	, enabled(other.enabled)
{
	assert(parent != NULL);

	lua_State *state = parent->state;

	assert(other.parent != NULL && state == other.parent->state && state != NULL);

	//Make a copy of Lua references.
	if (other.func != LUA_REFNIL) {
		lua_rawgeti(state, LUA_REGISTRYINDEX, other.func);
		func = luaL_ref(state, LUA_REGISTRYINDEX);
	}
	for (int a : other.arguments) {
		lua_rawgeti(state, LUA_REGISTRYINDEX, a);
		arguments.push_back(luaL_ref(state, LUA_REGISTRYINDEX));
	}

	//Link ourself to the parent.
	parent->delayExecutionObjects.push_back(this);
	index = parent->delayExecutionObjects.size() - 1;
}

ScriptDelayExecution::~ScriptDelayExecution() {
	//Remove Lua references.
	if (parent->state) {
		if (func != LUA_REFNIL) {
			luaL_unref(parent->state, LUA_REGISTRYINDEX, func);
		}
		for (int a : arguments) {
			if (a != LUA_REFNIL) {
				luaL_unref(parent->state, LUA_REGISTRYINDEX, a);
			}
		}
	}

	func = LUA_REFNIL;
	arguments.clear();

	//Unlink from parent.
	if (index >= 0 && index < (int)parent->delayExecutionObjects.size()) {
		assert(parent->delayExecutionObjects[index] == this);
		parent->delayExecutionObjects[index] = NULL;
	}
}

void ScriptDelayExecution::execute() {
	lua_State *state = parent->state;

	assert(state != NULL);

	//Check if reference is empty.
	if (func == LUA_REFNIL) return;

	//Get the function
	lua_rawgeti(state, LUA_REGISTRYINDEX, func);

	//Check if it's function.
	if (!lua_isfunction(state, -1)) {
		return;
	}

	//Backup the old "this"
	lua_getglobal(state, "this");
	int oldThisIndex = luaL_ref(state, LUA_REGISTRYINDEX);

	//Set the new "this" to ourself.
	createUserData(state, "delayExecution");
	lua_setglobal(state, "this");

	//Push arguments to stack.
	for (int a : arguments) {
		lua_rawgeti(state, LUA_REGISTRYINDEX, a);
	}

	//Now execute the script on the top of Lua stack.
	//WARNING: After this point we may get deleted, so don't use any member variables.
	int ret = lua_pcall(state, arguments.size(), 0, 0);

	//Restore "this" back to oringinal value.
	lua_rawgeti(state, LUA_REGISTRYINDEX, oldThisIndex);
	luaL_unref(state, LUA_REGISTRYINDEX, oldThisIndex);
	lua_setglobal(state, "this");

	//Check if there's an error.
	if (ret != LUA_OK){
		std::cerr << "LUA ERROR: " << lua_tostring(state, -1) << std::endl;
		return;
	}
}

void ScriptDelayExecution::updateTimer() {
	//Check if we are enabled.
	if (!enabled) return;

	//Sanity check for repeat interval.
	if (repeatInterval <= 0) {
		if (repeatCount != 0) repeatCount = 1;
	}

	//Check if we should delete ourself.
	if (repeatCount == 0) {
		delete this;
		return;
	}

	bool shouldExecute = (--time) <= 0;

	if (shouldExecute) {
		//Decrease the repeat count if it's not infinity.
		if (repeatCount > 0) repeatCount--;

		//Reset the timer, if repeat interval is invalid we set it to a big enough number.
		time = (repeatInterval <= 0) ? 0x40000000 : repeatInterval;

		//Increase the execution time.
		executionTime++;

		//Now execute the script on the top of Lua stack.
		//WARNING: After this point we may get deleted, so don't use any member variables.
		execute();
	}
}

ScriptDelayExecutionList::ScriptDelayExecutionList()
	: state(NULL)
{
}

ScriptDelayExecutionList::ScriptDelayExecutionList(const ScriptDelayExecutionList& other)
	: state(other.state)
{
	assert(state != NULL);

	for (auto obj : other.delayExecutionObjects) {
		if (obj) {
			//Create new object, which will be inserted in the object list automatically.
			new ScriptDelayExecution(this, *obj);
		}
	}
}

ScriptDelayExecutionList::~ScriptDelayExecutionList() {
	destroy();
}

void ScriptDelayExecutionList::destroy() {
	//This will make the code in ScriptDelayExecution::~ScriptDelayExecution() runs faster.
	decltype(delayExecutionObjects) tmp;
	std::swap(tmp, delayExecutionObjects);

	for (auto obj : tmp) {
		delete obj;
	}
}

void ScriptDelayExecutionList::updateTimer() {
	assert(state != NULL);

	//Get the number of objects we are going to process.
	//NOTE: We get this number at the beginning, since during execution new objects may come in, and we don't process newly added objects.
	int m = delayExecutionObjects.size();

	for (int i = 0; i < m; i++) {
		if (delayExecutionObjects[i]) delayExecutionObjects[i]->updateTimer();
	}

	//Now remove the deleted objects in the list.
	int j = 0;
	m = delayExecutionObjects.size();
	for (int i = 0; i < m; i++) {
		if (delayExecutionObjects[i] == NULL) {
			//We found an empty slot.
			j++;
		} else if (j > 0) {
			//We move the object to the empty slot and update the index of it.
			(delayExecutionObjects[i - j] = delayExecutionObjects[i])->index = i - j;
		}
	}

	//Resize the list if necessary.
	if (j > 0) {
		delayExecutionObjects.resize(m - j);
	}
}
