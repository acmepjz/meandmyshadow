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
#include "ImageManager.h"

#include <iostream>
using namespace std;

//The ThemeStack that is be used by the GameState.
ThemeStack objThemes;

ThemeObjectInstance::ThemeObjectInstance()
	: picture(NULL), parent(NULL), animation(0)
{
}

ThemeBlockStateInstance::ThemeBlockStateInstance()
	: parent(NULL), animation(0)
{
}

void ThemeBlockStateInstance::draw(SDL_Renderer& renderer, int x, int y, int w, int h, const SDL_Rect *clipRect){
	for (unsigned int i = 0; i<objects.size(); i++){
		objects[i].draw(renderer, x, y, w, h, clipRect);
	}
}

void ThemeBlockStateInstance::updateAnimation(){
	for (unsigned int i = 0; i<objects.size(); i++){
		objects[i].updateAnimation();
	}
	animation++;
}

ThemeBlockInstance::ThemeBlockInstance()
	: currentState(-1)
{
}

bool ThemeBlockInstance::draw(SDL_Renderer& renderer, int x, int y, int w, int h, const SDL_Rect *clipRect){
	if (currentState >= 0 && currentState < (int)states.size()) {
		states[currentState].draw(renderer, x, y, w, h, clipRect);
		return true;
	}
	return false;
}

bool ThemeBlockInstance::drawState(const string& s, SDL_Renderer& renderer, int x, int y, int w, int h, SDL_Rect *clipRect){
	auto it = blockStates.find(s);
	if (it != blockStates.end() && it->second >= 0 && it->second < (int)states.size()) {
		states[it->second].draw(renderer, x, y, w, h, clipRect);
		return true;
	}
	return false;
}

bool ThemeBlockInstance::changeState(const string& s, bool reset, bool onlyIfStateChanged) {
	//Check if we don't need to change state at all.
	if (onlyIfStateChanged && s == currentStateName) {
		return true;
	}

	bool newState = false;

	//First check if there's a transition.
	{
		auto it = transitions.find(pair<string, string>(currentStateName, s));
		if (it != transitions.end() && it->second >= 0 && it->second < (int)states.size()) {
			currentState = it->second;
			//NOTE: We set the currentState name to target state name.
			//Worst case senario is that the animation is skipped when saving/loading at a checkpoint.
			currentStateName = s;
			newState = true;
		}
	}

	//If there isn't a transition go directly to the state.
	if (!newState){
		//Get the new state.
		auto it = blockStates.find(s);
		//Check if it exists.
		if (it != blockStates.end() && it->second >= 0 && it->second < (int)states.size()) {
			currentState = it->second;
			currentStateName = it->first;
			newState = true;
		}
	}

	//Check if a state has been found.
	if (newState){
		//If reset then reset the animation.
		if (reset) {
			ThemeBlockStateInstance &current = states[currentState];
			current.animation = 0;
			for (auto& obj : current.objects) {
				obj.animation = 0;
			}
		}
		return true;
	}

	//It doesn't so return false.
	return false;
}

ThemeOffsetData::ThemeOffsetData()
	: length(0)
{
}

void ThemeOffsetData::destroy(){
	//Set length to zero.
	length = 0;
	//And clear the offsetData vector.
	offsetData.clear();
}

//Constructor.
ThemePositioningData::ThemePositioningData()
	: horizontalAlign(REPEAT), verticalAlign(REPEAT)
{
}

//Method used to destroy the positioningData.
void ThemePositioningData::destroy(){
	horizontalAlign = REPEAT;
	verticalAlign = REPEAT;
}

ThemePicture::ThemePicture()
	:texture(NULL)//, x(0), y(0)
{
}

void ThemePicture::destroy(){
	//Freeing handled by ImageManager.
	//TODO: Unload unused images
	texture = NULL;
	//Destroy the offset data.
	offset.destroy();
}

ThemeObject::ThemeObject()
	:animationLength(0), animationLoopPoint(0), invisibleAtRunTime(false), invisibleAtDesignTime(false)
{
}

ThemeObject::~ThemeObject(){
	//Loop through the optionalPicture and delete them.
	for (unsigned int i = 0; i<optionalPicture.size(); i++){
		delete optionalPicture[i].second;
	}
}

void ThemeObject::destroy(){
	//Loop through the optionalPicture and delete them.
	for (unsigned int i = 0; i<optionalPicture.size(); i++){
		delete optionalPicture[i].second;
	}
	optionalPicture.clear();
	animationLength = 0;
	animationLoopPoint = 0;
	invisibleAtRunTime = false;
	invisibleAtDesignTime = false;
	picture.destroy();
	editorPicture.destroy();
	offset.destroy();
	positioning.destroy();
}

ThemeBlockState::ThemeBlockState()
	:oneTimeAnimationLength(0)
{
}

ThemeBlockState::~ThemeBlockState(){
	//Loop through the ThemeObjects and delete them.
	for (unsigned int i = 0; i<themeObjects.size(); i++){
		delete themeObjects[i];
	}
}

