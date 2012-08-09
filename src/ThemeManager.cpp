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

#include "ThemeManager.h"
#include "POASerializer.h"
#include "Functions.h"
#include "FileManager.h"
#include "Game.h"
#ifdef __APPLE__
#include <SDL_gfx/SDL_rotozoom.h>
#else
#include <SDL/SDL_rotozoom.h>
#endif
#include <string.h>
#include <iostream>
using namespace std;

//The ThemeStack that is be used by the GameState.
ThemeStack objThemes;

bool ThemeManager::loadFile(const string& fileName){
	POASerializer objSerializer;
	TreeStorageNode objNode;

	//First we destroy the current ThemeManager.
	destroy();

	//Now we try to load the file, if it fails we return false.
	if(!objSerializer.loadNodeFromFile(fileName.c_str(),&objNode,true)){
		cerr<<"ERROR: Unable to open theme file: "<<fileName<<endl;
		return false;
	}
	
	//Set the themePath.
	themePath=pathFromFileName(fileName);

	//Retrieve the name of the theme from the file.
	{
		vector<string> &v=objNode.attributes["name"];
		if(!v.empty()) themeName=v[0];
	}
	
	//Reset themeable colors to default
	themeTextColor.r=themeTextColor.g=themeTextColor.b=0;
	themeTextColorDialog.r=themeTextColorDialog.g=themeTextColorDialog.b=0;
	
	//Read themeable colors if any
	vector<string> &ct=objNode.attributes["textColor"];
	if(!ct.empty()){
		themeTextColor.r=atoi(ct[0].c_str());
		themeTextColor.g=atoi(ct[1].c_str());
		themeTextColor.b=atoi(ct[2].c_str());
	}
	
	vector<string> &ct2=objNode.attributes["textColorDialog"];
	if(!ct2.empty()){
		themeTextColorDialog.r=atoi(ct2[0].c_str());
		themeTextColorDialog.g=atoi(ct2[1].c_str());
		themeTextColorDialog.b=atoi(ct2[2].c_str());
	}
	
	//Loop the subnodes of the theme.
	for(unsigned int i=0;i<objNode.subNodes.size();i++){
		TreeStorageNode *obj=objNode.subNodes[i];
		
		//Check if it's a block or a background.
		if(obj->name=="block" && !obj->value.empty()){
			map<string,int>::iterator it=Game::blockNameMap.find(obj->value[0]);
			if(it!=Game::blockNameMap.end()){
				int idx=it->second;
				if(!objBlocks[idx]) objBlocks[idx]=new ThemeBlock;
				if(!objBlocks[idx]->loadFromNode(obj,themePath)){
					cerr<<"ERROR: Unable to load "<<Game::blockName[idx]<<" for theme "<<fileName<<endl;
					delete objBlocks[idx];
					objBlocks[idx]=NULL;
					return false;
				}
			}
		}else if(obj->name=="background" && !obj->value.empty()){
			if(!objBackground) objBackground=new ThemeBackground();
			if(!objBackground->addPictureFromNode(obj,themePath)){
				cerr<<"ERROR: Unable to load background for theme "<<fileName<<endl;
				delete objBackground;
				objBackground=NULL;
				return false;
			}
		}else if(obj->name=="character" && !obj->value.empty()){
			if(obj->value[0]=="Shadow"){
				if(!shadow) shadow=new ThemeCharacter();
				if(!shadow->loadFromNode(obj,themePath)){
					cerr<<"ERROR: Unable to load shadow for theme "<<fileName<<endl;
					delete shadow;
					shadow=NULL;
					return false;
				}
			}else if(obj->value[0]=="Player"){
				if(!player) player=new ThemeCharacter();
				if(!player->loadFromNode(obj,themePath)){
					cerr<<"ERROR: Unable to load player for theme "<<fileName<<endl;
					delete player;
					player=NULL;
					return false;
				}
			}
		}else if(obj->name=="menuBackground" && !obj->value.empty()){
			if(!menuBackground) menuBackground=new ThemeBackground();
			if(!menuBackground->addPictureFromNode(obj,themePath)){
				cerr<<"ERROR: Unable to load background for theme "<<fileName<<endl;
				delete menuBackground;
				menuBackground=NULL;
				return false;
			}
		}else if(obj->name=="menu" && obj->value[0]=="Block"){
			if(!menuBlock) menuBlock=new ThemeBlock;
			if(!menuBlock->loadFromNode(obj,themePath)){
				cerr<<"ERROR: Unable to load menu block for theme "<<fileName<<endl;
				delete menuBlock;
				menuBlock=NULL;
				return false;
			}
		}
	}
	
	//Done and nothing went wrong so return true.
	return true;
}

