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
#include "Classes.h"
#include "Globals.h"
#include "Functions.h"
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
using namespace std;

////////////////LEVEL PACK EDITOR////////////////////
class LevelPackEditor:public GUIEventCallback{
private:
	string sFileName;
	GUIObject *txtLvPackName;
	GUIListBox *lstLvPack;
	Level objLvPack;
private:
	void UpdateListBox(){
		lstLvPack->Item.clear();
		for(int i=0;i<objLvPack.get_level_count();i++){
			char s[32];
			sprintf(s,"%d.",i+1);
			lstLvPack->Item.push_back(s+objLvPack.get_level_name(i)+"("+objLvPack.get_level_file(i)+")");
		}
	}
	void pAddLevel(const string& s){
		TreeStorageNode obj;
		POASerializer objSerializer;
		if(objSerializer.LoadNodeFromFile(ProcessFileName(s).c_str(),&obj,true)){
			string sName;
			vector<string>& v=obj.Attributes["name"];
			if(v.size()>0) sName=v[0];
			objLvPack.add_level(s,sName,lstLvPack->Value);
			UpdateListBox();
		}
	}
	void pUpdateLevel(int lvl){
		TreeStorageNode obj;
		POASerializer objSerializer;
		if(objSerializer.LoadNodeFromFile(ProcessFileName(objLvPack.get_level_file(lvl)).c_str(),&obj,true)){
			string sName;
			vector<string>& v=obj.Attributes["name"];
			if(v.size()>0) sName=v[0];
			if(!sName.empty()) objLvPack.set_level_name(lvl,sName);
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
		SDL_SetAlpha(s_temp, SDL_SRCALPHA, 100);
		SDL_BlitSurface(s_temp,NULL,screen,NULL);
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
			if(FileDialog(s,"Load Level Pack","lst","\nCustom level pack\n%DATA%/data/level/\nMain level pack",false,true)){
				if(!objLvPack.load_levels(s,"")){
					MsgBox("Can't load level pack:\n"+s,MsgBoxOKOnly,"Error");
					s="";
				}
				txtLvPackName->Caption=objLvPack.LevelPackName;
				lstLvPack->Value=-1;
				UpdateListBox();
				sFileName=s;
			}
		}else if(Name=="cmdSave"){
			string s=sFileName;
			if(FileDialog(s,"Save Level Pack","lst","\nCustom level pack\n%DATA%/data/level/\nMain level pack",true,true)){
				objLvPack.LevelPackName=txtLvPackName->Caption;
				objLvPack.save_levels(s);
				sFileName=s;
			}
		}else if(Name=="cmdAdd"){
			string s;
			if(FileDialog(s,"Load Level","map","\nCustom level\n%DATA%/data/level/\nMain level",false,true)) pAddLevel(s);
		}else if(Name=="cmdMoveUp"){
			int i=lstLvPack->Value;
			if(i>0&&i<objLvPack.get_level_count()){
				objLvPack.swap_level(i,i-1);
				lstLvPack->Value=i-1;
				UpdateListBox();
			}
		}else if(Name=="cmdMoveDown"){
			int i=lstLvPack->Value;
			if(i>=0&&i<objLvPack.get_level_count()-1){
				objLvPack.swap_level(i,i+1);
				lstLvPack->Value=i+1;
				UpdateListBox();
			}
		}else if(Name=="cmdRemove"){
			int i=lstLvPack->Value;
			if(i>=0&&i<objLvPack.get_level_count()){
				objLvPack.remove_level(i);
				UpdateListBox();
			}
		}else if(Name=="cmdUpdate"){
			for(int i=0;i<objLvPack.get_level_count();i++) pUpdateLevel(i);
			MsgBox("OK!",MsgBoxOKOnly,"");
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

static bool m_bSnapToGrid=true;

//clipboard
static map<string,string> m_objClipboard;

static void pShowPropPage(int nPage){
	unsigned int k=(unsigned int)(nPage*10);
	for(unsigned int i=0;i<ObjectPropItemCollection.size();i++){
		ObjectPropItemCollection[i].objLabel->Visible=(i>=k&&i<k+10);
		ObjectPropItemCollection[i].objTextBox->Visible=(i>=k&&i<k+10);
	}
}

LevelEditor::LevelEditor(const char *lpsLevelName):Game(false)
{
	LEVEL_WIDTH = 800;
	LEVEL_HEIGHT = 600;
	
	if(lpsLevelName!=NULL && *lpsLevelName) load_level(lpsLevelName);

	i_current_type = TYPE_BLOCK;

	m_objClipboard.clear();
}

LevelEditor::~LevelEditor()
{
	for(unsigned int i=0;i<levelObjects.size();i++) delete levelObjects[i];
	levelObjects.clear();
}

void LevelEditor::put_object()
{
	int x, y;

	SDL_GetMouseState(&x, &y);
	x+=camera.x;
	y+=camera.y;

	if(m_bSnapToGrid){
		x=(x/50)*50;
		y=(y/50)*50;
	}else{
		x-=25;
		y-=25;
	}

	/*switch ( i_current_type )
	{
	default:
		{*/
			levelObjects.push_back( new Block(x, y, i_current_type, this));
			/*break;
		}

	case TYPE_START_PLAYER:
		{
			levelObjects.push_back( new StartObject( x, y, &o_player, this) );
			break;
		}

	case TYPE_START_SHADOW:
		{
			levelObjects.push_back( new StartObjectShadow( x, y, &o_shadow, this) );
			break;
		}
	}*/

	if(m_objClipboard.size()>0){
		levelObjects.back()->SetEditorData(m_objClipboard);
	}
}

void LevelEditor::delete_object()
{
	int x, y;

	SDL_GetMouseState(&x, &y);

	SDL_Rect mouse; mouse.x = x + camera.x; mouse.y = y + camera.y; mouse.w = 1; mouse.h = 1;

	for ( unsigned int o = 0; o < levelObjects.size(); o++ )
	{
		if ( check_collision( levelObjects[o]->get_box(), mouse ) == true )
		{
			delete levelObjects[o];
			levelObjects.erase(levelObjects.begin()+o);
		}
	}
}

void LevelEditor::copy_object(bool bDelete)
{
	int x, y;

	SDL_GetMouseState(&x, &y);

	SDL_Rect mouse; mouse.x = x + camera.x; mouse.y = y + camera.y; mouse.w = 1; mouse.h = 1;

	for ( unsigned int o = 0; o < levelObjects.size(); o++ )
	{
		if ( check_collision( levelObjects[o]->get_box(), mouse ) == true )
		{
			vector<pair<string,string> > obj;
			levelObjects[o]->GetEditorData(obj);
			m_objClipboard.clear();
			for(unsigned int i=0;i<obj.size();i++){
				m_objClipboard[obj[i].first]=obj[i].second;
			}
			i_current_type=levelObjects[o]->i_type;
			if(bDelete){
				delete levelObjects[o];
				levelObjects.erase(levelObjects.begin()+o);
			}
			break;
		}
	}
}

void LevelEditor::edit_object()
{
	int x, y;

	SDL_GetMouseState(&x, &y);

	SDL_Rect mouse; mouse.x = x + camera.x; mouse.y = y + camera.y; mouse.w = 1; mouse.h = 1;

	for ( unsigned int o = 0; o < levelObjects.size(); o++ )
	{
		if ( check_collision( levelObjects[o]->get_box(), mouse ) == true )
		{
			vector<pair<string,string> > objMap;
			levelObjects[o]->GetEditorData(objMap);
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
					int nType=levelObjects[o]->i_type;
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
				SDL_FillRect(screen,NULL,0);
				SDL_SetAlpha(s_temp, SDL_SRCALPHA, 100);
				SDL_BlitSurface(s_temp,NULL,screen,NULL);
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

void LevelEditor::save_level(string FileName)
{
	std::ofstream save( ProcessFileName(FileName).c_str() );
	if(!save) return;

	int maxX = 0;
	int maxY = 0;

	/*for ( int o = 0; o < (signed)levelObjects.size(); o++ )
	{
		SDL_Rect r=levelObjects[o]->get_box(BoxType_Base);
		int x=r.x+50;
		int y=r.y+50;
		if ( x > maxX )
		{
			maxX = x;
		}

		if ( y > maxY )
		{
			maxY = y;
		}
	}*/

	TreeStorageNode node;
	char s[64];

	/*if ( maxX < LEVEL_WIDTH )*/ maxX = LEVEL_WIDTH;
	sprintf(s,"%d",maxX);
	node.Attributes["size"].push_back(s);

	/*if ( maxY < LEVEL_HEIGHT )*/ maxY = LEVEL_HEIGHT;
	sprintf(s,"%d",maxY);
	node.Attributes["size"].push_back(s);

	//save additional data
	for(map<string,string>::iterator i=EditorData.begin();i!=EditorData.end();i++){
		if((!i->first.empty()) && (!i->second.empty())){
			node.Attributes[i->first].push_back(i->second);
		}
	}

	for ( int o = 0; o < (signed)levelObjects.size(); o++ )
	{
		int objectType = levelObjects[o]->i_type;

		if(objectType>=0 && objectType<TYPE_MAX){
			TreeStorageNode* obj1=new TreeStorageNode;
			node.SubNodes.push_back(obj1);

			obj1->Name="tile";

			sprintf(s,"%d",objectType);
			obj1->Value.push_back(g_sBlockName[objectType]);

			SDL_Rect box = levelObjects[o]->get_box(BoxType_Base);

			sprintf(s,"%d",box.x);
			obj1->Value.push_back(s);
			sprintf(s,"%d",box.y);
			obj1->Value.push_back(s);

			vector<pair<string,string> > obj;
			levelObjects[o]->GetEditorData(obj);
			for(unsigned int i=0;i<obj.size();i++){
				if((!obj[i].first.empty()) && (!obj[i].second.empty())){
					obj1->Attributes[obj[i].first].push_back(obj[i].second);
				}
			}
		}
	}

	POASerializer objSerializer;
	objSerializer.WriteNode(&node,save,true,true);

	LevelName=FileName;
}

///////////////EVENT///////////////////
void LevelEditor::handle_events()
{
	if ( event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_ESCAPE )
	{
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
			obj1=new GUIObject(8,325,284,25,GUIObjectCheckBox,"Snap to grid (Ctrl+G)",m_bSnapToGrid?1:0);
			obj1->Name="chkSnapToGrid";
			obj1->EventCallback=this;
			obj->ChildControls.push_back(obj1);
			obj->ChildControls.push_back(new GUIObject(8,350,284,25,GUIObjectLabel,"Cut (Ctrl+X)"));
			obj->ChildControls.push_back(new GUIObject(8,375,284,25,GUIObjectLabel,"Copy (Ctrl+C)"));
			obj->ChildControls.push_back(new GUIObject(8,400,284,25,GUIObjectLabel,"Edit current block (Enter)"));
		}
		//---
		SDL_FillRect(screen,NULL,0);
		SDL_SetAlpha(s_temp, SDL_SRCALPHA, 100);
		SDL_BlitSurface(s_temp,NULL,screen,NULL);
		while(GUIObjectRoot){
			while(SDL_PollEvent(&event)) GUIObjectHandleEvents();
			if(GUIObjectRoot) GUIObjectRoot->render();
			SDL_Flip(screen);
			SDL_Delay(30);
		}
		//---
		return;
	}
	Game::handle_events();
	if ( event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT )
	{
		put_object();
		return;
	}

	else if ( event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_WHEELDOWN )
	{
		m_objClipboard.clear();
		i_current_type++;
		if ( i_current_type >= TYPE_MAX )
		{
			i_current_type = 0;
		}
		return;
	}

	else if ( event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_WHEELUP )
	{
		m_objClipboard.clear();
		i_current_type--;
		if ( i_current_type < 0 )
		{
			i_current_type = TYPE_MAX - 1;
		}
		return;
	}

	if ( event.type  == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_RIGHT )
	{
		delete_object();
		return;
	}

	if ( event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_x && (event.key.keysym.mod & KMOD_CTRL) )
	{
		copy_object(true);
		return;
	}
	if ( event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_c && (event.key.keysym.mod & KMOD_CTRL) )
	{
		copy_object(false);
		return;
	}
	if ( event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_g && (event.key.keysym.mod & KMOD_CTRL) )
	{
		m_bSnapToGrid=!m_bSnapToGrid;
		return;
	}
	if ( event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_n && (event.key.keysym.mod & KMOD_CTRL) )
	{
		Destroy();
		return;
	}
	if ( event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_o && (event.key.keysym.mod & KMOD_CTRL))
	{
		string s=LevelName;
		if(FileDialog(s,"Load Level","map","\nCustom level\n%DATA%/data/level/\nMain level",false,true)){
			load_level(s);
		}
		return;
	}
	if ( event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_s && (event.key.keysym.mod & KMOD_CTRL))
	{
		string s=LevelName;
		if(FileDialog(s,"Save Level","map","\nCustom level\n%DATA%/data/level/\nMain level",true,true)){
			save_level(s);
		}
		return;
	}
	if ( event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_RETURN )
	{
		edit_object();
		return;
	}
}

void LevelEditor::GUIEventCallback_OnEvent(std::string Name,GUIObject* obj,int nEventType){
	if(nEventType==GUIEventClick){
		if(Name=="cmdExit"){
			if(GUIObjectRoot){
				delete GUIObjectRoot;
				GUIObjectRoot=NULL;
			}
			next_state(STATE_MENU);
		}else if(Name=="cmdOK"){
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
					MsgBox("You need to reload level to apply theme changes.",MsgBoxOKOnly,"Note");
				}
			}
			//
			if(GUIObjectRoot){
				delete GUIObjectRoot;
				GUIObjectRoot=NULL;
			}
		}else if(Name=="cmdObjPropOK"){
			if(GUIObjectRoot){
				//---
				map<string,string> objMap;
				for(unsigned int i=0;i<ObjectPropItemCollection.size();i++){
					objMap[ObjectPropItemCollection[i].sKey]=ObjectPropItemCollection[i].objTextBox->Caption;
				}
				ObjectPropOwner->SetEditorData(objMap);
				//---
				delete GUIObjectRoot;
				GUIObjectRoot=NULL;
			}
		}else if(Name=="cmdCancel"){
			if(GUIObjectRoot){
				delete GUIObjectRoot;
				GUIObjectRoot=NULL;
			}
		}else if(Name=="cmdLoad"){
			string s=LevelName;
			if(FileDialog(s,"Load Level","map","\nCustom level\n%DATA%/data/level/\nMain level",false,true)){
				if(GUIObjectRoot){
					delete GUIObjectRoot;
					GUIObjectRoot=NULL;
				}
				
				load_level(s);
			}
			//We render once to prevent any GUI to appear in the background.
			this->render();
			SDL_FillRect(screen,NULL,0);
			SDL_SetAlpha(s_temp, SDL_SRCALPHA, 100);
			SDL_BlitSurface(s_temp,NULL,screen,NULL);
		}else if(Name=="cmdSave"){
			string s=LevelName;
			if(FileDialog(s,"Save Level","map","\nCustom level\n%DATA%/data/level/\nMain level",true,true)){
				save_level(s);
			}
			
			//We render once to prevent any GUI to appear in the background.
			this->render();
			SDL_FillRect(screen,NULL,0);
			SDL_SetAlpha(s_temp, SDL_SRCALPHA, 100);
			SDL_BlitSurface(s_temp,NULL,screen,NULL);
		}else if(Name=="cmdLvPack"){
			LevelPackEditor objEditor;
			objEditor.show();
			
			//We render once to prevent any GUI to appear in the background.
			this->render();
			SDL_FillRect(screen,NULL,0);
			SDL_SetAlpha(s_temp, SDL_SRCALPHA, 100);
			SDL_BlitSurface(s_temp,NULL,screen,NULL);
		}else if(Name=="cmdObjPropPrev"){
			if(ObjectPropPage>0) pShowPropPage(--ObjectPropPage);
		}else if(Name=="cmdObjPropNext"){
			if(ObjectPropPage<ObjectPropPageMax-1) pShowPropPage(++ObjectPropPage);
		}else if(Name=="chkSnapToGrid"){
			m_bSnapToGrid=obj->Value?true:false;
		}
	}
}

////////////////LOGIC////////////////////
void LevelEditor::logic()
{
	Game::logic();

	set_camera();
}

void LevelEditor::show_current_object()
{
	int x, y;

	SDL_GetMouseState(&x, &y);
	x+=camera.x;
	y+=camera.y;

	if(m_bSnapToGrid){
		x=(x/50)*50;
		y=(y/50)*50;
	}else{
		x-=25;
		y-=25;
	}

	if(i_current_type>=0 && i_current_type<TYPE_MAX){
		ThemeBlock *obj=m_objThemes.GetBlock(i_current_type);
		if(obj){
			obj->EditorPicture.Draw(screen, x - camera.x, y - camera.y);
		}
	}
	//////////////////////////

}

/////////////////RENDER//////////////////////
void LevelEditor::render()
{
	/*apply_surface( 0, 0, background, screen, NULL );

	for ( unsigned int o = 0; o < levelObjects.size(); o++ )
	{
		levelObjects[o]->show();
	}*/

	Game::render();
	show_current_object();

	/*o_shadow.show();
	o_player.show();*/
}
