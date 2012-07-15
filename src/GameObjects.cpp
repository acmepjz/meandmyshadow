/*
 * Copyright (C) 2011-2012 Me and My Shadow
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
#include "Player.h"

GameObject::GameObject(Game* parent):type(0),parent(parent){}
GameObject::~GameObject(){}

SDL_Rect GameObject::getBox(int boxType){
	//This is the default implementation of getBox(int) method.
	switch(boxType){
	case BoxType_Current:
	case BoxType_Base:
	case BoxType_Previous:
		return box;
	}
	
	//Return an empty SDL_Rect.
	SDL_Rect tmp={0,0,0,0};
	return tmp;
}

void GameObject::setPosition(int x,int y){
	box.x=x;
	box.y=y;
}

void GameObject::saveState(){}
void GameObject::loadState(){}
void GameObject::reset(bool save){}

void GameObject::playAnimation(int flags){}
void GameObject::onEvent(int eventType){}

int GameObject::queryProperties(int propertyType,Player* obj){
	return 0;
}

void GameObject::getEditorData(std::vector<std::pair<std::string,std::string> >& obj){}
void GameObject::setEditorData(std::map<std::string,std::string>& obj){}

void GameObject::move(){}
