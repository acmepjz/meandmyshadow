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
#include "GameState.h"
#include "Globals.h"
#include "Functions.h"
#include "FileManager.h"
#include "GameObjects.h"
#include "ThemeManager.h"
#include "Objects.h"
#include "Levels.h"
#include "LevelEditor.h"
#include "TreeStorageNode.h"
#include "POASerializer.h"
#include "GUIListBox.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef WIN32
#include <windows.h>
#include <shlobj.h>
#else
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#endif
using namespace std;

////////////////LEVEL PACK EDITOR////////////////////
class LevelPackEditor:public GUIEventCallback{
private:
	string sFileName;
	GUIObject *txtLvPackName;
	GUIListBox *lstLvPack;
	Levels objLvPack;
private:
	void UpdateListBox(){
		lstLvPack->Item.clear();
		for(int i=0;i<objLvPack.getLevelCount();i++){
			char s[32];
			sprintf(s,"%d.",i+1);
			lstLvPack->Item.push_back(s+objLvPack.getLevelName(i)+"("+objLvPack.getLevelFile(i)+")");
		}
	}
	void pAddLevel(const string& s){
		TreeStorageNode obj;
		POASerializer objSerializer;
		if(objSerializer.LoadNodeFromFile(processFileName(s).c_str(),&obj,true)){
			string sName;
			vector<string>& v=obj.attributes["name"];
			if(v.size()>0) sName=v[0];
			objLvPack.addLevel(s,sName,lstLvPack->Value);
			UpdateListBox();
		}
	}
	void pUpdateLevel(int lvl){
		TreeStorageNode obj;
		POASerializer objSerializer;
		if(objSerializer.LoadNodeFromFile(processFileName(objLvPack.getLevelFile(lvl)).c_str(),&obj,true)){
			string sName;
			vector<string>& v=obj.attributes["name"];
			if(v.size()>0) sName=v[0];
			if(!sName.empty()) objLvPack.setLevelName(lvl,sName);
		}
	}
public:
	LevelPackEditor(){}
	void show(){
		GUIObject *obj,*tmp=GUIObjectRoot;
		//===
		GUIObjectRoot=new GUIObject(50,50,700,500,GUIObjectFrame,"Level Pack Editor");
		GUIObjectRoot->ChildControls.push_back(new GUIObject(8,20,184,36,GUIObjectLabel,"Level Pack Name"));
		txtLvPackName=new GUIObject(200,20,492,36,GUIObjectTextBox,"Untitled Level Pack");
		GUIObjectRoot->ChildControls.push_back(txtLvPackName);
		obj=new GUIObject(8,60,192,36,GUIObjectButton,"Add Level");
		obj->Name="cmdAdd";
		obj->EventCallback=this;
		GUIObjectRoot->ChildControls.push_back(obj);
		obj=new GUIObject(208,60,192,36,GUIObjectButton,"Remove Level");
		obj->Name="cmdRemove";
		obj->EventCallback=this;
		GUIObjectRoot->ChildControls.push_back(obj);
		obj=new GUIObject(8,100,192,36,GUIObjectButton,"Move Up");
		obj->Name="cmdMoveUp";
		obj->EventCallback=this;
		GUIObjectRoot->ChildControls.push_back(obj);
		obj=new GUIObject(208,100,192,36,GUIObjectButton,"Move Down");
		obj->Name="cmdMoveDown";
		obj->EventCallback=this;
		GUIObjectRoot->ChildControls.push_back(obj);
		obj=new GUIObject(408,100,240,36,GUIObjectButton,"Update Level Names");
		obj->Name="cmdUpdate";
		obj->EventCallback=this;
		GUIObjectRoot->ChildControls.push_back(obj);
		lstLvPack=new GUIListBox(8,140,684,316);
		lstLvPack->Name="lstLvPack";
		lstLvPack->EventCallback=this;
		GUIObjectRoot->ChildControls.push_back(lstLvPack);
		obj=new GUIObject(8,460,192,36,GUIObjectButton,"Load Level Pack");
		obj->Name="cmdLoad";
		obj->EventCallback=this;
		GUIObjectRoot->ChildControls.push_back(obj);
		obj=new GUIObject(208,460,192,36,GUIObjectButton,"Save Level Pack");
		obj->Name="cmdSave";
		obj->EventCallback=this;
		GUIObjectRoot->ChildControls.push_back(obj);
		obj=new GUIObject(564,460,128,36,GUIObjectButton,"Exit");
		obj->Name="cmdExit";
		obj->EventCallback=this;
		GUIObjectRoot->ChildControls.push_back(obj);
		//===
		SDL_FillRect(screen,NULL,0);
		SDL_SetAlpha(tempSurface, SDL_SRCALPHA, 100);
		SDL_BlitSurface(tempSurface,NULL,screen,NULL);
		while(GUIObjectRoot){
			while(SDL_PollEvent(&event)) GUIObjectHandleEvents();
			if(GUIObjectRoot) GUIObjectRoot->render();
			SDL_Flip(screen);
			SDL_Delay(30);
		}
		GUIObjectRoot=tmp;
		//===
		return;
	}
	void GUIEventCallback_OnEvent(std::string Name,GUIObject* obj,int nEventType){
		if(Name=="cmdExit"){
			if(GUIObjectRoot){
				delete GUIObjectRoot;
				GUIObjectRoot=NULL;
			}
		}else if(Name=="cmdLoad"){
			string s=sFileName;
			if(fileDialog(s,"Load Level Pack","","levelpacks/\nAddon levelpacks\n%DATA%/data/levelpacks/\nMain levelpacks",false,true,false)){
				if(!objLvPack.loadLevels(s+"/levels.lst","")){
					msgBox("Can't load level pack:\n"+s,MsgBoxOKOnly,"Error");
					s="";
				}
				txtLvPackName->Caption=objLvPack.levelpackDescription;
				lstLvPack->Value=-1;
				UpdateListBox();
				sFileName=s;
			}
		}else if(Name=="cmdSave"){
			string s=sFileName;
			if(fileDialog(s,"Save Level Pack","","levelpacks/\nAddon levelpacks\n%DATA%/data/levelpacks/\nMain levelpacks",true,true,false)){
				objLvPack.levelpackDescription=txtLvPackName->Caption;
				createDirectory(processFileName(s).c_str());
				
				objLvPack.saveLevels(s+"/levels.lst");
				sFileName=s+"/levels.lst";
			}
		}else if(Name=="cmdAdd"){
			string s;
			if(fileDialog(s,"Load Level","map","levels/\nAddon levels\n%DATA%/data/levels/\nMain levels",false,true)) pAddLevel(s);
		}else if(Name=="cmdMoveUp"){
			int i=lstLvPack->Value;
			if(i>0&&i<objLvPack.getLevelCount()){
				objLvPack.swapLevel(i,i-1);
				lstLvPack->Value=i-1;
				UpdateListBox();
			}
		}else if(Name=="cmdMoveDown"){
			int i=lstLvPack->Value;
			if(i>=0&&i<objLvPack.getLevelCount()-1){
				objLvPack.swapLevel(i,i+1);
				lstLvPack->Value=i+1;
				UpdateListBox();
			}
		}else if(Name=="cmdRemove"){
			int i=lstLvPack->Value;
			if(i>=0&&i<objLvPack.getLevelCount()){
				objLvPack.removeLevel(i);
				UpdateListBox();
			}
		}else if(Name=="cmdUpdate"){
			for(int i=0;i<objLvPack.getLevelCount();i++) pUpdateLevel(i);
			msgBox("OK!",MsgBoxOKOnly,"");
			UpdateListBox();
		}
	}
};