bool ThemeBlock::loadFromNode(TreeStorageNode* objNode, string themePath){
	destroy();
	
	//Loop the subNodes.
	for(unsigned int i=0;i<objNode->subNodes.size();i++){
		TreeStorageNode *obj=objNode->subNodes[i];
		
		//Check if the subnode is an editorPicture or a blockState.
		if(obj->name=="editorPicture"){
			if(!editorPicture.loadFromNode(obj,themePath)) return false;
		}else if(obj->name=="blockState" && !obj->value.empty()){
			string& s=obj->value[0];
			map<string,ThemeBlockState*>::iterator it=blockStates.find(s);
			if(it==blockStates.end()) blockStates[s]=new ThemeBlockState;
			if(!blockStates[s]->loadFromNode(obj,themePath)) return false;
		}
	}
	
	//Done and nothing went wrong so return true.
	return true;
}

bool ThemeBlockState::loadFromNode(TreeStorageNode* objNode, string themePath){
	destroy();
	
	//Retrieve the oneTimeAnimation attribute.
	{
		vector<string> &v=objNode->attributes["oneTimeAnimation"];
		
		//Check if there are enough values for the oneTimeAnimation attribute.
		if(v.size()>=2 && !v[0].empty()){
			oneTimeAnimationLength=atoi(v[0].c_str());
			nextState=v[1];
		}
	}
	
	//Loop the subNodes.
	for(unsigned int i=0;i<objNode->subNodes.size();i++){
		TreeStorageNode *obj=objNode->subNodes[i];
		if(obj->name=="object"){
			ThemeObject *obj1=new ThemeObject();
			if(!obj1->loadFromNode(obj,themePath)){
				delete obj1;
				return false;
			}
			themeObjects.push_back(obj1);
		}
	}
	
	//Done and nothing went wrong so return true.
	return true;
}

bool ThemeCharacter::loadFromNode(TreeStorageNode* objNode,string themePath){
	destroy();
	
	//Loop the subNodes.
	for(unsigned int i=0;i<objNode->subNodes.size();i++){
		TreeStorageNode *obj=objNode->subNodes[i];
		
		//Check if the subnode is an characterState.
		if(obj->name=="characterState" && !obj->value.empty()){
			string& s=obj->value[0];
			map<string,ThemeCharacterState*>::iterator it=characterStates.find(s);
			if(it==characterStates.end()) characterStates[s]=new ThemeCharacterState;
			if(!characterStates[s]->loadFromNode(obj,themePath)) return false;
		}
	}
	
	//Done and nothing went wrong so return true.
	return true;
}


bool ThemeCharacterState::loadFromNode(TreeStorageNode* objNode,string themePath){
	destroy();
	
	//Retrieve the oneTimeAnimation attribute.
	{
		vector<string> &v=objNode->attributes["oneTimeAnimation"];
		
		//Check if there are enough values for the oneTimeAnimation attribute.
		if(v.size()>=2 && !v[0].empty()){
			oneTimeAnimationLength=atoi(v[0].c_str());
			nextState=v[1];
		}
	}
	
	//Loop the subNodes.
	for(unsigned int i=0;i<objNode->subNodes.size();i++){
		TreeStorageNode *obj=objNode->subNodes[i];
		if(obj->name=="object"){
			ThemeObject *obj1=new ThemeObject();
			if(!obj1->loadFromNode(obj,themePath)){
				delete obj1;
				return false;
			}
			themeObjects.push_back(obj1);
		}
	}
	
	//Done and nothing went wrong so return true.
	return true;
}

