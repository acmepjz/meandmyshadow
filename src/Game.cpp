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

Game::Game()
{
	background = load_image("data/gfx/background.png");

	load_level();
	o_mylevels.save_levels();
}

Game::~Game()
{
	SDL_FreeSurface(background);
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
		case TYPE_BLOCK:
			{
				levelObjects.push_back( new Block ( box.x, box.y ) );
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
		case TYPE_EXIT:
			{
				levelObjects.push_back( new Exit(box.x, box.y ) );
				break;
			}

		case TYPE_SHADOW_BLOCK:
			{
				levelObjects.push_back( new Block ( box.x, box.y, TYPE_SHADOW_BLOCK ) );
				break;
			}

		case TYPE_SPIKES:
			{
				levelObjects.push_back( new Block ( box.x, box.y, TYPE_SPIKES) );
				break;
			}
		}
	}
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


