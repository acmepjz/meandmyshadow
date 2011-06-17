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
	int nFrameCount;
	int nFrameDisplayTime;
};

class ThemeOffsetData;
class ThemePicture;
class ThemeObject;
class ThemeBlockState;
class ThemeBlock;

class ThemeObjectInstance{
public:
	ThemePicture *Picture;
	ThemeObject *Parent;
	int nAnimation,nSavedAnimation;
public:
	ThemeObjectInstance():Picture(NULL),Parent(NULL),nAnimation(0),nSavedAnimation(0){
	}
	void Draw(SDL_Surface *dest,int x,int y,SDL_Rect *ClipRect=NULL);
	void UpdateAnimation();
	void ResetAnimation(){
		nAnimation=0;
		nSavedAnimation=0;
	}
	void SaveAnimation(){
		nSavedAnimation=nAnimation;
	}
	void LoadAnimation(){
		nAnimation=nSavedAnimation;
	}
};

class ThemeBlockStateInstance{
public:
	ThemeBlockState *Parent;
	vector<ThemeObjectInstance> Objects;
	int nAnimation,nSavedAnimation;
public:
	ThemeBlockStateInstance():Parent(NULL),nAnimation(0){
	}
	void Draw(SDL_Surface *dest,int x,int y,SDL_Rect *ClipRect=NULL){
		for(unsigned int i=0;i<Objects.size();i++){
			Objects[i].Draw(dest,x,y,ClipRect);
		}
	}
	void UpdateAnimation(){
		for(unsigned int i=0;i<Objects.size();i++){
			Objects[i].UpdateAnimation();
		}
		nAnimation++; //??? TODO:
	}
	void ResetAnimation(){
		for(unsigned int i=0;i<Objects.size();i++){
			Objects[i].ResetAnimation();
		}
		nAnimation=0;
		nSavedAnimation=0;
	}
	void SaveAnimation(){
		for(unsigned int i=0;i<Objects.size();i++){
			Objects[i].SaveAnimation();
		}
		nSavedAnimation=nAnimation;
	}
	void LoadAnimation(){
		for(unsigned int i=0;i<Objects.size();i++){
			Objects[i].LoadAnimation();
		}
		nAnimation=nSavedAnimation;
	}
};

class ThemeBlockInstance{
public:
	ThemeBlockStateInstance *CurrentState;
	string CurrentStateName;
	map<string,ThemeBlockStateInstance> BlockStates;
	string SavedStateName;
public:
	ThemeBlockInstance():CurrentState(NULL){
	}
	bool Draw(SDL_Surface *dest,int x,int y,SDL_Rect *ClipRect=NULL){
		if(CurrentState!=NULL){
			CurrentState->Draw(dest,x,y,ClipRect);
			return true;
		}
		return false;
	}
	bool DrawState(const string& s,SDL_Surface *dest,int x,int y,SDL_Rect *ClipRect=NULL){
		map<string,ThemeBlockStateInstance>::iterator it=BlockStates.find(s);
		if(it!=BlockStates.end()){
			it->second.Draw(dest,x,y,ClipRect);
			return true;
		}
		return false;
	}
	bool ChangeState(const string& s,bool bReset=true){
		map<string,ThemeBlockStateInstance>::iterator it=BlockStates.find(s);
		if(it!=BlockStates.end()){
			CurrentState=&(it->second);
			CurrentStateName=it->first;
			if(SavedStateName.empty()) SavedStateName=CurrentStateName; //???
			if(bReset) CurrentState->ResetAnimation();
			return true;
		}
		return false;
	}
	void UpdateAnimation();
	void ResetAnimation(){
		for(map<string,ThemeBlockStateInstance>::iterator it=BlockStates.begin();it!=BlockStates.end();it++){
			it->second.ResetAnimation();
		}
		SavedStateName.clear();
	}
	void SaveAnimation(){
		for(map<string,ThemeBlockStateInstance>::iterator it=BlockStates.begin();it!=BlockStates.end();it++){
			it->second.SaveAnimation();
		}
		SavedStateName=CurrentStateName;
	}
	void LoadAnimation(){
		for(map<string,ThemeBlockStateInstance>::iterator it=BlockStates.begin();it!=BlockStates.end();it++){
			it->second.LoadAnimation();
		}
		ChangeState(SavedStateName,false);
	}
};