bool ThemeObject::loadFromNode(TreeStorageNode* objNode,string themePath){
	destroy();
	
	//Retrieve the animation attribute.
	{
		vector<string> &v=objNode->attributes["animation"];
		if(v.size()>=2){
			animationLength=atoi(v[0].c_str());
			animationLoopPoint=atoi(v[1].c_str());
		}
	}
	//Retrieve the oneTimeAnimation attribute.
	{
		vector<string> &v=objNode->attributes["oneTimeAnimation"];
		if(v.size()>=2){
			animationLength=atoi(v[0].c_str());
			animationLoopPoint=atoi(v[1].c_str())|0x80000000;
		}
	}
	//Retrieve the invisibleAtRunTime attribute.
	{
		vector<string> &v=objNode->attributes["invisibleAtRunTime"];
		if(!v.empty() && !v[0].empty()){
			invisibleAtRunTime=atoi(v[0].c_str())?true:false;
		}
	}
	//Retrieve the invisibleAtDesignTime attribute.
	{
		vector<string> &v=objNode->attributes["invisibleAtDesignTime"];
		if(!v.empty() && !v[0].empty()){
			invisibleAtDesignTime=atoi(v[0].c_str())?true:false;
		}
	}
	
	//Loop the subnodes.
	for(unsigned int i=0;i<objNode->subNodes.size();i++){
		TreeStorageNode *obj=objNode->subNodes[i];
		if(obj->name=="picture" || obj->name=="pictureAnimation"){
			if(!picture.loadFromNode(obj,themePath)){
				return false;
			}
		}else if(obj->name=="editorPicture"){
			if(!editorPicture.loadFromNode(obj,themePath)){
				return false;
			}
		}else if(obj->name=="optionalPicture" && obj->value.size()>=6){
			ThemePicture *objPic=new ThemePicture();
			double f=atof(obj->value[5].c_str());
			if(!objPic->loadFromNode(obj,themePath)){
				delete objPic;
				return false;
			}
			optionalPicture.push_back(pair<double,ThemePicture*>(f,objPic));
		}else if(obj->name=="offset" || obj->name=="offsetAnimation"){
			if(!offset.loadFromNode(obj)) return false;
		}
	}
	
	//Done and nothing went wrong so return true.
	return true;
}

bool ThemePicture::loadFromNode(TreeStorageNode* objNode,string themePath){
	destroy();
	
	//Check if the node has enough values.
	if(!objNode->value.empty()){
		//Load teh picture.
		picture=loadImage(themePath+objNode->value[0]);
		if(picture==NULL) return false;
		
		//Check if it's an animation.
		if(objNode->name=="pictureAnimation"){
			if(!offset.loadFromNode(objNode)) return false;
			return true;
		}else if(objNode->value.size()>=5){
			typeOffsetPoint r={atoi(objNode->value[1].c_str()),
				atoi(objNode->value[2].c_str()),
				atoi(objNode->value[3].c_str()),
				atoi(objNode->value[4].c_str()),0,0};
			offset.offsetData.push_back(r);
			offset.length=0;
			return true;
		}
	}
	
	//Done and nothing went wrong so return true.
	return false;
}

bool ThemeOffsetData::loadFromNode(TreeStorageNode* objNode){
	destroy();
	
	//Check what kind of offset it is.
	if(objNode->name=="pictureAnimation"){
		for(unsigned int i=0;i<objNode->subNodes.size();i++){
			TreeStorageNode* obj=objNode->subNodes[i];
			if(obj->name=="point" && obj->value.size()>=4){
				typeOffsetPoint r={atoi(obj->value[0].c_str()),
					atoi(obj->value[1].c_str()),
					atoi(obj->value[2].c_str()),
					atoi(obj->value[3].c_str()),1,1};
				if(obj->value.size()>=5) r.frameCount=atoi(obj->value[4].c_str());
				if(obj->value.size()>=6) r.frameDisplayTime=atoi(obj->value[5].c_str());
				offsetData.push_back(r);
				length+=r.frameCount*r.frameDisplayTime;
			}
		}
		return true;
	}else if(objNode->name=="offsetAnimation"){
		for(unsigned int i=0;i<objNode->subNodes.size();i++){
			TreeStorageNode* obj=objNode->subNodes[i];
			if(obj->name=="point" && obj->value.size()>=2){
				typeOffsetPoint r={atoi(obj->value[0].c_str()),
					atoi(obj->value[1].c_str()),0,0,1,1};
				if(obj->value.size()>=3) r.frameCount=atoi(obj->value[2].c_str());
				if(obj->value.size()>=4) r.frameDisplayTime=atoi(obj->value[3].c_str());
				offsetData.push_back(r);
				length+=r.frameCount*r.frameDisplayTime;
			}
		}
		return true;
	}else if(objNode->name=="offset" && objNode->value.size()>=2){
		typeOffsetPoint r={atoi(objNode->value[0].c_str()),
			atoi(objNode->value[1].c_str()),0,0,0,0};
		offsetData.push_back(r);
		length=0;
		return true;
	}
	
	//Done and nothing went wrong so return true.
	return false;
}