/////////////////////////////////////////////////////

//warning: weak reference only
static GUIObject *txtWidth,*txtHeight,*txtName,*txtTheme;

struct typeObjectPropItem{
	string sKey;
	GUIObject *objTextBox,*objLabel;
};

static vector<typeObjectPropItem> ObjectPropItemCollection;
static GameObject *ObjectPropOwner;
static int ObjectPropPage,ObjectPropPageMax;
//over

static bool snapToGrid=true;

//clipboard
static map<string,string> m_objClipboard;

static void pShowPropPage(int nPage){
	unsigned int k=(unsigned int)(nPage*10);
	for(unsigned int i=0;i<ObjectPropItemCollection.size();i++){
		ObjectPropItemCollection[i].objLabel->Visible=(i>=k&&i<k+10);
		ObjectPropItemCollection[i].objTextBox->Visible=(i>=k&&i<k+10);
	}
}

LevelEditor::LevelEditor():Game(false){
	LEVEL_WIDTH = 800;
	LEVEL_HEIGHT = 600;
	
	loadLevel(getDataPath()+"misc/Empty.map");
	currentType=TYPE_BLOCK;

	m_objClipboard.clear();
}

LevelEditor::~LevelEditor(){
	for(unsigned int i=0;i<levelObjects.size();i++) delete levelObjects[i];
	levelObjects.clear();
}

