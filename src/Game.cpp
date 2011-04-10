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
#include "Game.h"
#include "TreeStorageNode.h"
#include "POASerializer.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <stdio.h>
#include <stdlib.h>
using namespace std;

const char* Game::g_sBlockName[TYPE_MAX]={"Block","PlayerStart","ShadowStart",
"Exit","ShadowBlock","Spikes",
"Checkpoint","Swap","Fragile",
"MovingBlock","MovingShadowBlock","MovingSpikes",
"Teleporter","Button","Switch",
};

map<string,int> Game::g_BlockNameMap;

static bool bInitBlockNameMap=false;

Game::Game(bool bLoadLevel):o_player(this),o_shadow(this),objLastCheckPoint(NULL),b_reset(false),bmLevelName(NULL)
{
	if(!bInitBlockNameMap){
		for(int i=0;i<TYPE_MAX;i++){
			g_BlockNameMap[g_sBlockName[i]]=i;
		}
		bInitBlockNameMap=true;
	}

	background = load_image("data/gfx/background.png");

	if(bLoadLevel){
		load_level("data/level/"+o_mylevels.give_level_name());
		o_mylevels.save_levels();
	}
}

Game::~Game()
{
	Destroy();
}

void Game::Destroy(){
	for(unsigned int i=0;i<levelObjects.size();i++) delete levelObjects[i];
	levelObjects.clear();

	LevelName="";
	EditorData.clear();
	if(bmLevelName!=NULL) SDL_FreeSurface(bmLevelName);
	bmLevelName=NULL;
}

void Game::load_level(string FileName)
{
	TreeStorageNode obj;
	POASerializer objSerializer;
	if(!objSerializer.LoadNodeFromFile(FileName.c_str(),&obj,true)) return;

	Destroy();

	SDL_Rect box;

	LEVEL_WIDTH=800;
	LEVEL_HEIGHT=600;

	//load additional data
	for(map<string,vector<string> >::iterator i=obj.Attributes.begin();i!=obj.Attributes.end();i++){
		if(i->first=="size"){
			if(i->second.size()>=2){
				LEVEL_WIDTH=atoi(i->second[0].c_str());
				LEVEL_HEIGHT=atoi(i->second[1].c_str());
			}
		}else if(i->second.size()>0){
			EditorData[i->first]=i->second[0];
		}
	}

	for(unsigned int i=0;i<obj.SubNodes.size();i++){
		TreeStorageNode* obj1=obj.SubNodes[i];
		if(obj1==NULL) continue;
		if(obj1->Name=="tile" && obj1->Value.size()>=3){
			int objectType = g_BlockNameMap[obj1->Value[0]];

			box.x=atoi(obj1->Value[1].c_str());
			box.y=atoi(obj1->Value[2].c_str());

			map<string,string> obj;

			for(map<string,vector<string> >::iterator i=obj1->Attributes.begin();i!=obj1->Attributes.end();i++){
				if(i->second.size()>0) obj[i->first]=i->second[0];
			}

			switch ( objectType )
			{
			default:
				{
					levelObjects.push_back( new Block ( box.x, box.y, objectType, this) );
					break;
				}
			case TYPE_START_PLAYER:
				{
					levelObjects.push_back( new StartObject( box.x, box.y, &o_player, this) );
					break;
				}
			case TYPE_START_SHADOW:
				{
					levelObjects.push_back( new StartObjectShadow( box.x, box.y, &o_shadow, this) );
					break;
				}
			}

			levelObjects.back()->SetEditorData(obj);
		}
	}

	LevelName=FileName;

	//extra data
	if(stateID!=STATE_LEVEL_EDITOR){
		stringstream s;
		s<<"Level "<<o_mylevels.get_level()<<" "<<EditorData["name"];
		SDL_Color fg={0,0,0,0},bg={192,192,192,0};
		bmLevelName=TTF_RenderText_Shaded(font,s.str().c_str(),fg,bg);
		SDL_SetAlpha(bmLevelName,SDL_SRCALPHA,192);
	}
}


