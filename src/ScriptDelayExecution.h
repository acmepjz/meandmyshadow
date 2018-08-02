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

#ifndef SCRIPTDELAYEXECUTION_H
#define SCRIPTDELAYEXECUTION_H

#include <vector>

#include "ScriptUserData.h"

class ScriptDelayExecutionList;

class ScriptDelayExecution : public ScriptUserClass<'D', 'L', 'E', 'X', ScriptDelayExecution> {
public:
	//The script executor.
	ScriptDelayExecutionList *parent;

	//The index of this object in the delay execution list.
	int index;

	//The function to be executed, stored in LUA_REGISTRYINDEX.
	int func;

	//The arguments, stored in LUA_REGISTRYINDEX.
	std::vector<int> arguments;

	//The remaining time until the next execution.
	int time;

	//The number of times the function will be executed.
	int repeatCount;

	//The repeat interval.
	int repeatInterval;

	//The execution time.
	int executionTime;

	//Enabled of a delay execution. A disabled one will not count down its timer.
	bool enabled;

	/*//These are used when properties are changed during function execution.
	bool timeChanged, repeatCountChanged;*/

public:
	//Construct an empty delay execution object and insert to the script executor.
	ScriptDelayExecution(ScriptDelayExecutionList *parent);

	//Construct a delay execution object from existing one and insert to the script executor.
	ScriptDelayExecution(ScriptDelayExecutionList *parent, const ScriptDelayExecution& other);

	//Destructor.
	virtual ~ScriptDelayExecution();

	//Execute the function.
	void execute();

	//Update timer and possibly execute the function.
	void updateTimer();

private:
	ScriptDelayExecution(const ScriptDelayExecution& other) = delete;
	const ScriptDelayExecution& operator=(const ScriptDelayExecution& other) = delete;
};

class ScriptDelayExecutionList {
public:
	//The delay execution objects.
	std::vector<ScriptDelayExecution*> delayExecutionObjects;

	//The Lua state.
	lua_State *state;

public:
	//The default constructor.
	ScriptDelayExecutionList();

	//The copy constructor.
	ScriptDelayExecutionList(const ScriptDelayExecutionList& other);

	//Destructor.
	virtual ~ScriptDelayExecutionList();

	//Destroy all execution objects.
	void destroy();

	//Update timer.
	void updateTimer();

private:
	const ScriptDelayExecutionList& operator=(const ScriptDelayExecutionList& other) = delete;
};

#endif
