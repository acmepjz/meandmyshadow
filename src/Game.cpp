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
"ConveyorBelt","ShadowConveyorBelt","NotificationBlock",
};

map<string,int> Game::g_BlockNameMap;

Game::Game(bool bLoadLevel):b_reset(false),Background(NULL),CustomTheme(NULL),GameTipIndex(0),
player(this),shadow(this),objLastCheckPoint(NULL){
	memset(bmTips,0,sizeof(bmTips));

	if(bLoadLevel){
		//Check if the level is in the userpath.
		if(levels.addon){
			setPathPrefix(getUserPath());
		}
		loadLevel(levels.getLevelFile());
		levels.saveLevelProgress();
		//And reset the pathPrefix.
		if(levels.addon){
			setPathPrefix("");
		}
	}
}

Game::~Game(){
	destroy();
}

void Game::destroy(){
	for(unsigned int i=0;i<levelObjects.size();i++) delete levelObjects[i];
	levelObjects.clear();

	LevelName.clear();
	EditorData.clear();
	for(int i=0;i<TYPE_MAX;i++){
		if(bmTips[i]) SDL_FreeSurface(bmTips[i]);
	}
	memset(bmTips,0,sizeof(bmTips));
	Background=NULL;
	if(CustomTheme) objThemes.removeTheme();
	CustomTheme=NULL;
}

void Game::loadLevel(string fileName){
	TreeStorageNode obj;
	{
		POASerializer objSerializer;
		string s=processFileName(fileName);
		if(!objSerializer.LoadNodeFromFile(s.c_str(),&obj,true)){
			cout<<"Can't load level file "<<s<<endl;
			return;
		}
	}

	destroy();

	SDL_Rect box;

	LEVEL_WIDTH=800;
	LEVEL_HEIGHT=600;

	//load additional data
	for(map<string,vector<string> >::iterator i=obj.attributes.begin();i!=obj.attributes.end();i++){
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
		string theme = getSettings()->getValue("theme");
		
		//First try the main themes.
		if(theme!="default") {
			CustomTheme=objThemes.appendThemeFromFile(processFileName("%DATA%/themes/"+theme+"/theme.mnmstheme"));
			if(!CustomTheme) {
				//Then try the addon themes.
				//We load a theme from the user path so change the pathprefix.
				setPathPrefix(getUserPath());
				CustomTheme=objThemes.appendThemeFromFile(processFileName("%USER%/themes/"+theme+"/theme.mnmstheme"));
				if(!CustomTheme) {
					cout<<"ERROR: Can't load configured theme file "<<theme<<endl;	
				}
				//And change it back.
				setPathPrefix("");
			}
		}
			  
		//Check if level themes are enabled.
		if(getSettings()->getBoolValue("leveltheme")) {
			string &s=EditorData["theme"];
			if(!s.empty()){
				CustomTheme=objThemes.appendThemeFromFile(processFileName("%DATA%/themes/"+theme+"/theme.mnmstheme"));
			      if(!CustomTheme) {
					//Then try the addon themes.
					//We load a theme from the user path so change the pathprefix.
					setPathPrefix(getUserPath());
					CustomTheme=objThemes.appendThemeFromFile(processFileName("%USER%/themes/"+theme+"/theme.mnmstheme"));
					if(!CustomTheme) {
						cout<<"ERROR: Can't load configured theme file "<<theme<<endl;	
					}
					//And change it back.
					setPathPrefix("");
				}
			}
		}
		
		//Set the Appearance of the player and the shadow.
		objThemes.getCharacter(false)->createInstance(&player.Appearance);
		objThemes.getCharacter(true)->createInstance(&shadow.Appearance);
	}

	for(unsigned int i=0;i<obj.subNodes.size();i++){
		TreeStorageNode* obj1=obj.subNodes[i];
		if(obj1==NULL) continue;
		if(obj1->name=="tile" && obj1->value.size()>=3){
			int objectType = g_BlockNameMap[obj1->value[0]];

			box.x=atoi(obj1->value[1].c_str());
			box.y=atoi(obj1->value[2].c_str());

			map<string,string> obj;

			for(map<string,vector<string> >::iterator i=obj1->attributes.begin();i!=obj1->attributes.end();i++){
				if(i->second.size()>0) obj[i->first]=i->second[0];
			}

			levelObjects.push_back( new Block ( box.x, box.y, objectType, this) );
			levelObjects.back()->setEditorData(obj);
		}
	}

	LevelName=fileName;

	//extra data
	if(stateID!=STATE_LEVEL_EDITOR){
		stringstream s;
		if(levels.getLevelCount()>1){
			s<<"Level "<<(levels.getLevel()+1)<<" ";
		}
		s<<EditorData["name"];
		SDL_Color fg={0,0,0,0},bg={255,255,255,0};
		bmTips[0]=TTF_RenderText_Shaded(font,s.str().c_str(),fg,bg);
		if(bmTips[0]) SDL_SetAlpha(bmTips[0],SDL_SRCALPHA,160);
	}

	//get background
	Background=objThemes.getBackground();
	if(Background) Background->resetAnimation();
}


/////////////EVENT///////////////

