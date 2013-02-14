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
#include "Player.h"
#include "Block.h"
#include "Functions.h"
#include "Globals.h"
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
using namespace std;

Block::Block(int x,int y,int type,Game* parent):
	GameObject(parent),
	animation(0),
	animationSave(0),
	flags(0),
	flagsSave(0),
	temp(0),
	tempSave(0),
	dx(0),
	xSave(0),
	dy(0),
	ySave(0),
	loop(true),
	speed(0),
	speedSave(0),
	editorSpeed(0),
	editorFlags(0)
{
	//First set the location and size of the box.
	//The default size is 50x50.
	box.x=x;
	box.y=y;
	box.w=50;
	box.h=50;

	//Also set the
	boxBase.x=x;
	boxBase.y=y;
	boxBase.w=50;
	boxBase.h=50;

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
	inAir=true;
	inAirSave=true;
	xVel=yVel=xVelBase=yVelBase=0;
	xVelSave=yVelSave=xVelBaseSave=yVelBaseSave=0;

	//And load the appearance.
	objThemes.getBlock(type)->createInstance(&appearance);
}

Block::~Block(){}

void Block::show(){
	//Check if the block is visible.
	if(checkCollision(camera,box)==true || (stateID==STATE_LEVEL_EDITOR && checkCollision(camera,boxBase)==true)){
		SDL_Rect r={0,0,50,50};
		
		//What we need to draw depends on the type of block.
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
		case TYPE_CONVEYOR_BELT:
		case TYPE_SHADOW_CONVEYOR_BELT:
			if(animation){
				r.x=50-animation;
				r.w=animation;
				appearance.draw(screen,box.x-camera.x-50+animation,box.y-camera.y,&r);
				r.x=0;
				r.w=50-animation;
				appearance.draw(screen,box.x-camera.x+animation,box.y-camera.y,&r);
				return;
			}
			break;
		case TYPE_NOTIFICATION_BLOCK:
			if(message.empty()==false){
				appearance.draw(screen, box.x - camera.x, box.y - camera.y);
				return;
			}
			break;
		}

		//Always draw the base.
		appearance.drawState("base", screen, boxBase.x - camera.x, boxBase.y - camera.y);
		//Now draw normal.
		appearance.draw(screen, box.x - camera.x, box.y - camera.y);

		//Some types need to draw something on top of the base/default.
		switch(type){
		case TYPE_BUTTON:
			if(flags&4){
				if(animation<5) animation++;
			}else{
				if(animation>0) animation--;
			}
			appearance.drawState("button",screen,box.x-camera.x,box.y-camera.y-5+animation);
			break;
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
		return r;
	case BoxType_Current:
		return box;
	}
	return r;
}

