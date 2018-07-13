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

#include "Commands.h"
#include "GameObjects.h"
#include "Block.h"
#include "LevelEditor.h"
#include "Globals.h"
#include "Functions.h"

#include <algorithm>
#include <string>
#include <vector>
#include <map>

#include <stdio.h>

#include "libs/tinyformat/tinyformat.h"

Command::~Command() {

}

//////////////////////////////MoveGameObjectCommand///////////////////////////////////
MoveGameObjectCommand::MoveGameObjectCommand(LevelEditor* levelEditor, GameObject* gameObject, int x, int y, int w, int h)
	: editor(levelEditor), objects(1, gameObject)
	, resizeCommand(NULL)
{
	SDL_Rect r = gameObject->getBox();
	r.x = x;
	r.y = y;
	if (w >= 0) r.w = w;
	if (h >= 0) r.h = h;

	newPosition.push_back(r);

	init();
}

MoveGameObjectCommand::MoveGameObjectCommand(LevelEditor* levelEditor, std::vector<GameObject*>& gameObjects, int dx, int dy)
	: editor(levelEditor), objects(gameObjects)
	, resizeCommand(NULL)
{
	for (auto obj : objects) {
		SDL_Rect r = obj->getBox();
		r.x += dx;
		r.y += dy;
		newPosition.push_back(r);
	}

	init();
}

void MoveGameObjectCommand::init() {
	//Initialize old position.
	for (auto obj : objects) {
		SDL_Rect r = obj->getBox();
		oldPosition.push_back(r);
	}

	// Create resize command if necessary.
	resizeCommand = ResizeLevelCommand::createAndShiftIfNecessary(editor, newPosition);
}

MoveGameObjectCommand::~MoveGameObjectCommand(){
	if (resizeCommand) delete resizeCommand;
}

void MoveGameObjectCommand::execute(){
	// First resize the level if necessary.
	if (resizeCommand) resizeCommand->execute();

	// Set the obj at its new position.
	for (int i = 0; i < (int)objects.size(); i++) {
		const SDL_Rect &r = newPosition[i];
		GameObject *obj = objects[i];
		obj->setBaseLocation(r.x, r.y);
		obj->setBaseSize(r.w, r.h);
	}
}

void MoveGameObjectCommand::unexecute() {
	// First undo the resize if necessary.
	if (resizeCommand) resizeCommand->unexecute();

	// Set the obj at its old position.
	for (int i = 0; i < (int)objects.size(); i++) {
		const SDL_Rect &r = oldPosition[i];
		GameObject *obj = objects[i];
		obj->setBaseLocation(r.x, r.y);
		obj->setBaseSize(r.w, r.h);
	}
}

ResizeLevelCommand::ResizeLevelCommand(LevelEditor* levelEditor, int newWidth, int newHeight, int diffx, int diffy)
	: editor(levelEditor), newLevelWidth(newWidth), newLevelHeight(newHeight), diffx(diffx), diffy(diffy)
{
	oldLevelWidth = LEVEL_WIDTH;
	oldLevelHeight = LEVEL_HEIGHT;
}

ResizeLevelCommand::~ResizeLevelCommand() {
}

void ResizeLevelCommand::execute() {
	resizeLevel(editor, newLevelWidth, newLevelHeight, diffx, diffy);
}

void ResizeLevelCommand::unexecute() {
	resizeLevel(editor, oldLevelWidth, oldLevelHeight, -diffx, -diffy);
}

void ResizeLevelCommand::resizeLevel(LevelEditor* levelEditor, int newWidth, int newHeight, int diffx, int diffy) {
	LEVEL_WIDTH = newWidth;
	LEVEL_HEIGHT = newHeight;

	if (diffx != 0 || diffy != 0) {
		camera.x += diffx;
		camera.y += diffy;

		for (unsigned int o = 0; o < levelEditor->levelObjects.size(); o++){
			SDL_Rect r = levelEditor->levelObjects[o]->getBox();
			levelEditor->levelObjects[o]->setBaseLocation(r.x + diffx, r.y + diffy);
		}
		for (auto it = levelEditor->sceneryLayers.begin(); it != levelEditor->sceneryLayers.end(); ++it) {
			for (unsigned int o = 0; o < it->second.size(); o++){
				SDL_Rect r = it->second[o]->getBox();
				it->second[o]->setBaseLocation(r.x + diffx, r.y + diffy);
			}
		}
	}
}

