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
 * Me And My Shadow is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Me and My Shadow.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Block.h"
#include "Functions.h"
#include "LevelEditor.h"
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
using namespace std;

Block::Block(Game* parent,int x,int y,int w,int h,int type):
	GameObject(parent),
	animation(0),
	animationSave(0),
	flags(0),
	flagsSave(0),
	temp(0),
	tempSave(0),
	dx(0),
	dxSave(0),
	dy(0),
	dySave(0),
	loop(true),
	speed(0),
	speedSave(0),
	editorSpeed(0),
	editorFlags(0),
	visible(true),
	visibleSave(true),
	visibleBase(true)
{
	//Make sure the type is set, if not init should be called somewhere else with this information.
	if(type>=0 && type<TYPE_MAX)
		init(x,y,w,h,type);
}

Block::~Block(){}

void Block::init(int x,int y,int w,int h,int type){
	//First set the location and size of the box.
	//The default size is 50x50.
	box.x=boxBase.x=x;
	box.y=boxBase.y=y;
	box.w=boxBase.w=w;
	box.h=boxBase.h=h;

	//Set the save values.
	boxSave.x=x;
	boxSave.y=y;
	boxSave.w=w;
	boxSave.h=h;
	
	//Set the type.
	this->type=type;

	//Some types need type specific code.
	if(type==TYPE_START_PLAYER){
		//This is the player start so set the player here.
		//We center the player, the player is 23px wide.
		parent->player.setLocation(box.x+(box.w-23)/2,box.y);
		parent->player.fx=box.x+(box.w-23)/2;
		parent->player.fy=box.y;
	}else if(type==TYPE_START_SHADOW){
		//This is the shadow start so set the shadow here.
		//We center the shadow, the shadow is 23px wide.
		parent->shadow.setLocation(box.x+(box.w-23)/2,box.y);
		parent->shadow.fx=box.x+(box.w-23)/2;
		parent->shadow.fy=box.y;
	}

	objCurrentStand=NULL;
	inAir=inAirSave=true;
	xVel=yVel=xVelBase=yVelBase=0;
	xVelSave=yVelSave=xVelBaseSave=yVelBaseSave=0;

	//And load the appearance.
	objThemes.getBlock(type)->createInstance(&appearance);
}

void Block::show(SDL_Renderer& renderer){
	//Make sure we are visible.
	if (!visible && (stateID != STATE_LEVEL_EDITOR || dynamic_cast<LevelEditor*>(parent)->isPlayMode()))
		return;
	
	//Check if the block is visible.
	if(checkCollision(camera,box)==true || (stateID==STATE_LEVEL_EDITOR && checkCollision(camera,boxBase)==true)){
		//Some type of block needs additional state check.
		switch(type){
		case TYPE_CHECKPOINT:
			//Check if the checkpoint is last used.
			if(parent!=NULL && parent->objLastCheckPoint==this){
				if(!temp) appearance.changeState("activated");
				temp=1;
			}else{
				if(temp) appearance.changeState("default");
				temp=0;
			}
			break;
		}

		//Always draw the base.
		appearance.drawState("base", renderer, boxBase.x - camera.x, boxBase.y - camera.y, boxBase.w, boxBase.h);

		//What we need to draw depends on the type of block.
		switch (type) {
		default:
			//Draw normal.
			appearance.draw(renderer, box.x - camera.x, box.y - camera.y, box.w, box.h);
			break;
		case TYPE_CONVEYOR_BELT:
		case TYPE_SHADOW_CONVEYOR_BELT:
			//Draw conveyor belt.
			if (animation) {
				// FIXME: ad-hoc code. Should add a new animation type in theme system.
				const int a = animation / 10;
				const SDL_Rect r = { box.x - camera.x, box.y - camera.y, box.w, box.h };
				appearance.draw(renderer, box.x - camera.x - 50 + a, box.y - camera.y, box.w + 50 - a, box.h, &r);
			} else {
				appearance.draw(renderer, box.x - camera.x, box.y - camera.y, box.w, box.h);
			}
			break;
		}

		//Some types need to draw something on top of the base/default.
		switch(type){
		case TYPE_BUTTON:
			if(flags&4){
				if(animation<5) animation++;
			}else{
				if(animation>0) animation--;
			}
            appearance.drawState("button",renderer,box.x-camera.x,box.y-camera.y-5+animation);
			break;
		}

		//Draw some stupid icons during edit mode.
		if (stateID == STATE_LEVEL_EDITOR) {
			auto bmGUI = static_cast<LevelEditor*>(parent)->getGuiTexture();
			if (!bmGUI) {
				return;
			}

			int x = box.x - camera.x + 2;

			//Scripted blocks
			if (!scripts.empty()){
				const SDL_Rect r = { 0, 32, 16, 16 };
				const SDL_Rect dstRect = { x, box.y - camera.y + 2, 16, 16 };
				SDL_RenderCopy(&renderer, bmGUI.get(), &r, &dstRect);
				x += 16;
			}

			//Invisible blocks
			if (!visibleBase) {
				const SDL_Rect r = { 16, 48, 16, 16 };
				const SDL_Rect dstRect = { x, box.y - camera.y + 2, 16, 16 };
				SDL_RenderCopy(&renderer, bmGUI.get(), &r, &dstRect);
				x += 16;
			}
		}
	}
}

