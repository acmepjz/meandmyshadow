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

#include "SceneryLayer.h"
#include "ImageManager.h"
#include "Render.h"
#include "POASerializer.h"
#include "LevelEditor.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sstream>

SceneryLayer::SceneryLayer()
	: speedX(0), speedY(0)
	, cameraX(1), cameraY(1)
	, currentX(0), currentY(0)
	, savedX(0), savedY(0)
{
}

SceneryLayer::~SceneryLayer() {
	for (auto o : objects) {
		delete o;
	}
	objects.clear();
}

void SceneryLayer::loadFromNode(Game *parent, ImageManager& imageManager, SDL_Renderer& renderer, TreeStorageNode* obj) {
	//Retrieve the speed.
	{
		auto &v = obj->attributes["speed"];
		if (v.size() >= 2) {
			speedX = atof(v[0].c_str());
			speedY = atof(v[1].c_str());
		}
	}

	//Retrieve the camera speed.
	{
		auto &v = obj->attributes["cameraSpeed"];
		if (v.size() >= 2) {
			cameraX = atof(v[0].c_str());
			cameraY = atof(v[1].c_str());
		}
	}

	//Loop through the sub nodes.
	for (auto obj2 : obj->subNodes){
		if (obj2 == NULL) continue;

		if (obj2->name == "object" || obj2->name == "scenery"){
			//Load the scenery from node.
			Scenery* scenery = new Scenery(parent);
			if (!scenery->loadFromNode(imageManager, renderer, obj2)){
				delete scenery;
				continue;
			}

			objects.push_back(scenery);
		}
	}
}

void SceneryLayer::updateAnimation() {
	//Update the animation of the layer itself.
	currentX += speedX;
	currentY += speedY;

	//Update the animation of objects.
	for (auto obj : objects) {
		obj->move();
	}
}

void SceneryLayer::resetAnimation(bool save) {
	currentX = 0.0f;
	currentY = 0.0f;
	if (save){
		savedX = 0.0f;
		savedY = 0.0f;
	}

	for (auto obj : objects) {
		obj->reset(save);
	}
}

void SceneryLayer::saveAnimation() {
	savedX = currentX;
	savedY = currentY;

	for (auto obj : objects) {
		obj->saveState();
	}
}

void SceneryLayer::loadAnimation() {
	currentX = savedX;
	currentY = savedY;

	for (auto obj : objects) {
		obj->loadState();
	}
}

void SceneryLayer::show(SDL_Renderer& renderer) {
	int offsetX = 0, offsetY = 0;

	//Offset the objects by currentX/Y. Only in play mode.
	if (stateID != STATE_LEVEL_EDITOR || dynamic_cast<LevelEditor*>(currentState)->isPlayMode()) {
		offsetX = (int)floor(currentX + (1.0f - cameraX) * (float)camera.x + 0.5f);
		offsetY = (int)floor(currentY + (1.0f - cameraY) * (float)camera.y + 0.5f);
	}

	//Show objects.
	for (auto obj : objects) {
		obj->showScenery(renderer, offsetX, offsetY);
	}
}

void SceneryLayer::saveToNode(TreeStorageNode* obj) {
	char s[64];

	//Save the speed.
	sprintf(s, "%g", speedX);
	obj->attributes["speed"].push_back(s);
	sprintf(s, "%g", speedY);
	obj->attributes["speed"].push_back(s);

	//Save the camera speed.
	sprintf(s, "%g", cameraX);
	obj->attributes["cameraSpeed"].push_back(s);
	sprintf(s, "%g", cameraY);
	obj->attributes["cameraSpeed"].push_back(s);

	//Loop through the scenery blocks and save them.
	for (auto scenery : objects) {
		TreeStorageNode* obj1 = new TreeStorageNode;
		obj->subNodes.push_back(obj1);

		// Check if it's custom scenery block
		if (scenery->themeBlock == &(scenery->internalThemeBlock)) {
			// load the dump of TreeStorageNode
			POASerializer serializer;
			std::istringstream i(scenery->customScenery_);
			serializer.readNode(i, obj1, true);

			// custom scenery
			obj1->name = "object";

			// clear the value in case that the serializer is buggy
			obj1->value.clear();

			// clear the attributes in case that the user inputs some attributes
			obj1->attributes.clear();
		} else {
			// predefined scenery
			obj1->name = "scenery";

			//Write away the name of the scenery.
			obj1->value.push_back(scenery->sceneryName_);
		}

		//Get the box for the location of the scenery.
		SDL_Rect box = scenery->getBox(BoxType_Base);
		//Put the location in the storageNode.
		sprintf(s, "%d", box.x);
		obj1->value.push_back(s);
		sprintf(s, "%d", box.y);
		obj1->value.push_back(s);
		//Only save the size when it is not of default size.
		if (box.h != 50) {
			sprintf(s, "%d", box.w);
			obj1->value.push_back(s);
			sprintf(s, "%d", box.h);
			obj1->value.push_back(s);
		} else if (box.w != 50) {
			sprintf(s, "%d", box.w);
			obj1->value.push_back(s);
		}

		//Get the repeat mode of the scenery if it's not default value
		if (scenery->repeatMode) {
			std::vector<std::string> &v = obj1->attributes["repeatMode"];
			for (int i = 0; i < 4; i++) {
				sprintf(s, "%d", ((scenery->repeatMode) >> (i * 8)) & 0xFF);
				v.push_back(s);
			}
		}
	}
}