ResizeLevelCommand* ResizeLevelCommand::createAndShiftIfNecessary(LevelEditor* levelEditor, std::vector<SDL_Rect>& position) {
	// Calculate new level size, shift, etc.
	int newLevelWidth = LEVEL_WIDTH;
	int newLevelHeight = LEVEL_HEIGHT;
	int diffx = 0, diffy = 0;

	for (int i = 0; i < (int)position.size(); i++) {
		const SDL_Rect &r = position[i];
		if (r.x + r.w > newLevelWidth) {
			newLevelWidth = r.x + r.w;
		}
		if (r.y + r.h > newLevelHeight) {
			newLevelHeight = r.y + r.h;
		}
		if (r.x + diffx < 0) diffx = -r.x;
		if (r.y + diffy < 0) diffy = -r.y;
	}

	newLevelWidth += diffx;
	newLevelHeight += diffy;

	if (newLevelWidth != LEVEL_WIDTH || newLevelHeight != LEVEL_HEIGHT || diffx || diffy) {
		if (diffx || diffy) {
			for (int i = 0; i < (int)position.size(); i++) {
				SDL_Rect &r = position[i];
				r.x += diffx;
				r.y += diffy;
			}
		}
		return new ResizeLevelCommand(levelEditor, newLevelWidth, newLevelHeight, diffx, diffy);
	} else {
		return NULL;
	}
}

std::string ResizeLevelCommand::describe() {
	return _("Resize level");
}

//////////////////////////////AddRemoveGameObjectCommand///////////////////////////////////
AddRemoveGameObjectCommand::AddRemoveGameObjectCommand(LevelEditor* levelEditor, GameObject* gameObject, bool isAdd_)
	: editor(levelEditor), objects(1, gameObject), isAdd(isAdd_), ownObject(isAdd_)
	, resizeCommand(NULL), removeStartCommand(NULL), oldTriggers(NULL)
{
	init();
}

AddRemoveGameObjectCommand::AddRemoveGameObjectCommand(LevelEditor* levelEditor, std::vector<GameObject*>& gameObjects, bool isAdd_)
	: editor(levelEditor), objects(gameObjects), isAdd(isAdd_), ownObject(isAdd_)
	, resizeCommand(NULL), removeStartCommand(NULL), oldTriggers(NULL)
{
	init();
}

void AddRemoveGameObjectCommand::init() {
	if (isAdd) {
		theLayer = editor->selectedLayer;

		// adjust the object position if necessary
		std::vector<SDL_Rect> position;
		for (auto obj : objects) {
			position.push_back(obj->getBox());
		}
		resizeCommand = ResizeLevelCommand::createAndShiftIfNecessary(editor, position);
		if (resizeCommand) {
			for (int i = 0; i < (int)objects.size(); i++) {
				const SDL_Rect &r = position[i];
				objects[i]->setBaseLocation(r.x, r.y);
			}
		}

		// remove the existing player/shadow start if necessary
		std::vector<bool> typesToBeRemoved((int)TYPE_MAX, false);
		bool removeSomething = false;
		for (auto obj : objects) {
			if (obj->type == TYPE_START_PLAYER || obj->type == TYPE_START_SHADOW) {
				typesToBeRemoved[obj->type] = true;
				removeSomething = true;
			}
		}
		if (removeSomething) {
			std::vector<GameObject*> objectsToBeRemoved;
			for (auto obj : editor->levelObjects) {
				if (typesToBeRemoved[obj->type]) objectsToBeRemoved.push_back(obj);
			}
			if (!objectsToBeRemoved.empty()) {
				removeStartCommand = new AddRemoveGameObjectCommand(editor, objectsToBeRemoved, false);
			}
		}
	}
}

