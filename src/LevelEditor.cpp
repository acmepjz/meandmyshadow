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
#include <fstream>
#include <iostream>
#include <vector>
#include <string.h>
#include <stdio.h>
using namespace std;

//warning: weak reference only
static GUIObject *txt1,*txt2;

struct typeObjectPropItem{
	string sKey;
	GUIObject *objTextBox,*objLabel;
};

static vector<typeObjectPropItem> ObjectPropItemCollection;
static GameObject *ObjectPropOwner;
static int ObjectPropPage,ObjectPropPageMax;
//over

static void pShowOpen(GUIEventCallback* _this,std::string& LevelName){
	GUIObject* obj;
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}
	GUIObjectRoot=new GUIObject(100,200,600,200,GUIObjectFrame,"Load Level");
	GUIObjectRoot->ChildControls.push_back(new GUIObject(8,20,184,42,GUIObjectLabel,"File Name"));
	txt1=new GUIObject(160,20,432,42,GUIObjectTextBox,LevelName.c_str());
	GUIObjectRoot->ChildControls.push_back(txt1);
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
	txt1=new GUIObject(160,20,432,42,GUIObjectTextBox,LevelName.c_str());
	GUIObjectRoot->ChildControls.push_back(txt1);
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
	
	if(lpsLevelName!=NULL && *lpsLevelName) load_level(lpsLevelName);

	i_current_type = TYPE_BLOCK;
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

	x=((x+camera.x)/50)*50;
	y=((y+camera.y)/50)*50;

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
			unsigned int m=objMap.size();
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
				GUIObjectRoot=new GUIObject(100,(SCREEN_HEIGHT-nHeight)/2,600,nHeight,GUIObjectFrame,"Object Properties");
				for(i=0;i<(int)objMap.size();i++){
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
		int x=levelObjects[o]->get_box().x+50;
		int y=levelObjects[o]->get_box().y+50;
		if ( x > maxX )
		{
			maxX = x;
		}

		if ( y > maxY )
		{
			maxY = y;
		}
	}

	if ( maxX < LEVEL_WIDTH ) maxX = LEVEL_WIDTH;
	save << maxX << " ";

	if ( maxY < LEVEL_HEIGHT ) maxY = LEVEL_HEIGHT;
	save << maxY << " ";

	//save additional data
	{
		map<string,string> obj_new;
		for(map<string,string>::iterator i=EditorData.begin();i!=EditorData.end();i++){
			if(i->first[0] && i->second[0]){
				obj_new[i->first]=i->second;
			}
		}
		save<<obj_new.size()<<" ";
		for(map<string,string>::iterator i=obj_new.begin();i!=obj_new.end();i++){
			save << i->first << " " << i->second << " ";
		}
		save<<endl;
	}

	for ( int o = 0; o < (signed)levelObjects.size(); o++ )
	{
		int objectType = levelObjects[o]->i_type;

		if(objectType>=0){
			save << objectType << " ";

			SDL_Rect box = levelObjects[o]->get_box_base();

			save << box.x << " ";
			save << box.y << " ";

			vector<pair<string,string> > obj,obj_new;
			levelObjects[o]->GetEditorData(obj);
			for(unsigned int i=0;i<obj.size();i++){
				if(obj[i].first[0] && obj[i].second[0]){
					obj_new.push_back(obj[i]);
				}
			}
			save<<obj_new.size()<<" ";
			for(unsigned int i=0;i<obj_new.size();i++){
				save << obj_new[i].first << " " << obj_new[i].second << " ";
			}
			save<<endl;
		}
	}

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
			obj1=new GUIObject(8,20,184,42,GUIObjectLabel,"Level Width");
			obj->ChildControls.push_back(obj1);
			obj1=new GUIObject(8,70,184,42,GUIObjectLabel,"Level Height");
			obj->ChildControls.push_back(obj1);
			itoa(LEVEL_WIDTH,s,10);
			txt1=new GUIObject(200,20,192,42,GUIObjectTextBox,s);
			obj->ChildControls.push_back(txt1);
			itoa(LEVEL_HEIGHT,s,10);
			txt2=new GUIObject(200,70,192,42,GUIObjectTextBox,s);
			obj->ChildControls.push_back(txt2);
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
		i_current_type++;
		if ( i_current_type > TYPE_MAX )
		{
			i_current_type = 0;
		}
		return;
	}

	else if ( event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_WHEELUP )
	{
		i_current_type--;
		if ( i_current_type < 0 )
		{
			i_current_type = TYPE_MAX;
		}
		return;
	}

	if ( event.type  == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_RIGHT )
	{
		delete_object();
		return;
	}

	if ( event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_c )
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
			i=atoi(txt1->Caption.c_str());
			if(i>0&&i<=10000) LEVEL_WIDTH=i;
			i=atoi(txt2->Caption.c_str());
			if(i>0&&i<=10000) LEVEL_HEIGHT=i;
			//
			if(GUIObjectRoot){
				delete GUIObjectRoot;
				GUIObjectRoot=NULL;
			}
		}else if(Name=="cmdLoadOK"){
			std::string s=txt1->Caption;
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
			std::string s=txt1->Caption;
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
	//Za�asni grid prikaz/////////////////
	int x, y;

	SDL_GetMouseState(&x, &y);

	x=((x+camera.x)/50)*50;
	y=((y+camera.y)/50)*50;

	if(i_current_type>=0 && i_current_type<=TYPE_MAX){
		SDL_Rect r={0,0,50,50};
		apply_surface ( x - camera.x, y - camera.y , s_blocks[i_current_type], screen, &r );
	}
	//////////////////////////

}

/////////////////RENDER//////////////////////
void LevelEditor::render()
{
	apply_surface( 0, 0, background, screen, NULL );

	for ( int o = 0; o < (signed)levelObjects.size(); o++ )
	{
		levelObjects[o]->show();
	}

	show_current_object();

	o_shadow.show();
	o_player.show();
}
