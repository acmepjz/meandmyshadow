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
#include <sstream>
#include <fstream>
#include <iostream>
#include <vector>
using namespace std;

Game::Game():o_player(this),o_shadow(this),objLastCheckPoint(NULL),objLastCheckPoint_1(NULL)
{
	background = load_image("data/gfx/background.png");

	load_level();
	o_mylevels.save_levels();
}

Game::~Game()
{
	SDL_FreeSurface(background);

	for(unsigned int i=0;i<levelObjects.size();i++) delete levelObjects[i];
	levelObjects.clear();
}

void Game::load_level()
{
	std::stringstream levelname;

	levelname << "data/level/" << o_mylevels.give_level_name().c_str();

	std::ifstream load ( levelname.str().c_str() );

	cout << levelname.str().c_str() << endl;

	SDL_Rect box;

	load >> LEVEL_WIDTH;
	load >> LEVEL_HEIGHT;

	while ( !(load.eof()) )
	{
		int objectType = -1;

		load >> objectType;

		load >> box.x;
		load >> box.y;

		switch ( objectType )
		{
		default:
			{
				levelObjects.push_back( new Block ( box.x, box.y, objectType ) );
				break;
			}
		case TYPE_START_PLAYER:
			{
				levelObjects.push_back( new StartObject( box.x, box.y, &o_player ) );
				break;
			}
		case TYPE_START_SHADOW:
			{
				levelObjects.push_back( new StartObjectShadow( box.x, box.y, &o_shadow ) );
				break;
			}
		}
	}

	for(unsigned int i=0;i<levelObjects.size();i++) levelObjects[i]->m_objParent=this;
}


/////////////EVENT///////////////

void Game::handle_events()
{
	if ( SDL_PollEvent(&event) )
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
		}
}

/////////////////LOGIC///////////////////
void Game::logic()
{
	o_player.shadow_set_state();
	o_player.shadow_give_state(&o_shadow);
	o_player.jump();
	o_player.move(levelObjects);
	o_player.other_check(&o_shadow);
	o_player.set_mycamera();
	o_player.reset();
	

	o_shadow.move_logic();
	o_shadow.jump();
	o_shadow.move(levelObjects);
	o_shadow.other_check(&o_player);
	o_shadow.reset();

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
	

	SDL_Flip(screen);
}


//new
bool Game::save_state(){
	if(!o_player.b_dead && !o_shadow.b_dead){
		o_player.save_state();
		o_shadow.save_state();
		//TODO:save other state, for example moving blocks
		return true;
	}
	return false;
}

void Game::load_state(){
	o_player.load_state();
	o_shadow.load_state();
	//TODO:load other state, for example moving blocks
}