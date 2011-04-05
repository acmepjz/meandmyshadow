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

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_mixer.h>
#include <string>
#include "Globals.h"
#include "Objects.h"
#include "Player.h"
#include "GameObjects.h"
#include "Timer.h"
#include "Levels.h"
#include "Title_Menu.h"
#include "LevelEditor.h"
#include "Game.h"
#include "LevelSelect.h"
#include "ImageManager.h"
using namespace std;

ImageManager m_objImageManager;

SDL_Surface * load_image ( string file )
{
	return m_objImageManager.load_image(file);
}

void apply_surface ( int x, int y, SDL_Surface * src, SDL_Surface * dst, SDL_Rect * clip )
{
	SDL_Rect offset;
	offset.x = x; offset.y = y;

	SDL_BlitSurface ( src, clip, dst, &offset );
}

bool init()
{
	if ( SDL_Init(SDL_INIT_EVERYTHING) == -1 )
	{
		return false;
	}

	if ( Mix_OpenAudio( 22050, MIX_DEFAULT_FORMAT, 2 , 512 ) == -1 )
	{
		return false;
	}

	if ( TTF_Init() == -1 )
	{
		return false;
	}

	screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BPP, SDL_HWSURFACE | SDL_DOUBLEBUF /*|SDL_FULLSCREEN*/ );

	if ( screen == NULL )
	{
		return false;
	}


	SDL_WM_SetCaption("Me and my shadow", NULL );
	SDL_EnableUNICODE(1);

	return true;
}

bool load_files()
{
	s_dark_block = load_image("data/gfx/dark.png");
	s_black = load_image("data/gfx/black.png");
	music = Mix_LoadMUS("data/sfx/music.mid");
	font = TTF_OpenFont("data/font/ComicBook.ttf", 24);

	return true;
}

void clean()
{
	delete currentState;

	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}
	m_objImageManager.Destroy();
	SDL_Quit();
	Mix_CloseAudio();
}

void next_state( int newstate )
{
	if ( nextState != STATE_EXIT )
	{
		nextState = newstate;
	}
}

void change_state()
{
	if ( nextState != STATE_NULL )
	{
		if ( nextState != STATE_EXIT )
		{
			delete currentState;
		}

		stateID = nextState;
		nextState = STATE_NULL;

		switch ( stateID )
		{
		case STATE_GAME:
			{
				currentState = new Game();
				break;
			}

		case STATE_MENU:
			{
				currentState = new Menu();
				break;
			}

		case STATE_HELP:
			{
				currentState = new Help();
				break;
			}
		case STATE_LEVEL_SELECT:
			{
				currentState = new LevelSelect();
				break;
			}
		case STATE_LEVEL_EDITOR:
			{
				currentState = new LevelEditor(m_sLevelName.c_str());
				break;
			}
		}

		//fade
		SDL_Surface *s_temp = SDL_CreateRGBSurface(SDL_HWSURFACE|SDL_SRCALPHA,
			screen->w,screen->h,screen->format->BitsPerPixel,
			screen->format->Rmask,screen->format->Gmask,screen->format->Bmask,0);
		SDL_BlitSurface(screen,NULL,s_temp,NULL);
		int i;
		for(i=255;i>=0;i-=17)
		{
			SDL_FillRect(screen,NULL,0);
			SDL_SetAlpha(s_temp, SDL_SRCALPHA, i);
			SDL_BlitSurface(s_temp,NULL,screen,NULL);
			SDL_Flip(screen);
			SDL_Delay(20);
		}
		SDL_FreeSurface(s_temp);


	}
}

bool check_collision( SDL_Rect A, SDL_Rect B )
{
	if ( A.x >= B.x + B.w )
	{
		return false;
	}

	if ( A.x + A.w <= B.x )
	{
		return false;
	}

	if ( A.y >= B.y + B.h )
	{
		return false;
	}

	if ( A.y + A.h <= B.y )
	{
		return false;
	}

	return true;
}

void set_camera()
{
	if ( stateID == STATE_LEVEL_EDITOR )
	{
		int x, y;

		SDL_GetMouseState(&x,&y);

		x += camera.x;

		if ( x > camera.x + 950 )
		{
			camera.x += 10;
		}

		if ( x < camera.x + 100 )
		{
			camera.x -= 10;
		}

		if ( camera.x < 0 )
		{
			camera.x = 0;
		}

		if ( camera.x + camera.w > 2500 )
		{
			camera.x = 2500 - camera.w;
		}

		y += camera.y;

		if ( y > camera.y + 700 )
		{
			camera.y += 10;
		}

		if ( y < camera.y + 100 )
		{
			camera.y -= 10;
		}

		if ( camera.y < 0 )
		{
			camera.y = 0;
		}

		if ( camera.y + camera.h > 2500 )
		{
			camera.y = 2500 - camera.h;
		}
	}
}
	