void ThemeObjectInstance::draw(SDL_Surface *dest,int x,int y,SDL_Rect *clipRect){
	//Get the picture.
	SDL_Surface *src=picture->picture;
	if(src==NULL) return;
	int ex=0,ey=0,xx=0,yy=0,ww=0,hh=0;
	int animationNew=animation&0x7FFFFFFF;
	{
		vector<typeOffsetPoint> &v=picture->offset.offsetData;
		if(picture->offset.length==0 || animationNew<v[0].frameDisplayTime){
			xx=v[0].x;
			yy=v[0].y;
			ww=v[0].w;
			hh=v[0].h;
		}else if(animationNew>=picture->offset.length){
			int i=v.size()-1;
			xx=v[i].x;
			yy=v[i].y;
			ww=v[i].w;
			hh=v[i].h;
		}else{
			int t=animationNew-v[0].frameDisplayTime;
			for(unsigned int i=1;i<v.size();i++){
				int tt=t/v[i].frameDisplayTime;
				if(tt>=0 && tt<v[i].frameCount){
					xx=(int)((float)v[i-1].x+(float)(v[i].x-v[i-1].x)*(float)(tt+1)/(float)v[i].frameCount+0.5f);
					yy=(int)((float)v[i-1].y+(float)(v[i].y-v[i-1].y)*(float)(tt+1)/(float)v[i].frameCount+0.5f);
					ww=(int)((float)v[i-1].w+(float)(v[i].w-v[i-1].w)*(float)(tt+1)/(float)v[i].frameCount+0.5f);
					hh=(int)((float)v[i-1].h+(float)(v[i].h-v[i-1].h)*(float)(tt+1)/(float)v[i].frameCount+0.5f);
					break;
				}else{
					t-=v[i].frameCount*v[i].frameDisplayTime;
				}
			}
		}
	}
	//Get the offset.
	{
		vector<typeOffsetPoint> &v=parent->offset.offsetData;
		if(v.empty()){
			ex=0;
			ey=0;
		}else if(parent->offset.length==0 || animationNew<v[0].frameDisplayTime){
			ex=v[0].x;
			ey=v[0].y;
		}else if(animationNew>=parent->offset.length){
			int i=v.size()-1;
			ex=v[i].x;
			ey=v[i].y;
		}else{
			int t=animationNew-v[0].frameDisplayTime;
			for(unsigned int i=1;i<v.size();i++){
				int tt=t/v[i].frameDisplayTime;
				if(tt>=0 && tt<v[i].frameCount){
					ex=(int)((float)v[i-1].x+(float)(v[i].x-v[i-1].x)*(float)(tt+1)/(float)v[i].frameCount+0.5f);
					ey=(int)((float)v[i-1].y+(float)(v[i].y-v[i-1].y)*(float)(tt+1)/(float)v[i].frameCount+0.5f);
					break;
				}else{
					t-=v[i].frameCount*v[i].frameDisplayTime;
				}
			}
		}
	}
	
	//And finally draw the ThemeObjectInstance.
	if(clipRect){
		int d;
		d=clipRect->x-ex;
		if(d>0){
			ex+=d;
			xx+=d;
			ww-=d;
		}
		d=clipRect->y-ey;
		if(d>0){
			ey+=d;
			yy+=d;
			hh-=d;
		}
		if(ww>clipRect->w) ww=clipRect->w;
		if(hh>clipRect->h) hh=clipRect->h;
	}
	if(ww>0&&hh>0){
		SDL_Rect r1={xx,yy,ww,hh};
		SDL_Rect r2={x+ex,y+ey,0,0};
		SDL_BlitSurface(src,&r1,dest,&r2);
	}
}

void ThemeObjectInstance::updateAnimation(){
	//First get the animation length.
	int m;
	m=parent->animationLength;
	
	//If it's higher than 0 then we have an animation.
	if(m>0 && animation>=0){
		//Increase the animation frame.
		animation++;
		//Check if the animation is beyond the length, if so set it to the looppoint.
		if(animation>=m)
			animation=parent->animationLoopPoint;
	}
}

