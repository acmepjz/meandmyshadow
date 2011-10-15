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
#include "Functions.h"
#include "Globals.h"
#include "Objects.h"
#include "LevelSelect.h"
#include "GUIObject.h"
#include "GUIScrollBar.h"
#include <SDL/SDL_ttf.h>
#include <SDL/SDL.h>
#include <string>
#include <sstream>
#include <iostream>
using namespace std;

////////////////////NUMBER////////////////////////
Number::Number( )
{
	s_image = NULL;
	s_level = NULL;
	number = 0;

	myBox.x = 0; myBox.y = 0; myBox.h = 50; myBox.w = 50;
}

Number::~Number()
{
	if(s_image) SDL_FreeSurface(s_image);
}

void Number::init(int number, SDL_Rect box )
{
	Number::number = number;
	update_lock();

	std::stringstream text;

	number++;
	text << number;

	SDL_Color black = { 0,0,0 };

	if(s_image) SDL_FreeSurface(s_image);
	s_image = TTF_RenderText_Blended(number>=100?font_small:font, text.str().c_str(), black);

	myBox.x = box.x; myBox.y = box.y; myBox.h = 50; myBox.w = 50; 
}

void Number::show( int dy )
{
	apply_surface( myBox.x,myBox.y-dy, s_level, screen,NULL );
	apply_surface( (myBox.x + 25 - (s_image->w / 2)), (myBox.y + 25 - (s_image->h / 2))-dy, s_image, screen, NULL );
}

void Number::update_lock(){
	if ( o_mylevels.get_locked(number) == false )
	{
		s_level = load_image(get_data_path()+"gfx/level.png");
	}

	else { s_level = load_image(get_data_path()+"gfx/levellocked.png"); }
}

/////////////////////LEVEL SELECT/////////////////////

static GUIScrollBar *m_oLvScrollBar=NULL;
static GUIObject *m_oLvPackName=NULL;

LevelSelect::LevelSelect()
{
	s_background = load_image(get_data_path()+"gfx/menu/levelselect.png");

	//create GUI (test only)
	GUIObject* obj;
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}

	GUIObjectRoot=new GUIObject(0,0,800,600);
	m_oLvScrollBar=new GUIScrollBar(768,140,16,370,ScrollBarVertical,0,0,0,1,5,true,false);
	GUIObjectRoot->ChildControls.push_back(m_oLvScrollBar);
	m_oLvPackName=new GUIObject(60,64,800,32,GUIObjectLabel);
	GUIObjectRoot->ChildControls.push_back(m_oLvPackName);

	obj=new GUIObject(60,96,200,32,GUIObjectButton,"Levelpacks");
	obj->Name="cmdLvPack";
	obj->EventCallback=this;
	GUIObjectRoot->ChildControls.push_back(obj);
	obj=new GUIObject(270,96,200,32,GUIObjectButton,"Levels");
	obj->Name="cmdLoadLv";
	obj->EventCallback=this;
	GUIObjectRoot->ChildControls.push_back(obj);
	obj=new GUIObject(60,540,200,32,GUIObjectButton,"Clear progress");
	obj->Name="cmdReset";
	obj->EventCallback=this;
	GUIObjectRoot->ChildControls.push_back(obj);
	
	if(get_settings()->getBoolValue("internet")) {
	  obj=new GUIObject(480,540,200,32,GUIObjectButton,"Addons");
	  obj->Name="cmdAddon";
	  obj->EventCallback=this;
	  GUIObjectRoot->ChildControls.push_back(obj);
	}

	//show level list
	refresh();
}

void LevelSelect::refresh(){
	int m=o_mylevels.get_level_count();
	o_number.clear();

	for ( int n = 0; n < m; n++ )
	{
		o_number.push_back( Number () );
	}

	for ( int n = 0; n < m; n++ )
	{
		SDL_Rect box={(n%10)*64+60,(n/10)*80+140,0,0};
		o_number[n].init( n, box );
	}

	if(m>50){
		m_oLvScrollBar->Max=(m-41)/10;
		m_oLvScrollBar->Visible=true;
	}else{
		m_oLvScrollBar->Max=0;
		m_oLvScrollBar->Visible=false;
	}
	m_oLvPackName->Caption="Level pack: "+o_mylevels.LevelPackName;
}

LevelSelect::~LevelSelect()
{
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}
	m_oLvScrollBar=NULL;
	m_oLvPackName=NULL;
}

void LevelSelect::handleEvents()
{
	if ( event.type == SDL_QUIT )
	{
		next_state(STATE_EXIT);
	}

	if ( event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT )
	{
		check_mouse();
	}

	if ( event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_ESCAPE )
	{
		next_state(STATE_MENU);
	}

	if ( event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_s )
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
	else if ( event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_WHEELDOWN && m_oLvScrollBar)
	{
		if(m_oLvScrollBar->Value<m_oLvScrollBar->Max) m_oLvScrollBar->Value++;
		return;
	}

	else if ( event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_WHEELUP && m_oLvScrollBar)
	{
		if(m_oLvScrollBar->Value>0) m_oLvScrollBar->Value--;
		return;
	}
}

