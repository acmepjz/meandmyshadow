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
#include "Game.h"
#include "Scenery.h"
#include "Objects.h"
#include "Functions.h"
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
using namespace std;

Scenery::Scenery(Game* parent):
	GameObject(parent),
	xSave(0),
	ySave(0),
	dx(0),
	dy(0)
{}

Scenery::~Scenery(){
	//Destroy the themeBlock since it isn't needed anymore.
	themeBlock.destroy();
}

void Scenery::show(){
	//Check if the scenery is visible.
	if(checkCollision(camera,box)==true || (stateID==STATE_LEVEL_EDITOR && checkCollision(camera,boxBase)==true)){
		//Now draw normal.
		appearance.draw(screen, box.x - camera.x, box.y - camera.y);
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


void Scenery::playAnimation(int flags){}

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

bool Scenery::loadFromNode(TreeStorageNode* objNode){
	//Make sure there are enough arguments.
	if(objNode->value.size()<4)
		return false;

	//Load position and size.
	box.x=boxBase.x=atoi(objNode->value[0].c_str());
	box.y=boxBase.y=atoi(objNode->value[1].c_str());
	box.w=boxBase.w=atoi(objNode->value[2].c_str());
	box.h=boxBase.h=atoi(objNode->value[3].c_str());

	//Load the appearance.
	themeBlock.loadFromNode(objNode,levels->levelpackPath);
	themeBlock.createInstance(&appearance);

	return true;
}

void Scenery::prepareFrame(){
	//Reset the delta variables.
	dx=dy=0;
}

void Scenery::move(){
	//Update our appearance.
	appearance.updateAnimation();
}