SDL_Rect Block::getBox(int boxType){
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
		r.x=xVel;
		r.y=yVel;
		//NOTE: In case of the pushable block we sometimes need to substract one from the vertical velocity.
		//The yVel is set to one when it's resting, but should be handled as zero in collision.
		if(type==TYPE_PUSHABLE && !inAir)
			r.y=0;
		return r;
	case BoxType_Current:
		return box;
	}
	return r;
}

void Block::moveTo(int x,int y){
	//The block has moved so calculate the delta.
	//NOTE: Every delta is summed since they all happened within one frame and for collision/movement we need the resulting delta.
	int delta=(x-box.x);
	dx+=delta;
	xVel+=delta;
	delta=(y-box.y);
	dy+=delta;
	yVel+=delta;

	//And set the new location.
	box.x=x;
	box.y=y;
}

void Block::growTo(int w,int h){
	//The block has changed size 
	//NOTE: Every delta is summed since they all happened within one frame and for collision/movement we need the resulting delta.
	int delta=(w-box.w);
	dx+=delta;
	xVel+=delta;
	delta=(h-box.h);
	dy+=delta;
	yVel+=delta;

	//And set the new location.
	box.w=w;
	box.h=h;
}

void Block::saveState(){
	animationSave=animation;
	flagsSave=flags;
	tempSave=temp;
	dxSave=dx;
	dySave=dy;
	boxSave.x=box.x-boxBase.x;
	boxSave.y=box.y-boxBase.y;
	boxSave.w=box.w-boxBase.w;
	boxSave.h=box.h-boxBase.h;
	xVelSave=xVel;
	yVelSave=yVel;
	visibleSave=visible;
	appearance.saveAnimation();

	//In case of a certain blocks we need to save some more.
	switch(type){
		case TYPE_PUSHABLE:
			xVelBaseSave=xVelBase;
			yVelBaseSave=yVelBase;
			inAirSave=inAir;
			break;
		case TYPE_CONVEYOR_BELT:
		case TYPE_SHADOW_CONVEYOR_BELT:
			speedSave=speed;
			break;
	}
}

void Block::loadState(){
	//Restore the flags and animation var.
	animation=animationSave;
	flags=flagsSave;
	temp=tempSave;
	dx=dxSave;
	dy=dySave;
	//Restore the location.
	box.x=boxBase.x+boxSave.x;
	box.y=boxBase.y+boxSave.y;
	box.w=boxBase.w+boxSave.w;
	box.h=boxBase.h+boxSave.h;
	//And the velocity.
	xVel=xVelSave;
	yVel=yVelSave;
	//The enabled status.
	visible=visibleSave;

	//Handle block type specific variables.
	switch(type){
		case TYPE_PUSHABLE:
			xVelBase=xVelBaseSave;
			yVelBase=yVelBaseSave;
			inAir=inAirSave;
			break;
		case TYPE_CONVEYOR_BELT:
		case TYPE_SHADOW_CONVEYOR_BELT:
			speed=speedSave;
			break;
	}

	//And load the animation.
	appearance.loadAnimation();
}