void AddRemoveGameObjectCommand::backupTriggers() {
	if (oldTriggers == NULL) oldTriggers = new LevelEditor::Triggers(editor->triggers);
}
	
AddRemoveGameObjectCommand::~AddRemoveGameObjectCommand(){
	//Remove the objects if we own them.
	if (ownObject) {
		for (auto obj : objects) {
			delete obj;
		}
	}
	
	//Delete internal commands.
	if (resizeCommand) delete resizeCommand;
	if (removeStartCommand) delete removeStartCommand;
	
	//Delete old triggers
	if (oldTriggers) delete oldTriggers;
}
	
void AddRemoveGameObjectCommand::addGameObject(){
	// some sanity check
	assert(ownObject);

	// Remove old start position if necessary.
	if (removeStartCommand) removeStartCommand->execute();

	// Resize the level if necessary.
	if (resizeCommand) resizeCommand->execute();

	// Add each object to level
	for (int index = 0; index < (int)objects.size(); index++) {
		GameObject *obj = objects[index];

		//Increase totalCollectables everytime we add a new collectable
		if (obj->type == TYPE_COLLECTABLE) {
			editor->totalCollectables++;
		}

		Block* block = dynamic_cast<Block*>(obj);
		Scenery* scenery = dynamic_cast<Scenery*>(obj);

		//Add it to the levelObjects.
		if (theLayer.empty()) {
			assert(block != NULL);
			editor->levelObjects.push_back(block);
		} else {
			assert(scenery != NULL);
			editor->sceneryLayers[theLayer].push_back(scenery);
		}

		//GameObject type specific stuff.
		switch (obj->type){
		case TYPE_BUTTON:
		case TYPE_SWITCH:
		case TYPE_PORTAL:
		{
			//If block doesn't have an id (new object).
			if (block->getEditorProperty("id").empty()) {
				//Give it it's own id.
				char s[64];
				sprintf(s, "%u", editor->currentId);
				editor->currentId++;
				block->setEditorProperty("id", s);
			}

			break;
		}
		case TYPE_MOVING_BLOCK:
		case TYPE_MOVING_SHADOW_BLOCK:
		case TYPE_MOVING_SPIKES:
		{
			//Add the object to the moving blocks.
			vector<MovingPosition> positions;
			editor->movingBlocks[block] = positions;

			//Get the moving position.
			const vector<SDL_Rect> &movingPos = block->movingPos;

			//Add the object to the movingBlocks vector.
			editor->movingBlocks[block].clear();

			for (int i = 0, m = movingPos.size(); i < m; i++) {
				MovingPosition position(movingPos[i].x, movingPos[i].y, movingPos[i].w);
				editor->movingBlocks[block].push_back(position);
			}

			//If block doesn't have an id.
			if (block->getEditorProperty("id").empty()) {
				//Give it it's own id.
				char s[64];
				sprintf(s, "%u", editor->currentId);
				editor->currentId++;
				block->setEditorProperty("id", s);
			}

			break;
		}
		default:
			break;
		}
	}

	// now we doesn't own the object anymore.
	ownObject = false;
}
	