void ThemeBlockInstance::updateAnimation(){
	//Make sure the currentState isn't null.
	if(currentState!=NULL){
		//Call the updateAnimation method of the currentState.
		currentState->updateAnimation();
		
		//Get the length of the animation.
		int m=currentState->parent->oneTimeAnimationLength;
		
		//If it's higher than 0 then we have an animation.
		//Also check if it's past the lenght, meaning done.
		if(m>0 && currentState->animation>=m){
			//Now we can change the state to the nextState.
			changeState(currentState->parent->nextState);
		}
	}
}

void ThemeCharacterInstance::updateAnimation(){
	//Make sure the currentState isn't null.
	if(currentState!=NULL){
		//Call the updateAnimation method of the currentState.
		currentState->updateAnimation();
		
		//Get the length of the animation.
		int m=currentState->parent->oneTimeAnimationLength;
		
		//If it's higher than 0 then we have an animation.
		//Also check if it's past the lenght, meaning done.
		if(m>0 && currentState->animation>=m){
			//Now we can change the state to the nextState.
			changeState(currentState->parent->nextState);
		}
	}
}

void ThemeBlock::createInstance(ThemeBlockInstance* obj){
	//Make sure the given ThemeBlockInstance is ready.
	obj->blockStates.clear();
	obj->currentState=NULL;
	
	//Loop through the blockstates.
	for(map<string,ThemeBlockState*>::iterator it=blockStates.begin();it!=blockStates.end();++it){
		//Get the themeBlockStateInstance of the given ThemeBlockInstance.
		ThemeBlockStateInstance &obj1=obj->blockStates[it->first];
		//Set the parent of the state instance.
		obj1.parent=it->second;
		//Get the vector with themeObjects.
		vector<ThemeObject*> &v=it->second->themeObjects;
		
		//Loop through them.
		for(unsigned int i=0;i<v.size();i++){
			//Create an instance for every one.
			ThemeObjectInstance p;
			//Set the parent.
			p.parent=v[i];
			
			//Choose the picture.
			if(stateID==STATE_LEVEL_EDITOR){
				if(p.parent->invisibleAtDesignTime)
					continue;
				if(p.parent->editorPicture.picture!=NULL)
					p.picture=&p.parent->editorPicture;
			}else{
				if(p.parent->invisibleAtRunTime)
					continue;
			}
			
			//Get the number of optional Pictures.
			int m=p.parent->optionalPicture.size();
			//If p.picture is null, not an editor picture, and there are optional pictures then give one random.
			if(p.picture==NULL && m>0){
				double f=0.0,f1=1.0/256.0;
				for(int j=0;j<8;j++){
					f+=f1*(double)(rand()&0xff);
					f1*=(1.0/256.0);
				}
				for(int j=0;j<m;j++){
					f-=p.parent->optionalPicture[j].first;
					if(f<0.0){
						p.picture=p.parent->optionalPicture[j].second;
						break;
					}
				}
			}
			
			//If random turned out to give nothing then give the non optional picture.
			if(p.picture==NULL && p.parent->picture.picture!=NULL)
				p.picture=&p.parent->picture;
			//If the picture isn't null then can we give it to the ThemeBlockStateInstance.
			if(p.picture!=NULL)
				obj1.objects.push_back(p);
		}
	}
	
	//Change the state to the default one.
	//FIXME: Is that needed?
	obj->changeState("default");
}

