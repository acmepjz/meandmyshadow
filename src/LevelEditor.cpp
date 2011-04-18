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
#include "Objects.h"
#include "LevelEditor.h"
#include "TreeStorageNode.h"
#include "POASerializer.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
using namespace std;

//warning: weak reference only
static GUIObject *txtWidth,*txtHeight,*txtName;

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

static void pShowOpen(GUIEventCallback* _this,std::string& LevelName){
	GUIObject* obj;
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}
	GUIObjectRoot=new GUIObject(100,200,600,200,GUIObjectFrame,"Load Level");
	GUIObjectRoot->ChildControls.push_back(new GUIObject(8,20,184,42,GUIObjectLabel,"File Name"));
	txtName=new GUIObject(160,20,432,42,GUIObjectTextBox,LevelName.c_str());
	GUIObjectRoot->ChildControls.push_back(txtName);
	obj=new GUIObject(200,70,192,42,GUIObjectButton,"OK");
	obj->Name="cmdLoadOK";
	obj->EventCallback=_this;
	GUIObjectRoot->ChildControls.push_back(obj);
	obj=new GUIObject(400,70,192,42,GUIObjectButton,"Cancel");
	obj->Name="cmdCancel";
	obj->EventCallback=_this;
	GUIObjectRoot->ChildControls.push_back(obj);
}

static void pShowSave(GUIEventCallback* _this,std::string& LevelName){
	GUIObject* obj;
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}
	GUIObjectRoot=new GUIObject(100,200,600,200,GUIObjectFrame,"Save Level");
	GUIObjectRoot->ChildControls.push_back(new GUIObject(8,20,184,42,GUIObjectLabel,"File Name"));
	txtName=new GUIObject(160,20,432,42,GUIObjectTextBox,LevelName.c_str());
	GUIObjectRoot->ChildControls.push_back(txtName);
	obj=new GUIObject(200,70,192,42,GUIObjectButton,"OK");
	obj->Name="cmdSaveOK";
	obj->EventCallback=_this;
	GUIObjectRoot->ChildControls.push_back(obj);
	obj=new GUIObject(400,70,192,42,GUIObjectButton,"Cancel");
	obj->Name="cmdCancel";
	obj->EventCallback=_this;
	GUIObjectRoot->ChildControls.push_back(obj);
}

static void pShowPropPage(int nPage){
	unsigned int k=(unsigned int)(nPage*10);
	for(unsigned int i=0;i<ObjectPropItemCollection.size();i++){
		ObjectPropItemCollection[i].objLabel->Visible=(i>=k&&i<k+10);
		ObjectPropItemCollection[i].objTextBox->Visible=(i>=k&&i<k+10);
	}
}

