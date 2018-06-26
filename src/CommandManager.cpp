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
* Me And My Shadow is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Me and My Shadow.  If not, see <http://www.gnu.org/licenses/>.
*/

// An undo/redo system based on <http://www.codeproject.com/Articles/2500/A-Basic-Undo-Redo-Framework-For-C>.
// Originally written by squarecross <https://forum.freegamedev.net/viewtopic.php?f=48&t=5432>.

#include "CommandManager.h"
#include "Commands.h"
#include "Functions.h"

#include "libs/tinyformat/tinyformat.h"

#include <iostream>

bool CommandManager::canUndo() const {
	return currentCommand > 0;
}

bool CommandManager::canRedo() const {
	return currentCommand < (int)undoList.size();
}

std::string CommandManager::describeUndo() {
	if (canUndo()) {
		return tfm::format(_("Undo %s"), undoList[currentCommand - 1]->describe());
	} else {
		return _("Can't undo");
	}
}

std::string CommandManager::describeRedo() {
	if (canRedo()) {
		return tfm::format(_("Redo %s"), undoList[currentCommand]->describe());
	} else {
		return _("Can't redo");
	}
}

int CommandManager::getUndoLevel() const {
	return undoLevel;
}

void CommandManager::setUndoLevel(int newValue) {
	undoLevel = newValue;

	// check if the current undo list is too big
	const int n = undoList.size() - undoLevel;
	if (n > 0) {
		currentCommand -= n;
		pivot = (pivot < n) ? -1 : (pivot - n);

		for (int i = 0; i < n; i++) {
			delete undoList[i];
		}

		undoList.erase(undoList.begin(), undoList.begin() + n);
	}
}

bool CommandManager::isChanged() const {
	return currentCommand != pivot;
}

void CommandManager::doCommand(Command* command) {
	// clear the redo list
	if (currentCommand < (int)undoList.size()) {
		if (pivot > currentCommand) pivot = -1;

		for (int i = currentCommand; i < (int)undoList.size(); i++) {
			delete undoList[i];
		}

		undoList.erase(undoList.begin() + currentCommand, undoList.end());
	}

	// debug
#ifdef _DEBUG
	std::cout << "Do command: " << command->describe() << std::endl;
#endif
	
	// Execute the command and add it to undo list
	command->execute();
	undoList.push_back(command);
	currentCommand++;

	// check if the current undo list is too big
	setUndoLevel(undoLevel);
}

void CommandManager::undo() {
	if (canUndo()) {
		//Move the pointer
		currentCommand--;

		//Gets the command in undoList.
		Command* command = undoList[currentCommand];

		// debug
#ifdef _DEBUG
		std::cout << "Undo command: " << command->describe() << std::endl;
#endif

		//undoing the command
		command->unexecute();
	}
}

void CommandManager::redo() {
	if (canRedo()) {
		//Gets the command in undoList.
		Command* command = undoList[currentCommand];

		// debug
#ifdef _DEBUG
		std::cout << "Redo command: " << command->describe() << std::endl;
#endif

		//redoing the command
		command->execute();

		//Move the pointer
		currentCommand++;
	}
}

void CommandManager::destroy() {
	for (int i = 0; i < (int)undoList.size(); i++) {
		delete undoList[i];
	}
	undoList.clear();

	currentCommand = 0;
	pivot = -1;
}

void CommandManager::resetChange() {
	pivot = currentCommand;
}