void LevelEditor::putObject(){
	int x, y;

	SDL_GetMouseState(&x, &y);
	x+=camera.x;
	y+=camera.y;

	if(snapToGrid){
		x=(x/50)*50;
		y=(y/50)*50;
	}else{
		x-=25;
		y-=25;
	}

	levelObjects.push_back( new Block(x, y, currentType, this));


	if(m_objClipboard.size()>0){
		levelObjects.back()->setEditorData(m_objClipboard);
	}
}

void LevelEditor::deleteObject(){
	int x, y;

	SDL_GetMouseState(&x, &y);
	SDL_Rect mouse; mouse.x = x + camera.x; mouse.y = y + camera.y; mouse.w = 1; mouse.h = 1;

	for(unsigned int o=0; o<levelObjects.size(); o++){
		if(checkCollision(levelObjects[o]->getBox(),mouse)==true){
			delete levelObjects[o];
			levelObjects.erase(levelObjects.begin()+o);
		}
	}
}

void LevelEditor::copyObject(bool bDelete){
	int x, y;

	SDL_GetMouseState(&x, &y);
	SDL_Rect mouse; mouse.x = x + camera.x; mouse.y = y + camera.y; mouse.w = 1; mouse.h = 1;

	for(unsigned int o=0; o<levelObjects.size(); o++){
		if(checkCollision(levelObjects[o]->getBox(),mouse)==true){
			vector<pair<string,string> > obj;
			levelObjects[o]->getEditorData(obj);
			m_objClipboard.clear();
			for(unsigned int i=0;i<obj.size();i++){
				m_objClipboard[obj[i].first]=obj[i].second;
			}
			currentType=levelObjects[o]->type;
			if(bDelete){
				delete levelObjects[o];
				levelObjects.erase(levelObjects.begin()+o);
			}
			break;
		}
	}
}

