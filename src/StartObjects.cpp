#include "GameObjects.h"
#include "Functions.h"
#include "Globals.h"
#include "Classes.h"

StartObject::StartObject(int x, int y, class Player * player)
{
	box.x = x;
	box.y = y;
	box.w = 50;
	box.h = 50;

	i_type = TYPE_START_PLAYER;

	surface = load_image("data/gfx/playerstart.png");
	player->set_position(box.x, box.y);
	player->i_fx = box.x;
	player->i_fy = box.y;
}

void StartObject::show()
{
	if ( stateID == STATE_LEVEL_EDITOR )
	{
		apply_surface( box.x - camera.x, box.y - camera.y, surface, screen, NULL );
	}
}

StartObjectShadow::StartObjectShadow( int x, int y, Shadow * player )
{
	box.x = x;
	box.y = y;
	box.w = 50;
	box.h = 50;

	i_type = TYPE_START_SHADOW;

	surface = load_image("data/gfx/shadowstart.png");
	player->set_position(box.x, box.y);
	player->i_fx = box.x;
	player->i_fy = box.y;
}

void StartObjectShadow::show()
{
	if ( stateID == STATE_LEVEL_EDITOR )
	{
		apply_surface( box.x - camera.x, box.y - camera.y, surface, screen, NULL );
	}
}

Exit::Exit(int x, int y)
{
	box.x = x;
	box.y = y;
	box.w = 50;
	box.h = 50;

	surface = load_image("data/gfx/exit.png");
	i_type = TYPE_EXIT;
}

void Exit::show()
{
	if ( check_collision( box, camera ) == true )
	{
		apply_surface( box.x - camera.x, box.y - camera.y, surface, screen, NULL);
	}
}
