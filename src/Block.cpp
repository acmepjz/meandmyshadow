#include "GameObjects.h"
#include "Functions.h"
#include "Globals.h"
#include <iostream>
#include <cstdlib>

Block::Block( int x, int y, int type )
{
	box.x = x; box.y = y;
	box.w = 50; box.h = 50;
	
	if ( type == TYPE_BLOCK )
	{
		switch ( rand() % 10 )
		{
		case 0:
			surface = load_image("data/gfx/block2.png");
			break;

		case 1:
			surface = load_image("data/gfx/block3.png");
			break;

		default:
			surface = load_image("data/gfx/block.png");
			break;
		}

		i_type = TYPE_BLOCK;
	}
	else if ( type == TYPE_SHADOW_BLOCK )
	{
		surface = load_image("data/gfx/shadowblock.png");
		i_type = TYPE_SHADOW_BLOCK;
	}
	
	else if ( type == TYPE_SPIKES )
	{
		surface = load_image("data/gfx/spikes.png");
		i_type = TYPE_SPIKES;
	}
}

Block::~Block()
{
	SDL_FreeSurface(surface);
}

void Block::show()
{
	if ( check_collision(camera, box) == true )
	{
		apply_surface( box.x - camera.x, box.y - camera.y, surface, screen, NULL ); 
	}
}