void ThemeBlockState::destroy(){
	//Loop through the ThemeObjects and delete them.
	for (unsigned int i = 0; i<themeObjects.size(); i++){
		delete themeObjects[i];
	}
	//Clear the themeObjects vector.
	themeObjects.clear();
	//Set the length to 0.
	oneTimeAnimationLength = 0;
	//Clear the nextState string.
	nextState.clear();
}

ThemeBlock::ThemeBlock()
{
}

ThemeBlock::~ThemeBlock(){
	//Loop through the ThemeBlockStates and delete them,
	for (map<string, ThemeBlockState*>::iterator i = blockStates.begin(); i != blockStates.end(); ++i){
		delete i->second;
	}
	//Loop through the ThemeBlockStates and delete them,
	for (map<pair<string, string>, ThemeBlockState*>::iterator i = transitions.begin(); i != transitions.end(); ++i){
		delete i->second;
	}
}

void ThemeBlock::destroy(){
	//Loop through the ThemeBlockStates and delete them,
	for (map<string, ThemeBlockState*>::iterator i = blockStates.begin(); i != blockStates.end(); ++i){
		delete i->second;
	}
	//Loop through the ThemeBlockStates transitions and delete them,
	for (map<pair<string, string>, ThemeBlockState*>::iterator i = transitions.begin(); i != transitions.end(); ++i){
		delete i->second;
	}
	//Clear the blockStates map.
	blockStates.clear();
	transitions.clear();
	editorPicture.destroy();
}

ThemeBackgroundPicture::ThemeBackgroundPicture(){
	//Set some default values.
	texture = NULL;
	memset(&srcSize, 0, sizeof(srcSize));
	memset(&destSize, 0, sizeof(destSize));
	memset(&cachedSrcSize, 0, sizeof(cachedSrcSize));
	memset(&cachedDestSize, 0, sizeof(cachedDestSize));
	scale = true;
	repeatX = true;
	repeatY = true;
	speedX = 0.0f;
	speedY = 0.0f;
	cameraX = 0.0f;
	cameraY = 0.0f;
	currentX = 0.0f;
	currentY = 0.0f;
	savedX = 0.0f;
	savedY = 0.0f;
}

void ThemeBackgroundPicture::updateAnimation(){
	//Move the picture along the x-axis.
	currentX += speedX;
	if (repeatX && destSize.w>0){
		float f = (float)destSize.w;
		if (currentX>f || currentX<-f) currentX -= f*floor(currentX / f);
	}

	//Move the picture along the y-axis.
	currentY += speedY;
	if (repeatY && destSize.h>0){
		float f = (float)destSize.h;
		if (currentY>f || currentY<-f) currentY -= f*floor(currentY / f);
	}
}

void ThemeBackgroundPicture::resetAnimation(bool save){
	currentX = 0.0f;
	currentY = 0.0f;
	if (save){
		savedX = 0.0f;
		savedY = 0.0f;
	}
}

void ThemeBackgroundPicture::saveAnimation(){
	savedX = currentX;
	savedY = currentY;
}

void ThemeBackgroundPicture::loadAnimation(){
	currentX = savedX;
	currentY = savedY;
}

void ThemeBackground::updateAnimation(){
	for (unsigned int i = 0; i<picture.size(); i++){
		picture[i].updateAnimation();
	}
}

void ThemeBackground::resetAnimation(bool save){
	for (unsigned int i = 0; i<picture.size(); i++){
		picture[i].resetAnimation(save);
	}
}

void ThemeBackground::saveAnimation(){
	for (unsigned int i = 0; i<picture.size(); i++){
		picture[i].saveAnimation();
	}
}

void ThemeBackground::loadAnimation(){
	for (unsigned int i = 0; i<picture.size(); i++){
		picture[i].loadAnimation();
	}
}

void ThemeBackground::scaleToScreen(){
	for (unsigned int i = 0; i<picture.size(); i++){
		picture[i].scaleToScreen();
	}
}

void ThemeBackground::draw(SDL_Renderer& renderer){
	for (unsigned int i = 0; i<picture.size(); i++){
		picture[i].draw(renderer);
	}
}

bool ThemeBackground::addPictureFromNode(TreeStorageNode* objNode, string themePath, ImageManager& imageManager, SDL_Renderer& renderer){
	picture.push_back(ThemeBackgroundPicture());
	return picture.back().loadFromNode(objNode, themePath, imageManager, renderer);
}

ThemeManager::ThemeManager(){
	//Make sure the pointers are set to NULL.
	objBackground = NULL;
	//Reserve enough memory for the ThemeBlocks.
	memset(objBlocks, 0, sizeof(objBlocks));
	shadow = NULL;
	player = NULL;
	menuBackground = NULL;
	menuBlock = NULL;
	menuShadowBlock = NULL;
	hasThemeTextColor = hasThemeTextColorDialog = false;
}

ThemeManager::~ThemeManager(){
	//Just call destroy().
	destroy();
}

