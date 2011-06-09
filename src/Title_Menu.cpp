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
#include "Functions.h"
#include "Classes.h"
#include "Globals.h"
#include "Title_Menu.h"

#include <iostream>
using namespace std;

static int m_nHighlight=0;

Menu::Menu()
{
	s_menu = load_image(GetDataPath()+"data/gfx/menu/menu.png");
	m_nHighlight=0;
}

Menu::~Menu()
{
}

void Menu::handle_events()
{
	int x, y;

	SDL_GetMouseState(&x, &y);

	m_nHighlight=0;
	if(x>=200&&x<600&&y>=150&&y<550){
		m_nHighlight=(y-70)/80;
	}

	if ( event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT )
	{
		switch(m_nHighlight){
		case 1:
			next_state(STATE_LEVEL_SELECT);
			break;
		case 2:
			next_state(STATE_OPTIONS);
			break;
		case 3:
			m_sLevelName="leveledit.map";
			next_state(STATE_LEVEL_EDITOR);
			break;
		case 4:
			next_state(STATE_HELP);
			break;
		case 5:
			next_state(STATE_EXIT);
			break;
		}
	}

	if ( event.type == SDL_QUIT )
	{
		next_state(STATE_EXIT);
	}
}

void Menu::logic()
{

}

void Menu::render()
{
	apply_surface( 0,0,s_menu,screen,NULL );
	if(m_nHighlight>0){
		SDL_Rect r,r1;
		r.x=200;
		r.y=70+80*m_nHighlight;
		r.w=400;
		r.h=1;
		SDL_FillRect(screen,&r,0);
		r1.x=200;
		r1.y=r.y;
		r1.w=1;
		r1.h=80;
		SDL_FillRect(screen,&r1,0);
		r1.x=600;
		SDL_FillRect(screen,&r1,0);
		r.y+=80;
		SDL_FillRect(screen,&r,0);
	}
}

Help::Help()
{
	s_help = load_image(GetDataPath()+"data/gfx/menu/help.png");
}

Help::~Help()
{
}

void Help::handle_events()
{
		if ( event.type == SDL_KEYUP || event.type == SDL_MOUSEBUTTONUP )
		{
			next_state(STATE_MENU);
		}

		if ( event.type == SDL_QUIT )
		{
			next_state(STATE_EXIT);
		}

		/*if ( event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_ESCAPE )
		{
			next_state(STATE_MENU);
		}*/
}

void Help::logic()
{

}


void Help::render()
{
	apply_surface( 0, 0, s_help, screen, NULL);
}

static bool m_sound, m_fullscreen;

Options::Options()
{
	s_options = load_image(GetDataPath()+"data/gfx/menu/options.png");
	
	
	//OPTIONS menu
	//create GUI (test only)
	GUIObject* obj;
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}
	GUIObjectRoot=new GUIObject(100,(SCREEN_HEIGHT-400)/2 + 50,600,350,GUIObjectFrame,"");
	
	GUIObject *soundCheck=new GUIObject(50,50,240,36,GUIObjectCheckBox,"Sound",m_sound?1:0);
	soundCheck->Name="chkSound";
	soundCheck->EventCallback=this;
	GUIObjectRoot->ChildControls.push_back(soundCheck);
		
	GUIObject *fullscreenCheck=new GUIObject(50,100,240,36,GUIObjectCheckBox,"Fullscreen",m_fullscreen?1:0);
	fullscreenCheck->Name="chkFullscreen";
	fullscreenCheck->EventCallback=this;
	GUIObjectRoot->ChildControls.push_back(fullscreenCheck);
		
	GUIObject *cancel=new GUIObject(10,300,284,36,GUIObjectButton,"Cancel");
	cancel->Name="cmdExit";
	cancel->EventCallback=this;
	GUIObjectRoot->ChildControls.push_back(cancel);
		
	GUIObject *save=new GUIObject(306,300,284,36,GUIObjectButton,"Save");
	save->Name="cmdSave";
	save->EventCallback=this;
	GUIObjectRoot->ChildControls.push_back(save);

	//======
}

Options::~Options()
{
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}
}

void Options::GUIEventCallback_OnEvent(std::string Name,GUIObject* obj,int nEventType){
	if(nEventType==GUIEventClick){
		if(Name=="cmdExit"){
			next_state(STATE_MENU);
		}
		else if(Name=="cmdSave"){
			save_settings();
		}
		else if(Name=="chkSound"){
			m_sound=obj->Value?true:false;
			if ( !m_sound )
			{
				Mix_HaltMusic();
			}
			else 
			{
				Mix_PlayMusic(music,-1);
			}
		}
		else if(Name=="chkFullscreen"){
			m_fullscreen=obj->Value?true:false;
		}
	}
}

void Options::handle_events()
{
	if ( event.type == SDL_QUIT )
	{
		next_state(STATE_EXIT);
	}

	if (event.key.keysym.sym == SDLK_ESCAPE )
	{
		next_state(STATE_MENU);
	}
}

void Options::logic()
{

}


void Options::render()
{
	apply_surface( 0, 0, s_options, screen, NULL);
	if(GUIObjectRoot)
		GUIObjectRoot->render();
}

