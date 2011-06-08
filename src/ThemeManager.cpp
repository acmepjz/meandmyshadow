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

#include "ThemeManager.h"
#include "POASerializer.h"
#include "Functions.h"
#include "Game.h"
#include <string.h>
#include <iostream>
using namespace std;

ThemeStack m_objThemes;

bool ThemeManager::LoadFile(const string& FileName){
	POASerializer objSerializer;
	TreeStorageNode objNode;
	//===
	Destroy();
	//===
	if(!objSerializer.LoadNodeFromFile(FileName.c_str(),&objNode,true)) return false;
	//get name
	{
		vector<string> &v=objNode.Attributes["name"];
		if(v.size()>0) ThemeName=v[0];
	}
	//get subnodes
	for(unsigned int i=0;i<objNode.SubNodes.size();i++){
		TreeStorageNode *obj=objNode.SubNodes[i];
		if(obj->Name=="block" && obj->Value.size()>0){
			map<string,int>::iterator it=Game::g_BlockNameMap.find(obj->Value[0]);
			if(it!=Game::g_BlockNameMap.end()){
				int idx=it->second;
				if(!m_objBlocks[idx]) m_objBlocks[idx]=new ThemeBlock;
				if(!m_objBlocks[idx]->LoadFromNode(obj)){
					delete m_objBlocks[idx];
					m_objBlocks[idx]=NULL;
					return false;
				}
			}
		}
	}
	//over
	return true;
}

bool ThemeBlock::LoadFromNode(TreeStorageNode* objNode){
	Destroy();
	//get subnodes
	for(unsigned int i=0;i<objNode->SubNodes.size();i++){
		TreeStorageNode *obj=objNode->SubNodes[i];
		if(obj->Name=="editorPicture"){
			if(!EditorPicture.LoadFromNode(obj)) return false;
		}else if(obj->Name=="blockState" && obj->Value.size()>0){
			string& s=obj->Value[0];
			map<string,ThemeBlockState*>::iterator it=BlockStates.find(s);
			if(it==BlockStates.end()) BlockStates[s]=new ThemeBlockState;
			if(!BlockStates[s]->LoadFromNode(obj)) return false;
		}
	}
	//over
	return true;
}

bool ThemeBlockState::LoadFromNode(TreeStorageNode* objNode){
	Destroy();
	//get attributes
	{
		vector<string> &v=objNode->Attributes["oneTimeAnimation"];
		if(v.size()>=2 && !v[0].empty()){
			OneTimeAnimationLength=atoi(v[0].c_str());
			NextState=v[1];
		}
	}
	//get subnodes
	for(unsigned int i=0;i<objNode->SubNodes.size();i++){
		TreeStorageNode *obj=objNode->SubNodes[i];
		if(obj->Name=="object"){
			ThemeObject *obj1=new ThemeObject();
			if(!obj1->LoadFromNode(obj)){
				delete obj1;
				return false;
			}
			ThemeObjects.push_back(obj1);
		}
	}
	//over
	return true;
}

bool ThemeObject::LoadFromNode(TreeStorageNode* objNode){
	Destroy();
	//get attributes
	{
		vector<string> &v=objNode->Attributes["animation"];
		if(v.size()>=2){
			AnimationLength=atoi(v[0].c_str());
			AnimationLoopPoint=atoi(v[1].c_str());
		}
	}
	{
		vector<string> &v=objNode->Attributes["invisibleAtRunTime"];
		if(v.size()>0 && !v[0].empty()){
			InvisibleAtRunTime=atoi(v[0].c_str())?true:false;
		}
	}
	{
		vector<string> &v=objNode->Attributes["invisibleAtDesignTime"];
		if(v.size()>0 && !v[0].empty()){
			InvisibleAtDesignTime=atoi(v[0].c_str())?true:false;
		}
	}
	//get subnodes
	for(unsigned int i=0;i<objNode->SubNodes.size();i++){
		TreeStorageNode *obj=objNode->SubNodes[i];
		if(obj->Name=="picture" || obj->Name=="pictureAnimation"){
			if(!Picture.LoadFromNode(obj)){
				return false;
			}
		}else if(obj->Name=="editorPicture"){
			if(!EditorPicture.LoadFromNode(obj)){
				return false;
			}
		}else if(obj->Name=="optionalPicture" && obj->Value.size()>=6){
			ThemePicture *objPic=new ThemePicture();
			double f=atof(obj->Value[5].c_str());
			if(!objPic->LoadFromNode(obj)){
				delete objPic;
				return false;
			}
			OptionalPicture.push_back(pair<double,ThemePicture*>(f,objPic));
		}else if(obj->Name=="offset" || obj->Name=="offsetAnimation"){
			if(!Offset.LoadFromNode(obj)) return false;
		}
	}
	//over
	return true;
}