void ThemeManager::destroy(){
	//Delete the ThemeBlock of the shadow.
	if (shadow) {
		delete shadow;
		shadow = NULL;
	}
	//Delete the ThemeBlock of the player.
	if (player) {
		delete player;
		player = NULL;
	}
	//Loop through the ThemeBlocks and delete them.
	for (int i = 0; i<TYPE_MAX; i++){
		if (objBlocks[i]) {
			delete objBlocks[i];
			objBlocks[i] = NULL;
		}
	}
	//Delete all scenery blocks
	for (auto it = objScenery.begin(); it != objScenery.end(); ++it) {
		delete it->second;
	}
	objScenery.clear();
	//Delete the ThemeBackgrounds, etc.
	if (objBackground) {
		delete objBackground;
		objBackground = NULL;
	}
	if (menuBackground) {
		delete menuBackground;
		menuBackground = NULL;
	}
	if (menuBlock) {
		delete menuBlock;
		menuBlock = NULL;
	}
	if (menuShadowBlock) {
		delete menuShadowBlock;
		menuShadowBlock = NULL;
	}

	//And clear the themeName, etc.
	themeName.clear();
	themePath.clear();
}

bool ThemeManager::loadFile(const string& fileName, ImageManager &imageManager, SDL_Renderer &renderer){
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
	hasThemeTextColor = hasThemeTextColorDialog = false;
	themeTextColor.r=themeTextColor.g=themeTextColor.b=0;
	themeTextColorDialog.r=themeTextColorDialog.g=themeTextColorDialog.b=0;
	
	//Read themeable colors if any
	vector<string> &ct=objNode.attributes["textColor"];
	if(!ct.empty()){
		hasThemeTextColor = true;
		themeTextColor.r=atoi(ct[0].c_str());
		themeTextColor.g=atoi(ct[1].c_str());
		themeTextColor.b=atoi(ct[2].c_str());
	}
	
	vector<string> &ct2=objNode.attributes["textColorDialog"];
	if(!ct2.empty()){
		hasThemeTextColorDialog = true;
		themeTextColorDialog.r=atoi(ct2[0].c_str());
		themeTextColorDialog.g=atoi(ct2[1].c_str());
		themeTextColorDialog.b=atoi(ct2[2].c_str());
	}
	
	//Loop the subnodes of the theme.
	for(unsigned int i=0;i<objNode.subNodes.size();i++){
		TreeStorageNode *obj=objNode.subNodes[i];
		
		//Check if it's a block or a background.
		if (obj->name == "block" && !obj->value.empty()){
			map<string, int>::iterator it = Game::blockNameMap.find(obj->value[0]);
			if (it != Game::blockNameMap.end()){
				int idx = it->second;
				if (!objBlocks[idx]) objBlocks[idx] = new ThemeBlock;
				if (!objBlocks[idx]->loadFromNode(obj, themePath, imageManager, renderer)){
					cerr << "ERROR: Unable to load " << Game::blockName[idx] << " for theme " << fileName << endl;
					delete objBlocks[idx];
					objBlocks[idx] = NULL;
					return false;
				}
			}
		} else if (obj->name == "scenery" && !obj->value.empty()){
			std::string& name = obj->value[0];
			if (!objScenery[name]) objScenery[name] = new ThemeBlock;
			if (!objScenery[name]->loadFromNode(obj, themePath, imageManager, renderer)){
				cerr << "ERROR: Unable to load scenery '" << name << "' for theme " << fileName << endl;
				delete objScenery[name];
				objScenery[name] = NULL;
				return false;
			}
		}else if(obj->name=="background" && !obj->value.empty()){
			if(!objBackground) objBackground=new ThemeBackground();
            if(!objBackground->addPictureFromNode(obj,themePath, imageManager, renderer)){
				cerr<<"ERROR: Unable to load background for theme "<<fileName<<endl;
				delete objBackground;
				objBackground=NULL;
				return false;
			}
		}else if(obj->name=="character" && !obj->value.empty()){
			if(obj->value[0]=="Shadow"){
				if(!shadow) shadow=new ThemeBlock();
                if(!shadow->loadFromNode(obj,themePath, imageManager, renderer)){
					cerr<<"ERROR: Unable to load shadow for theme "<<fileName<<endl;
					delete shadow;
					shadow=NULL;
					return false;
				}
			}else if(obj->value[0]=="Player"){
				if(!player) player=new ThemeBlock();
                if(!player->loadFromNode(obj,themePath, imageManager, renderer)){
					cerr<<"ERROR: Unable to load player for theme "<<fileName<<endl;
					delete player;
					player=NULL;
					return false;
				}
			}
		}else if(obj->name=="menuBackground" && !obj->value.empty()){
			if(!menuBackground) menuBackground=new ThemeBackground();
            if(!menuBackground->addPictureFromNode(obj,themePath, imageManager, renderer)){
				cerr<<"ERROR: Unable to load background for theme "<<fileName<<endl;
				delete menuBackground;
				menuBackground=NULL;
				return false;
			}
		}else if(obj->name=="menu" && obj->value[0]=="Block"){
			if(!menuBlock) menuBlock=new ThemeBlock;
            if(!menuBlock->loadFromNode(obj,themePath, imageManager, renderer)){
				cerr<<"ERROR: Unable to load menu block for theme "<<fileName<<endl;
				delete menuBlock;
				menuBlock=NULL;
				return false;
			}
		} else if (obj->name == "menu" && obj->value[0] == "ShadowBlock"){
			if (!menuShadowBlock) menuShadowBlock = new ThemeBlock;
			if (!menuShadowBlock->loadFromNode(obj, themePath, imageManager, renderer)){
				cerr << "ERROR: Unable to load menu shadow block for theme " << fileName << endl;
				delete menuShadowBlock;
				menuShadowBlock = NULL;
				return false;
			}
		}
	}
	
	//Done and nothing went wrong so return true.
	return true;
}

