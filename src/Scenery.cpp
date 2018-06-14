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

Scenery::Scenery(Game* objParent) :
	GameObject(objParent),
	xSave(0),
	ySave(0),
	dx(0),
	dy(0),
	themeBlock(NULL)
{}

Scenery::Scenery(Game* objParent, int x, int y, int w, int h, const std::string& sceneryName) :
	GameObject(objParent),
	xSave(0),
	ySave(0),
	dx(0),
	dy(0),
	themeBlock(NULL)
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

void Scenery::show(SDL_Renderer& renderer){
	//Check if the scenery is visible.
	if(checkCollision(camera,box)==true || (stateID==STATE_LEVEL_EDITOR && checkCollision(camera,boxBase)==true)){
		//Now draw normal.
		appearance.draw(renderer, box.x - camera.x, box.y - camera.y, box.w, box.h);

		//Draw a stupid icon for custom scenery blocks in edit mode.
		if (stateID == STATE_LEVEL_EDITOR && themeBlock == &internalThemeBlock){
			auto bmGUI = static_cast<LevelEditor*>(parent)->getGuiTexture();
			if (!bmGUI) {
				return;
			}
			const SDL_Rect r = { 48, 16, 16, 16 };
			const SDL_Rect dstRect = { box.x - camera.x + 2, box.y - camera.y + 2, 16, 16 };
			SDL_RenderCopy(&renderer, bmGUI.get(), &r, &dstRect);
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
	
}

void Scenery::setEditorData(std::map<std::string,std::string>& obj){

}

std::string Scenery::getEditorProperty(std::string property){
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

void Scenery::setEditorProperty(std::string property,std::string value){
	//Create a map to hold the property.
	std::map<std::string,std::string> editorData;
	editorData[property]=value;

	//And call the setEditorData method.
	setEditorData(editorData);
}

bool Scenery::loadFromNode(ImageManager& imageManager, SDL_Renderer& renderer, TreeStorageNode* objNode){
	sceneryName_.clear();
	customScenery_.clear();

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
		POASerializer serializer;
		std::ostringstream o;
		serializer.writeNode(objNode, o, false, true);
		customScenery_ = o.str();

		//Load the appearance.
		if (!internalThemeBlock.loadFromNode(objNode, levels->levelpackPath, imageManager, renderer)) return false;
		themeBlock = &internalThemeBlock;
		themeBlock->createInstance(&appearance);

		return true;
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

		return true;
	}

	return false;
}

void Scenery::prepareFrame(){
	//Reset the delta variables.
	dx=dy=0;
}

void Scenery::move(){
	//Update our appearance.
	appearance.updateAnimation();
}
