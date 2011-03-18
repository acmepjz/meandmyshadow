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
#include <fstream>
#include <iostream>
#include <vector>
#include <SDL/SDL_mixer.h>
#include <string.h>
using namespace std;

LevelEditor::LevelEditor():o_player(NULL),o_shadow(NULL) //??? TODO:
{
	LEVEL_WIDTH = 2500;
	LEVEL_HEIGHT = 2500;

	test = load_image("data/gfx/test.png");

	memset(s_blocks,0,sizeof(s_blocks));
	s_blocks[TYPE_BLOCK] = load_image("data/gfx/blocks/block.png");
	s_blocks[TYPE_START_PLAYER] = load_image("data/gfx/blocks/playerstart.png");
	s_blocks[TYPE_START_SHADOW] = load_image("data/gfx/blocks/shadowstart.png");
	s_blocks[TYPE_EXIT] = load_image("data/gfx/blocks/exit.png");
	s_blocks[TYPE_SHADOW_BLOCK] = load_image("data/gfx/blocks/shadowblock.png");
	s_blocks[TYPE_SPIKES] = load_image("data/gfx/blocks/spikes.png");
	s_blocks[TYPE_CHECKPOINT] = load_image("data/gfx/blocks/checkpoint.png");
	
	for ( int x = 0, y = 0, g = 0; true; x += 50, g++ )
	{
		if ( x > LEVEL_WIDTH ) { x = 0; y += 50; }
		if ( y > LEVEL_HEIGHT ) { break; }
		grid.push_back(SDL_Rect());

		grid[g].x = x; 
		grid[g].y = y; 
		grid[g].w = 50; 
		grid[g].h = 50;
	}

	load_level();

	i_current_type = TYPE_BLOCK;
	i_current_object = 0;
}

LevelEditor::~LevelEditor()
{
	SDL_FreeSurface(test);

	for(int i=0;i<=TYPE_MAX;i++){
		if(s_blocks[i]) SDL_FreeSurface(s_blocks[i]);
	}

	grid.clear();

	save_level();

	for(unsigned int i=0;i<levelObjects.size();i++) delete levelObjects[i];
	levelObjects.clear();
}

void LevelEditor::put_object( std::vector<GameObject*> &levelObjects )
{
	int x, y;

	SDL_GetMouseState(&x, &y);

	SDL_Rect mouse; mouse.x = x + camera.x; mouse.y = y + camera.y; mouse.w = 5; mouse.h = 5;

	for ( int g = 0; g < (signed)grid.size(); g++ )
	{
		if ( check_collision( grid[g], mouse ) == true )
		{
			switch ( i_current_type )
			{
			default:
				{
					levelObjects.push_back( new Block(grid[g].x, grid[g].y, i_current_type));
					break;
				}

			case TYPE_START_PLAYER:
				{
					levelObjects.push_back( new StartObject( grid[g].x, grid[g].y, &o_player ) );
					break;
				}

			case TYPE_START_SHADOW:
				{
					levelObjects.push_back( new StartObjectShadow( grid[g].x, grid[g].y, &o_shadow) );
					break;
				}
			}
		}
	}
}

void LevelEditor::delete_object(std::vector<GameObject*> &LevelObjects)
{
	int x, y;

	SDL_GetMouseState(&x, &y);

	SDL_Rect mouse; mouse.x = x + camera.x; mouse.y = y + camera.y; mouse.w = 5; mouse.h = 5;

	for ( int o = 0; o < (signed)levelObjects.size(); o++ )
	{
		if ( check_collision( LevelObjects[o]->get_box(), mouse ) == true )
		{
			LevelObjects.erase(LevelObjects.begin()+o);
			//break;
		}
	}
}

void LevelEditor::save_level()
{
	std::ofstream save ( "leveledit.map" );

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

	if ( maxX > 800 ) {
		save << maxX << " ";}
	else { save << 800 << " "; }

	if ( maxY > 600 ){
		save << maxY << " ";}
	else { save << 600 << " ";}

	for ( int o = 0; o < (signed)levelObjects.size(); o++ )
	{
		save << levelObjects[o]->i_type << " ";

		SDL_Rect box = levelObjects[o]->get_box();

		save << box.x << " ";
		save << box.y << " ";
	}
}

void LevelEditor::load_level()
{
	std::ifstream load ( "leveledit.map" );
	if(!load) return;

	SDL_Rect box;

	load >> box.x;
	load >> box.y;

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
				levelObjects.push_back( new Block ( box.x, box.y, objectType) );
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
	if ( SDL_PollEvent(&event) )
		{
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
				levelObjects.clear();
			}

			if ( event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_ESCAPE )
			{
				next_state( STATE_MENU );
			}

			o_player.handle_input(&o_shadow);
			//o_shadow.handle_input();

			if ( event.type == SDL_QUIT )
			{
				next_state( STATE_EXIT );
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

////////////////LOGIC////////////////////
void LevelEditor::logic()
{
	o_player.shadow_set_state();
	o_player.shadow_give_state(&o_shadow);
	o_player.jump();
	o_player.move(levelObjects);
	o_player.other_check(&o_shadow);
	
	o_player.reset();
	

	o_shadow.move_logic();
	o_shadow.jump();
	o_shadow.move(levelObjects);
	o_shadow.other_check(&o_player);
	
	o_shadow.reset();
	

	set_camera();
}

void LevelEditor::show_current_object()
{
	//Za�asni grid prikaz/////////////////
	int x, y;

	SDL_GetMouseState(&x, &y);

	SDL_Rect mouse; mouse.x = x + camera.x; mouse.y = y + camera.y; mouse.w = 5; mouse.h = 5;

	for ( int g = 0; g < (signed)grid.size(); g++ )
	{
		if ( check_collision( grid[g], mouse ) == true )
		{
			if(i_current_type>=0 && i_current_type<=TYPE_MAX){
				apply_surface ( grid[g].x - camera.x, grid[g].y - camera.y , s_blocks[i_current_type], screen, NULL );
			}
		}
	}
	//////////////////////////

}

/////////////////RENDER//////////////////////
void LevelEditor::render()
{
	apply_surface( 0, 0, test, screen, NULL );

	show_current_object();

	for ( int o = 0; o < (signed)levelObjects.size(); o++ )
	{
		levelObjects[o]->show();
	}

	o_shadow.show();
	o_player.show();
	
	SDL_Flip(screen);
}
