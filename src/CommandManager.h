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

#ifndef COMMANDMANAGER_H
#define COMMANDMANAGER_H

#include <vector>

class Command;

//Class to manage command history using a redo and an undo stack.
class CommandManager {
private:
	//Keeps track of all commands that can be undone.
	std::vector<Command*> undoList;
	
	//Maximum depth of the undo stack.
	int undoLevel;

	//Point to the current undo command
	int currentCommand;

	//If current undo command equal to this then we say the document is unchanged. Can be -1.
	int pivot;

public:
	//Constructor, default undoLevel is 100. 
	CommandManager(int nUndoLevel = 100) : undoLevel(nUndoLevel), currentCommand(0), pivot(0) {}
	
	//Deconstructor: Clears the stacks.
	~CommandManager() {
		destroy();
	}

public:
	//Checks if anything is in the undo stack.
	bool canUndo() const;
	
	//Checks if anything is in the redo stack.
	bool canRedo() const;

	//A short description of the undo operation
	std::string describeUndo();

	//A short description of the redo operation
	std::string describeRedo();

	//Return  undoLevel.
	int getUndoLevel() const;
	
	//Set undoLevel.
	void setUndoLevel(int newValue);
	
	//Check if the document is changed.
	bool isChanged() const;

public:
	//Performs the given command and adds it to the undo list.
	void doCommand(Command* command);
	
	//Undo the command and add command to redo stack.
	void undo();
	
	//Redo the command and adds command to undo stack.
	void redo();
	
	//Clear the stacks.
	void destroy();
	
	//Set the changeCounter to 0.
	void resetChange();
};


#endif
