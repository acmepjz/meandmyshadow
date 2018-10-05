/*
 * Copyright (C) 2013 Me and My Shadow
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

#include "GameObjects.h"
#include "Scenery.h"
#include "Functions.h"
#include "LevelEditor.h"
#include "POASerializer.h"
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <stdio.h>
using namespace std;

#include "libs/tinyformat/tinyformat.h"

Scenery::Scenery(Game* objParent) :
	GameObject(objParent),
	xSave(0),
	ySave(0),
	dx(0),
	dy(0),
	themeBlock(NULL),
	repeatMode(0)
{}

Scenery::Scenery(Game* objParent, int x, int y, int w, int h, const std::string& sceneryName) :
	GameObject(objParent),
	xSave(0),
	ySave(0),
	dx(0),
	dy(0),
	themeBlock(NULL),
	repeatMode(0)
{
	box.x = boxBase.x = x;
	box.y = boxBase.y = y;
	box.w = boxBase.w = w;
	box.h = boxBase.h = h;

	if (sceneryName.empty()) {
		themeBlock = &internalThemeBlock;
	} else {
		// Load the appearance.
		themeBlock = objThemes.getScenery(sceneryName);
		if (themeBlock) {
			sceneryName_ = sceneryName;
		} else {
			fprintf(stderr, "ERROR: Can't find scenery with name '%s'.\n", sceneryName.c_str());
			themeBlock = &internalThemeBlock;
		}
	}
	themeBlock->createInstance(&appearance);
}

Scenery::~Scenery(){
	//Destroy the themeBlock since it isn't needed anymore.
	internalThemeBlock.destroy();
}

static inline int getNewCoord(unsigned char rm, int default_, int cameraX, int cameraW, int levelW, int offset) {
	switch (rm) {
	case Scenery::NEGATIVE_INFINITY:
		return cameraX;
	case Scenery::ZERO:
		return std::max(cameraX, offset);
	case Scenery::LEVEL_SIZE:
		return std::min(cameraX + cameraW, levelW + offset);
	case Scenery::POSITIVE_INFINITY:
		return cameraX + cameraW;
	default:
		return default_;
	}
}

void Scenery::show(SDL_Renderer& renderer) {
	showScenery(renderer, 0, 0);
}

void Scenery::showScenery(SDL_Renderer& renderer, int offsetX, int offsetY) {
	//The box which is offset by the input.
	const SDL_Rect box = {
		this->box.x + offsetX,
		this->box.y + offsetY,
		this->box.w,
		this->box.h,
	};

	//The real box according to repeat mode.
	SDL_Rect theBox = {
		getNewCoord(repeatMode, box.x, camera.x, camera.w, LEVEL_WIDTH, offsetX),
		getNewCoord(repeatMode >> 16, box.y, camera.y, camera.h, LEVEL_HEIGHT, offsetX),
		getNewCoord(repeatMode >> 8, box.x + box.w, camera.x, camera.w, LEVEL_WIDTH, offsetY),
		getNewCoord(repeatMode >> 24, box.y + box.h, camera.y, camera.h, LEVEL_HEIGHT, offsetY),
	};
	theBox.w -= theBox.x;
	theBox.h -= theBox.y;

	//Check if the scenery is visible.
	if (theBox.w > 0 && theBox.h > 0 && checkCollision(camera, theBox)) {
		//Snap the size to integral multiple of box.w and box.h
		if (box.w > 1) {
			theBox.w += theBox.x;
			if (repeatMode & 0xFFu) {
				theBox.x = box.x + int(floor(float(theBox.x - box.x) / float(box.w))) * box.w;
			}
			if (repeatMode & 0xFF00u) {
				theBox.w = box.x + int(ceil(float(theBox.w - box.x) / float(box.w))) * box.w;
			}
			theBox.w -= theBox.x;
		}
		if (box.h > 1) {
			theBox.h += theBox.y;
			if (repeatMode & 0xFF0000u) {
				theBox.y = box.y + int(floor(float(theBox.y - box.y) / float(box.h))) * box.h;
			}
			if (repeatMode & 0xFF000000u) {
				theBox.h = box.y + int(ceil(float(theBox.h - box.y) / float(box.h))) * box.h;
			}
			theBox.h -= theBox.y;
		}

		//Now draw normal.
		if (theBox.w > 0 && theBox.h > 0) {
			appearance.draw(renderer, theBox.x - camera.x, theBox.y - camera.y, theBox.w, theBox.h);
		}
	}

	//Draw some stupid icons in edit mode.
	if (stateID == STATE_LEVEL_EDITOR && checkCollision(camera, box)) {
		auto bmGUI = static_cast<LevelEditor*>(parent)->getGuiTexture();
		if (!bmGUI) {
			return;
		}

		int x = box.x - camera.x + 2;

		//Draw a stupid icon for custom scenery.
		if (themeBlock == &internalThemeBlock) {
			const SDL_Rect r = { 48, 16, 16, 16 };
			const SDL_Rect dstRect = { x, box.y - camera.y + 2, 16, 16 };
			SDL_RenderCopy(&renderer, bmGUI.get(), &r, &dstRect);
			x += 16;
		}

		//Draw a stupid icon for horizonal repeat.
		if (repeatMode & 0x0000FFFFu) {
			const SDL_Rect r = { 64, 32, 16, 16 };
			const SDL_Rect dstRect = { x, box.y - camera.y + 2, 16, 16 };
			SDL_RenderCopy(&renderer, bmGUI.get(), &r, &dstRect);
			x += 16;
		}

		//Draw a stupid icon for vertical repeat.
		if (repeatMode & 0xFFFF0000u) {
			const SDL_Rect r = { 64, 48, 16, 16 };
			const SDL_Rect dstRect = { x, box.y - camera.y + 2, 16, 16 };
			SDL_RenderCopy(&renderer, bmGUI.get(), &r, &dstRect);
			x += 16;
		}
	}
}

SDL_Rect Scenery::getBox(int boxType){
	SDL_Rect r={0,0,0,0};
	switch(boxType){
	case BoxType_Base:
		return boxBase;
	case BoxType_Previous:
		r.x=box.x-dx;
		r.y=box.y-dy;
		r.w=box.w;
		r.h=box.h;
		return r;
	case BoxType_Delta:
		r.x=dx;
		r.y=dy;
		return r;
	case BoxType_Velocity:
		return r;
	case BoxType_Current:
		return box;
	}
	return r;
}

void Scenery::setLocation(int x,int y){
	//The scenery has moved so calculate the delta.
	dx=x-box.x;
	dy=y-box.y;

	//And set its new location.
	box.x=x;
	box.y=y;
}

void Scenery::saveState(){
	//Store the location.
	xSave=box.x-boxBase.x;
	ySave=box.y-boxBase.y;

	//And any animations.
	appearance.saveAnimation();
}

void Scenery::loadState(){
	//Restore the location.
	box.x=boxBase.x+xSave;
	box.y=boxBase.y+ySave;

	//And load the animation.
	appearance.loadAnimation();
}

void Scenery::reset(bool save){
	//Reset the scenery to its original location.
	box.x=boxBase.x;
	box.y=boxBase.y;

	if(save)
		xSave=ySave=0;

	//Also reset the appearance.
	appearance.resetAnimation(save);
	appearance.changeState("default");
	//NOTE: We load the animation right after changing it to prevent a transition.
	if(save)
		appearance.loadAnimation();
}


void Scenery::playAnimation(){}

void Scenery::onEvent(int eventType){
	//NOTE: Scenery should not interact with the player or vice versa.
}

int Scenery::queryProperties(int propertyType,Player* obj){
	//NOTE: Scenery doesn't have any properties.
	return 0;
}

void Scenery::getEditorData(std::vector<std::pair<std::string,std::string> >& obj){
	obj.push_back(pair<string, string>("sceneryName", sceneryName_));
	obj.push_back(pair<string, string>("customScenery", customScenery_));
	obj.push_back(pair<string, string>("repeatMode", tfm::format("%d", repeatMode)));
}

void Scenery::setEditorData(std::map<std::string,std::string>& obj){
	// NOTE: currently the sceneryName cannot be changed by this method.

	auto it = obj.find("customScenery");
	if (it != obj.end()) {
		customScenery_ = it->second;
	}

	it = obj.find("repeatMode");
	if (it != obj.end()) {
		repeatMode = atoi(it->second.c_str());
	}
}

std::string Scenery::getEditorProperty(const std::string& property){
	//First get the complete editor data.
	vector<pair<string,string> > objMap;
	vector<pair<string,string> >::iterator it;
	getEditorData(objMap);

	//Loop through the entries.
	for(it=objMap.begin();it!=objMap.end();++it){
		if(it->first==property)
			return it->second;
	}

	//Nothing found.
	return "";
}

void Scenery::setEditorProperty(const std::string& property, const std::string& value){
	//Create a map to hold the property.
	std::map<std::string,std::string> editorData;
	editorData[property]=value;

	//And call the setEditorData method.
	setEditorData(editorData);
}

bool Scenery::loadFromNode(ImageManager& imageManager, SDL_Renderer& renderer, TreeStorageNode* objNode){
	sceneryName_.clear();
	customScenery_.clear();
	repeatMode = 0;

	if (objNode->name == "object") {
		//Make sure there are enough arguments.
		if (objNode->value.size() < 2)
			return false;

		//Load position and size.
		box.x = boxBase.x = atoi(objNode->value[0].c_str());
		box.y = boxBase.y = atoi(objNode->value[1].c_str());
		box.w = boxBase.w = (objNode->value.size() >= 3) ? atoi(objNode->value[2].c_str()) : 50;
		box.h = boxBase.h = (objNode->value.size() >= 4) ? atoi(objNode->value[3].c_str()) : 50;

		//Dump the current TreeStorageNode.
		//NOTE: we temporarily remove all attributes since they are not related to theme.
		std::map<std::string, std::vector<std::string> > tmpAttributes;
		std::swap(objNode->attributes, tmpAttributes);

		std::ostringstream o;
		POASerializer().writeNode(objNode, o, false, true);
		customScenery_ = o.str();

		//restore old attributes
		std::swap(objNode->attributes, tmpAttributes);

		//Load the appearance.
		if (!internalThemeBlock.loadFromNode(objNode, levels->levelpackPath, imageManager, renderer)) return false;
		themeBlock = &internalThemeBlock;
		themeBlock->createInstance(&appearance);
	} else if (objNode->name == "scenery") {
		//Make sure there are enough arguments.
		if (objNode->value.size() < 3)
			return false;

		//Load position and size.
		box.x = boxBase.x = atoi(objNode->value[1].c_str());
		box.y = boxBase.y = atoi(objNode->value[2].c_str());
		box.w = boxBase.w = (objNode->value.size() >= 4) ? atoi(objNode->value[3].c_str()) : 50;
		box.h = boxBase.h = (objNode->value.size() >= 5) ? atoi(objNode->value[4].c_str()) : 50;

		//Load the appearance.
		themeBlock = objThemes.getScenery(objNode->value[0]);
		if (!themeBlock) {
			fprintf(stderr, "ERROR: Can't find scenery with name '%s'.\n", objNode->value[0].c_str());
			return false;
		}
		themeBlock->createInstance(&appearance);

		//Save the scenery name.
		sceneryName_ = objNode->value[0];
	} else {
		//Unsupported node name for scenery block
		fprintf(stderr, "ERROR: Unsupported node name '%s' for scenery block.\n", objNode->name.c_str());
		return false;
	}

	auto it = objNode->attributes.find("repeatMode");
	if (it != objNode->attributes.end() && it->second.size() >= 4) {
		repeatMode = atoi(it->second[0].c_str())
			| (atoi(it->second[1].c_str()) << 8)
			| (atoi(it->second[2].c_str()) << 16)
			| (atoi(it->second[3].c_str()) << 24);
	}

	return true;
}

bool Scenery::updateCustomScenery(ImageManager& imageManager, SDL_Renderer& renderer) {
	POASerializer serializer;
	std::istringstream i(customScenery_);
	TreeStorageNode objNode;

	//Load the node from text dump
	if (!serializer.readNode(i, &objNode, true)) return false;

	//Load the appearance.
	if (!internalThemeBlock.loadFromNode(&objNode, levels->levelpackPath, imageManager, renderer)) return false;
	themeBlock = &internalThemeBlock;
	themeBlock->createInstance(&appearance);

	// Clear the scenery name since we are using custom scenery
	sceneryName_.clear();

	return true;
}

void Scenery::move(){
	//Update our appearance.
	appearance.updateAnimation();
}