void Block::setLocation(int x,int y){
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

void Block::saveState(){
	animationSave=animation;
	flagsSave=flags;
	tempSave=temp;
	xSave=box.x-boxBase.x;
	ySave=box.y-boxBase.y;
	xVelSave=xVel;
	yVelSave=yVel;
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
	//Restore the location.
	box.x=boxBase.x+xSave;
	box.y=boxBase.y+ySave;
	//And the velocity.
	xVel=xVelSave;
	yVel=yVelSave;

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
		animation=animationSave=xSave=ySave=0;
		flags=flagsSave=editorFlags;
		temp=tempSave=0;
	}else{
		animation=0;
		flags=editorFlags;
		temp=0;
	}

	//Reset the block to it's original location.
	box.x=boxBase.x;
	box.y=boxBase.y;

	//Reset any velocity.
	xVel=yVel=xVelBase=yVelBase=0;
	if(save)
		xVelSave=yVelSave=xVelBaseSave=yVelBaseSave=0;

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


void Block::playAnimation(int flags){
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
	//Iterator used to check if the map contains certain entries.
	map<int,string>::iterator it;

	//Check if there's a script for the event.
	it=scripts.find(eventType);
	if(it!=scripts.end()){
		//There is a script so execute it and return.
		getScriptExecutor()->executeScript(it->second);
		return;
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

	//Block specific properties.
	switch(type){
	case TYPE_MOVING_BLOCK:
	case TYPE_MOVING_SHADOW_BLOCK:
	case TYPE_MOVING_SPIKES:
		{
			char s[64],s0[64];
			sprintf(s,"%d",(int)movingPos.size());
			obj.push_back(pair<string,string>("MovingPosCount",s));
			obj.push_back(pair<string,string>("disabled",(editorFlags&0x1)?"1":"0"));
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
			obj.push_back(pair<string,string>("disabled",(editorFlags&0x1)?"1":"0"));
			sprintf(s,"%d",editorSpeed);
			obj.push_back(pair<string,string>("speed",s));
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

			//Check if the disabled key is in the data.
			it=obj.find("disabled");
			if(it!=obj.end()){
				string s=obj["disabled"];
				editorFlags=0;
				if(s=="true" || atoi(s.c_str())) editorFlags|=0x1;
				flags=flagsSave=editorFlags;
			}

			//Check if the loop key is in the data.
			it=obj.find("loop");
			if(it!=obj.end()){
				string s=obj["loop"];
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
			it=obj.find("speed");
			if(it!=obj.end()){
				editorSpeed=atoi(obj["speed"].c_str());
				speed=speedSave=editorSpeed;
			}

			//Check if the disabled key is in the data.
			it=obj.find("disabled");
			if(it!=obj.end()){
				string s=obj["disabled"];
				editorFlags=0;
				if(s=="true" || atoi(s.c_str())) editorFlags|=0x1;
				flags=flagsSave=editorFlags;
			}
		}
		break;
	case TYPE_PORTAL:
		{
			//Check if the automatic key is in the data.
			it=obj.find("automatic");
			if(it!=obj.end()){
				string s=obj["automatic"];
				editorFlags=0;
				if(s=="true" || atoi(s.c_str())) editorFlags|=0x1;
				flags=flagsSave=editorFlags;
			}

			//Check if the destination key is in the data.
			it=obj.find("destination");
			if(it!=obj.end()){
				destination=obj["destination"];
			}
		}
		break;
	case TYPE_BUTTON:
	case TYPE_SWITCH:
		{
			//Check if the behaviour key is in the data.
			it=obj.find("behaviour");
			if(it!=obj.end()){
				string s=obj["behaviour"];
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
				message=obj["message"];
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
				editorFlags=atoi(obj["state"].c_str());
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
				if(t==0&&r1.w==0){
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
				}else if(t==(int)r1.w){
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
						cerr<<"Warning: invalid button id!"<<endl;
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
			animation=(animation+speed)%50;
			if(animation<0) animation+=50;

			//Set the velocity NOTE This isn't the actual velocity of the block, but the speed of the player/shadow standing on it.
			xVel=speed;
		}
		break;
	case TYPE_PUSHABLE:
		{
			objCurrentStand=NULL;
			
			if(inAir==true){
				yVel+=1;
				
				//Cap fall speed to 13.
				if(yVel>13){
					yVel=13;
				}
			}
			
			//An array that will hold all the GameObjects that are involved in the collision/movement.
			vector<GameObject*> objects;
			
			//Determine the collision frame.
			SDL_Rect frame={box.x,box.y,box.w,box.h};
			//Keep the horizontal movement of the player in mind.
			if(xVel+xVelBase>=0) {
				frame.w+=(xVel+xVelBase);
			}else{
				frame.x+=(xVel+xVelBase);
				frame.w-=(xVel+xVelBase);
			}
			//And the vertical movement.
			if(yVel+yVelBase>=0) {
				frame.h+=(yVel+yVelBase);
			}else{
				frame.y+=(yVel+yVelBase);
				frame.h-=(yVel+yVelBase);
			}

			//Loop through the game objects.
			for(unsigned int o=0; o<parent->levelObjects.size(); o++){
				//Check if the player can collide with this game object.
				//NOTE: As opposed to the player and the shadow, pushable blocks can collide with spikes.
				if(!parent->levelObjects[o]->queryProperties(GameObjectProperty_PlayerCanWalkOn,NULL)
					&& !(parent->levelObjects[o]->type==TYPE_SPIKES || parent->levelObjects[o]->type==TYPE_MOVING_SPIKES))
					continue;

				if(parent->levelObjects[o]==this)
					continue;

				//Check if the block is inside the frame.
				if(checkCollision(frame,parent->levelObjects[o]->getBox())){
					objects.push_back(parent->levelObjects[o]);
					continue;
				}

				//Additional checks need to be made for moving blocks.
				if(parent->levelObjects[o]->type==TYPE_MOVING_BLOCK || parent->levelObjects[o]->type==TYPE_MOVING_SHADOW_BLOCK || parent->levelObjects[o]->type==TYPE_MOVING_SPIKES) {
					//Check the movement of these blocks to see if they will collide.
					SDL_Rect v=parent->levelObjects[o]->getBox(BoxType_Velocity);
					SDL_Rect r=parent->levelObjects[o]->getBox();
					r.x+=v.x;
					r.y+=v.y;
					if(checkCollision(frame,r)) {
						objects.push_back(parent->levelObjects[o]);
					}
				}
			}

			//The current location of the player, used to set the player back if he's squashed to prevent displacement.
			int lastX=box.x;
			int lastY=box.y;

			//Move the player.
			box.x+=xVel;

			//Loop through the objects related to the (horizontal) collision/movement.
			for(unsigned int o=0; o<objects.size(); o++){
				//Get the collision box of the levelobject.
				SDL_Rect r=objects[o]->getBox();

				//Check collision with the player.
				//NOTE: Although the object is inside the collision frame we need to check if it collides with the player box (xVel applied).
				if(checkCollision(box,r)){
					//We have collision, get the velocity of the box.
					SDL_Rect v=objects[o]->getBox(BoxType_Delta);

					//Check on which side of the box the player is.
					if(box.x + box.w/2 <= r.x + r.w/2){
						//The left side of the block.
						if(xVel+xVelBase>v.x){
							if(box.x>r.x-box.w){
								if(objects[o]->type==TYPE_PUSHABLE){
									//box.x=r.x-box.w+(xVel+xVelBase)/2;
									(dynamic_cast<Block*>(objects[o]))->xVel=(xVel+xVelBase)/2;
								}
								box.x=r.x-box.w;
							}
						}
					}else{
						//The right side of the block.
						if(xVel+xVelBase<v.x){
							if(box.x<r.x+r.w){
								if(objects[o]->type==TYPE_PUSHABLE){
									//box.x=r.x-box.w+(xVel+xVelBase)/2;
									(dynamic_cast<Block*>(objects[o]))->xVel=(xVel+xVelBase)/2;
								}
								box.x=r.x+r.w;
							}
						}
					}
				}
			}

			//Now apply the yVel. (gravity, jumping, etc..)
			box.y+=yVel;

			//Assume we are in air and are able to move unless proven otherwise (???).
			inAir=true;

			//Loop through all the objects related to the (vertical) collision/movement.
			for(unsigned int o=0; o<objects.size(); o++){
				//Get the collision box of the levelobject.
				SDL_Rect r=objects[o]->getBox();
				
				//NOTE: Although the object is inside the collision frame we need to check if it collides with the player box (yVel applied).
				if(checkCollision(box,r)){
					//Get the velocity of the gameobject.
					SDL_Rect v=objects[o]->getBox(BoxType_Delta);

					//Check which side of the object the player is.
					if(box.y+box.h/2<=r.y+r.h/2){
						if(yVel>=v.y || yVel>=0){
							inAir=false;
							box.y=r.y-box.h;
							yVel=1;

							objCurrentStand=objects[o];
						}
					}else{
						//FIXME: The player can have a yVel of 0 and get squashed if he is standing on the other.
						if(yVel<=v.y+1){
							yVel=v.y>0?v.y:0;
							if(box.y<r.y+r.h){
								box.y=r.y+r.h;
							}
						}
					}
				}
			}

			dx=box.x-lastX;
			dy=box.y-lastY;
			xVel=0;
		}
		break;
	}
}
