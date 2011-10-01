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
#include <string.h>
using namespace std;

const char* Game::g_sBlockName[TYPE_MAX]={"Block","PlayerStart","ShadowStart",
"Exit","ShadowBlock","Spikes",
"Checkpoint","Swap","Fragile",
"MovingBlock","MovingShadowBlock","MovingSpikes",
"Teleporter","Button","Switch",
"ConveyorBelt","ShadowConveyorBelt",
};

map<string,int> Game::g_BlockNameMap;

Game::Game(bool bLoadLevel):b_reset(false),Background(NULL),CustomTheme(NULL),GameTipIndex(0),
o_player(this),o_shadow(this),objLastCheckPoint(NULL)
{

	memset(bmTips,0,sizeof(bmTips));

	if(bLoadLevel){
		load_level(o_mylevels.get_level_file(), o_mylevels.m_bAddon);
		o_mylevels.save_level_progress();
	}
}

Game::~Game()
{
	Destroy();
}

void Game::Destroy(){
	for(unsigned int i=0;i<levelObjects.size();i++) delete levelObjects[i];
	levelObjects.clear();

	LevelName.clear();
	EditorData.clear();
	for(int i=0;i<TYPE_MAX;i++){
		if(bmTips[i]) SDL_FreeSurface(bmTips[i]);
	}
	memset(bmTips,0,sizeof(bmTips));
	Background=NULL;
	if(CustomTheme) m_objThemes.RemoveTheme();
	CustomTheme=NULL;
}

void Game::load_level(string FileName, bool addon)
{
	TreeStorageNode obj;
	{
		POASerializer objSerializer;
		string s=ProcessFileName(FileName, addon);
		if(!objSerializer.LoadNodeFromFile(s.c_str(),&obj,true)){
			cout<<"Can't load level file "<<s<<endl;
			return;
		}
	}

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

	//get theme
	{	
		//If a theme is configured then load it.
		string theme = get_settings()->getValue("theme");
		
		//First try the main themes.
		if(theme!="default") {
			CustomTheme=m_objThemes.AppendThemeFromFile(ProcessFileName("%DATA%/themes/"+theme+"/theme.mnmstheme"));
			if(!CustomTheme) {
				//Then try the addon themes.
				CustomTheme=m_objThemes.AppendThemeFromFile(ProcessFileName("%USER%/themes/"+theme+"/theme.mnmstheme"));
				if(!CustomTheme) {
					cout<<"Error: Can't load configured theme file "<<theme<<endl;	
				}
			}
		}
			  
		//Check if level themes are enabled.
		if(get_settings()->getBoolValue("leveltheme")) {
			string &s=EditorData["theme"];
			if(!s.empty()){
				CustomTheme=m_objThemes.AppendThemeFromFile(ProcessFileName(s));
				if(!CustomTheme) cout<<"Error: Can't load custom theme file "<<s<<endl;
			}
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

			/*switch ( objectType )
			{
			default:
				{*/
					levelObjects.push_back( new Block ( box.x, box.y, objectType, this) );
					/*break;
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
			}*/

			levelObjects.back()->SetEditorData(obj);
		}
	}

	LevelName=FileName;

	//extra data
	if(stateID!=STATE_LEVEL_EDITOR){
		stringstream s;
		if(o_mylevels.get_level_count()>1){
			s<<"Level "<<(o_mylevels.get_level()+1)<<" ";
		}
		s<<EditorData["name"];
		SDL_Color fg={0,0,0,0},bg={255,255,255,0};
		bmTips[0]=TTF_RenderText_Shaded(font,s.str().c_str(),fg,bg);
		if(bmTips[0]) SDL_SetAlpha(bmTips[0],SDL_SRCALPHA,160);
	}

	//get background
	Background=m_objThemes.GetBackground();
	if(Background) Background->ResetAnimation();
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
		o_mylevels.save_level_progress();
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
	o_player.set_mycamera();

	o_shadow.move_logic();
	o_shadow.jump();
	o_shadow.move(levelObjects);

	//move object
	for(unsigned int i=0;i<levelObjects.size();i++){
		levelObjects[i]->move();
	}

	//process event
	for(unsigned int idx=0;idx<EventQueue.size();idx++){
		typeGameObjectEvent &e=EventQueue[idx];
		if(e.nFlags|1){
			for(unsigned int i=0;i<levelObjects.size();i++){
				if(e.nObjectType<0 || levelObjects[i]->i_type==e.nObjectType){
					Block *obj=dynamic_cast<Block*>(levelObjects[i]);
					if(obj!=NULL && obj->id==e.id){
						levelObjects[i]->OnEvent(e.nEventType);
					}
				}
			}
		}else{
			for(unsigned int i=0;i<levelObjects.size();i++){
				if(e.nObjectType<0 || levelObjects[i]->i_type==e.nObjectType){
					levelObjects[i]->OnEvent(e.nEventType);
				}
			}
		}
	}
	EventQueue.clear();

	o_player.other_check(&o_shadow);
	o_shadow.other_check(&o_player);

	if(b_reset) reset();
	b_reset=false;
}