bool ThemePicture::LoadFromNode(TreeStorageNode* objNode){
	Destroy();
	if(objNode->Value.size()>0){
		Picture=load_image(ProcessFileName(objNode->Value[0]));
		if(Picture==NULL) return false;
		if(objNode->Name=="pictureAnimation"){
			if(!Offset.LoadFromNode(objNode)) return false;
			return true;
		}else if(objNode->Value.size()>=5){
			typeOffsetPoint r={atoi(objNode->Value[1].c_str()),
				atoi(objNode->Value[2].c_str()),
				atoi(objNode->Value[3].c_str()),
				atoi(objNode->Value[4].c_str()),0,0};
			Offset.OffsetData.push_back(r);
			Offset.Length=0;
			return true;
		}
	}
	//over
	return false;
}

bool ThemeOffsetData::LoadFromNode(TreeStorageNode* objNode){
	Destroy();
	if(objNode->Name=="pictureAnimation"){
		for(unsigned int i=0;i<objNode->SubNodes.size();i++){
			TreeStorageNode* obj=objNode->SubNodes[i];
			if(obj->Name=="point" && obj->Value.size()>=4){
				typeOffsetPoint r={atoi(obj->Value[0].c_str()),
					atoi(obj->Value[1].c_str()),
					atoi(obj->Value[2].c_str()),
					atoi(obj->Value[3].c_str()),1,1};
				if(obj->Value.size()>=5) r.nFrameCount=atoi(obj->Value[4].c_str());
				if(obj->Value.size()>=6) r.nFrameDisplayTime=atoi(obj->Value[5].c_str());
				OffsetData.push_back(r);
				Length+=r.nFrameCount*r.nFrameDisplayTime;
			}
		}
		return true;
	}else if(objNode->Name=="offsetAnimation"){
		for(unsigned int i=0;i<objNode->SubNodes.size();i++){
			TreeStorageNode* obj=objNode->SubNodes[i];
			if(obj->Name=="point" && obj->Value.size()>=2){
				typeOffsetPoint r={atoi(obj->Value[0].c_str()),
					atoi(obj->Value[1].c_str()),0,0,1,1};
				if(obj->Value.size()>=3) r.nFrameCount=atoi(obj->Value[2].c_str());
				if(obj->Value.size()>=4) r.nFrameDisplayTime=atoi(obj->Value[3].c_str());
				OffsetData.push_back(r);
				Length+=r.nFrameCount*r.nFrameDisplayTime;
			}
		}
		return true;
	}else if(objNode->Name=="offset" && objNode->Value.size()>=2){
		typeOffsetPoint r={atoi(objNode->Value[0].c_str()),
			atoi(objNode->Value[1].c_str()),0,0,0,0};
		OffsetData.push_back(r);
		Length=0;
		return true;
	}
	//over
	return false;
}