void ThemeCharacter::createInstance(ThemeCharacterInstance* obj){
	//Make sure the given ThemeCharacterInstance is ready.
	obj->characterStates.clear();
	obj->currentState=NULL;
	
	//Loop through the characterstates.
	for(map<string,ThemeCharacterState*>::iterator it=characterStates.begin();it!=characterStates.end();++it){
		//Get the themeCharacterStateInstance of the given ThemeCharacterInstance.
		ThemeCharacterStateInstance &obj1=obj->characterStates[it->first];
		//Set the parent of the state instance.
		obj1.parent=it->second;
		//Get the vector with themeObjects.
		vector<ThemeObject*> &v=it->second->themeObjects;
		
		//Loop through them.
		for(unsigned int i=0;i<v.size();i++){
			//Create an instance for every one.
			ThemeObjectInstance p;
			//Set the parent.
			p.parent=v[i];
			
			//Make sure it isn't invisible at runtime.
			if(p.parent->invisibleAtRunTime)
				continue;

			//Get the number of optional Pictures.
			int m=p.parent->optionalPicture.size();
			//If p.picture is null, not an editor picture, and there are optional pictures then give one random.
			if(p.picture==NULL && m>0){
				double f=0.0,f1=1.0/256.0;
				for(int j=0;j<8;j++){
					f+=f1*(double)(rand()&0xff);
					f1*=(1.0/256.0);
				}
				for(int j=0;j<m;j++){
					f-=p.parent->optionalPicture[j].first;
					if(f<0.0){
						p.picture=p.parent->optionalPicture[j].second;
						break;
					}
				}
			}
			
			//If random turned out to give nothing then give the non optional picture.
			if(p.picture==NULL && p.parent->picture.picture!=NULL)
				p.picture=&p.parent->picture;
			//If the picture isn't null then can we give it to the ThemeCharacterStateInstance.
			if(p.picture!=NULL)
				obj1.objects.push_back(p);
		}
	}
	
	//Set it to the standing right state.
	obj->changeState("standright");
}

void ThemePicture::draw(SDL_Surface *dest,int x,int y,int animation,SDL_Rect *clipRect){
	//Get the Picture.
	if(picture==NULL) return;
	int ex=0,ey=0,xx,yy,ww,hh;
	{
		vector<typeOffsetPoint> &v=offset.offsetData;
		if(offset.length==0 || animation<v[0].frameDisplayTime){
			xx=v[0].x;
			yy=v[0].y;
			ww=v[0].w;
			hh=v[0].h;
		}else if(animation>=offset.length){
			int i=v.size()-1;
			xx=v[i].x;
			yy=v[i].y;
			ww=v[i].w;
			hh=v[i].h;
		}else{
			int t=animation-v[0].frameDisplayTime;
			for(unsigned int i=1;i<v.size();i++){
				int tt=t/v[i].frameDisplayTime;
				if(tt>=0 && tt<v[i].frameCount){
					xx=(int)((float)v[i-1].x+(float)(v[i].x-v[i-1].x)*(float)(tt+1)/(float)v[i].frameCount+0.5f);
					yy=(int)((float)v[i-1].y+(float)(v[i].y-v[i-1].y)*(float)(tt+1)/(float)v[i].frameCount+0.5f);
					ww=(int)((float)v[i-1].w+(float)(v[i].w-v[i-1].w)*(float)(tt+1)/(float)v[i].frameCount+0.5f);
					hh=(int)((float)v[i-1].h+(float)(v[i].h-v[i-1].h)*(float)(tt+1)/(float)v[i].frameCount+0.5f);
					break;
				}else{
					t-=v[i].frameCount*v[i].frameDisplayTime;
				}
			}
		}
	}
	
	//Draw the Picture.
	if(clipRect){
		int d;
		d=clipRect->x-ex;
		if(d>0){
			ex+=d;
			xx+=d;
			ww-=d;
		}
		d=clipRect->y-ey;
		if(d>0){
			ey+=d;
			yy+=d;
			hh-=d;
		}
		if(ww>clipRect->w) ww=clipRect->w;
		if(hh>clipRect->h) hh=clipRect->h;
	}
	if(ww>0&&hh>0){
		SDL_Rect r1={xx,yy,ww,hh};
		SDL_Rect r2={x+ex,y+ey,0,0};
		SDL_BlitSurface(picture,&r1,dest,&r2);
	}
}

//This method will scale the background picture (if needed and configured) to the current SCREEN_WIDTH and SCREEN_HEIGHT.
void ThemeBackgroundPicture::scaleToScreen(){
	//Only scale if needed.
	if(scale){
		//Free the surface of the scaled picture, if scaled.
		if(picture!=cachedPicture)
			SDL_FreeSurface(picture);
		//Set src and destSize back to the initial cached value.
		srcSize=cachedSrcSize;
		destSize=cachedDestSize;
		
		//Scale the image.
		//Calculate the x and y factors.
		double xFactor=double(SCREEN_WIDTH)/double(100);
		double yFactor=double(SCREEN_HEIGHT)/double(100);
		
		//The default scaling method is chosen (destSize in precentages).
		destSize.x*=xFactor;
		destSize.w*=xFactor;
		
		destSize.y*=yFactor;
		destSize.h*=yFactor;
		
		//Now update the image.
		xFactor=(double(destSize.w)/double(srcSize.w));
		yFactor=(double(destSize.h)/double(srcSize.h));
		if(xFactor!=1 || yFactor!=1){
			picture=zoomSurface(cachedPicture,xFactor,yFactor,0);
			//Also update the source size.
			srcSize.x*=xFactor;
			srcSize.y*=yFactor;
			srcSize.w*=xFactor;
			srcSize.h*=yFactor;
		}else{
			//We don't need to scale the image
			picture=cachedPicture;
		}
	}
}