/////////////////RENDER//////////////////
void Game::render()
{
	//draw background
	{
		ThemeBackground *bg=Background;
		if(bg==NULL && m_objThemes.ThemeCount()>0){
			bg=m_objThemes[0]->GetBackground();
		}
		if(bg){
			bg->Draw(screen);
			if(bg==Background) bg->UpdateAnimation();
		}else{
			SDL_Rect r={0,0,SCREEN_WIDTH,SCREEN_HEIGHT};
			SDL_FillRect(screen,&r,-1);
		}
	}

	for ( unsigned int o = 0; o < levelObjects.size(); o++ )
	{
		levelObjects[o]->show();
	}

	o_player.show();
	o_shadow.show();

	//show level name
	if(stateID!=STATE_LEVEL_EDITOR && bmTips[0]!=NULL){
		apply_surface(0,SCREEN_HEIGHT-bmTips[0]->h,bmTips[0],screen,NULL);
	}
	//show tips
	if(GameTipIndex>2 && GameTipIndex<TYPE_MAX){
		if(bmTips[GameTipIndex]==NULL){
			const char* s=NULL;
			switch(GameTipIndex){
			case TYPE_CHECKPOINT:
				s="Press DOWN key to save the game.";
				break;
			case TYPE_SWAP:
				s="Press DOWN key to swap the position of player and shadow.";
				break;
			case TYPE_SWITCH:
				s="Press DOWN key to activate the switch.";
				break;
			case TYPE_PORTAL:
				s="Press DOWN key to teleport.";
				break;
			}
			if(s!=NULL){
				SDL_Color fg={0,0,0,0},bg={255,255,255,0};
				bmTips[GameTipIndex]=TTF_RenderText_Shaded(font_small,s,fg,bg);
				SDL_SetAlpha(bmTips[GameTipIndex],SDL_SRCALPHA,160);
			}
		}
		if(bmTips[GameTipIndex]!=NULL){
			apply_surface(0,0,bmTips[GameTipIndex],screen,NULL);
		}
	}
	GameTipIndex=0;
	//die?
	if(o_player.b_dead){
		SDL_Surface *bm=NULL;
		if(o_player.can_load_state()){
			if(bmTips[2]==NULL){
				SDL_Color fg={0,0,0,0},bg={255,255,255,0};
				bmTips[2]=TTF_RenderText_Shaded(font_small,
					"Press R to restart current level or press F3 to load the game.",
					fg,bg);
				SDL_SetAlpha(bmTips[2],SDL_SRCALPHA,160);
			}
			bm=bmTips[2];
		}else{
			if(bmTips[1]==NULL){
				SDL_Color fg={0,0,0,0},bg={255,255,255,0};
				bmTips[1]=TTF_RenderText_Shaded(font_small,
					"Press R to restart current level.",
					fg,bg);
				SDL_SetAlpha(bmTips[1],SDL_SRCALPHA,160);
			}
			bm=bmTips[1];
		}
		if(bm!=NULL) apply_surface(0,0,bm,screen,NULL);
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
		if(Background) Background->SaveAnimation();
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
		if(Background) Background->LoadAnimation();
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
	if(Background) Background->ResetAnimation();
}

void Game::BroadcastObjectEvent(int nEventType,int nObjectType,const char* id){
	typeGameObjectEvent e;
	e.nEventType=nEventType;
	e.nObjectType=nObjectType;
	e.nFlags=0;
	if(id){
		e.nFlags|=1;
		e.id=id;
	}
	EventQueue.push_back(e);
}