void Block::reset(bool save){
	//We need to reset so we clear the animation and saves.
	if(save){
		animation=animationSave=0;
		boxSave.x=boxSave.y=boxSave.w=boxSave.h=0;
		flags=flagsSave=editorFlags;
		temp=tempSave=0;
		dx=dxSave=0;
		dy=dySave=0;
	}else{
		animation=0;
		flags=editorFlags;
		temp=0;
		dx=0;
		dy=0;
	}

	//Reset the block to its original location.
	box.x=boxBase.x;
	box.y=boxBase.y;
	box.w=boxBase.w;
	box.h=boxBase.h;

	//Reset any velocity.
	xVel=yVel=xVelBase=yVelBase=0;
	if(save)
		xVelSave=yVelSave=xVelBaseSave=yVelBaseSave=0;

	//TODO: Add proper code to save properties before script change them.
	//Reset the visible status.
	visible = visibleBase;
	if (save)
		visibleSave = visibleBase;

	//Also reset the appearance.
	appearance.resetAnimation(save);
	appearance.changeState("default");
	//NOTE: We load the animation right after changing it to prevent a transition.
	if(save)
		appearance.loadAnimation();
	
	//Some types of block requires type specific code.
	switch(type){
		case TYPE_FRAGILE:
			{
				const char* s=(flags==0)?"default":((flags==1)?"fragile1":((flags==2)?"fragile2":"fragile3"));
				appearance.changeState(s);
			}
			break;
		case TYPE_PUSHABLE:
			inAir=false;
			if(save)
				inAirSave=false;
			break;
		case TYPE_CONVEYOR_BELT:
		case TYPE_SHADOW_CONVEYOR_BELT:
			if(save)
				speed=speedSave=editorSpeed;
			else
				speed=editorSpeed;
			break;
	}
}


void Block::playAnimation(){
	switch(type){
	case TYPE_SWAP:
		appearance.changeState("activated");
		break;
	case TYPE_SWITCH:
		temp^=1;
		appearance.changeState(temp?"activated":"default");
		break;
	}
}

void Block::onEvent(int eventType){
	//Make sure we are visible, otherwise no events should be handled.
	if(!visible)
		return;
	
	//Iterator used to check if the map contains certain entries.
	map<int,int>::iterator it;

	//Check if there's a script for the event.
	it=compiledScripts.find(eventType);
	if(it!=compiledScripts.end()){
		//There is a script so execute it and check return value.
		int ret=getScriptExecutor()->executeScript(it->second,this);

		//Return value 1 means do default event process.
		//Other values are coming soon...
		if(ret!=1) return;
	}

	//Event handling.
	switch(eventType){
	case GameObjectEvent_PlayerWalkOn:
		switch(type){
		case TYPE_FRAGILE:
			flags++;
			{
				const char* s=(flags==0)?"default":((flags==1)?"fragile1":((flags==2)?"fragile2":"fragile3"));
				appearance.changeState(s);
			}
			break;
		}
		break;
	case GameObjectEvent_PlayerIsOn:
		switch(type){
		case TYPE_BUTTON:
			temp=1;
			break;
		}
		break;
	case GameObjectEvent_OnPlayerInteraction:
		switch (type) {
		case TYPE_SWITCH:
			//Make sure that the id isn't emtpy.
			if (!id.empty()) {
				parent->broadcastObjectEvent(0x10000 | (flags & 3),
					-1, id.c_str());
			} else {
				cerr << "WARNING: invalid switch id!" << endl;
			}
			break;
		}
		break;
	case GameObjectEvent_OnToggle:
		switch(type){
		case TYPE_MOVING_BLOCK:
		case TYPE_MOVING_SHADOW_BLOCK:
		case TYPE_MOVING_SPIKES:
		case TYPE_CONVEYOR_BELT:
		case TYPE_SHADOW_CONVEYOR_BELT:
			flags^=1;
			break;
		case TYPE_PORTAL:
			appearance.changeState("activated");
			break;
		case TYPE_COLLECTABLE:
			appearance.changeState("inactive");
			flags=1;
			break;
		}
		break;
	case GameObjectEvent_OnSwitchOn:
		switch(type){
		case TYPE_MOVING_BLOCK:
		case TYPE_MOVING_SHADOW_BLOCK:
		case TYPE_MOVING_SPIKES:
		case TYPE_CONVEYOR_BELT:
		case TYPE_SHADOW_CONVEYOR_BELT:
			flags&=~1;
			break;
		case TYPE_EXIT:
			appearance.changeState("default");
			break;
		}
		break;
	case GameObjectEvent_OnSwitchOff:
		switch(type){
		case TYPE_MOVING_BLOCK:
		case TYPE_MOVING_SHADOW_BLOCK:
		case TYPE_MOVING_SPIKES:
		case TYPE_CONVEYOR_BELT:
		case TYPE_SHADOW_CONVEYOR_BELT:
			flags|=1;
			break;
		case TYPE_EXIT:
			appearance.changeState("closed");
			break;
		}
		break;
	}
}