void LevelEditor::editObject(){
	int x, y;

	SDL_GetMouseState(&x, &y);
	SDL_Rect mouse; mouse.x = x + camera.x; mouse.y = y + camera.y; mouse.w = 1; mouse.h = 1;

	for(unsigned int o=0; o<levelObjects.size(); o++){
		if(checkCollision(levelObjects[o]->getBox(),mouse)==true){
			vector<pair<string,string> > objMap;
			levelObjects[o]->getEditorData(objMap);
			int m=objMap.size();
			if(m>0){
				ObjectPropOwner=levelObjects[o];
				ObjectPropPage=0;
				ObjectPropPageMax=(m+9)/10;
				ObjectPropItemCollection.clear();
				//========
				if(GUIObjectRoot){
					delete GUIObjectRoot;
					GUIObjectRoot=NULL;
				}
				int i=m>10?10:m;
				int nHeight=i*40+100;
				{
					string s;
					int nType=levelObjects[o]->type;
					if(nType>=0&&nType<TYPE_MAX) s=g_sBlockName[nType];
					else s="Object";
					s+=" Properties";
					GUIObjectRoot=new GUIObject(100,(SCREEN_HEIGHT-nHeight)/2,600,nHeight,GUIObjectFrame,s.c_str());
				}
				for(i=0;i<m;i++){
					typeObjectPropItem t;
					int y=(m>10?54:20)+(i%10)*40;
					t.sKey=objMap[i].first;
					t.objLabel=new GUIObject(8,y,240,36,GUIObjectLabel,t.sKey.c_str());
					t.objTextBox=new GUIObject(240,y,352,36,GUIObjectTextBox,objMap[i].second.c_str());
					GUIObjectRoot->ChildControls.push_back(t.objLabel);
					GUIObjectRoot->ChildControls.push_back(t.objTextBox);
					ObjectPropItemCollection.push_back(t);
				}
				//========
				GUIObject *obj;
				if(m>10){
					obj=new GUIObject(8,20,72,32,GUIObjectButton,"<<");
					obj->Name="cmdObjPropPrev";
					obj->EventCallback=this;
					GUIObjectRoot->ChildControls.push_back(obj);
					obj=new GUIObject(520,20,72,32,GUIObjectButton,">>");
					obj->Name="cmdObjPropNext";
					obj->EventCallback=this;
					GUIObjectRoot->ChildControls.push_back(obj);
					pShowPropPage(0);
				}
				obj=new GUIObject(100,nHeight-44,150,36,GUIObjectButton,"OK");
				obj->Name="cmdObjPropOK";
				obj->EventCallback=this;
				GUIObjectRoot->ChildControls.push_back(obj);
				obj=new GUIObject(350,nHeight-44,150,36,GUIObjectButton,"Cancel");
				obj->Name="cmdCancel";
				obj->EventCallback=this;
				GUIObjectRoot->ChildControls.push_back(obj);
				//========
				//Draw screen to the tempSurface once.
				SDL_BlitSurface(screen,NULL,tempSurface,NULL);
				SDL_FillRect(screen,NULL,0);
				SDL_SetAlpha(tempSurface,SDL_SRCALPHA, 100);
				SDL_BlitSurface(tempSurface,NULL,screen,NULL);
				while(GUIObjectRoot){
					while(SDL_PollEvent(&event)) GUIObjectHandleEvents();
					if(GUIObjectRoot) GUIObjectRoot->render();
					SDL_Flip(screen);
					SDL_Delay(30);
				}
				//========
				ObjectPropOwner=NULL;
				ObjectPropItemCollection.clear();
				//========
				return;
			}
		}
	}
}

void LevelEditor::saveLevel(string fileName){
	std::ofstream save(fileName.c_str());
	if(!save) return;

	int maxX = 0;
	int maxY = 0;

	TreeStorageNode node;
	char s[64];

	maxX = LEVEL_WIDTH;
	sprintf(s,"%d",maxX);
	node.attributes["size"].push_back(s);

	maxY = LEVEL_HEIGHT;
	sprintf(s,"%d",maxY);
	node.attributes["size"].push_back(s);

	//save additional data
	for(map<string,string>::iterator i=EditorData.begin();i!=EditorData.end();i++){
		if((!i->first.empty()) && (!i->second.empty())){
			node.attributes[i->first].push_back(i->second);
		}
	}

	for ( int o = 0; o < (signed)levelObjects.size(); o++ )
	{
		int objectType = levelObjects[o]->type;

		if(objectType>=0 && objectType<TYPE_MAX){
			TreeStorageNode* obj1=new TreeStorageNode;
			node.subNodes.push_back(obj1);

			obj1->name="tile";

			sprintf(s,"%d",objectType);
			obj1->value.push_back(g_sBlockName[objectType]);

			SDL_Rect box = levelObjects[o]->getBox(BoxType_Base);

			sprintf(s,"%d",box.x);
			obj1->value.push_back(s);
			sprintf(s,"%d",box.y);
			obj1->value.push_back(s);

			vector<pair<string,string> > obj;
			levelObjects[o]->getEditorData(obj);
			for(unsigned int i=0;i<obj.size();i++){
				if((!obj[i].first.empty()) && (!obj[i].second.empty())){
					obj1->attributes[obj[i].first].push_back(obj[i].second);
				}
			}
		}
	}

	POASerializer objSerializer;
	objSerializer.WriteNode(&node,save,true,true);

	LevelName=fileName;
}