void AddRemoveGameObjectCommand::removeGameObject(){
	// some sanity check
	assert(!ownObject);

	//Make sure we don't access the removed object through
	//moving/linking.
	editor->movingBlock = nullptr;
	editor->linkingTrigger = nullptr;
	editor->moving = false;
	editor->linking = false;

	// Remove objects
	for (int index = 0; index < (int)objects.size(); index++) {
		GameObject *obj = objects[index];

		std::vector<GameObject*>::iterator it;
		std::map<Block*, vector<GameObject*> >::iterator mapIt;

		//Decrease totalCollectables everytime we remove a collectable
		if (obj->type == TYPE_COLLECTABLE){
			editor->totalCollectables--;
		}

		//Check if the object is in the selection.
		it = find(editor->selection.begin(), editor->selection.end(), obj);
		if (it != editor->selection.end()){
			//It is so we delete it.
			editor->selection.erase(it);
		}

		Block *theBlock = dynamic_cast<Block*>(obj);
		Scenery *theScenery = dynamic_cast<Scenery*>(obj);

		//Check if the object is in the triggers.
		if (theBlock) {
			mapIt = editor->triggers.find(theBlock);
			if (mapIt != editor->triggers.end()){
				//Back up triggers if it's remove mode.
				if (!isAdd) backupTriggers();

				//Remove the object from triggers.
				editor->triggers.erase(mapIt);
			}
		}

		//Boolean if it could be a target.
		if (obj->type == TYPE_MOVING_BLOCK || obj->type == TYPE_MOVING_SHADOW_BLOCK || obj->type == TYPE_MOVING_SPIKES
			|| obj->type == TYPE_CONVEYOR_BELT || obj->type == TYPE_SHADOW_CONVEYOR_BELT || obj->type == TYPE_PORTAL)
		{
			for (mapIt = editor->triggers.begin(); mapIt != editor->triggers.end(); ++mapIt){
				//Now loop the target vector.
				for (int o = mapIt->second.size() - 1; o >= 0; o--){
					//Check if the obj is in the target vector.
					if (mapIt->second[o] == obj){
						//Back up triggers if it's remove mode.
						if (!isAdd) backupTriggers();

						//Remove the object from triggers.
						mapIt->second.erase(mapIt->second.begin() + o);
					}
				}
			}
		}

		//Check if the object is in the movingObjects.
		if (theBlock) {
			std::map<Block*, vector<MovingPosition> >::iterator movIt;
			movIt = editor->movingBlocks.find(theBlock);
			if (movIt != editor->movingBlocks.end()){
				//It is so we remove it.
				editor->movingBlocks.erase(movIt);
			}
		}

		//Check if the block isn't being configured with a window one way or another.
		for (;;) {
			std::map<GUIObject*, GameObject*>::iterator confIt;
			for (confIt = editor->objectWindows.begin(); confIt != editor->objectWindows.end(); ++confIt){
				if (confIt->second == obj) break;
			}
			if (confIt == editor->objectWindows.end()) break;
			editor->destroyWindow(confIt->first);
		}

		//Now we remove the object from the levelObjects and/or scenery.
		if (theBlock){
			std::vector<Block*>::iterator it;
			it = find(editor->levelObjects.begin(), editor->levelObjects.end(), theBlock);
			if (it != editor->levelObjects.end()){
				editor->levelObjects.erase(it);
				theLayer.clear(); // record the layer of this object
			}
		} else if (theScenery) {
			for (auto it = editor->sceneryLayers.begin(); it != editor->sceneryLayers.end(); ++it){
				auto it2 = find(it->second.begin(), it->second.end(), theScenery);
				if (it2 != it->second.end()) {
					it->second.erase(it2);
					theLayer = it->first; // record the layer of this object
					break;
				}
			}
		}
	}

	// Resize the level if necessary.
	if (resizeCommand) resizeCommand->unexecute();

	// Restore old start position if necessary.
	if (removeStartCommand) removeStartCommand->unexecute();

	// now we own this object
	ownObject = true;

	//Set dirty of selection popup
	editor->selectionDirty();
}

void AddRemoveGameObjectCommand::execute(){
	if (isAdd) {
		addGameObject();
	} else {
		removeGameObject();
	}
}

void AddRemoveGameObjectCommand::unexecute(){
	if (isAdd) {
		removeGameObject();
	} else {
		addGameObject();
	}

	// Restore old triggers if necessary
	if (oldTriggers) editor->triggers = *oldTriggers;
}

//////////////////////////////AddPathCommand///////////////////////////////////
AddPathCommand::AddPathCommand(LevelEditor* levelEditor,Block* movingBlock, MovingPosition movingPosition)
	:editor(levelEditor), target(movingBlock), movePos(movingPosition){
}

AddPathCommand::~AddPathCommand(){
}

