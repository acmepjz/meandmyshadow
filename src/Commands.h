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

#ifndef COMMANDS_H
#define COMMANDS_H

#include "LevelEditor.h"

#include <vector>
#include <string>
#include <map>

class Block;
class GameObject;
class LevelEditor;
class MovingPosition;

// Abstract class for commands.
class Command {
public:
	// Do the command.
    virtual void execute() = 0;
    
    // Undo the command.    
	virtual void unexecute() = 0;

	// The short description to the command.
	virtual std::string describe() = 0;
	
	// Deconstructor for derived classes.
	virtual ~Command();
};

//Class for resize level.
class ResizeLevelCommand : public Command {
private:
	LevelEditor* editor;

	//New level size.
	int newLevelWidth, newLevelHeight;

	//Differences used for map dimension changes to shift position of other objects.
	int diffx, diffy;

	//Old level size.
	int oldLevelWidth, oldLevelHeight;

public:
	ResizeLevelCommand(LevelEditor* levelEditor, int newWidth, int newHeight, int diffx, int diffy);
	virtual ~ResizeLevelCommand();

	virtual void execute();

	virtual void unexecute();

	static void resizeLevel(LevelEditor* levelEditor, int newWidth, int newHeight, int diffx, int diffy);

	// create a ResizeLevelCommand and shift the input position if any of them is outside the map area
	static ResizeLevelCommand* createAndShiftIfNecessary(LevelEditor* levelEditor, std::vector<SDL_Rect>& position);

	virtual std::string describe();
};

//Class for moving game objects.
class MoveGameObjectCommand: public Command{
private:
	LevelEditor* editor;
	
	//Object being moved.
	std::vector<GameObject*> objects;
	
	//New position.
	std::vector<SDL_Rect> newPosition;
	
	//Old position.
	std::vector<SDL_Rect> oldPosition;

	//Internal resize command. Can be NULL.
	ResizeLevelCommand* resizeCommand;

public:
	// Move or resize one object.
	MoveGameObjectCommand(LevelEditor* levelEditor, GameObject* gameObject, int x, int y, int w, int h);

	// Move a bunch of objects
	MoveGameObjectCommand(LevelEditor* levelEditor, std::vector<GameObject*>& gameObjects, int dx, int dy);

	virtual ~MoveGameObjectCommand();
	
	//Moves the object to the specified position.
	virtual void execute();
	
	//Moves object back to its previous position.
	virtual void unexecute();

	virtual std::string describe();

private:
	// initialize new/old positions, etc. of objects
	void init();
};

//Class for add/remove game objects.
class AddRemoveGameObjectCommand: public Command {
private:
	LevelEditor* editor;
	
	//Object being added or removed.
	std::vector<GameObject*> objects;

	//The layer of the object.
	std::string theLayer;
	
	//Internal resize command for object placement. Can be NULL.
	ResizeLevelCommand* resizeCommand;

	//Internal command used to remove player/shadow start if we are adding new start point. Can be NULL.
	AddRemoveGameObjectCommand* removeStartCommand;

	//The working mode.
	bool isAdd;

	//Boolean indicates if we own this object.
	bool ownObject;
	
	//A copy of the level editor's triggers. Can be NULL.
	LevelEditor::Triggers *oldTriggers;

private:
	//Adds the gameObject.
	void addGameObject();
	
	//Removes the gameObject.
	void removeGameObject();

public:
	AddRemoveGameObjectCommand(LevelEditor* levelEditor, GameObject* gameObject, bool isAdd_);
	AddRemoveGameObjectCommand(LevelEditor* levelEditor, std::vector<GameObject*>& gameObjects, bool isAdd_);

	virtual void execute();
	
	virtual void unexecute();

	virtual ~AddRemoveGameObjectCommand();

	virtual std::string describe();

private:
	// initialize new positions, etc. of objects
	void init();

	// Back up old triggers. It can be only called once.
	void backupTriggers();
};

//Class that adds a moving position to the target moving block.
class AddPathCommand : public Command{
private:
	LevelEditor* editor;
	
	//Point on the path that is being added.
	MovingPosition movePos;
	
	//Block that the path is being added to.
	Block* target;
public: 
	AddPathCommand(LevelEditor* levelEditor, Block* movingBlock, MovingPosition movingPosition);
	
	virtual ~AddPathCommand();
	
	//Add the MovingPosition.
	virtual void execute();
	
	//Remove the MovingPosition.
	virtual void unexecute();

	virtual std::string describe();
};

//Class that clears the moving position of the target moving block.
class RemovePathCommand : public Command{
private:
	LevelEditor* editor;
	
	//Block that the path is being removed from.
	Block* target;
	
	//The MovingPositions of the target to be restored when unexecuting.
	std::vector<MovingPosition> movePositions;
public:
	RemovePathCommand(LevelEditor* levelEditor, Block* targetBlock);
	
	virtual ~RemovePathCommand();
	
	//Remove the path.
	virtual void execute();
	
	//Re-add the path.
	virtual void unexecute();

	virtual std::string describe();
};

//Class that adds a link from the target object ot the selected object.
class AddLinkCommand : public Command{
private:
	LevelEditor* editor;
	
	//Object that was clicked upon when adding the link.
	GameObject* clickedObj;
	
	//Object that is linking to another.
	Block* target;
	
	//Previous link of the target block if it is a portal (portal has only 1 link).
	GameObject* oldPortalLink;
	
	//Previous destination of portal.
	std::string destination;
	
	//Previous id of target.
	std::string id;
public: 
	AddLinkCommand(LevelEditor* levelEditor, Block* linkingTrigger, GameObject* clickedObject);
	
	virtual ~AddLinkCommand();
	
	//Add the link, clicked object will get the same id as the target block.
	virtual void execute();
	
	//Remove the link.
	virtual void unexecute();

	virtual std::string describe();
};

//Class that clears the links from the target object.
class RemoveLinkCommand : public Command{
private:
	LevelEditor* editor;
	
	//Block whose links are being removed.
	Block* target;
	
	//Copy of the links being removed.
	std::vector<GameObject*> links;
	
	//Previous destination of portal.
	std::string destination;
	
	//Previous target id.
	std::string id;
public: 
	RemoveLinkCommand(LevelEditor* levelEditor, Block* targetBlock);
	
	virtual ~RemoveLinkCommand();
	
	//Remove all the links, changes the id of the object.
	virtual void execute();
	
	//Add all the links, restores the object's previous id.
	virtual void unexecute();

	virtual std::string describe();
};

//Class that modifies an editor property of an object.

class SetEditorPropertyCommand :public Command {
private:
	LevelEditor* editor;
	ImageManager& imageManager;
	SDL_Renderer& renderer;

	//Object being modified.
	GameObject* target;

	//The property being modified.
	std::string prop;

	//The new value of the property.
	std::string newValue;

	//The old value of the property.
	std::string oldValue;

	//The description of the property.
	std::string desc;

public:
	SetEditorPropertyCommand(LevelEditor* levelEditor, ImageManager& imageManager, SDL_Renderer& renderer, GameObject* targetBlock,
		const std::string& propertyName, const std::string& propertyValue, const std::string& propertyDescription);

	virtual ~SetEditorPropertyCommand();

	virtual void execute();

	virtual void unexecute();

	virtual std::string describe();

private:
	void updateCustomScenery();
};

#endif