void ThemeBackgroundPicture::draw(SDL_Surface *dest){
	//Check if the picture is visible.
	if(!(picture&&srcSize.w>0&&srcSize.h>0&&destSize.w>0&&destSize.h>0))
		return;
	
	//Calculate the draw area.
	int sx=(int)((float)destSize.x+currentX-cameraX*(float)camera.x+0.5f);
	int sy=(int)((float)destSize.y+currentY-cameraY*(float)camera.y+0.5f);
	int ex,ey;
	
	//Include repeating.
	if(repeatX){
		sx%=destSize.w;
		if(sx>0) sx-=destSize.w;
		ex=SCREEN_WIDTH;
	}else{
		if(sx<=-(int)destSize.w || sx>=SCREEN_WIDTH) return;
		ex=sx+1;
	}
	if(repeatY){
		sy%=destSize.h;
		if(sy>0) sy-=destSize.h;
		ey=SCREEN_HEIGHT;
	}else{
		if(sy<=-(int)destSize.h || sy>=SCREEN_HEIGHT) return;
		ey=sy+1;
	}
	
	//And finally draw the ThemeBackgroundPicture.
	for(int x=sx;x<ex;x+=destSize.w){
		for(int y=sy;y<ey;y+=destSize.h){
			SDL_Rect r={x,y,0,0};
			SDL_BlitSurface(picture,&srcSize,dest,&r);
		}
	}
}

bool ThemeBackgroundPicture::loadFromNode(TreeStorageNode* objNode,string themePath){
	//Load the picture.
	picture=loadImage(themePath+objNode->value[0]);
	//Store pointer to the cached picture.
	cachedPicture=picture;
	if(picture==NULL) return false;
	
	//Retrieve the source size.
	{
		vector<string> &v=objNode->attributes["srcSize"];
		if(v.size()>=4){
			srcSize.x=atoi(v[0].c_str());
			srcSize.y=atoi(v[1].c_str());
			srcSize.w=atoi(v[2].c_str());
			srcSize.h=atoi(v[3].c_str());
		}else{
			srcSize.x=0;
			srcSize.y=0;
			srcSize.w=picture->w;
			srcSize.h=picture->h;
		}
		
		//Cache the sourcesize.
		cachedSrcSize=srcSize;
	}
	
	//Retrieve the destination size.
	{
		vector<string> &v=objNode->attributes["destSize"];
		if(v.size()>=4){
			destSize.x=atoi(v[0].c_str());
			destSize.y=atoi(v[1].c_str());
			destSize.w=atoi(v[2].c_str());
			destSize.h=atoi(v[3].c_str());
		}else{
			destSize.x=0;
			destSize.y=0;
			destSize.w=100;
			destSize.h=100;
		}
		
		//Cache the destsize.
		cachedDestSize=destSize;
	}
	
	//Retrieve if we should scale to screen.
	{
		//Get scaleToScreen.
		vector<string> &v=objNode->attributes["scaleToScreen"];
		//Boolean if the image should be scaled, default is true.
		scale=true;
		if(v.size()>=1){
			scale=atoi(v[0].c_str());
		}
		
		//Now scaleToScreen.
		//NOTE: We don't check if scaleToScreen is true or false since that is done in scaleToScreen();
		scaleToScreen();
	}
	
	//Retrieve if it should be repeated.
	{
		vector<string> &v=objNode->attributes["repeat"];
		if(v.size()>=2){
			repeatX=atoi(v[0].c_str())?true:false;
			repeatY=atoi(v[1].c_str())?true:false;
		}else{
			repeatX=true;
			repeatY=true;
		}
	}
	
	//Retrieve the speed.
	{
		vector<string> &v=objNode->attributes["speed"];
		if(v.size()>=2){
			speedX=atof(v[0].c_str());
			speedY=atof(v[1].c_str());
		}else{
			speedX=0.0f;
			speedY=0.0f;
		}
	}
	
	//Retrieve the camera speed.
	{
		vector<string> &v=objNode->attributes["cameraSpeed"];
		if(v.size()>=2){
			cameraX=atof(v[0].c_str());
			cameraY=atof(v[1].c_str());
		}else{
			cameraX=0.0f;
			cameraY=0.0f;
		}
	}
	
	//Done and nothing went wrong so return true.
	return true;
}