void AddPathCommand::execute(){
	//Add movePos to the path.
	editor->movingBlocks[target].push_back(movePos);
	
	//Write the path to the moving block.
	std::map<std::string,std::string> editorData;
	char s[64], s0[64];

	sprintf(s,"%d",int(editor->movingBlocks[target].size()));
	editorData["MovingPosCount"]=s;
	//Loop through the positions.
	for(unsigned int o=0; o<editor->movingBlocks[target].size(); o++) {
		sprintf(s0+1,"%u",o);
		sprintf(s,"%d",editor->movingBlocks[target][o].x);
		s0[0]='x';
		editorData[s0]=s;
		sprintf(s,"%d",editor->movingBlocks[target][o].y);
		s0[0]='y';
		editorData[s0]=s;
		sprintf(s,"%d",editor->movingBlocks[target][o].time);
		s0[0]='t';
		editorData[s0]=s;
	}
	target->setEditorData(editorData);
}

void AddPathCommand::unexecute(){
	//Remove the last point in the path.
	editor->movingBlocks[target].pop_back();
	
	//Write the path to the moving block.
	std::map<std::string,std::string> editorData;
	char s[64], s0[64];

	sprintf(s,"%d",int(editor->movingBlocks[target].size()));
	editorData["MovingPosCount"]=s;
	//Loop through the positions.
	for(unsigned int o=0; o<editor->movingBlocks[target].size(); o++) {
		sprintf(s0+1,"%u",o);
		sprintf(s,"%d",editor->movingBlocks[target][o].x);
		s0[0]='x';
		editorData[s0]=s;
		sprintf(s,"%d",editor->movingBlocks[target][o].y);
		s0[0]='y';
		editorData[s0]=s;
		sprintf(s,"%d",editor->movingBlocks[target][o].time);
		s0[0]='t';
		editorData[s0]=s;
	}
	target->setEditorData(editorData);
}

//////////////////////////////RemovePathCommand///////////////////////////////////
RemovePathCommand::RemovePathCommand(LevelEditor* levelEditor, Block* targetBlock)
	:editor(levelEditor), target(targetBlock){
}
	
RemovePathCommand::~RemovePathCommand(){
}
	
void RemovePathCommand::execute(){
	//Set the number of movingPositions to 0.
	target->setEditorProperty("MovingPosCount","0");

	std::map<Block*,vector<MovingPosition> >::iterator it;
	//Check if target is in movingBlocks.
	it = editor->movingBlocks.find(target);
	if(it!=editor->movingBlocks.end()){
		//Store the movingPositions for unexecute.
		movePositions = it->second;
	
		//Clear all its movingPositions
		it->second.clear();
	}
}

void RemovePathCommand::unexecute(){
	std::map<Block*,vector<MovingPosition> >::iterator it;
	//Find target in movingBlocks
	it = editor->movingBlocks.find(target);
	
	if(it!=editor->movingBlocks.end()){
		//Restore its movingPositions
		it->second = movePositions;
		
		//Write the path to the moving block.
		std::map<std::string,std::string> editorData;
		char s[64], s0[64];

		sprintf(s,"%d",int(editor->movingBlocks[target].size()));
		editorData["MovingPosCount"]=s;
		//Loop through the positions.
		for(unsigned int o=0; o<editor->movingBlocks[target].size(); o++) {
			sprintf(s0+1,"%u",o);
			sprintf(s,"%d",editor->movingBlocks[target][o].x);
			s0[0]='x';
			editorData[s0]=s;
			sprintf(s,"%d",editor->movingBlocks[target][o].y);
			s0[0]='y';
			editorData[s0]=s;
			sprintf(s,"%d",editor->movingBlocks[target][o].time);
			s0[0]='t';
			editorData[s0]=s;
		}
		target->setEditorData(editorData);
	}
	
}

//////////////////////////////AddLinkCommand///////////////////////////////////
AddLinkCommand::AddLinkCommand(LevelEditor* levelEditor, Block* linkingTrigger, GameObject* clickedObject)
	:editor(levelEditor), target(linkingTrigger), clickedObj(clickedObject),oldPortalLink(NULL), destination(""), id(""){
}

