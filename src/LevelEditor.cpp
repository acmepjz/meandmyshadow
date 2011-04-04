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
GUIObject *txt1,*txt2;

LevelEditor::LevelEditor(bool bLoadLevel):Game(false)
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
	
	LevelName="leveledit.map";
	load_level(LevelName);

	i_current_type = TYPE_BLOCK;
	i_current_object = 0;
}

LevelEditor::~LevelEditor()
{
	for(unsigned int i=0;i<levelObjects.size();i++) delete levelObjects[i];
	levelObjects.clear();
}

void LevelEditor::put_object( std::vector<GameObject*> &levelObjects )
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

void LevelEditor::delete_object(std::vector<GameObject*> &LevelObjects)
{
	int x, y;

	SDL_GetMouseState(&x, &y);

	SDL_Rect mouse; mouse.x = x + camera.x; mouse.y = y + camera.y; mouse.w = 1; mouse.h = 1;

	for ( int o = 0; o < (signed)levelObjects.size(); o++ )
	{
		if ( check_collision( LevelObjects[o]->get_box(), mouse ) == true )
		{
			LevelObjects.erase(LevelObjects.begin()+o);
			//break;
		}
	}
}

void LevelEditor::save_level(string FileName)
{
	std::ofstream save ( FileName.c_str() );

	int maxX = 0;
	int maxY = 0;

	for ( int o = 0; o < (signed)levelObjects.size(); o++ )
	{
		if ( levelObjects[o]->get_box().x > maxX )
		{
			maxX = levelObjects[o]->get_box().x + 50;
		}

		if ( levelObjects[o]->get_box().y > maxY )
		{
			maxY = levelObjects[o]->get_box().y + 50;
		}
	}

	if ( maxX < LEVEL_WIDTH ) maxX = LEVEL_WIDTH;
	save << maxX << " ";

	if ( maxY < LEVEL_HEIGHT ) maxY = LEVEL_HEIGHT;
	save << maxY << " ";

	for ( int o = 0; o < (signed)levelObjects.size(); o++ )
	{
		save << levelObjects[o]->i_type << " ";

		SDL_Rect box = levelObjects[o]->get_box();

		save << box.x << " ";
		save << box.y << " ";
	}
}

void LevelEditor::switch_currentObject( int next )
{
	switch ( next )
	{
	case 0:
		i_current_type = TYPE_BLOCK;
		break;

	case 1:
		i_current_type = TYPE_START_PLAYER;
		break;
		
	}
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
			//itoa(LEVEL_WIDTH,s,10);
			txt1=new GUIObject(200,20,192,42,GUIObjectTextBox,s);
			obj->ChildControls.push_back(txt1);
			//itoa(LEVEL_HEIGHT,s,10);
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
		put_object(levelObjects);
	}

	else if ( event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_WHEELDOWN )
	{
		i_current_type++;
		if ( i_current_type > TYPE_MAX )
		{
			i_current_type = 0;
		}
	}

	else if ( event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_WHEELUP )
	{
		i_current_type--;
		if ( i_current_type < 0 )
		{
			i_current_type = TYPE_MAX;
		}
	}

	if ( event.type  == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_RIGHT )
	{
		delete_object(levelObjects);
	}

	if ( event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_c )
	{
		Destroy();
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
				LevelName=s;
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
				LevelName=s;
			}
		}else if(Name=="cmdCancel"){
			if(GUIObjectRoot){
				delete GUIObjectRoot;
				GUIObjectRoot=NULL;
			}
		}else if(Name=="cmdLoad"){
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
			obj->EventCallback=this;
			GUIObjectRoot->ChildControls.push_back(obj);
			obj=new GUIObject(400,70,192,42,GUIObjectButton,"Cancel");
			obj->Name="cmdCancel";
			obj->EventCallback=this;
			GUIObjectRoot->ChildControls.push_back(obj);
		}else if(Name=="cmdSave"){
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
			obj->EventCallback=this;
			GUIObjectRoot->ChildControls.push_back(obj);
			obj=new GUIObject(400,70,192,42,GUIObjectButton,"Cancel");
			obj->Name="cmdCancel";
			obj->EventCallback=this;
			GUIObjectRoot->ChildControls.push_back(obj);
		}
	}
}

////////////////LOGIC////////////////////
void LevelEditor::logic()
{
	Game::logic();
	/*
	o_player.shadow_set_state();
	o_player.shadow_give_state(&o_shadow);
	o_player.jump();
	o_player.move(levelObjects);
	o_player.other_check(&o_shadow);
	

	o_shadow.move_logic();
	o_shadow.jump();
	o_shadow.move(levelObjects);
	o_shadow.other_check(&o_player);
	
	
	if(b_reset) reset();
	b_reset=false;

	set_camera();*/
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
