/*
 * Copyright (C) 2011-2013 Me and My Shadow
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

#include "GameObjects.h"
#include "Functions.h"
#include "Globals.h"

GameObject::GameObject(Game* parent):type(0),parent(parent){}
GameObject::~GameObject(){}

SDL_Rect GameObject::getBox(int boxType){
	//This is the default implementation of getBox(int) method.
	switch(boxType){
	case BoxType_Current:
	case BoxType_Previous:
		return box;
	case BoxType_Base:
		return boxBase;		
	}
	
	//Return an empty SDL_Rect.
	SDL_Rect tmp={0,0,0,0};
	return tmp;
}

void GameObject::setLocation(int x,int y){
	box.x=x;
	box.y=y;
}
void GameObject::setBaseLocation(int x,int y){
	box.x=x;
	box.y=y;
	boxBase.x=x;
	boxBase.y=y;
}

void GameObject::setSize(int w,int h){
	box.w=w;
	box.h=h;
}
void GameObject::setBaseSize(int w,int h){
	box.w=w;
	box.h=h;
	boxBase.w=w;
	boxBase.h=h;
}

void GameObject::saveState(){}
void GameObject::loadState(){}
void GameObject::reset(bool save){}

void GameObject::playAnimation(){}
void GameObject::onEvent(int eventType){}

int GameObject::queryProperties(int propertyType,Player* obj){
	return 0;
}

void GameObject::getEditorData(std::vector<std::pair<std::string,std::string> >& obj){}
void GameObject::setEditorData(std::map<std::string,std::string>& obj){}
std::string GameObject::getEditorProperty(const std::string& property){return "";}
void GameObject::setEditorProperty(const std::string& property, const std::string& value){}

bool GameObject::loadFromNode(ImageManager&, SDL_Renderer&, TreeStorageNode*){return true;}

void GameObject::move(){}