AddLinkCommand::~AddLinkCommand(){
}
	
void AddLinkCommand::execute(){
	//Check if the target can handle multiple or only one link.
	switch(target->type) {
		case TYPE_PORTAL: {
			//Store the last portal link.
			if(!editor->triggers[target].empty()){
				oldPortalLink = editor->triggers[target].back();
			}
			
			//Portals can only link to one so remove all existing links.
			editor->triggers[target].clear();
			
			editor->triggers[target].push_back(clickedObj);
			break;
		}
		default: {
			//The others can handle multiple links.
			editor->triggers[target].push_back(clickedObj);
			break;
		}
	}

	//Check if it's a portal.
	if(target->type==TYPE_PORTAL) {
		//Store the previous destination.
		destination = target->getEditorProperty("destination");
		
		//Portals need to get the id of the other instead of give it's own id.
		char s[64];
		sprintf(s,"%d",atoi(clickedObj->getEditorProperty("id").c_str()));
		target->setEditorProperty("destination",s);
	} else{
		//Store the previous id.
		id = clickedObj->getEditorProperty("id");
		
		//Give the clickedObject the same id as the trigger.
		char s[64];
		sprintf(s,"%d",atoi(target->getEditorProperty("id").c_str()));
		clickedObj->setEditorProperty("id",s);
	}
}

void AddLinkCommand::unexecute(){
	//Check if the target can handle multiple or only one link.
	switch(target->type) {
		case TYPE_PORTAL: {
			//Portals can only link to one so remove all existing links.
			editor->triggers[target].clear();
				
			//Put the previous portal link back.
			if(oldPortalLink != NULL) 
				editor->triggers[target].push_back(oldPortalLink);
			
			break;
		}
		default:{
			std::vector<GameObject*>::iterator it;
			//Find the clickedObj in the target's triggers. 
			it = std::find(editor->triggers[target].begin(), editor->triggers[target].end(), clickedObj);
			if(it != editor->triggers[target].end()){
				//Remove it.
				editor->triggers[target].erase(it);
			}
			break;
		}
	}

	//Check if it's a portal.
	if(target->type==TYPE_PORTAL) {
		//Restore previous destination.
		target->setEditorProperty("destination",destination);
	} else{
		//Restore clickedObj's previous id.
		clickedObj->setEditorProperty("id",id);
	}
}

//////////////////////////////RemoveLinkCommand///////////////////////////////////
RemoveLinkCommand::RemoveLinkCommand(LevelEditor* levelEditor, Block* targetBlock)
	: editor(levelEditor), target(targetBlock), destination(""), id(""){
}

RemoveLinkCommand::~RemoveLinkCommand(){
}
	
void RemoveLinkCommand::execute(){
	std::map<Block*,vector<GameObject*> >::iterator it;
	//Find target in triggers.
	it = editor->triggers.find(target);
	if(it!=editor->triggers.end()) {
		//Copy the objects the target was linked to.
		links = it->second;
	
		//Remove the links.
		it->second.clear();
	}

	//In case of a portal remove its destination field.
	if(target->type==TYPE_PORTAL) {
		//Copy its previous destination.
		destination = target->getEditorProperty("destination");
		
		//Erase its destination.
		target->setEditorProperty("destination","");
	} else{
		//Copy its previous id.
		id = target->getEditorProperty("id");
		
		//Give the trigger a new id to prevent activating unlinked targets.
		char s[64];
		sprintf(s,"%u",editor->currentId);
		editor->currentId++;
		target->setEditorProperty("id",s);
	}
}

void RemoveLinkCommand::unexecute(){
	std::map<Block*,vector<GameObject*> >::iterator it;
	//Find target in triggers.
	it = editor->triggers.find(target);
	if(it!=editor->triggers.end()) {
		//Restore objects it was linked to.
		it->second = links;
	}
	
	if(target->type== TYPE_PORTAL){
		//Restore old destination.
		target->setEditorProperty("destination", destination);
	} else{
		//Restore old id.
		target->setEditorProperty("id", id);
	}
}