void ThemeManager::scaleToScreen(){
	//We only need to scale the background.
	if (objBackground)
		objBackground->scaleToScreen();
}

ThemeBlock* ThemeManager::getBlock(int index, bool menu){
	if (!menu)
		return objBlocks[index];
	else
		if (index == TYPE_BLOCK)
			if (menuBlock)
				return menuBlock;
			else
				return objBlocks[TYPE_BLOCK];
		else if (index == TYPE_SHADOW_BLOCK)
			if (menuShadowBlock)
				return menuShadowBlock;
			else if (menuBlock)
				return menuBlock;
			else
				return objBlocks[TYPE_SHADOW_BLOCK];
		else
			return objBlocks[index];
}

ThemeBlock* ThemeManager::getScenery(const std::string& name){
	auto it = objScenery.find(name);
	if (it == objScenery.end())
		return NULL;
	else
		return it->second;
}

void ThemeManager::getSceneryBlockNames(std::set<std::string> &s) {
	for (auto it = objScenery.begin(); it != objScenery.end(); ++it) {
		s.insert(it->first);
	}
}

ThemeBlock* ThemeManager::getCharacter(bool isShadow){
	if (isShadow)
		return shadow;
	return player;
}

ThemeBackground* ThemeManager::getBackground(bool menu){
	if (menu&&menuBackground)
		return menuBackground;
	else
		return objBackground;
}

bool ThemeManager::getTextColor(bool isDialog, SDL_Color& color) {
	if (isDialog) {
		if (hasThemeTextColorDialog) color = themeTextColorDialog;
		return hasThemeTextColorDialog;
	} else {
		if (hasThemeTextColor) color = themeTextColor;
		return hasThemeTextColor;
	}
}

bool ThemeBlock::loadFromNode(TreeStorageNode* objNode, string themePath, ImageManager &imageManager, SDL_Renderer &renderer){
	destroy();
	
	//Loop the subNodes.
	for(unsigned int i=0;i<objNode->subNodes.size();i++){
		TreeStorageNode *obj=objNode->subNodes[i];
		
		//Check if the subnode is an editorPicture or a blockState.
		if(obj->name=="editorPicture"){
            if(!editorPicture.loadFromNode(obj,themePath, imageManager, renderer)) return false;
			//NOTE: blockState and characterState are for backwards compatability, use state instead.
		}else if((obj->name=="blockState" || obj->name=="characterState" || obj->name=="state") && !obj->value.empty()){
			string& s=obj->value[0];
			map<string,ThemeBlockState*>::iterator it=blockStates.find(s);
			if(it==blockStates.end()) blockStates[s]=new ThemeBlockState;
            if(!blockStates[s]->loadFromNode(obj,themePath, imageManager, renderer)) return false;
		}else if(obj->name=="transitionState" && obj->value.size()==2){
			pair<string,string> s=pair<string,string>(obj->value[0],obj->value[1]);
			map<pair<string,string>,ThemeBlockState*>::iterator it=transitions.find(s);
			if(it==transitions.end()) transitions[s]=new ThemeBlockState;
            if(!transitions[s]->loadFromNode(obj,themePath, imageManager, renderer)) return false;
		}
	}
	
	//Done and nothing went wrong so return true.
	return true;
}

bool ThemeBlockState::loadFromNode(TreeStorageNode* objNode, string themePath, ImageManager& imageManager, SDL_Renderer& renderer){
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
            if(!obj1->loadFromNode(obj,themePath, imageManager, renderer)){
				delete obj1;
				return false;
			}
			themeObjects.push_back(obj1);
		}
	}
	
	//Done and nothing went wrong so return true.
	return true;
}

bool ThemeObject::loadFromNode(TreeStorageNode* objNode,string themePath, ImageManager& imageManager, SDL_Renderer& renderer){
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
            if(!picture.loadFromNode(obj,themePath, imageManager, renderer)){
				return false;
			}
		}else if(obj->name=="editorPicture"){
            if(!editorPicture.loadFromNode(obj,themePath, imageManager, renderer)){
				return false;
			}
		}else if(obj->name=="optionalPicture" && obj->value.size()>=6){
			ThemePicture *objPic=new ThemePicture();
			double f=atof(obj->value[5].c_str());
            if(!objPic->loadFromNode(obj,themePath, imageManager, renderer)){
				delete objPic;
				return false;
			}
			optionalPicture.push_back(pair<double,ThemePicture*>(f,objPic));
		}else if(obj->name=="offset" || obj->name=="offsetAnimation"){
			if(!offset.loadFromNode(obj)) return false;
		}else if(obj->name=="positioning"){
			if(!positioning.loadFromNode(obj)) return false;
		}
	}
	
	//Done and nothing went wrong so return true.
	return true;
}