void Game::handleEvents(){
	player.handleInput(&shadow);

	if(event.type==SDL_QUIT){
		setNextState(STATE_EXIT);
	}

	if(event.type==SDL_KEYUP && event.key.keysym.sym==SDLK_ESCAPE){
		setNextState(STATE_LEVEL_SELECT);
		levels.saveLevelProgress();
	}

	if(event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_s && event.key.keysym.mod==0){
		if(Mix_PlayingMusic()==1){
			Mix_HaltMusic();
		}else{
			Mix_PlayMusic(music,-1);
		}				
	}
	if(event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_r){
		b_reset=true;
	}
	if(event.type==SDL_KEYDOWN && event.key.keysym.sym == SDLK_e && (event.key.keysym.mod & KMOD_CTRL) && stateID != STATE_LEVEL_EDITOR ){
		levelName=LevelName;
		setNextState(STATE_LEVEL_EDITOR);
	}
}

/////////////////LOGIC///////////////////
void Game::logic()
{
	player.shadowSetState();
	player.shadowGiveState(&shadow);
	player.jump();
	player.move(levelObjects);
	player.setMyCamera();

	shadow.move_logic();
	shadow.jump();
	shadow.move(levelObjects);

	//move object
	for(unsigned int i=0;i<levelObjects.size();i++){
		levelObjects[i]->move();
	}

	//process event
	for(unsigned int idx=0;idx<EventQueue.size();idx++){
		typeGameObjectEvent &e=EventQueue[idx];
		if(e.nFlags|1){
			for(unsigned int i=0;i<levelObjects.size();i++){
				if(e.nObjectType<0 || levelObjects[i]->type==e.nObjectType){
					Block *obj=dynamic_cast<Block*>(levelObjects[i]);
					if(obj!=NULL && obj->id==e.id){
						levelObjects[i]->onEvent(e.nEventType);
					}
				}
			}
		}else{
			for(unsigned int i=0;i<levelObjects.size();i++){
				if(e.nObjectType<0 || levelObjects[i]->type==e.nObjectType){
					levelObjects[i]->onEvent(e.nEventType);
				}
			}
		}
	}
	EventQueue.clear();

	player.otherCheck(&shadow);
	shadow.otherCheck(&player);

	if(b_reset) reset();
	b_reset=false;
}

/////////////////RENDER//////////////////
void Game::render()
{
	//draw background
	{
		ThemeBackground *bg=Background;
		if(bg==NULL && objThemes.themeCount()>0){
			bg=objThemes[0]->getBackground();
		}
		if(bg){
			bg->draw(screen);
			if(bg==Background) bg->updateAnimation();
		}else{
			SDL_Rect r={0,0,SCREEN_WIDTH,SCREEN_HEIGHT};
			SDL_FillRect(screen,&r,-1);
		}
	}

	for(unsigned int o=0; o<levelObjects.size(); o++){
		levelObjects[o]->show();
	}

	player.show();
	shadow.show();

	//show level name
	if(stateID!=STATE_LEVEL_EDITOR && bmTips[0]!=NULL){
		applySurface(0,SCREEN_HEIGHT-bmTips[0]->h,bmTips[0],screen,NULL);
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
				bmTips[GameTipIndex]=TTF_RenderText_Shaded(fontSmall,s,fg,bg);
				SDL_SetAlpha(bmTips[GameTipIndex],SDL_SRCALPHA,160);
			}
		}
		if(bmTips[GameTipIndex]!=NULL){
			applySurface(0,0,bmTips[GameTipIndex],screen,NULL);
		}
	}
	GameTipIndex=0;
	//die?
	if(player.b_dead){
		SDL_Surface *bm=NULL;
		if(player.canLoadState()){
			if(bmTips[2]==NULL){
				SDL_Color fg={0,0,0,0},bg={255,255,255,0};
				bmTips[2]=TTF_RenderText_Shaded(fontSmall,
					"Press R to restart current level or press F3 to load the game.",
					fg,bg);
				SDL_SetAlpha(bmTips[2],SDL_SRCALPHA,160);
			}
			bm=bmTips[2];
		}else{
			if(bmTips[1]==NULL){
				SDL_Color fg={0,0,0,0},bg={255,255,255,0};
				bmTips[1]=TTF_RenderText_Shaded(fontSmall,
					"Press R to restart current level.",
					fg,bg);
				SDL_SetAlpha(bmTips[1],SDL_SRCALPHA,160);
			}
			bm=bmTips[1];
		}
		if(bm!=NULL) applySurface(0,0,bm,screen,NULL);
	}
}


//new
bool Game::save_state(){
	if(player.canSaveState() && shadow.canSaveState()){
		player.saveState();
		shadow.saveState();
		//save other state, for example moving blocks
		for(unsigned int i=0;i<levelObjects.size();i++){
			levelObjects[i]->saveState();
		}
		if(Background) Background->saveAnimation();
		//
		return true;
	}
	return false;
}

bool Game::load_state(){
	if(player.canLoadState() && shadow.canLoadState()){
		player.loadState();
		shadow.loadState();
		//load other state, for example moving blocks
		for(unsigned int i=0;i<levelObjects.size();i++){
			levelObjects[i]->loadState();
		}
		if(Background) Background->loadAnimation();
		//
		return true;
	}
	return false;
}

void Game::reset(){
	player.reset();
	shadow.reset();
	objLastCheckPoint=NULL;
	//reset other state, for example moving blocks
	for(unsigned int i=0;i<levelObjects.size();i++){
		levelObjects[i]->reset();
	}
	if(Background) Background->resetAnimation();
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