SetEditorPropertyCommand::SetEditorPropertyCommand(LevelEditor* levelEditor, ImageManager& imageManager, SDL_Renderer& renderer, GameObject* targetBlock,
	const std::string& propertyName, const std::string& propertyValue, const std::string& propertyDescription)
	: editor(levelEditor), imageManager(imageManager), renderer(renderer)
	, target(targetBlock), prop(propertyName), newValue(propertyValue), desc(propertyDescription)
{
	oldValue = target->getEditorProperty(prop);
}

SetEditorPropertyCommand::~SetEditorPropertyCommand() {
}

void SetEditorPropertyCommand::execute() {
	target->setEditorProperty(prop, newValue);
	updateCustomScenery();
}

void SetEditorPropertyCommand::unexecute() {
	target->setEditorProperty(prop, oldValue);
	updateCustomScenery();
}

void SetEditorPropertyCommand::updateCustomScenery() {
	Scenery *scenery = dynamic_cast<Scenery*>(target);
	if (target && prop == "customScenery") {
		scenery->updateCustomScenery(imageManager, renderer);
	}
}

SetLevelPropertyCommand::~SetLevelPropertyCommand() {
}

void SetLevelPropertyCommand::execute() {
	setLevelProperty(newProperty);
}

void SetLevelPropertyCommand::unexecute() {
	setLevelProperty(oldProperty);
}

std::string SetLevelPropertyCommand::describe() {
	return _("Modify level property");
}

SetScriptCommand::SetScriptCommand(LevelEditor* levelEditor, Block* targetBlock, const std::map<int, std::string>& script, const std::string& id)
	: editor(levelEditor), target(targetBlock), newScript(script), id(id)
{
	if (target) {
		oldScript = target->scripts;
		oldId = target->id;
	} else {
		oldScript = editor->scripts;
	}
}

SetScriptCommand::~SetScriptCommand() {
}

void SetScriptCommand::execute() {
	setScript(newScript, id);
}

void SetScriptCommand::unexecute() {
	setScript(oldScript, oldId);
}

void SetScriptCommand::setScript(const std::map<int, std::string>& script, const std::string& id) {
	if (target) {
		target->scripts = script;

		//Set the new id for the target block.
		//TODO: Check for trigger links etc...
		target->id = id;
	} else {
		editor->scripts = script;
	}
}

AddRemoveLayerCommand::AddRemoveLayerCommand(LevelEditor* levelEditor, const std::string& layerName, bool isAdd_)
	: editor(levelEditor), theLayer(layerName), isAdd(isAdd_), ownObject(isAdd_)
{
	if (!isAdd) {
		auto it = editor->sceneryLayers.find(theLayer);
		if (it != editor->sceneryLayers.end()) {
			objects = it->second;
		}
	}
}

void AddRemoveLayerCommand::execute() {
	if (isAdd) {
		addLayer();
	} else {
		removeLayer();
	}
}

void AddRemoveLayerCommand::unexecute() {
	if (isAdd) {
		removeLayer();
	} else {
		addLayer();
	}
}

void AddRemoveLayerCommand::addLayer() {
	assert(ownObject);
	assert(editor->sceneryLayers.find(theLayer) == editor->sceneryLayers.end());

	// add this layer
	editor->sceneryLayers[theLayer] = objects;

	// show and select the newly created layer
	editor->layerVisibility[theLayer] = true;
	editor->selectedLayer = theLayer;

	// deselect all
	editor->deselectAll();

	// change the ownership
	ownObject = false;
}

void AddRemoveLayerCommand::removeLayer() {
	assert(!ownObject);

	// deselect all
	editor->deselectAll();

	// select the Blocks layer
	editor->selectedLayer.clear();

	// remove this layer
	editor->layerVisibility.erase(theLayer);
	editor->sceneryLayers.erase(theLayer);

	// change the ownership
	ownObject = true;
}