bool ThemePicture::loadFromNode(TreeStorageNode* objNode, string themePath, ImageManager &imageManager, SDL_Renderer &renderer){
	destroy();
	
	//Check if the node has enough values.
	if(!objNode->value.empty()){
        //Load the texture.
        texture=imageManager.loadTexture(themePath+objNode->value[0], renderer);
        if(!texture) {
            return false;
        }
		
		//Check if it's an animation.
		if(objNode->name=="pictureAnimation"){
			if(!offset.loadFromNode(objNode)) return false;
			return true;
		}else if(objNode->value.size()>=5){
			ThemeOffsetPoint r={atoi(objNode->value[1].c_str()),
				atoi(objNode->value[2].c_str()),
				atoi(objNode->value[3].c_str()),
				atoi(objNode->value[4].c_str()),0,0};
			offset.offsetData.push_back(r);
			offset.length=0;
			return true;
		}
	}

	cerr << "ERROR: The structure of theme picture node '" << objNode->name << "' is incorrect" << endl;

	return false;
}

bool ThemeOffsetData::loadFromNode(TreeStorageNode* objNode){
	destroy();
	
	//Check what kind of offset it is.
	if(objNode->name=="pictureAnimation"){
		for(unsigned int i=0;i<objNode->subNodes.size();i++){
			TreeStorageNode* obj=objNode->subNodes[i];
			if(obj->name=="point" && obj->value.size()>=4){
				ThemeOffsetPoint r={atoi(obj->value[0].c_str()),
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
				ThemeOffsetPoint r={atoi(obj->value[0].c_str()),
					atoi(obj->value[1].c_str()),0,0,1,1};
				if(obj->value.size()>=3) r.frameCount=atoi(obj->value[2].c_str());
				if(obj->value.size()>=4) r.frameDisplayTime=atoi(obj->value[3].c_str());
				if(obj->value.size()>=5) r.w=atoi(obj->value[4].c_str());
				if(obj->value.size()>=6) r.h=atoi(obj->value[5].c_str());
				offsetData.push_back(r);
				length+=r.frameCount*r.frameDisplayTime;
			}
		}
		return true;
	}else if(objNode->name=="offset" && objNode->value.size()>=2){
		ThemeOffsetPoint r={atoi(objNode->value[0].c_str()),
			atoi(objNode->value[1].c_str()),0,0,0,0};
		if(objNode->value.size()>2)
			r.w=atoi(objNode->value[2].c_str());
		if(objNode->value.size()>3)
			r.h=atoi(objNode->value[3].c_str());
		offsetData.push_back(r);
		length=0;
		return true;
	}
	
	cerr << "ERROR: The structure of theme offset data node '" << objNode->name << "' is incorrect" << endl;

	return false;
}

bool ThemePositioningData::loadFromNode(TreeStorageNode* objNode){
	destroy();

	//Check if enough values are set.
	if(objNode->value.size()>=2){
		//Check horizontal alignment.
		if(objNode->value[0]=="left"){
			horizontalAlign=LEFT;
		}else if(objNode->value[0]=="centre" || objNode->value[0]=="center"){
			horizontalAlign=CENTRE;
		}else if(objNode->value[0]=="right"){
			horizontalAlign=RIGHT;
		}else if(objNode->value[0]=="repeat"){
			horizontalAlign=REPEAT;
		} else if (objNode->value[0] == "stretch") {
			horizontalAlign = STRETCH;
		} else {
			cerr << "ERROR: Unknown horizontal align mode: " << objNode->value[0] << endl;
			return false;
		}
		//Check vertical alignment.
		if(objNode->value[1]=="top"){
			verticalAlign=TOP;
		}else if(objNode->value[1]=="middle"){
			verticalAlign=MIDDLE;
		}else if(objNode->value[1]=="bottom"){
			verticalAlign=BOTTOM;
		}else if(objNode->value[1]=="repeat"){
			verticalAlign=REPEAT;
		} else if (objNode->value[1] == "stretch") {
			verticalAlign = STRETCH;
		} else {
			cerr << "ERROR: Unknown vertical align mode: " << objNode->value[1] << endl;
			return false;
		}
		//Done and nothing went wrong so return true.
		return true;
	}

	cerr << "ERROR: The structure of theme positioning data node '" << objNode->name << "' is incorrect" << endl;

	return false;
}

void ThemeObjectInstance::draw(SDL_Renderer& renderer,int x,int y,int w,int h,const SDL_Rect *clipRect){
	//Get the picture.
    //SDL_Surface *src=picture->picture;
    SDL_Texture* src = picture->texture.get();
	if(src==NULL) return;

	//The offset to the left and top of the destination rectangle.
	int ex = 0, ey = 0;

	//The offset to the right and bottom of the destination rectangle. Only used when the position mode is REPEAT or STRETCH.
	int ew = 0, eh = 0;

	//The x,y,width,height of the source rectangle.
	int xx=0,yy=0,ww=0,hh=0;

	int animationNew=animation&0x7FFFFFFF;

	//Get the source rectangle.
	{
        const vector<ThemeOffsetPoint> &v=picture->offset.offsetData;
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
		vector<ThemeOffsetPoint> &v=parent->offset.offsetData;
		if(v.empty()){
			ex=0;
			ey=0;
		}else if(parent->offset.length==0 || animationNew<v[0].frameDisplayTime){
			ex=v[0].x;
			ey=v[0].y;
			ew=v[0].w;
			eh=v[0].h;
		}else if(animationNew>=parent->offset.length){
			int i=v.size()-1;
			ex=v[i].x;
			ey=v[i].y;
			ew=v[i].w;
			eh=v[i].h;
		}else{
			int t=animationNew-v[0].frameDisplayTime;
			for(unsigned int i=1;i<v.size();i++){
				int tt=t/v[i].frameDisplayTime;
				if(tt>=0 && tt<v[i].frameCount){
					ex=(int)((float)v[i-1].x+(float)(v[i].x-v[i-1].x)*(float)(tt+1)/(float)v[i].frameCount+0.5f);
					ey=(int)((float)v[i-1].y+(float)(v[i].y-v[i-1].y)*(float)(tt+1)/(float)v[i].frameCount+0.5f);
					ew=(int)((float)v[i-1].w+(float)(v[i].w-v[i-1].w)*(float)(tt+1)/(float)v[i].frameCount+0.5f);
					eh=(int)((float)v[i-1].h+(float)(v[i].h-v[i-1].h)*(float)(tt+1)/(float)v[i].frameCount+0.5f);
					break;
				}else{
					t-=v[i].frameCount*v[i].frameDisplayTime;
				}
			}
		}
	}
	
	//And finally draw the ThemeObjectInstance.
	if(ww>0&&hh>0){
		Alignment hAlign = parent->positioning.horizontalAlign;
		Alignment vAlign = parent->positioning.verticalAlign;

		//If the destination size is not set then assume it's the same as the source size.
		//In this case we also disable the align.
		if (w <= 0) {
			w = ww;
			hAlign = LEFT;
		}
		if (h <= 0) {
			h = hh;
			vAlign = TOP;
		}

		//The destination rectangle (NOTE: the w,h are actually the right and bottom)
		SDL_Rect r2={x+ex,y+ey,0,0};

		//Align horizontally.
		switch (hAlign){
		case CENTRE:
			r2.x += (w - ww) / 2;
			break;
		case RIGHT:
			r2.x += w - ww;
			break;
		}

		//Align vertically.
		switch (vAlign){
		case MIDDLE:
			r2.y += (h - hh) / 2;
			break;
		case BOTTOM:
			r2.y += h - hh;
			break;
		}

		//Calculate the correct right and bottom of the destination rectangle (esp. in REPEAT and STRETCH mode)
		r2.w = (hAlign == REPEAT || hAlign == STRETCH) ? (x + w - ew) : r2.x + ww;
		r2.h = (vAlign == REPEAT || vAlign == STRETCH) ? (y + h - eh) : r2.y + hh;

		//For STRETCH mode we have to use SDL builtin clip rect function
		//otherwise the texture coordinate is hard to calculate
		bool useSDLClipRect = false;

		if (clipRect) {
			//Clip the right and bottom
			if (r2.w > clipRect->x + clipRect->w) {
				if (hAlign == STRETCH) useSDLClipRect = true;
				else r2.w = clipRect->x + clipRect->w;
			}
			if (r2.h > clipRect->y + clipRect->h) {
				if (vAlign == STRETCH) useSDLClipRect = true;
				else r2.h = clipRect->y + clipRect->h;
			}

			//Clip the left and top (ad-hoc code)
			if (r2.x < clipRect->x) {
				if (hAlign == STRETCH) useSDLClipRect = true;
				else r2.x += ((clipRect->x - r2.x) / ww) * ww;
			}
			if (r2.y < clipRect->y) {
				if (vAlign == STRETCH) useSDLClipRect = true;
				else r2.y += ((clipRect->y - r2.y) / hh) * hh;
			}
		}

		//Set the SDL clip rect if necessary
		if (useSDLClipRect) {
			SDL_RenderSetClipRect(&renderer, clipRect);
		}

		//As long as we haven't exceeded the horizontal target keep drawing.
		while (r2.x < r2.w){
			//Store the y position for when more than one column has to be drawn.
			const int y2 = r2.y;
			//As long as we haven't exceeded the vertical target keep drawing.
			while (r2.y < r2.h){
				//The source rectangle which will be modified by clipping.
				SDL_Rect srcrect = { xx, yy, ww, hh };

				//Check if we should clip the right and bottom.
				if (r2.x + ww > r2.w && hAlign != STRETCH) srcrect.w = r2.w - r2.x;
				if (r2.y + hh > r2.h && vAlign != STRETCH) srcrect.h = r2.h - r2.y;

				//The destination rectangle which will be modified by clipping.
				SDL_Rect dstrect = { r2.x, r2.y, 0, 0 };

				//Clip the left and top
				if (clipRect) {
					int d = clipRect->x - dstrect.x;
					if (d > 0 && hAlign != STRETCH) {
						srcrect.x += d; srcrect.w -= d; dstrect.x += d;
					}
					d = clipRect->y - dstrect.y;
					if (d > 0 && vAlign != STRETCH) {
						srcrect.y += d; srcrect.h -= d; dstrect.y += d;
					}
				}

				if (srcrect.w > 0 && srcrect.h > 0) {
					dstrect.w = (hAlign == STRETCH) ? (r2.w - r2.x) : srcrect.w;
					dstrect.h = (vAlign == STRETCH) ? (r2.h - r2.y) : srcrect.h;
					SDL_RenderCopy(&renderer, src, &srcrect, &dstrect);
				}
				if (vAlign == STRETCH) break; //For STRETCH mode draw once is enough
				r2.y += hh;
			}
			if (hAlign == STRETCH) break; //For STRETCH mode draw once is enough
			r2.x += ww;

			//Reset the y position before drawing a new column.
			r2.y = y2;
		}
		
		//Reset the SDL clip rect if necessary
		if (useSDLClipRect) {
			SDL_RenderSetClipRect(&renderer, NULL);
		}
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
	if (currentState >= 0 && currentState < (int)states.size()) {
		ThemeBlockStateInstance &current = states[currentState];

		//Call the updateAnimation method of the currentState.
		current.updateAnimation();

		//Get the length of the animation.
		int m = current.parent->oneTimeAnimationLength;

		//If it's higher than 0 then we have an animation.
		//Also check if it's past the lenght, meaning done.
		if (m > 0 && current.animation >= m){
			//Now we can change the state to the nextState.
			changeState(current.parent->nextState);
		}
	}
}

void ThemeBlockInstance::clear() {
	currentState = -1;
	currentStateName.clear();
	blockStates.clear();
	transitions.clear();
	states.clear();
}

void ThemeBlock::createInstance(ThemeBlockInstance* obj, const std::string& initialState) {
	//Make sure the given ThemeBlockInstance is ready.
	obj->clear();

	//Loop through the blockstates.
	for (auto it = blockStates.begin(); it != blockStates.end(); ++it) {
		//Create a new themeBlockStateInstance.
		obj->states.emplace_back();
		ThemeBlockStateInstance &obj1 = obj->states.back();

		//Register it with given name.
		obj->blockStates[it->first] = obj->states.size() - 1;

		//Set the parent of the state instance.
		obj1.parent = it->second;

		//Create the state instance.
		createStateInstance(&obj1);
	}

	//Loop through the transitions.
	for (auto it = transitions.begin(); it != transitions.end(); ++it) {
		//Create a new themeBlockStateInstance.
		obj->states.emplace_back();
		ThemeBlockStateInstance &obj1 = obj->states.back();

		//Register it with given name.
		obj->transitions[it->first] = obj->states.size() - 1;

		//Set the parent of the state instance.
		obj1.parent = it->second;

		//Create the state instance.
		createStateInstance(&obj1);
	}
	
	//Change the state to the default one.
	//FIXME: Is that needed?
	obj->changeState(initialState);
}

void ThemeBlock::createStateInstance(ThemeBlockStateInstance* obj){
	//Get the vector with themeObjects.
	vector<ThemeObject*> &v=obj->parent->themeObjects;

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
            if(p.parent->editorPicture.texture!=NULL)
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
        if(p.picture==NULL && p.parent->picture.texture!=NULL)
			p.picture=&p.parent->picture;
		//If the picture isn't null then can we give it to the ThemeBlockStateInstance.
		if(p.picture!=NULL)
			obj->objects.push_back(p);
	}
}

void ThemePicture::draw(SDL_Renderer& renderer,int x,int y,int animation,SDL_Rect *clipRect){
	//Get the Picture.
    if(texture==NULL) return;
	int ex=0,ey=0,xx,yy,ww,hh;
	{
        const vector<ThemeOffsetPoint> &v=offset.offsetData;
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
        SDL_Rect r2={x+ex,y+ey,ww,hh};
        SDL_RenderCopy(&renderer, texture.get(), &r1, &r2);
	}
}

//This method will scale the background picture (if needed and configured) to the current SCREEN_WIDTH and SCREEN_HEIGHT.
void ThemeBackgroundPicture::scaleToScreen(){
	//Only scale if needed.
	if(scale){
        // SDL2 allows us to scale the texture when rendering, so
        // we only need to adjust the size of the destination rect.
        destSize.w = SCREEN_WIDTH;
        destSize.h = SCREEN_HEIGHT;
	}
}

void ThemeBackgroundPicture::draw(SDL_Renderer &dest){
	//Check if the picture is visible.
    if(!(texture&&srcSize.w>0&&srcSize.h>0&&destSize.w>0&&destSize.h>0))
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
            // NOTE: Rendercopy cares about w/h here
            // so had to add it for SDL2 port.
            SDL_Rect r={x,y,destSize.w,destSize.h};
            //SDL_BlitSurface(picture,&srcSize,dest,&r);
            SDL_RenderCopy(&dest, texture.get(), &srcSize, &r);
		}
	}
}