void LevelSelect::check_mouse()
{
	int x,y,dy=0,m=o_mylevels.get_level_count();

	SDL_GetMouseState(&x,&y);

	if(m_oLvScrollBar) dy=m_oLvScrollBar->Value;
	if(m>dy*10+50) m=dy*10+50;
	y+=dy*80;

	SDL_Rect mouse = { x,y,0,0};

	for ( int n = dy*10; n < m; n++ )
	{
		if ( o_mylevels.get_locked(n) == false )
		{
			if ( check_collision( mouse, o_number[n].myBox ) == true )
			{
				o_mylevels.set_level(n);
				next_state(STATE_GAME);
			}
		}
	}
}


void LevelSelect::logic()
{
}

void LevelSelect::render()
{
	int x,y,dy=0,m=o_mylevels.get_level_count();
	int idx=-1;

	SDL_GetMouseState(&x,&y);

	if(m_oLvScrollBar) dy=m_oLvScrollBar->Value;
	if(m>dy*10+50) m=dy*10+50;
	y+=dy*80;

	SDL_Rect mouse = { x,y,0,0};

	apply_surface( 0 , 0, s_background, screen, NULL );

	for ( int n = dy*10; n < m; n++ )
	{
		o_number[n].show(dy*80);
		if ( o_mylevels.get_locked(n) == false && check_collision( mouse, o_number[n].myBox ) == true ) idx=n;
	}
	//show tool tip text
	if(idx>=0){
		SDL_Color bg={255,255,255},fg={0,0,0};
		SDL_Surface *s=TTF_RenderText_Shaded(font_small, o_mylevels.get_level_name(idx).c_str(), fg, bg);
		if(s!=NULL){
			SDL_Rect r=o_number[idx].myBox;
			r.y-=dy*80;
			if(r.y>SCREEN_HEIGHT-200){
				r.y-=s->h+4;
			}else{
				r.y+=r.h+4;
			}
			if(r.x+s->w>SCREEN_WIDTH-50) r.x=SCREEN_WIDTH-50-s->w;
			SDL_BlitSurface(s,NULL,screen,&r);
			r.x--;
			r.y--;
			r.w=s->w+1;
			r.h=1;
			SDL_FillRect(screen,&r,0);
			SDL_Rect r1={r.x,r.y,1,s->h+1};
			SDL_FillRect(screen,&r1,0);
			r1.x+=r.w;
			SDL_FillRect(screen,&r1,0);
			r.y+=r1.h;
			SDL_FillRect(screen,&r,0);
			SDL_FreeSurface(s);
		}
	}
}

void LevelSelect::GUIEventCallback_OnEvent(std::string Name,GUIObject* obj,int nEventType){
	string s;
	if(Name=="cmdLvPack"){
		if(!FileDialog(s,"Load Level Pack","","%DATA%/levelpacks/\nMain levelpacks\n%USER%/levelpacks/\nAddon levelpacks",false,true,false)) return;
	}else if(Name=="cmdLoadLv"){
		if(FileDialog(s,"Load Level","map","%DATA%/levels/\nMain levels\n%USER%/levels/\nAddon levels",false,true)){
			o_mylevels.clear();
			o_mylevels.add_level(s,"");
			o_mylevels.set_level(0);
			next_state(STATE_GAME);
		}
		return;
	}else if(Name=="cmdReset"){
		if(MsgBox("Do you really want to reset level progress?",MsgBoxYesNo,"Warning")==MsgBoxYes){
			for(int i=0;i<o_mylevels.get_level_count();i++){
				o_mylevels.set_locked(i,i>0?true:false);
				o_number[i].update_lock();
			}
			o_mylevels.save_level_progress();
		}
		return;
	}else if(Name=="cmdAddon"){
		next_state(STATE_ADDONS);
		return;
	}else{
		return;
	}
	string s1;
	if(s.compare(0,6,"%DATA%")==0){
		int i=s.find_last_of("/\\");
		if(i!=string::npos) s1=s.substr(i+1);
		else s1=s.substr(6);
		s1="%USER%/progress/"+s1+".progress";
	}else{
 		int i=s.find_last_of("/\\");
		if(i!=string::npos) s1=s.substr(i+1);
		else s1=s.substr(6);
		s1="%USER%/progress/"+s1+".progress";
	}
	//load file
	o_mylevels.m_bAddon=(s.compare(0,6,"%USER%")==0);
	if(!o_mylevels.load_levels(s+"/levels.lst",s1)){
		MsgBox("Can't load level pack:\n"+s,MsgBoxOKOnly,"Error");
	}
	refresh();
}