AddRemoveLayerCommand::~AddRemoveLayerCommand() {
	if (ownObject) {
		for (auto obj : objects) {
			delete obj;
		}
	}
}

std::string AddRemoveLayerCommand::describe() {
	return tfm::format(isAdd ? _("Add scenery layer %s") : _("Delete scenery layer %s"), theLayer);
}

RenameLayerCommand::RenameLayerCommand(LevelEditor* levelEditor, const std::string& oldName, const std::string& newName)
	: editor(levelEditor), oldName(oldName), newName(newName)
{
}

void RenameLayerCommand::execute() {
	rename(oldName, newName);
}

void RenameLayerCommand::unexecute() {
	rename(newName, oldName);
}

RenameLayerCommand::~RenameLayerCommand() {
}

std::string RenameLayerCommand::describe() {
	return tfm::format(_("Rename scenery layer %s to %s"), oldName, newName);
}

void RenameLayerCommand::rename(const std::string& oldName, const std::string& newName) {
	assert(editor->sceneryLayers.find(oldName) != editor->sceneryLayers.end() && editor->sceneryLayers.find(newName) == editor->sceneryLayers.end());

	// create a temp variable, save the old layer to it, remove the old layer
	std::vector<Scenery*> tmp;
	std::swap(editor->sceneryLayers[oldName], tmp);
	editor->sceneryLayers.erase(oldName);

	// then save the temp variable to the new layer
	std::swap(editor->sceneryLayers[newName], tmp);

	// sanity check
	assert(tmp.empty());

	// show and select the newly created layer
	editor->layerVisibility[newName] = editor->layerVisibility[oldName];
	editor->layerVisibility.erase(oldName);
	editor->selectedLayer = newName;
}

MoveToLayerCommand::MoveToLayerCommand(LevelEditor* levelEditor, std::vector<GameObject*>& gameObjects, const std::string& oldName, const std::string& newName)
	: editor(levelEditor), oldName(oldName), newName(newName), createNewLayer(false)
{
	if (editor->sceneryLayers.find(newName) == editor->sceneryLayers.end()) createNewLayer = true;
	for (auto obj : gameObjects) {
		Scenery *scenery = dynamic_cast<Scenery*>(obj);
		if (scenery) objects.push_back(scenery);
	}
}

void MoveToLayerCommand::execute() {
	removeGameObject();
	addGameObject(newName);

	// show and select the new layer
	editor->layerVisibility[newName] = true;
	editor->selectedLayer = newName;

	// deselect all
	editor->deselectAll();
}

void MoveToLayerCommand::unexecute() {
	removeGameObject();
	addGameObject(oldName);

	if (createNewLayer) {
		auto it = editor->sceneryLayers.find(newName);
		if (it != editor->sceneryLayers.end()) {
			assert(it->second.empty());

			// remove this layer
			editor->layerVisibility.erase(newName);
			editor->sceneryLayers.erase(newName);
		}
	}

	// show and select the old layer
	editor->layerVisibility[oldName] = true;
	editor->selectedLayer = oldName;

	// deselect all
	editor->deselectAll();
}

MoveToLayerCommand::~MoveToLayerCommand() {
}

std::string MoveToLayerCommand::describe() {
	return tfm::format(_("Move %d object(s) from layer %s to layer %s"), objects.size(), oldName, newName);
}

void MoveToLayerCommand::removeGameObject() {
	// Remove objects
	for (int index = 0; index < (int)objects.size(); index++) {
		Scenery *scenery = objects[index];

		//Now we remove the object from scenery.
		for (auto it = editor->sceneryLayers.begin(); it != editor->sceneryLayers.end(); ++it){
			auto it2 = find(it->second.begin(), it->second.end(), scenery);
			if (it2 != it->second.end()) {
				it->second.erase(it2);
				break;
			}
		}
	}
}

void MoveToLayerCommand::addGameObject(const std::string& layer) {
	// Add objects
	std::vector<Scenery*> &target = editor->sceneryLayers[layer];
	for (int index = 0; index < (int)objects.size(); index++) {
		target.push_back(objects[index]);
	}
}