int Block::queryProperties(int propertyType,Player* obj){
	switch(propertyType){
	case GameObjectProperty_PlayerCanWalkOn:
		switch(type){
		case TYPE_BLOCK:
		case TYPE_MOVING_BLOCK:
		case TYPE_CONVEYOR_BELT:
		case TYPE_BUTTON:
		case TYPE_PUSHABLE:
			return 1;
		case TYPE_SHADOW_BLOCK:
		case TYPE_MOVING_SHADOW_BLOCK:
		case TYPE_SHADOW_CONVEYOR_BELT:
			if(obj!=NULL && obj->isShadow()) return 1;
			break;
		case TYPE_FRAGILE:
			if(flags<3) return 1;
			break;
		}
		break;
	case GameObjectProperty_IsSpikes:
		switch(type){
		case TYPE_SPIKES:
		case TYPE_MOVING_SPIKES:
			return 1;
		}
		break;
	case GameObjectProperty_Flags:
		return flags;
		break;
	default:
		break;
	}
	return 0;
}

void Block::getEditorData(std::vector<std::pair<std::string,std::string> >& obj){
	//Every block has an id.
	obj.push_back(pair<string,string>("id",id));

	//And visibility.
	obj.push_back(pair<string, string>("visible", visibleBase ? "1" : "0"));

	//Block specific properties.
	switch(type){
	case TYPE_MOVING_BLOCK:
	case TYPE_MOVING_SHADOW_BLOCK:
	case TYPE_MOVING_SPIKES:
		{
			char s[64],s0[64];
			sprintf(s,"%d",(int)movingPos.size());
			obj.push_back(pair<string,string>("MovingPosCount",s));
			obj.push_back(pair<string,string>("activated",(editorFlags&0x1)?"0":"1"));
			obj.push_back(pair<string,string>("loop",loop?"1":"0"));
			for(unsigned int i=0;i<movingPos.size();i++){
				sprintf(s0+1,"%u",i);
				sprintf(s,"%d",movingPos[i].x);
				s0[0]='x';
				obj.push_back(pair<string,string>(s0,s));
				sprintf(s,"%d",movingPos[i].y);
				s0[0]='y';
				obj.push_back(pair<string,string>(s0,s));
				sprintf(s,"%d",movingPos[i].w);
				s0[0]='t';
				obj.push_back(pair<string,string>(s0,s));
			}
		}
		break;
	case TYPE_CONVEYOR_BELT:
	case TYPE_SHADOW_CONVEYOR_BELT:
		{
			char s[64];
			obj.push_back(pair<string,string>("activated",(editorFlags&0x1)?"0":"1"));
			sprintf(s,"%d",editorSpeed);
			obj.push_back(pair<string,string>("speed10",s));
		}
		break;
	case TYPE_PORTAL:
		obj.push_back(pair<string,string>("automatic",(editorFlags&0x1)?"1":"0"));
		obj.push_back(pair<string,string>("destination",destination));
		break;
	case TYPE_BUTTON:
	case TYPE_SWITCH:
		{
			string s;
			switch(editorFlags&0x3){
			case 1:
				s="on";
				break;
			case 2:
				s="off";
				break;
			default:
				s="toggle";
				break;
			}
			obj.push_back(pair<string,string>("behaviour",s));
		}
		break;
	case TYPE_NOTIFICATION_BLOCK:
		{
			string value=message;
			//Change \n with the characters '\n'.
			while(value.find('\n',0)!=string::npos){
				size_t pos=value.find('\n',0);
				value=value.replace(pos,1,"\\n");
			}

			obj.push_back(pair<string,string>("message",value));
		}
		break;
	case TYPE_FRAGILE:
		{
			char s[64];
			sprintf(s,"%d",editorFlags);
			obj.push_back(pair<string,string>("state",s));
		}
		break;
	}
}

