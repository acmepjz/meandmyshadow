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

//Class that adds/removes a moving position to the target moving block.
class AddRemovePathCommand : public Command{
private:
	LevelEditor* editor;
	
	//Point on the path that is being added.
	MovingPosition movePos;
	
	//Block that the path is being added to.
	Block* target;

	//The working mode.
	bool isAdd;
public:
	AddRemovePathCommand(LevelEditor* levelEditor, Block* movingBlock, MovingPosition movingPosition, bool isAdd_);
	
	virtual ~AddRemovePathCommand();
	
	//Add the MovingPosition.
	virtual void execute();
	
	//Remove the MovingPosition.
	virtual void unexecute();

	virtual std::string describe();

private:
	void addPath();

	void removePath();

	void setEditorData();
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

	//Previous switch/button that was linked to target (can be NULL)
	Block* oldTrigger;
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
class SetEditorPropertyCommand : public Command {
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

//Class that modifies the level settings.
class SetLevelPropertyCommand : public Command {
public:
	struct LevelProperty {
		std::string levelName;
		std::string levelTheme;
		std::string levelMusic;
		bool arcade;
		int levelTime;
		int levelRecordings;
	};

private:
	LevelEditor* editor;

	LevelProperty oldProperty;
	LevelProperty newProperty;

public:
	SetLevelPropertyCommand(LevelEditor* levelEditor, const LevelProperty& levelProperty);

	virtual ~SetLevelPropertyCommand();

	virtual void execute();

	virtual void unexecute();

	virtual std::string describe();

private:
	void setLevelProperty(const LevelProperty& levelProperty);
};

//Class that modifies the scripting for block or level
// NOTE: currently the scripting for scenery block is unsupported.
class SetScriptCommand : public Command {
private:
	LevelEditor* editor;

	//Object being modified. Can be NULL.
	Block* target;

	std::map<int, std::string> newScript;
	std::map<int, std::string> oldScript;

	//the new id for the target block.
	std::string id;

	std::string oldId;

public:
	SetScriptCommand(LevelEditor* levelEditor, Block* targetBlock, const std::map<int, std::string>& script, const std::string& id = std::string());

	virtual ~SetScriptCommand();

	virtual void execute();

	virtual void unexecute();

	virtual std::string describe();

private:
	void setScript(const std::map<int, std::string>& script, const std::string& id);
};

//Class for add/remove scenery layer.
class AddRemoveLayerCommand : public Command {
private:
	LevelEditor* editor;

	//Layer added or removed.
	SceneryLayer* layer;

	//The layer of the object.
	std::string theLayer;

	//The working mode.
	bool isAdd;

	//Boolean indicates if we own this object.
	bool ownObject;

private:
	//Adds the layer.
	void addLayer();

	//Removes the layer.
	void removeLayer();

public:
	AddRemoveLayerCommand(LevelEditor* levelEditor, const std::string& layerName, bool isAdd_);

	virtual void execute();

	virtual void unexecute();

	virtual ~AddRemoveLayerCommand();

	virtual std::string describe();
};

//Class for editing scenery layer properties (including rename scenery layer).
class SetLayerPropertyCommand : public Command {
public:
	struct LayerProperty {
		std::string name;
		float speedX, speedY;
		float cameraX, cameraY;
	};

private:
	LevelEditor* editor;

	LayerProperty oldProperty;
	LayerProperty newProperty;

public:
	SetLayerPropertyCommand(LevelEditor* levelEditor, const std::string& oldName, const LayerProperty& newProperty);

	virtual void execute();

	virtual void unexecute();

	virtual ~SetLayerPropertyCommand();

	virtual std::string describe();

private:
	void setLayerProperty(const std::string& oldName, const LayerProperty& newProperty);
};

//Class for move object between layers.
class MoveToLayerCommand : public Command {
private:
	LevelEditor* editor;

	std::vector<Scenery*> objects;

	std::string oldName;
	std::string newName;

	AddRemoveLayerCommand* createNewLayer;

public:
	MoveToLayerCommand(LevelEditor* levelEditor, std::vector<GameObject*>& gameObjects, const std::string& oldName, const std::string& newName);

	virtual void execute();

	virtual void unexecute();

	virtual ~MoveToLayerCommand();

	virtual std::string describe();

private:
	void removeGameObject();

	void addGameObject(const std::string& layer);
};

#endif