/////////////EVENT///////////////

void Game::handle_events()
{
	o_player.handle_input(&o_shadow);

	if ( event.type == SDL_QUIT )
	{
		next_state( STATE_EXIT );
	}

	if ( event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_ESCAPE )
	{
		next_state(STATE_MENU);
		o_mylevels.save_levels();
	}

	if ( event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_s && event.key.keysym.mod == 0)
	{
		if ( Mix_PlayingMusic() == 1 )
		{
			Mix_HaltMusic();
		}

		else 
		{
			Mix_PlayMusic(music,-1);
		}				
	}
	if(event.type==SDL_KEYDOWN && event.key.keysym.sym == SDLK_r){
		b_reset=true;
	}
	if(event.type==SDL_KEYDOWN && event.key.keysym.sym == SDLK_e && (event.key.keysym.mod & KMOD_CTRL) && stateID != STATE_LEVEL_EDITOR ){
		m_sLevelName=LevelName;
		next_state(STATE_LEVEL_EDITOR);
	}
}

/////////////////LOGIC///////////////////
void Game::logic()
{
	o_player.shadow_set_state();
	o_player.shadow_give_state(&o_shadow);
	o_player.jump();
	o_player.move(levelObjects);
	//o_player.other_check(&o_shadow);
	o_player.set_mycamera();

	o_shadow.move_logic();
	o_shadow.jump();
	o_shadow.move(levelObjects);
	//o_shadow.other_check(&o_player);

	for(unsigned int i=0;i<levelObjects.size();i++){
		levelObjects[i]->move();
	}

	o_player.other_check(&o_shadow);
	o_shadow.other_check(&o_player);

	if(b_reset) reset();
	b_reset=false;
}

/////////////////RENDER//////////////////
void Game::render()
{
	apply_surface( 0, 0, background, screen, NULL );

	for ( int o = 0; o < (signed)levelObjects.size(); o++ )
	{
		levelObjects[o]->show();
	}

	o_player.show();
	o_shadow.show();

	if(stateID!=STATE_LEVEL_EDITOR && bmLevelName!=NULL){
		apply_surface(0,SCREEN_HEIGHT-bmLevelName->h,bmLevelName,screen,NULL);
	}
}


//new
bool Game::save_state(){
	if(o_player.can_save_state() && o_shadow.can_save_state()){
		o_player.save_state();
		o_shadow.save_state();
		//save other state, for example moving blocks
		for(unsigned int i=0;i<levelObjects.size();i++){
			levelObjects[i]->save_state();
		}
		//
		return true;
	}
	return false;
}

bool Game::load_state(){
	if(o_player.can_load_state() && o_shadow.can_load_state()){
		o_player.load_state();
		o_shadow.load_state();
		//load other state, for example moving blocks
		for(unsigned int i=0;i<levelObjects.size();i++){
			levelObjects[i]->load_state();
		}
		//
		return true;
	}
	return false;
}

void Game::reset(){
	o_player.reset();
	o_shadow.reset();
	objLastCheckPoint=NULL;
	//reset other state, for example moving blocks
	for(unsigned int i=0;i<levelObjects.size();i++){
		levelObjects[i]->reset();
	}
<<<<<<< .mine
}

void Game::BroadcastObjectEvent(int nEventType,int nObjectType,const char* id){
	if(id==NULL){
		for(unsigned int i=0;i<levelObjects.size();i++){
			if(nObjectType<0 || levelObjects[i]->i_type==nObjectType){
				levelObjects[i]->OnEvent(nEventType);
			}
		}
	}else{
		string s=id;
		for(unsigned int i=0;i<levelObjects.size();i++){
			if(nObjectType<0 || levelObjects[i]->i_type==nObjectType){
				Block *obj=dynamic_cast<Block*>(levelObjects[i]);
				if(obj!=NULL && obj->id==s){
					levelObjects[i]->OnEvent(nEventType);
				}
			}
		}
	}
}
=======
}
>>>>>>> .r31
