/****************************************************************************
** Copyright (C) 2011 Luka Horvat <redreaper132 at gmail.com>
** Copyright (C) 2011 Edward Lii <edward_iii at myway.com>
** Copyright (C) 2011 O. Bahri Gordebak <gordebak at gmail.com>
**
**
** This file may be used under the terms of the GNU General Public
** License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
**  You should have received a copy of the GNU General Public License
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/

#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include "Globals.h"
#include "TreeStorageNode.h"
#include <string.h>
#include <math.h>
#include <string>
#include <vector>
#include <utility>
using namespace std;

struct typeOffsetPoint{
	int x,y,w,h;
	int frameCount;
	int frameDisplayTime;
};

class ThemeOffsetData;
class ThemePicture;
class ThemeObject;
class ThemeBlockState;
class ThemeBlock;
class ThemeCharacterState;
class ThemeCharacter;

class ThemeObjectInstance{
public:
	ThemePicture *picture;
	ThemeObject *parent;
	int animation,savedAnimation;
public:
	ThemeObjectInstance():picture(NULL),parent(NULL),animation(0),savedAnimation(0){
	}
	void draw(SDL_Surface *dest,int x,int y,SDL_Rect *clipRect=NULL);
	void updateAnimation();
	void resetAnimation(){
		animation=0;
		savedAnimation=0;
	}
	void saveAnimation(){
		savedAnimation=animation;
	}
	void loadAnimation(){
		animation=savedAnimation;
	}
};

class ThemeBlockStateInstance{
public:
	ThemeBlockState *parent;
	vector<ThemeObjectInstance> objects;
	int animation,savedAnimation;
public:
	ThemeBlockStateInstance():parent(NULL),animation(0){}
	
	void draw(SDL_Surface *dest,int x,int y,SDL_Rect *clipRect=NULL){
		for(unsigned int i=0;i<objects.size();i++){
			objects[i].draw(dest,x,y,clipRect);
		}
	}
	
	void updateAnimation(){
		for(unsigned int i=0;i<objects.size();i++){
			objects[i].updateAnimation();
		}
		animation++; //??? TODO:
	}
	
	void resetAnimation(){
		for(unsigned int i=0;i<objects.size();i++){
			objects[i].resetAnimation();
		}
		animation=0;
		savedAnimation=0;
	}
	
	void saveAnimation(){
		for(unsigned int i=0;i<objects.size();i++){
			objects[i].saveAnimation();
		}
		savedAnimation=animation;
	}
	
	void loadAnimation(){
		for(unsigned int i=0;i<objects.size();i++){
			objects[i].loadAnimation();
		}
		animation=savedAnimation;
	}
};

class ThemeBlockInstance{
public:
	ThemeBlockStateInstance *currentState;
	string currentStateName;
	map<string,ThemeBlockStateInstance> blockStates;
	string savedStateName;
public:
	ThemeBlockInstance():currentState(NULL){}
	
	bool draw(SDL_Surface *dest,int x,int y,SDL_Rect *clipRect=NULL){
		if(currentState!=NULL){
			currentState->draw(dest,x,y,clipRect);
			return true;
		}
		return false;
	}
	
	bool drawState(const string& s,SDL_Surface *dest,int x,int y,SDL_Rect *clipRect=NULL){
		map<string,ThemeBlockStateInstance>::iterator it=blockStates.find(s);
		if(it!=blockStates.end()){
			it->second.draw(dest,x,y,clipRect);
			return true;
		}
		return false;
	}
	bool changeState(const string& s,bool reset=true){
		map<string,ThemeBlockStateInstance>::iterator it=blockStates.find(s);
		if(it!=blockStates.end()){
			currentState=&(it->second);
			currentStateName=it->first;
			if(savedStateName.empty()) savedStateName=currentStateName; //???
			if(reset) currentState->resetAnimation();
			return true;
		}
		return false;
	}
	
	void updateAnimation();
	void resetAnimation(){
		for(map<string,ThemeBlockStateInstance>::iterator it=blockStates.begin();it!=blockStates.end();it++){
			it->second.resetAnimation();
		}
		savedStateName.clear();
	}
	void saveAnimation(){
		for(map<string,ThemeBlockStateInstance>::iterator it=blockStates.begin();it!=blockStates.end();it++){
			it->second.saveAnimation();
		}
		savedStateName=currentStateName;
	}
	void loadAnimation(){
		for(map<string,ThemeBlockStateInstance>::iterator it=blockStates.begin();it!=blockStates.end();it++){
			it->second.loadAnimation();
		}
		changeState(savedStateName,false);
	}
};

class ThemeCharacterStateInstance{
public:
	ThemeCharacterState *parent;
	vector<ThemeObjectInstance> objects;
	int animation,savedAnimation;
public:
	ThemeCharacterStateInstance():parent(NULL),animation(0){}
	
	void draw(SDL_Surface *dest,int x,int y,SDL_Rect *clipRect=NULL){
		for(unsigned int i=0;i<objects.size();i++){
			objects[i].draw(dest,x,y,clipRect);
		}
	}
	
	void updateAnimation(){
		for(unsigned int i=0;i<objects.size();i++){
			objects[i].updateAnimation();
		}
		animation++; //??? TODO:
	}
	
	void resetAnimation(){
		for(unsigned int i=0;i<objects.size();i++){
			objects[i].resetAnimation();
		}
		animation=0;
		savedAnimation=0;
	}
	
	void saveAnimation(){
		for(unsigned int i=0;i<objects.size();i++){
			objects[i].saveAnimation();
		}
		savedAnimation=animation;
	}
	
	void loadAnimation(){
		for(unsigned int i=0;i<objects.size();i++){
			objects[i].loadAnimation();
		}
		animation=savedAnimation;
	}
};

class ThemeCharacterInstance{
public:
	ThemeCharacterStateInstance *currentState;
	string currentStateName;
	map<string,ThemeCharacterStateInstance> characterStates;
	string savedStateName;
public:
	ThemeCharacterInstance():currentState(NULL){}
	
	bool draw(SDL_Surface *dest,int x,int y,SDL_Rect *clipRect=NULL){
		if(currentState!=NULL){
			currentState->draw(dest,x,y,clipRect);
			return true;
		}
		return false;
	}
	
	bool drawState(const string& s,SDL_Surface *dest,int x,int y,SDL_Rect *clipRect=NULL){
		map<string,ThemeCharacterStateInstance>::iterator it=characterStates.find(s);
		if(it!=characterStates.end()){
			it->second.draw(dest,x,y,clipRect);
			return true;
		}
		return false;
	}
	bool changeState(const string& s,bool reset=true){
		map<string,ThemeCharacterStateInstance>::iterator it=characterStates.find(s);
		if(it!=characterStates.end()){
			currentState=&(it->second);
			currentStateName=it->first;
			if(savedStateName.empty()) savedStateName=currentStateName; //???
			if(reset) currentState->resetAnimation();
			return true;
		}
		return false;
	}
	
	void updateAnimation();
	void resetAnimation(){
		for(map<string,ThemeCharacterStateInstance>::iterator it=characterStates.begin();it!=characterStates.end();it++){
			it->second.resetAnimation();
		}
		savedStateName.clear();
	}
	void saveAnimation(){
		for(map<string,ThemeCharacterStateInstance>::iterator it=characterStates.begin();it!=characterStates.end();it++){
			it->second.saveAnimation();
		}
		savedStateName=currentStateName;
	}
	void loadAnimation(){
		for(map<string,ThemeCharacterStateInstance>::iterator it=characterStates.begin();it!=characterStates.end();it++){
			it->second.loadAnimation();
		}
		changeState(savedStateName,false);
	}
};


class ThemeOffsetData{
public:
	vector<typeOffsetPoint> offsetData;
	int length;
public:
	ThemeOffsetData():length(0){}
	
	void destroy(){
		length=0;
		offsetData.clear();
	}
	
	~ThemeOffsetData(){}
	
	bool loadFromNode(TreeStorageNode* objNode);
};

class ThemePicture{
public:
	SDL_Surface* picture;
	ThemeOffsetData offset;
public:
	ThemePicture():picture(NULL){}
	
	void destroy(){
		picture=NULL;//SDL_FreeSurface ??? (use ImageManager)
		offset.destroy();
	}
	~ThemePicture(){}
	bool loadFromNode(TreeStorageNode* objNode);
	void draw(SDL_Surface *dest,int x,int y,int animation=0,SDL_Rect *clipRect=NULL);
};

class ThemeObject{
public:
	int animationLength,animationLoopPoint;
	bool invisibleAtRunTime,invisibleAtDesignTime;
	ThemePicture picture,editorPicture;
	vector<pair<double,ThemePicture*> > optionalPicture;
	ThemeOffsetData offset;
public:
	ThemeObject():animationLength(0),animationLoopPoint(0),invisibleAtRunTime(false),invisibleAtDesignTime(false){}
	
	void destroy(){
		for(unsigned int i=0;i<optionalPicture.size();i++){
			delete optionalPicture[i].second;
		}
		optionalPicture.clear();
		animationLength=0;
		animationLoopPoint=0;
		invisibleAtRunTime=false;
		invisibleAtDesignTime=false;
		picture.destroy();
		editorPicture.destroy();
		offset.destroy();
	}
	~ThemeObject(){
		for(unsigned int i=0;i<optionalPicture.size();i++){
			delete optionalPicture[i].second;
		}
	}
	bool loadFromNode(TreeStorageNode* objNode);
};

class ThemeBlockState{
public:
	int oneTimeAnimationLength;
	string nextState;
	vector<ThemeObject*> themeObjects;
public:
	ThemeBlockState():oneTimeAnimationLength(0){}
	void destroy(){
		for(unsigned int i=0;i<themeObjects.size();i++){
			delete themeObjects[i];
		}
		themeObjects.clear();
		oneTimeAnimationLength=0;
		nextState.clear();
	}
	~ThemeBlockState(){
		for(unsigned int i=0;i<themeObjects.size();i++){
			delete themeObjects[i];
		}
	}
	bool loadFromNode(TreeStorageNode* objNode);
};

class ThemeBlock{
public:
	ThemePicture editorPicture;
	map<string,ThemeBlockState*> blockStates;
public:
	ThemeBlock(){}
	void destroy(){
		for(map<string,ThemeBlockState*>::iterator i=blockStates.begin();i!=blockStates.end();i++){
			delete i->second;
		}
		blockStates.clear();
		editorPicture.destroy();
	}
	~ThemeBlock(){
		for(map<string,ThemeBlockState*>::iterator i=blockStates.begin();i!=blockStates.end();i++){
			delete i->second;
		}
	}
	bool loadFromNode(TreeStorageNode* objNode);
	void createInstance(ThemeBlockInstance* obj);
};

class ThemeCharacterState{
public:
	int oneTimeAnimationLength;
	string nextState;
	vector<ThemeObject*> themeObjects;
public:
	ThemeCharacterState():oneTimeAnimationLength(0){}
	void destroy(){
		for(unsigned int i=0;i<themeObjects.size();i++){
			delete themeObjects[i];
		}
		themeObjects.clear();
		oneTimeAnimationLength=0;
		nextState.clear();
	}
	~ThemeCharacterState(){
		for(unsigned int i=0;i<themeObjects.size();i++){
			delete themeObjects[i];
		}
	}
	bool loadFromNode(TreeStorageNode* objNode);
};

class ThemeCharacter{
public:
	map<string,ThemeCharacterState*> characterStates;
public:
	ThemeCharacter(){}
	void destroy(){
		for(map<string,ThemeCharacterState*>::iterator i=characterStates.begin();i!=characterStates.end();i++){
			delete i->second;
		}
		characterStates.clear();
	}
	~ThemeCharacter(){
		for(map<string,ThemeCharacterState*>::iterator i=characterStates.begin();i!=characterStates.end();i++){
			delete i->second;
		}
	}
	bool loadFromNode(TreeStorageNode* objNode);
	void createInstance(ThemeCharacterInstance* obj);
};


class ThemeBackgroundPicture{
private:
	SDL_Surface *picture;
	SDL_Rect srcSize;
	SDL_Rect destSize;
	bool repeatX;
	bool repeatY;
	float speedX;
	float speedY;
	float cameraX;
	float cameraY;
private:
	float currentX;
	float currentY;
	float savedX;
	float savedY;
public:
	ThemeBackgroundPicture(){
		picture=NULL;
		memset(&srcSize,0,sizeof(srcSize));
		memset(&destSize,0,sizeof(destSize));
		repeatX=true;
		repeatY=true;
		speedX=0.0f;
		speedY=0.0f;
		cameraX=0.0f;
		cameraY=0.0f;
		currentX=0.0f;
		currentY=0.0f;
		savedX=0.0f;
		savedY=0.0f;
	}
	void updateAnimation(){
		currentX+=speedX;
		if(repeatX && destSize.w>0){
			float f=(float)destSize.w;
			if(currentX>f || currentX<-f) currentX-=f*floor(currentX/f);
		}
		currentY+=speedY;
		if(repeatY && destSize.h>0){
			float f=(float)destSize.h;
			if(currentY>f || currentY<-f) currentY-=f*floor(currentY/f);
		}
	}
	void resetAnimation(){
		currentX=0.0f;
		currentY=0.0f;
		savedX=0.0f;
		savedY=0.0f;
	}
	void saveAnimation(){
		savedX=currentX;
		savedY=currentY;
	}
	void loadAnimation(){
		currentX=savedX;
		currentY=savedY;
	}
	void draw(SDL_Surface *dest);
	bool loadFromNode(TreeStorageNode* objNode);
};

class ThemeBackground{
private:
	vector<ThemeBackgroundPicture> picture;
public:
	void updateAnimation(){
		for(unsigned int i=0;i<picture.size();i++){
			picture[i].updateAnimation();
		}
	}
	void resetAnimation(){
		for(unsigned int i=0;i<picture.size();i++){
			picture[i].resetAnimation();
		}
	}
	void saveAnimation(){
		for(unsigned int i=0;i<picture.size();i++){
			picture[i].saveAnimation();
		}
	}
	void loadAnimation(){
		for(unsigned int i=0;i<picture.size();i++){
			picture[i].loadAnimation();
		}
	}
	void draw(SDL_Surface *dest){
		for(unsigned int i=0;i<picture.size();i++){
			picture[i].draw(dest);
		}
	}
	bool addPictureFromNode(TreeStorageNode* objNode){
		picture.push_back(ThemeBackgroundPicture());
		return picture.back().loadFromNode(objNode);
	}
};

class ThemeManager{
private:
	ThemeCharacter* shadow;
	ThemeCharacter* player;
	ThemeBlock* objBlocks[TYPE_MAX];
	ThemeBackground* objBackground;
public:
	string themeName;
public:
	ThemeManager(){
		objBackground=NULL;
		memset(objBlocks,0,sizeof(objBlocks));
		shadow=NULL;
		player=NULL;
	}
	void destroy(){
		if(shadow) delete shadow;
		if(player) delete player;
		for(int i=0;i<TYPE_MAX;i++){
			if(objBlocks[i]) delete objBlocks[i];
		}
		memset(objBlocks,0,sizeof(objBlocks));
		if(objBackground) delete objBackground;
		objBackground=NULL;
		themeName.clear();
	}
	~ThemeManager(){
		for(int i=0;i<TYPE_MAX;i++){
			if(objBlocks[i]) delete objBlocks[i];
		}
		if(objBackground) delete objBackground;
	}
	bool loadFile(const string& fileName);
	ThemeBlock* getBlock(int index){
		return objBlocks[index];
	}
	ThemeCharacter* getCharacter(bool isShadow){
		if(isShadow) return shadow;
		return player;
	}
	ThemeBackground* getBackground(){
		return objBackground;
	}
};

class ThemeStack{
private:
	vector<ThemeManager*> objThemes;
public:
	ThemeStack(){}
	void destroy(){
		for(unsigned int i=0;i<objThemes.size();i++) delete objThemes[i];
		objThemes.clear();
	}
	~ThemeStack(){
		for(unsigned int i=0;i<objThemes.size();i++) delete objThemes[i];
	}
	void appendTheme(ThemeManager* obj){
		objThemes.push_back(obj);
	}
	void removeTheme(){
		if(!objThemes.empty()){
			delete objThemes.back();
			objThemes.pop_back();
		}
	}
	ThemeManager* appendThemeFromFile(const string& fileName){
		ThemeManager* obj=new ThemeManager();
		if(!obj->loadFile(fileName)){
			delete obj;
			return NULL;
		}else{
			objThemes.push_back(obj);
			return obj;
		}
	}
	int themeCount(){
		return (int)objThemes.size();
	}
	ThemeManager* operator[](int i){
		return objThemes[i];
	}
	ThemeBlock* getBlock(int index){
		for(int i=objThemes.size()-1;i>=0;i--){
			ThemeBlock* obj=objThemes[i]->getBlock(index);
			if(obj) return obj;
		}
		return NULL;
	}
	ThemeCharacter* getCharacter(bool isShadow){
		for(int i=objThemes.size()-1;i>=0;i--){
			ThemeCharacter* obj=objThemes[i]->getCharacter(isShadow);
			if(obj) return obj;
		}
		return NULL;
	}
	ThemeBackground* getBackground(){
		for(int i=objThemes.size()-1;i>=0;i--){
			ThemeBackground* obj=objThemes[i]->getBackground();
			if(obj) return obj;
		}
		return NULL;
	}
};

extern ThemeStack objThemes;

#endif