void Block::setEditorData(std::map<std::string,std::string>& obj){
	//Iterator used to check if the map contains certain entries.
	map<string,string>::iterator it;

	//Check if the data contains the id block.
	it=obj.find("id");
	if(it!=obj.end()){
		//Set the id of the block.
		id=obj["id"];
	}

	//Check if the data contains the visibility
	it = obj.find("visible");
	if (it != obj.end()) {
		//Set the visibility.
		const string& s = it->second;
		visibleBase = (s == "true" || atoi(s.c_str()));
	}

	//Block specific properties.
	switch(type){
	case TYPE_MOVING_BLOCK:
	case TYPE_MOVING_SHADOW_BLOCK:
	case TYPE_MOVING_SPIKES:
		{
			//Make sure that the editor data contains MovingPosCount.
			it=obj.find("MovingPosCount");
			if(it!=obj.end()){
				char s0[64];
				int m=atoi(obj["MovingPosCount"].c_str());
				movingPos.clear();
				for(int i=0;i<m;i++){
					SDL_Rect r={0,0,0,0};
					sprintf(s0+1,"%d",i);
					s0[0]='x';
					r.x=atoi(obj[s0].c_str());
					s0[0]='y';
					r.y=atoi(obj[s0].c_str());
					s0[0]='t';
					r.w=atoi(obj[s0].c_str());
					movingPos.push_back(r);
				}
			}

			//Check if the activated or disabled key is in the data.
			//NOTE: 'disabled' is obsolete in V0.5.
			it=obj.find("activated");
			if(it!=obj.end()){
				const string& s=it->second;
				editorFlags=0;
				if(!(s=="true" || atoi(s.c_str()))) editorFlags|=0x1;
				flags=flagsSave=editorFlags;
			}else{
				it=obj.find("disabled");
				if(it!=obj.end()){
					const string& s=it->second;
					editorFlags=0;
					if(s=="true" || atoi(s.c_str())) editorFlags|=0x1;
					flags=flagsSave=editorFlags;
				}
			}

			//Check if the loop key is in the data.
			it=obj.find("loop");
			if(it!=obj.end()){
				const string& s=it->second;
				loop=false;
				if(s=="true" || atoi(s.c_str()))
					loop=true;
			}

		}
		break;
	case TYPE_CONVEYOR_BELT:
	case TYPE_SHADOW_CONVEYOR_BELT:
		{
			//Check if there's a speed key in the editor data.
			//NOTE: 'speed' is obsolete in V0.5.
			it=obj.find("speed10");
			if(it!=obj.end()){
				editorSpeed=atoi(it->second.c_str());
				speed=speedSave=editorSpeed;
			}else{
				it = obj.find("speed");
				if (it != obj.end()){
					editorSpeed = atoi(it->second.c_str()) * 10;
					speed = speedSave = editorSpeed;
				}
			}

			//Check if the activated or disabled key is in the data.
			//NOTE: 'disabled' is obsolete in V0.5.
			it=obj.find("activated");
			if(it!=obj.end()){
				const string& s=it->second;
				editorFlags=0;
				if(!(s=="true" || atoi(s.c_str()))) editorFlags|=0x1;
				flags=flagsSave=editorFlags;
			}else{
				it=obj.find("disabled");
				if(it!=obj.end()){
					const string& s=it->second;
					editorFlags=0;
					if(s=="true" || atoi(s.c_str())) editorFlags|=0x1;
					flags=flagsSave=editorFlags;
				}
			}
		}
		break;
	case TYPE_PORTAL:
		{
			//Check if the automatic key is in the data.
			it=obj.find("automatic");
			if(it!=obj.end()){
				const string& s=it->second;
				editorFlags=0;
				if(s=="true" || atoi(s.c_str())) editorFlags|=0x1;
				flags=flagsSave=editorFlags;
			}

			//Check if the destination key is in the data.
			it=obj.find("destination");
			if(it!=obj.end()){
				destination=it->second;
			}
		}
		break;
	case TYPE_BUTTON:
	case TYPE_SWITCH:
		{
			//Check if the behaviour key is in the data.
			it=obj.find("behaviour");
			if(it!=obj.end()){
				const string& s=it->second;
				editorFlags=0;
				if(s=="on" || s==_("On")) editorFlags|=1;
				else if(s=="off" || s==_("Off")) editorFlags|=2;
				flags=flagsSave=editorFlags;
			}
		}
		break;
	case TYPE_NOTIFICATION_BLOCK:
		{
			//Check if the message key is in the data.
			it=obj.find("message");
			if(it!=obj.end()){
				message=it->second;
				//Change the characters '\n' to a real \n
				while(message.find("\\n")!=string::npos){
					message=message.replace(message.find("\\n"),2,"\n");
				}
			}
		}
		break;
	case TYPE_FRAGILE:
		{
			//Check if the status is in the data.
			it=obj.find("state");
			if(it!=obj.end()){
				editorFlags=atoi(it->second.c_str());
				flags=editorFlags;
				{
					const char* s=(flags==0)?"default":((flags==1)?"fragile1":((flags==2)?"fragile2":"fragile3"));
					appearance.changeState(s);
				}
			}
		}
	}
}