//Constructor.
ThemeStack::ThemeStack(){
}

//Destructor.
ThemeStack::~ThemeStack(){
	//Loop through the themes and delete them.
	for(unsigned int i=0;i<objThemes.size();i++)
		delete objThemes[i];
}

//Method that will destroy the ThemeStack.
void ThemeStack::destroy(){
	//Loop through the themes and delete them.
	for(unsigned int i=0;i<objThemes.size();i++)
		delete objThemes[i];
	//Clear the vector to prevent dangling pointers.
	objThemes.clear();
}

//Method that will append a theme to the stack.
//obj: The ThemeManager to add.
void ThemeStack::appendTheme(ThemeManager* obj){
	objThemes.push_back(obj);
	//debug
#if defined(DEBUG) || defined(_DEBUG)
	cout<<"ThemeStack::appendTheme(): theme count="<<objThemes.size()<<endl;
#endif
}
//Method that will remove the last theme added to the stack.
void ThemeStack::removeTheme(){
	//Make sure that the stack isn't empty.
	if(!objThemes.empty()){
		delete objThemes.back();
		objThemes.pop_back();
	}
}

//Method that will append a theme that will be loaded from file.
//fileName: The file to load the theme from.
//Returns: Pointer to the newly added theme, NULL if failed.
ThemeManager* ThemeStack::appendThemeFromFile(const string& fileName){
	//Create a new themeManager.
	ThemeManager* obj=new ThemeManager();
	
	//Let it load from the given file.
	if(!obj->loadFile(fileName)){
		//Failed thus delete the theme and return null.
		cerr<<"ERROR: Failed loading theme "<<fileName<<endl;
		delete obj;
		return NULL;
	}else{
		//Succeeded, add it to the stack and return it.
		objThemes.push_back(obj);
		return obj;
	}
}

//Method that is used to let the themes scale.
void ThemeStack::scaleToScreen(){
	//Loop through the themes and call their scaleToScreen method.
	for(unsigned int i=0;i<objThemes.size();i++)
		objThemes[i]->scaleToScreen();
}

//Get a pointer to the ThemeBlock of a given block type.
//index: The type of block.
//Returns: Pointer to the ThemeBlock.
ThemeBlock* ThemeStack::getBlock(int index,bool menu){
	//Loop through the themes from top to bottom.
	for(int i=objThemes.size()-1;i>=0;i--){
		//Get the block from the theme.
		ThemeBlock* obj=objThemes[i]->getBlock(index,menu);
		//Check if it isn't null.
		if(obj)
			return obj;
	}
	
	//Nothing found.
	return NULL;
}
//Get a pointer to the ThemeCharacter of the shadow or the player.
//isShadow: Boolean if it's the shadow
//Returns: Pointer to the ThemeCharacter.
ThemeCharacter* ThemeStack::getCharacter(bool isShadow){
	//Loop through the themes from top to bottom.
	for(int i=objThemes.size()-1;i>=0;i--){
		//Get the ThemeCharacter from the theme.
		ThemeCharacter* obj=objThemes[i]->getCharacter(isShadow);
		//Check if it isn't null.
		if(obj)
			return obj;
	}
	
	//Nothing found.
	return NULL;
}
//Get a pointer to the ThemeBackground of the theme.
//Returns: Pointer to the ThemeBackground.
ThemeBackground* ThemeStack::getBackground(bool menu){
	//Loop through the themes from top to bottom.
	for(int i=objThemes.size()-1;i>=0;i--){
		//Get the ThemeBackground from the theme.
		ThemeBackground* obj=objThemes[i]->getBackground(menu);
		//Check if it isn't null.
		if(obj)
			return obj;
	}
	
	//Nothing found.
	return NULL;
}