class ThemeOffsetData{
public:
	vector<typeOffsetPoint> OffsetData;
	int Length;
public:
	ThemeOffsetData():Length(0){
	}
	void Destroy(){
		Length=0;
		OffsetData.clear();
	}
	~ThemeOffsetData(){
	}
	bool LoadFromNode(TreeStorageNode* objNode);
};

class ThemePicture{
public:
	SDL_Surface *Picture;
	ThemeOffsetData Offset;
public:
	ThemePicture():Picture(NULL){
	}
	void Destroy(){
		Picture=NULL;//SDL_FreeSurface ??? (use ImageManager)
		Offset.Destroy();
	}
	~ThemePicture(){
	}
	bool LoadFromNode(TreeStorageNode* objNode);
	void Draw(SDL_Surface *dest,int x,int y,int nAnimation=0,SDL_Rect *ClipRect=NULL);
};

class ThemeObject{
public:
	int AnimationLength,AnimationLoopPoint;
	bool InvisibleAtRunTime,InvisibleAtDesignTime;
	ThemePicture Picture,EditorPicture;
	vector<pair<double,ThemePicture*> > OptionalPicture;
	ThemeOffsetData Offset;
public:
	ThemeObject():AnimationLength(0),AnimationLoopPoint(0),InvisibleAtRunTime(false),InvisibleAtDesignTime(false){
	}
	void Destroy(){
		for(unsigned int i=0;i<OptionalPicture.size();i++){
			delete OptionalPicture[i].second;
		}
		OptionalPicture.clear();
		AnimationLength=0;
		AnimationLoopPoint=0;
		InvisibleAtRunTime=false;
		InvisibleAtDesignTime=false;
		Picture.Destroy();
		EditorPicture.Destroy();
		Offset.Destroy();
	}
	~ThemeObject(){
		for(unsigned int i=0;i<OptionalPicture.size();i++){
			delete OptionalPicture[i].second;
		}
	}
	bool LoadFromNode(TreeStorageNode* objNode);
};

class ThemeBlockState{
public:
	int OneTimeAnimationLength;
	string NextState;
	vector<ThemeObject*> ThemeObjects;
public:
	ThemeBlockState():OneTimeAnimationLength(0){
	}
	void Destroy(){
		for(unsigned int i=0;i<ThemeObjects.size();i++){
			delete ThemeObjects[i];
		}
		ThemeObjects.clear();
		OneTimeAnimationLength=0;
		NextState.clear();
	}
	~ThemeBlockState(){
		for(unsigned int i=0;i<ThemeObjects.size();i++){
			delete ThemeObjects[i];
		}
	}
	bool LoadFromNode(TreeStorageNode* objNode);
};

class ThemeBlock{
public:
	ThemePicture EditorPicture;
	map<string,ThemeBlockState*> BlockStates;
public:
	ThemeBlock(){
	}
	void Destroy(){
		for(map<string,ThemeBlockState*>::iterator i=BlockStates.begin();i!=BlockStates.end();i++){
			delete i->second;
		}
		BlockStates.clear();
		EditorPicture.Destroy();
	}
	~ThemeBlock(){
		for(map<string,ThemeBlockState*>::iterator i=BlockStates.begin();i!=BlockStates.end();i++){
			delete i->second;
		}
	}
	bool LoadFromNode(TreeStorageNode* objNode);
	void CreateInstance(ThemeBlockInstance* obj);
};

class ThemeBackgroundPicture{
private:
	SDL_Surface *Picture;
	SDL_Rect SrcSize;
	SDL_Rect DestSize;
	bool RepeatX;
	bool RepeatY;
	float SpeedX;
	float SpeedY;
	float CameraX;
	float CameraY;
private:
	float CurrentX;
	float CurrentY;
	float SavedX;
	float SavedY;
public:
	ThemeBackgroundPicture(){
		Picture=NULL;
		memset(&SrcSize,0,sizeof(SrcSize));
		memset(&DestSize,0,sizeof(DestSize));
		RepeatX=true;
		RepeatY=true;
		SpeedX=0.0f;
		SpeedY=0.0f;
		CameraX=0.0f;
		CameraY=0.0f;
		CurrentX=0.0f;
		CurrentY=0.0f;
		SavedX=0.0f;
		SavedY=0.0f;
	}
	void UpdateAnimation(){
		CurrentX+=SpeedX;
		if(RepeatX && DestSize.w>0){
			float f=(float)DestSize.w;
			if(CurrentX>f || CurrentX<-f) CurrentX-=f*floor(CurrentX/f);
		}
		CurrentY+=SpeedY;
		if(RepeatY && DestSize.h>0){
			float f=(float)DestSize.h;
			if(CurrentY>f || CurrentY<-f) CurrentY-=f*floor(CurrentY/f);
		}
	}
	void ResetAnimation(){
		CurrentX=0.0f;
		CurrentY=0.0f;
		SavedX=0.0f;
		SavedY=0.0f;
	}
	void SaveAnimation(){
		SavedX=CurrentX;
		SavedY=CurrentY;
	}
	void LoadAnimation(){
		CurrentX=SavedX;
		CurrentY=SavedY;
	}
	void Draw(SDL_Surface *dest);
	bool LoadFromNode(TreeStorageNode* objNode);
};