///////////////EVENT///////////////////
void LevelEditor::handleEvents()
{
	if(event.type==SDL_KEYUP && event.key.keysym.sym==SDLK_ESCAPE){
		event.type = 0;
		//---show menu
		if(GUIObjectRoot){
			delete GUIObjectRoot;
			GUIObjectRoot=NULL;
		}
		GUIObject *obj,*obj1;
		GUIObjectRoot=new GUIObject(0,0,SCREEN_WIDTH,SCREEN_HEIGHT);
		//level properties
		obj=new GUIObject(8,32,400,400,GUIObjectFrame,"Level Properties");
		GUIObjectRoot->ChildControls.push_back(obj);
		{
			char s[32];
			//
			obj1=new GUIObject(16,350,184,42,GUIObjectButton,"OK");
			obj1->Name="cmdOK";
			obj1->EventCallback=this;
			obj->ChildControls.push_back(obj1);
			obj1=new GUIObject(208,350,184,42,GUIObjectButton,"Cancel");
			obj1->Name="cmdCancel";
			obj1->EventCallback=this;
			obj->ChildControls.push_back(obj1);
			//
			obj1=new GUIObject(8,20,184,36,GUIObjectLabel,"Level Size");
			obj->ChildControls.push_back(obj1);
			obj1=new GUIObject(248,20,32,36,GUIObjectLabel,"X");
			obj->ChildControls.push_back(obj1);
			sprintf(s,"%d",LEVEL_WIDTH);
			txtWidth=new GUIObject(128,20,112,36,GUIObjectTextBox,s);
			obj->ChildControls.push_back(txtWidth);
			sprintf(s,"%d",LEVEL_HEIGHT);
			txtHeight=new GUIObject(280,20,112,36,GUIObjectTextBox,s);
			obj->ChildControls.push_back(txtHeight);
			//
			obj1=new GUIObject(8,60,184,36,GUIObjectLabel,"Level Name");
			obj->ChildControls.push_back(obj1);
			txtName=new GUIObject(128,60,264,36,GUIObjectTextBox,EditorData["name"].c_str());
			obj->ChildControls.push_back(txtName);
			//
			obj1=new GUIObject(8,100,184,36,GUIObjectLabel,"Theme File");
			obj->ChildControls.push_back(obj1);
			txtTheme=new GUIObject(128,100,264,36,GUIObjectTextBox,EditorData["theme"].c_str());
			obj->ChildControls.push_back(txtTheme);
		}
		//menu
		obj=new GUIObject(492,8,300,500,GUIObjectFrame);
		GUIObjectRoot->ChildControls.push_back(obj);
		{
			obj1=new GUIObject(8,8,284,36,GUIObjectButton,"Load Level (Ctrl+O)");
			obj1->Name="cmdLoad";
			obj1->EventCallback=this;
			obj->ChildControls.push_back(obj1);
			obj1=new GUIObject(8,48,284,36,GUIObjectButton,"Save Level (Ctrl+S)");
			obj1->Name="cmdSave";
			obj1->EventCallback=this;
			obj->ChildControls.push_back(obj1);
			obj1=new GUIObject(8,88,284,36,GUIObjectButton,"Level Pack Editor");
			obj1->Name="cmdLvPack";
			obj1->EventCallback=this;
			obj->ChildControls.push_back(obj1);
			obj1=new GUIObject(8,128,284,36,GUIObjectButton,"Exit Editor");
			obj1->Name="cmdExit";
			obj1->EventCallback=this;
			obj->ChildControls.push_back(obj1);
			//
			obj->ChildControls.push_back(new GUIObject(8,300,284,25,GUIObjectLabel,"New Level (Ctrl+N)"));
			obj1=new GUIObject(8,325,284,25,GUIObjectCheckBox,"Snap to grid (Ctrl+G)",snapToGrid?1:0);
			obj1->Name="chkSnapToGrid";
			obj1->EventCallback=this;
			obj->ChildControls.push_back(obj1);
			obj->ChildControls.push_back(new GUIObject(8,350,284,25,GUIObjectLabel,"Cut (Ctrl+X)"));
			obj->ChildControls.push_back(new GUIObject(8,375,284,25,GUIObjectLabel,"Copy (Ctrl+C)"));
			obj->ChildControls.push_back(new GUIObject(8,400,284,25,GUIObjectLabel,"Edit current block (Enter)"));
		}
		//---
		//Draw screen to the tempSurface once.
		SDL_BlitSurface(screen,NULL,tempSurface,NULL);
		SDL_FillRect(screen,NULL,0);
		SDL_SetAlpha(tempSurface, SDL_SRCALPHA, 100);
		SDL_BlitSurface(tempSurface,NULL,screen,NULL);
		while(GUIObjectRoot){
			while(SDL_PollEvent(&event)) GUIObjectHandleEvents();
			if(GUIObjectRoot) GUIObjectRoot->render();
			SDL_Flip(screen);
			SDL_Delay(30);
		}
		//---
		return;
	}
	
	//Let the game handle events.
	Game::handleEvents();
	if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button==SDL_BUTTON_LEFT){
		putObject();
		return;
	}else if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button==SDL_BUTTON_WHEELDOWN)	{
		m_objClipboard.clear();
		currentType++;
		if(currentType>=TYPE_MAX){
			currentType = 0;
		}
		return;
	}else if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button==SDL_BUTTON_WHEELUP){
		m_objClipboard.clear();
		currentType--;
		if(currentType<0){
			currentType = TYPE_MAX - 1;
		}
		return;
	}

	if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button==SDL_BUTTON_RIGHT){
		deleteObject();
		return;
	}

	if(event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_x && (event.key.keysym.mod & KMOD_CTRL)){
		copyObject(true);
		return;
	}
	if(event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_c && (event.key.keysym.mod & KMOD_CTRL)){
		copyObject(false);
		return;
	}
	if(event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_g && (event.key.keysym.mod & KMOD_CTRL)){
		snapToGrid=!snapToGrid;
		return;
	}
	if(event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_n && (event.key.keysym.mod & KMOD_CTRL)){
		destroy();
		return;
	}
	if(event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_o && (event.key.keysym.mod & KMOD_CTRL)){
		string s=LevelName;
		if(fileDialog(s,"Load Level","map","%USER%/levels/\nAddon levels\n%DATA%/levels/\nMain levels",false,true)){
			loadLevel(processFileName(s));
		}
		return;
	}
	if(event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_s && (event.key.keysym.mod & KMOD_CTRL)){
		string s=LevelName;
		if(fileDialog(s,"Save Level","map","%USER%/levels/\nAddon levels\n%DATA%/levels/\nMain levels",true,true)){
			saveLevel(processFileName(s));
		}
		return;
	}
	if(event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_RETURN){
		editObject();
		return;
	}
}