LevelEditor::LevelEditor(const char *lpsLevelName):Game(false)
{
	LEVEL_WIDTH = 2500;
	LEVEL_HEIGHT = 2500;

	memset(s_blocks,0,sizeof(s_blocks));
	s_blocks[TYPE_BLOCK] = load_image("data/gfx/blocks/block.png");
	s_blocks[TYPE_START_PLAYER] = load_image("data/gfx/blocks/playerstart.png");
	s_blocks[TYPE_START_SHADOW] = load_image("data/gfx/blocks/shadowstart.png");
	s_blocks[TYPE_EXIT] = load_image("data/gfx/blocks/exit.png");
	s_blocks[TYPE_SHADOW_BLOCK] = load_image("data/gfx/blocks/shadowblock.png");
	s_blocks[TYPE_SPIKES] = load_image("data/gfx/blocks/spikes.png");
	s_blocks[TYPE_CHECKPOINT] = load_image("data/gfx/blocks/checkpoint.png");
	s_blocks[TYPE_SWAP] = load_image("data/gfx/blocks/swap.png");
	s_blocks[TYPE_FRAGILE] = load_image("data/gfx/blocks/fragile.png");
	s_blocks[TYPE_MOVING_BLOCK] = load_image("data/gfx/blocks/moving_block.png");
	s_blocks[TYPE_MOVING_SHADOW_BLOCK] = load_image("data/gfx/blocks/moving_shadowblock.png");
	s_blocks[TYPE_MOVING_SPIKES] = load_image("data/gfx/blocks/moving_spikes.png");
	s_blocks[TYPE_PORTAL] = load_image("data/gfx/blocks/portal.png");
	s_blocks[TYPE_BUTTON] = load_image("data/gfx/blocks/button.png");
	s_blocks[TYPE_SWITCH] = load_image("data/gfx/blocks/switch.png");
	
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

	switch ( i_current_type )
	{
	default:
		{
			levelObjects.push_back( new Block(x, y, i_current_type, this));
			break;
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
	}

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
	std::ofstream save ( FileName.c_str() );
	if(!save) return;

	int maxX = 0;
	int maxY = 0;

	for ( int o = 0; o < (signed)levelObjects.size(); o++ )
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
	}

	TreeStorageNode node;
	char s[64];

	if ( maxX < LEVEL_WIDTH ) maxX = LEVEL_WIDTH;
	sprintf(s,"%d",maxX);
	node.Attributes["size"].push_back(s);

	if ( maxY < LEVEL_HEIGHT ) maxY = LEVEL_HEIGHT;
	sprintf(s,"%d",maxY);
	node.Attributes["size"].push_back(s);

	//save additional data
	for(map<string,string>::iterator i=EditorData.begin();i!=EditorData.end();i++){
		if(i->first[0] && i->second[0]){
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
				if(obj[i].first[0] && obj[i].second[0]){
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
		obj=new GUIObject(16,32,400,400,GUIObjectFrame,"Level Properties");
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
		}
		//menu
		obj=new GUIObject(584,32,200,400,GUIObjectFrame);
		GUIObjectRoot->ChildControls.push_back(obj);
		{
			obj1=new GUIObject(8,8,184,42,GUIObjectButton,"Load Level");
			obj1->Name="cmdLoad";
			obj1->EventCallback=this;
			obj->ChildControls.push_back(obj1);
			obj1=new GUIObject(8,58,184,42,GUIObjectButton,"Save Level");
			obj1->Name="cmdSave";
			obj1->EventCallback=this;
			obj->ChildControls.push_back(obj1);
			obj1=new GUIObject(8,108,184,42,GUIObjectButton,"Exit Editor");
			obj1->Name="cmdExit";
			obj1->EventCallback=this;
			obj->ChildControls.push_back(obj1);
		}
		//---
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
		pShowOpen(this,LevelName);
		//---
		while(GUIObjectRoot){
			while(SDL_PollEvent(&event)) GUIObjectHandleEvents();
			if(GUIObjectRoot) GUIObjectRoot->render();
			SDL_Flip(screen);
			SDL_Delay(30);
		}
		//---
		return;
	}
	if ( event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_s && (event.key.keysym.mod & KMOD_CTRL))
	{
		pShowSave(this,LevelName);
		//---
		while(GUIObjectRoot){
			while(SDL_PollEvent(&event)) GUIObjectHandleEvents();
			if(GUIObjectRoot) GUIObjectRoot->render();
			SDL_Flip(screen);
			SDL_Delay(30);
		}
		//---
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
			//
			if(GUIObjectRoot){
				delete GUIObjectRoot;
				GUIObjectRoot=NULL;
			}
		}else if(Name=="cmdLoadOK"){
			std::string s=txtName->Caption;
			if(GUIObjectRoot){
				delete GUIObjectRoot;
				GUIObjectRoot=NULL;
			}
			//
			FILE *f=fopen(s.c_str(),"rt");
			if(!f){
				GUIObjectRoot=new GUIObject(100,200,600,200,GUIObjectFrame,"Error");
				GUIObjectRoot->ChildControls.push_back(new GUIObject(8,20,584,42,GUIObjectLabel,string("Can't open file "+s+".").c_str()));
				obj=new GUIObject(200,150,200,42,GUIObjectButton,"OK");
				obj->Name="cmdCancel";
				obj->EventCallback=this;
				GUIObjectRoot->ChildControls.push_back(obj);
			}else{
				fclose(f);
				load_level(s);
			}
		}else if(Name=="cmdSaveOK"){
			std::string s=txtName->Caption;
			if(GUIObjectRoot){
				delete GUIObjectRoot;
				GUIObjectRoot=NULL;
			}
			//TODO:overwrite prompt
			FILE *f=fopen(s.c_str(),"wt");
			if(!f){
				GUIObjectRoot=new GUIObject(100,200,600,200,GUIObjectFrame,"Error");
				GUIObjectRoot->ChildControls.push_back(new GUIObject(8,20,584,42,GUIObjectLabel,string("Can't open file "+s+".").c_str()));
				obj=new GUIObject(200,150,200,42,GUIObjectButton,"OK");
				obj->Name="cmdCancel";
				obj->EventCallback=this;
				GUIObjectRoot->ChildControls.push_back(obj);
			}else{
				fclose(f);
				save_level(s);
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
			pShowOpen(this,LevelName);
		}else if(Name=="cmdSave"){
			pShowSave(this,LevelName);
		}else if(Name=="cmdObjPropPrev"){
			if(ObjectPropPage>0) pShowPropPage(--ObjectPropPage);
		}else if(Name=="cmdObjPropNext"){
			if(ObjectPropPage<ObjectPropPageMax-1) pShowPropPage(++ObjectPropPage);
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
		SDL_Rect r={0,0,50,50};
		apply_surface ( x - camera.x, y - camera.y , s_blocks[i_current_type], screen, &r );
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