class ThemeBackground{
private:
	vector<ThemeBackgroundPicture> Picture;
public:
	void UpdateAnimation(){
		for(unsigned int i=0;i<Picture.size();i++){
			Picture[i].UpdateAnimation();
		}
	}
	void ResetAnimation(){
		for(unsigned int i=0;i<Picture.size();i++){
			Picture[i].ResetAnimation();
		}
	}
	void SaveAnimation(){
		for(unsigned int i=0;i<Picture.size();i++){
			Picture[i].SaveAnimation();
		}
	}
	void LoadAnimation(){
		for(unsigned int i=0;i<Picture.size();i++){
			Picture[i].LoadAnimation();
		}
	}
	void Draw(SDL_Surface *dest){
		for(unsigned int i=0;i<Picture.size();i++){
			Picture[i].Draw(dest);
		}
	}
	bool AddPictureFromNode(TreeStorageNode* objNode){
		Picture.push_back(ThemeBackgroundPicture());
		return Picture.back().LoadFromNode(objNode);
	}
};

class ThemeManager{
private:
	ThemeBlock* m_objBlocks[TYPE_MAX];
	ThemeBackground* m_objBackground;
public:
	string ThemeName;
public:
	ThemeManager(){
		m_objBackground=NULL;
		memset(m_objBlocks,0,sizeof(m_objBlocks));
	}
	void Destroy(){
		for(int i=0;i<TYPE_MAX;i++){
			if(m_objBlocks[i]) delete m_objBlocks[i];
		}
		memset(m_objBlocks,0,sizeof(m_objBlocks));
		if(m_objBackground) delete m_objBackground;
		m_objBackground=NULL;
		ThemeName.clear();
	}
	~ThemeManager(){
		for(int i=0;i<TYPE_MAX;i++){
			if(m_objBlocks[i]) delete m_objBlocks[i];
		}
		if(m_objBackground) delete m_objBackground;
	}
	bool LoadFile(const string& FileName);
	ThemeBlock* GetBlock(int Index){
		return m_objBlocks[Index];
	}
	ThemeBackground* GetBackground(){
		return m_objBackground;
	}
};

class ThemeStack{
private:
	vector<ThemeManager*> m_objThemes;
public:
	ThemeStack(){
	}
	void Destroy(){
		for(unsigned int i=0;i<m_objThemes.size();i++) delete m_objThemes[i];
		m_objThemes.clear();
	}
	~ThemeStack(){
		for(unsigned int i=0;i<m_objThemes.size();i++) delete m_objThemes[i];
	}
	void AppendTheme(ThemeManager* obj){
		m_objThemes.push_back(obj);
	}
	void RemoveTheme(){
		if(!m_objThemes.empty()){
			delete m_objThemes.back();
			m_objThemes.pop_back();
		}
	}
	ThemeManager* AppendThemeFromFile(const string& FileName){
		ThemeManager* obj=new ThemeManager();
		if(!obj->LoadFile(FileName)){
			delete obj;
			return NULL;
		}else{
			m_objThemes.push_back(obj);
			return obj;
		}
	}
	int ThemeCount(){
		return (int)m_objThemes.size();
	}
	ThemeManager* operator[](int i){
		return m_objThemes[i];
	}
	ThemeBlock* GetBlock(int Index){
		for(int i=m_objThemes.size()-1;i>=0;i--){
			ThemeBlock* obj=m_objThemes[i]->GetBlock(Index);
			if(obj) return obj;
		}
		return NULL;
	}
	ThemeBackground* GetBackground(){
		for(int i=m_objThemes.size()-1;i>=0;i--){
			ThemeBackground* obj=m_objThemes[i]->GetBackground();
			if(obj) return obj;
		}
		return NULL;
	}
};

extern ThemeStack m_objThemes;

#endif