std::string Block::getEditorProperty(std::string property){
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

void Block::setEditorProperty(std::string property,std::string value){
	//Create a map to hold the property.
	std::map<std::string,std::string> editorData;
	editorData[property]=value;

	//And call the setEditorData method.
	setEditorData(editorData);
}

bool Block::loadFromNode(ImageManager&, SDL_Renderer&, TreeStorageNode* objNode){
	//Make sure there are enough parameters.
	if(objNode->value.size()<3)
		return false;

	//Load the type and location.
	int type=Game::blockNameMap[objNode->value[0]];
	int x=atoi(objNode->value[1].c_str());
	int y=atoi(objNode->value[2].c_str());
	int w=50;
	int h=50;
	if(objNode->value.size()>3)
		w=atoi(objNode->value[3].c_str());
	if(objNode->value.size()>4)
		h=atoi(objNode->value[4].c_str());
	//Call the init method.
	init(x,y,w,h,type);

	//Loop through the attributes as editorProperties.
	map<string,string> obj;
	for(map<string,vector<string> >::iterator i=objNode->attributes.begin();i!=objNode->attributes.end();++i){
		if(i->second.size()>0) obj[i->first]=i->second[0];
	}
	setEditorData(obj);

	//Loop through the subNodes.
	for(unsigned int i=0;i<objNode->subNodes.size();i++){
		//FIXME: Ugly variable naming.
		TreeStorageNode* obj=objNode->subNodes[i];
		if(obj==NULL) continue;

		//Check for a script block.
		if(obj->name=="script" && !obj->value.empty()){
			map<string,int>::iterator it=Game::gameObjectEventNameMap.find(obj->value[0]);
			if(it!=Game::gameObjectEventNameMap.end()){
				int eventType=it->second;
				const std::string& script=obj->attributes["script"][0];
				if(!script.empty()) scripts[eventType]=script;
			}
		}
	}
	
	return true;
}

void Block::prepareFrame(){
	//Reset the delta variables.
	dx=dy=0;
	//Also reset the velocity, these should be set in the move method.
	if(type!=TYPE_PUSHABLE)
		xVel=yVel=0;
}

/*//debug
int block_test_count=-1;
bool block_test_only=false;*/

void Block::move(){
	//Make sure we are visible, if not return.
	if(!visible)
		return;
	
	//First update the animation of the appearance.
	appearance.updateAnimation();
	
	//Block specific move code.
	switch(type){
	case TYPE_MOVING_BLOCK:
	case TYPE_MOVING_SHADOW_BLOCK:
	case TYPE_MOVING_SPIKES:
		{
			/*//debug
			if(block_test_only || parent->time==416){
				cout<<"Time:"<<(parent->time)<<" Recorded:"<<block_test_count<<" Coord:"<<box.x<<","<<box.y<<endl;
				block_test_only=false;
			}*/

			//Make sure the block is enabled, if so increase the time.
			if(!(flags&0x1)) temp++;
			int t=temp;
			SDL_Rect r0={0,0,0,0},r1;
			dx=0;
			dy=0;

			//Loop through the moving positions.
			for(unsigned int i=0;i<movingPos.size();i++){
				r1.x=movingPos[i].x;
				r1.y=movingPos[i].y;
				r1.w=movingPos[i].w;
				if(t==0&&r1.w==0){ // time == 0 means the block deactivates at this point automatically
					r1.w=1;
					flags|=0x1;
				}
				if(t>=0 && t<(int)r1.w){
					int newX=boxBase.x+(int)(float(r0.x)+(float(r1.x)-float(r0.x))*float(t)/float(r1.w));
					int newY=boxBase.y+(int)(float(r0.y)+(float(r1.y)-float(r0.y))*float(t)/float(r1.w));
					//Calculate the delta and velocity.
					xVel=dx=newX-box.x;
					yVel=dy=newY-box.y;
					//Set the new location of the moving block.
					box.x=newX;
					box.y=newY;
					return;
				} else if (t == (int)r1.w && i == movingPos.size() - 1) {
					//If the time is the time of the movingPosition then set it equal to the location.
					//We do this to prevent a slight edge between normal blocks and moving blocks.
					int newX=boxBase.x+r1.x;
					int newY=boxBase.y+r1.y;
					xVel=dx=newX-box.x;
					yVel=dy=newY-box.y;
					box.x=newX;
					box.y=newY;
					return;
				}
				t-=r1.w;
				r0.x=r1.x;
				r0.y=r1.y;
			}
			//Only reset the stuff when we're looping.
			if(loop){
				//Set the time back to zero.
				temp=0;
				//Calculate the delta movement.
				if(!movingPos.empty() && movingPos.back().x==0 && movingPos.back().y==0){
					dx=boxBase.x-box.x;
					dy=boxBase.y-box.y;
				}
				//Set the movingblock back to it's initial location.
				box.x=boxBase.x;
				box.y=boxBase.y;
			}
		}
		break;
	case TYPE_BUTTON:
		{
			//Check the third bit of flags to see if temp changed.
			int new_flags=temp?4:0;
			if((flags^new_flags)&4){
				//The button has been pressed or unpressed so change the third bit on flags.
				flags=(flags&~4)|new_flags;

				if(parent && (new_flags || (flags&3)==0)){
					//Make sure that id isn't empty.
					if(!id.empty()){
						parent->broadcastObjectEvent(0x10000|(flags&3),-1,id.c_str());
					}else{
						cerr<<"WARNING: invalid button id!"<<endl;
					}
				}
			}
			temp=0;
		}
		break;
	case TYPE_CONVEYOR_BELT:
	case TYPE_SHADOW_CONVEYOR_BELT:
		//Increase the conveyor belt animation.
		if((flags&1)==0){
			//Since now 1 speed = 0.1 pixel/s we need some more sophisticated calculation.
			int a = animation + speed, d = 0;
			if (a < 0) {
				//Add a delta value to make it positive
				d = (((-a) / 500) + 1) * 500;
			}

			//Set the velocity NOTE This isn't the actual velocity of the block, but the speed of the player/shadow standing on it.
			xVel = (a + d) / 10 - (animation + d) / 10;

			//Update animation value
			animation = (a + d) % 500;
			assert(animation >= 0);
		} else {
			//Clear the velocity NOTE This isn't the actual velocity of the block, but the speed of the player/shadow standing on it.
			xVel = 0;
		}
		break;
	case TYPE_PUSHABLE:
		{
			//Update the vertical velocity, horizontal is set by the player.
			if(inAir==true){
				yVel+=1;
			
				//Cap fall speed to 13.
				if(yVel>13)
					yVel=13;
			}
			if(objCurrentStand!=NULL){
				//Now get the velocity and delta of the object the player is standing on.
				SDL_Rect v=objCurrentStand->getBox(BoxType_Velocity);
				SDL_Rect delta=objCurrentStand->getBox(BoxType_Delta);
				
				switch(objCurrentStand->type){
				//For conveyor belts the velocity is transfered.
				case TYPE_CONVEYOR_BELT:
				case TYPE_SHADOW_CONVEYOR_BELT:
					{
						xVelBase+=v.x;
					}
					break;
				//In other cases, such as, player on shadow, player on crate... the change in x position must be considered.
				default:
					{
						if(delta.x != 0)
							xVelBase+=delta.x;
					}
				break;
				}
				//NOTE: Only copy the velocity of the block when moving down.
				//Upwards is automatically resolved before the player is moved.
				if(v.y>0)
					yVelBase=v.y;
				else
					yVelBase=0;
			}

			//Set the object the player is currently standing to NULL.
			objCurrentStand=NULL;

			//Store the location of the player.
			int lastX=box.x;
			int lastY=box.y;

			//An array that will hold all the GameObjects that are involved in the collision/movement.
			vector<Block*> objects;
			//All the blocks have moved so if there's collision with the player, the block moved into him.
			for(unsigned int o=0;o<parent->levelObjects.size();o++){
				//Make sure to only check visible blocks.
				if(!parent->levelObjects[o]->visible)
					continue;
				//Make sure we aren't the block.
				if(parent->levelObjects[o]==this)
					continue;
				//Make sure the object is solid for the player.
				if(!parent->levelObjects[o]->queryProperties(GameObjectProperty_PlayerCanWalkOn,&parent->player))
					continue;
				
				//Check for collision.
				if(checkCollision(box,parent->levelObjects[o]->getBox()))
					objects.push_back(parent->levelObjects[o]);
			}
			//There was collision so try to resolve it.
			if(!objects.empty()){
				//FIXME: When multiple moving blocks are overlapping the player can be "bounced" off depending on the block order.
				for(unsigned int o=0;o<objects.size();o++){
					SDL_Rect r=objects[o]->getBox();
					SDL_Rect delta=objects[o]->getBox(BoxType_Delta);
					
					//Check on which side of the box the player is.
					if(delta.x!=0){
						if(delta.x>0){
							if((r.x+r.w)-box.x<=delta.x)
								box.x=r.x+r.w;
						}else{
							if((box.x+box.w)-r.x<=-delta.x)
								box.x=r.x-box.w;
						}
					}
					if(delta.y!=0){
						if(delta.y>0){
							if((r.y+r.h)-box.y<=delta.y)
								box.y=r.y+r.h;
						}else{
							if((box.y+box.h)-r.y<=-delta.y)
								box.y=r.y-box.h;
						}
					}
				}
			}
			
			//Reuse the objects array, this time for blocks the block moves into.
			objects.clear();
			//Determine the collision frame.
			SDL_Rect frame={box.x,box.y,box.w,box.h};
			//Keep the horizontal movement of the block in mind.
			if(xVel+xVelBase>=0){
				frame.w+=(xVel+xVelBase);
			}else{
				frame.x+=(xVel+xVelBase);
				frame.w-=(xVel+xVelBase);
			}
			//And the vertical movement.
			if(yVel+yVelBase>=0){
				frame.h+=(yVel+yVelBase);
			}else{
				frame.y+=(yVel+yVelBase);
				frame.h-=(yVel+yVelBase);
			}
			//Loop through the game objects.
			for(unsigned int o=0; o<parent->levelObjects.size(); o++){
				//Make sure the object is visible.
				if(!parent->levelObjects[o]->visible)
					continue;
				//Make sure we aren't the block.
				if(parent->levelObjects[o]==this)
					continue;
				//Check if the player can collide with this game object.
				if(!parent->levelObjects[o]->queryProperties(GameObjectProperty_PlayerCanWalkOn,&parent->player))
					continue;
				
				//Check if the block is inside the frame.
				if(checkCollision(frame,parent->levelObjects[o]->getBox()))
					objects.push_back(parent->levelObjects[o]);
			}
			//Horizontal pass.
			if(xVel+xVelBase!=0){
				box.x+=xVel+xVelBase;
				for(unsigned int o=0;o<objects.size();o++){
					SDL_Rect r=objects[o]->getBox();
					if(!checkCollision(box,r))
						continue;

					if(xVel+xVelBase>0){
						//We came from the left so the right edge of the player must be less or equal than xVel+xVelBase.
						if((box.x+box.w)-r.x<=xVel+xVelBase)
							box.x=r.x-box.w;
					}else{
						//We came from the left so the right edge of the player must be less or equal than xVel+xVelBase.
						if(box.x-(r.x+r.w)<=-(xVel+xVelBase))
							box.x=r.x+r.w;
					}
				}
			}
			//Some variables that are used in vertical movement.
			Block* lastStand=NULL;
			inAir=true;

			//Vertical pass.
			if(yVel+yVelBase!=0){
				box.y+=yVel+yVelBase;
				for(unsigned int o=0;o<objects.size();o++){
					SDL_Rect r=objects[o]->getBox();
					if(!checkCollision(box,r))
						continue;

					//Now check how we entered the block (vertically or horizontally).
					if(yVel+yVelBase>0){
						//We came from the top so the bottom edge of the player must be less or equal than yVel+yVelBase.
						if((box.y+box.h)-r.y<=yVel+yVelBase){
							//NOTE: lastStand is handled later since the player can stand on only one block at the time.

							//Check if there's already a lastStand.
							if(lastStand){
								//There is one, so check 'how much' the player is on the blocks.
								SDL_Rect r=objects[o]->getBox();
								int w=0;
								if(box.x+box.w>r.x+r.w)
									w=(r.x+r.w)-box.x;
								else
									w=(box.x+box.w)-r.x;

								//Do the same for the other box.
								r=lastStand->getBox();
								int w2=0;
								if(box.x+box.w>r.x+r.w)
									w2=(r.x+r.w)-box.x;
								else
									w2=(box.x+box.w)-r.x;

								//NOTE: It doesn't matter which block the player is on if they are both stationary.
								SDL_Rect v=objects[o]->getBox(BoxType_Velocity);
								SDL_Rect v2=lastStand->getBox(BoxType_Velocity);

								if(v.y==v2.y){
									if(w>w2)
										lastStand=objects[o];
								}else if(v.y<v2.y){
									lastStand=objects[o];
								}
							}else{
								lastStand=objects[o];
							}
						}
					}else{
						//We came from the bottom so the upper edge of the player must be less or equal than yVel+yVelBase.
						if(box.y-(r.y+r.h)<=-(yVel+yVelBase)){
							box.y=r.y+r.h;
							yVel=0;
						}
					}
				}
			}
			if(lastStand){
				inAir=false;
				yVel=1;
				SDL_Rect r=lastStand->getBox();
				box.y=r.y-box.h;
			}

			//Block will currently be standing on whatever it was last standing on.
			objCurrentStand=lastStand;

			dx=box.x-lastX;
			dy=box.y-lastY;
			xVel=0;
			xVelBase=0;
		}
		break;
	}
}