bool ThemeBackgroundPicture::loadFromNode(TreeStorageNode* objNode, string themePath, ImageManager &imageManager, SDL_Renderer& renderer){
    //Load the picture directly into a texture.
    texture = imageManager.loadTexture(themePath+objNode->value[0], renderer);
    if (!texture) {
        return false;
    }

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
            // This gets the width and height of the texture.
            SDL_QueryTexture(texture.get(), NULL, NULL, &srcSize.w, &srcSize.h);
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
            destSize.w=SCREEN_WIDTH;
            destSize.h=SCREEN_HEIGHT;
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
		if(!v.empty()){
			scale=atoi(v[0].c_str())?true:false;
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
	hasThemeTextColor = hasThemeTextColorDialog = false;
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

	//Invalidates the cache.
	hasThemeTextColor = hasThemeTextColorDialog = false;
}

//Method that will append a theme to the stack.
//obj: The ThemeManager to add.
void ThemeStack::appendTheme(ThemeManager* obj){
	objThemes.push_back(obj);
	//debug
#if defined(DEBUG) || defined(_DEBUG)
	cout<<"ThemeStack::appendTheme(): theme count="<<objThemes.size()<<endl;
#endif
	//Invalidates the cache.
	hasThemeTextColor = hasThemeTextColorDialog = false;
}
//Method that will remove the last theme added to the stack.
void ThemeStack::removeTheme(){
	//Make sure that the stack isn't empty.
	if(!objThemes.empty()){
		delete objThemes.back();
		objThemes.pop_back();
	}
	//Invalidates the cache.
	hasThemeTextColor = hasThemeTextColorDialog = false;
}

//Method that will append a theme that will be loaded from file.
//fileName: The file to load the theme from.
//Returns: Pointer to the newly added theme, NULL if failed.
ThemeManager* ThemeStack::appendThemeFromFile(const string& fileName, ImageManager &imageManager, SDL_Renderer &renderer){
	//Invalidates the cache.
	hasThemeTextColor = hasThemeTextColorDialog = false;

	//Create a new themeManager.
	ThemeManager* obj=new ThemeManager();
	
	//Let it load from the given file.
    if(!obj->loadFile(fileName, imageManager, renderer)){
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
ThemeBlock* ThemeStack::getScenery(const std::string& name) {
	//Loop through the themes from top to bottom.
	for (int i = objThemes.size() - 1; i >= 0; i--){
		//Get the block from the theme.
		ThemeBlock* obj = objThemes[i]->getScenery(name);
		//Check if it isn't null.
		if (obj)
			return obj;
	}

	//Check if the input is a valid block name.
	if (name.size() > 8 && name.substr(name.size() - 8) == "_Scenery") {
		auto it = Game::blockNameMap.find(name.substr(0, name.size() - 8));
		if (it != Game::blockNameMap.end()){
			return getBlock(it->second);
		}
	}

	//Nothing found.
	return NULL;
}
void ThemeStack::getSceneryBlockNames(std::set<std::string> &s) {
	//Loop through the themes from top to bottom.
	for (int i = objThemes.size() - 1; i >= 0; i--){
		objThemes[i]->getSceneryBlockNames(s);
	}

	//Also add all block names to it.
	for (auto it = Game::blockNameMap.begin(); it != Game::blockNameMap.end(); ++it) {
		s.insert(it->first + "_Scenery");
	}
}
//Get a pointer to the ThemeBlock of the shadow or the player.
//isShadow: Boolean if it's the shadow
//Returns: Pointer to the ThemeBlock.
ThemeBlock* ThemeStack::getCharacter(bool isShadow){
	//Loop through the themes from top to bottom.
	for(int i=objThemes.size()-1;i>=0;i--){
		//Get the ThemeBlock from the theme.
		ThemeBlock* obj=objThemes[i]->getCharacter(isShadow);
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

SDL_Color ThemeStack::getTextColor(bool isDialog) {
	if (isDialog) {
		if (hasThemeTextColorDialog) return themeTextColorDialog;

		//Loop through the themes from top to bottom.
		for (int i = objThemes.size() - 1; i >= 0; i--) {
			if (objThemes[i]->getTextColor(isDialog, themeTextColorDialog)) {
				hasThemeTextColorDialog = true;
				return themeTextColorDialog;
			}
		}

		hasThemeTextColorDialog = true;
		themeTextColorDialog = BLACK;
		return themeTextColorDialog;
	} else {
		if (hasThemeTextColor) return themeTextColor;

		//Loop through the themes from top to bottom.
		for (int i = objThemes.size() - 1; i >= 0; i--) {
			if (objThemes[i]->getTextColor(isDialog, themeTextColor)) {
				hasThemeTextColor = true;
				return themeTextColor;
			}
		}

		hasThemeTextColor = true;
		themeTextColor = BLACK;
		return themeTextColor;
	}
}