void LevelEditor::GUIEventCallback_OnEvent(std::string name,GUIObject* obj,int eventType){
	if(eventType==GUIEventClick){
		if(name=="cmdExit"){
			if(GUIObjectRoot){
				delete GUIObjectRoot;
				GUIObjectRoot=NULL;
			}
			setNextState(STATE_MENU);
		}else if(name=="cmdOK"){
			int i;
			//Apply changes
			i=atoi(txtWidth->Caption.c_str());
			if(i>0&&i<=30000) LEVEL_WIDTH=i;
			i=atoi(txtHeight->Caption.c_str());
			if(i>0&&i<=30000) LEVEL_HEIGHT=i;
			EditorData["name"]=txtName->Caption;
			{
				string &s=EditorData["theme"];
				if(s!=txtTheme->Caption){
					s=txtTheme->Caption;
					msgBox("You need to reload level to apply theme changes.",MsgBoxOKOnly,"Note");
				}
			}
			//
			if(GUIObjectRoot){
				delete GUIObjectRoot;
				GUIObjectRoot=NULL;
			}
		}else if(name=="cmdObjPropOK"){
			if(GUIObjectRoot){
				//---
				map<string,string> objMap;
				for(unsigned int i=0;i<ObjectPropItemCollection.size();i++){
					objMap[ObjectPropItemCollection[i].sKey]=ObjectPropItemCollection[i].objTextBox->Caption;
				}
				ObjectPropOwner->setEditorData(objMap);
				//---
				delete GUIObjectRoot;
				GUIObjectRoot=NULL;
			}
		}else if(name=="cmdCancel"){
			if(GUIObjectRoot){
				delete GUIObjectRoot;
				GUIObjectRoot=NULL;
			}
		}else if(name=="cmdLoad"){
			string s=LevelName;
			if(fileDialog(s,"Load Level","map","%USER%/levels/\nAddon levels\n%DATA%/levels/\nMain levels",false,true)){
				if(GUIObjectRoot){
					delete GUIObjectRoot;
					GUIObjectRoot=NULL;
				}
				
				loadLevel(processFileName(s));
			}
			//We render once to prevent any GUI to appear in the background.
			this->render();
			SDL_FillRect(screen,NULL,0);
			SDL_SetAlpha(tempSurface, SDL_SRCALPHA, 100);
			SDL_BlitSurface(tempSurface,NULL,screen,NULL);
		}else if(name=="cmdSave"){
			string s=LevelName;
			if(fileDialog(s,"Save Level","map","%USER%/levels/\nAddon levels\n%DATA%/levels/\nMain levels",true,true)){
				saveLevel(processFileName(s));
			}
			
			//We render once to prevent any GUI to appear in the background.
			this->render();
			SDL_FillRect(screen,NULL,0);
			SDL_SetAlpha(tempSurface, SDL_SRCALPHA, 100);
			SDL_BlitSurface(tempSurface,NULL,screen,NULL);
		}else if(name=="cmdLvPack"){
			LevelPackEditor objEditor;
			objEditor.show();
			
			//We render once to prevent any GUI to appear in the background.
			this->render();
			SDL_FillRect(screen,NULL,0);
			SDL_SetAlpha(tempSurface, SDL_SRCALPHA, 100);
			SDL_BlitSurface(tempSurface,NULL,screen,NULL);
		}else if(name=="cmdObjPropPrev"){
			if(ObjectPropPage>0) pShowPropPage(--ObjectPropPage);
		}else if(name=="cmdObjPropNext"){
			if(ObjectPropPage<ObjectPropPageMax-1) pShowPropPage(++ObjectPropPage);
		}else if(name=="chkSnapToGrid"){
			snapToGrid=obj->Value?true:false;
		}
	}
}

////////////////LOGIC////////////////////
void LevelEditor::logic(){
	Game::logic();
	setCamera();
}

void LevelEditor::showCurrentObject(){
	//Get the x and y location of the mouse.
	int x, y;
	SDL_GetMouseState(&x, &y);
	
	//Convert the location to the location in the level.
	x+=camera.x;
	y+=camera.y;

	//Check if we should snap the block to grid or not.
	if(snapToGrid){
		x=(x/50)*50;
		y=(y/50)*50;
	}else{
		x-=25;
		y-=25;
	}

	//Check if the currentType is a legal type.
	if(currentType>=0 && currentType<TYPE_MAX){
		ThemeBlock* obj=objThemes.getBlock(currentType);
		if(obj){
			obj->editorPicture.draw(screen, x - camera.x, y - camera.y);
		}
	}
}

/////////////////RENDER//////////////////////
void LevelEditor::render(){
	Game::render();
	showCurrentObject();
}