void ThemeObjectInstance::Draw(SDL_Surface *dest,int x,int y,SDL_Rect *ClipRect){
	//get picture
	SDL_Surface *src=Picture->Picture;
	int ex,ey,xx,yy,ww,hh;
	{
		vector<typeOffsetPoint> &v=Picture->Offset.OffsetData;
		if(Picture->Offset.Length==0 || nAnimation<v[0].nFrameDisplayTime){
			xx=v[0].x;
			yy=v[0].y;
			ww=v[0].w;
			hh=v[0].h;
		}else if(nAnimation>=Picture->Offset.Length){
			int i=v.size()-1;
			xx=v[i].x;
			yy=v[i].y;
			ww=v[i].w;
			hh=v[i].h;
		}else{
			int t=nAnimation-v[0].nFrameDisplayTime;
			for(unsigned int i=1;i<v.size();i++){
				int tt=t/v[i].nFrameDisplayTime;
				if(tt>=0 && tt<v[i].nFrameCount){
					xx=(int)((float)v[i-1].x+(float)(v[i].x-v[i-1].x)*(float)(tt+1)/(float)v[i].nFrameCount+0.5f);
					yy=(int)((float)v[i-1].y+(float)(v[i].y-v[i-1].y)*(float)(tt+1)/(float)v[i].nFrameCount+0.5f);
					ww=(int)((float)v[i-1].w+(float)(v[i].w-v[i-1].w)*(float)(tt+1)/(float)v[i].nFrameCount+0.5f);
					hh=(int)((float)v[i-1].h+(float)(v[i].h-v[i-1].h)*(float)(tt+1)/(float)v[i].nFrameCount+0.5f);
					break;
				}else{
					t-=v[i].nFrameCount*v[i].nFrameDisplayTime;
				}
			}
		}
	}
	//get offset
	{
		vector<typeOffsetPoint> &v=Parent->Offset.OffsetData;
		if(v.empty()){
			ex=0;
			ey=0;
		}else if(Parent->Offset.Length==0 || nAnimation<v[0].nFrameDisplayTime){
			ex=v[0].x;
			ey=v[0].y;
		}else if(nAnimation>=Parent->Offset.Length){
			int i=v.size()-1;
			ex=v[i].x;
			ey=v[i].y;
		}else{
			int t=nAnimation-v[0].nFrameDisplayTime;
			for(unsigned int i=1;i<v.size();i++){
				int tt=t/v[i].nFrameDisplayTime;
				if(tt>=0 && tt<v[i].nFrameCount){
					ex=(int)((float)v[i-1].x+(float)(v[i].x-v[i-1].x)*(float)(tt+1)/(float)v[i].nFrameCount+0.5f);
					ey=(int)((float)v[i-1].y+(float)(v[i].y-v[i-1].y)*(float)(tt+1)/(float)v[i].nFrameCount+0.5f);
					break;
				}else{
					t-=v[i].nFrameCount*v[i].nFrameDisplayTime;
				}
			}
		}
	}
	//draw
	if(ClipRect){
		int d;
		d=ClipRect->x-ex;
		if(d>0){
			ex+=d;
			xx+=d;
			ww-=d;
		}
		d=ClipRect->y-ey;
		if(d>0){
			ey+=d;
			yy+=d;
			hh-=d;
		}
		if(ww>ClipRect->w) ww=ClipRect->w;
		if(hh>ClipRect->h) hh=ClipRect->h;
	}
	if(ww>0&&hh>0){
		SDL_Rect r1={xx,yy,ww,hh};
		SDL_Rect r2={x+ex,y+ey,0,0};
		SDL_BlitSurface(src,&r1,dest,&r2);
	}
}

void ThemeObjectInstance::UpdateAnimation(){
	int m;
	m=Parent->AnimationLength;
	if(m>0){
		nAnimation++;
		if(nAnimation>=m) nAnimation=Parent->AnimationLoopPoint; //??? TODO:adjustable
	}
}

void ThemeBlockInstance::UpdateAnimation(){
	if(CurrentState!=NULL){
		CurrentState->UpdateAnimation();
		int m=CurrentState->Parent->OneTimeAnimationLength;
		if(m>0 && CurrentState->nAnimation>=m){
			ChangeState(CurrentState->Parent->NextState);
		}
	}
}

void ThemeBlock::CreateInstance(ThemeBlockInstance* obj){
	obj->BlockStates.clear();
	obj->CurrentState=NULL;
	//===
	for(map<string,ThemeBlockState*>::iterator it=BlockStates.begin();it!=BlockStates.end();it++){
		ThemeBlockStateInstance &obj1=obj->BlockStates[it->first];
		obj1.Parent=it->second;
		vector<ThemeObject*> &v=it->second->ThemeObjects;
		for(unsigned int i=0;i<v.size();i++){
			ThemeObjectInstance p;
			p.Parent=v[i];
			//choose picture
			if(stateID==STATE_LEVEL_EDITOR){
				if(p.Parent->InvisibleAtDesignTime) continue;
				if(p.Parent->EditorPicture.Picture!=NULL) p.Picture=&p.Parent->EditorPicture;
			}else{
				if(p.Parent->InvisibleAtRunTime) continue;
			}
			int m=p.Parent->OptionalPicture.size();
			if(p.Picture==NULL && m>0){
				double f=0.0,f1=1.0/256.0;
				for(int j=0;j<8;j++){
					f+=f1*(double)(rand()&0xff);
					f1*=(1.0/256.0);
				}
				for(int j=0;j<m;j++){
					f-=p.Parent->OptionalPicture[j].first;
					if(f<0.0){
						p.Picture=p.Parent->OptionalPicture[j].second;
						break;
					}
				}
			}
			if(p.Picture==NULL && p.Parent->Picture.Picture!=NULL) p.Picture=&p.Parent->Picture;
			//save
			if(p.Picture!=NULL) obj1.Objects.push_back(p);
		}
	}
	//===
	obj->ChangeState("default"); //???
}