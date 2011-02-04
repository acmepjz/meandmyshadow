#include "Functions.h"
#include "Classes.h"
#include "Globals.h"

Menu::Menu()
{
	s_menu = load_image("data/gfx/menu.png");

	titleA = 1;

	play.x =  330; play.y =  350; play.w = 455 - 330; play.h = 405 - 350;
	level.x = 200; level.y = 444; level.w = 580 - 200; level.h = 500 - 444;
	exit.x = 325; exit.y = 540; exit.w = 450 - 325; exit.h = 590 - 540;

}

Menu::~Menu()
{
	SDL_FreeSurface(s_menu);
}

void Menu::handle_events()
{
	while ( SDL_PollEvent(&event) )
	{
		if ( event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT )
		{
			int x, y;

			SDL_GetMouseState(&x, &y);

			SDL_Rect mouse = { x, y, 5, 5 };

			if ( check_collision(play, mouse) == true )
			{
				next_state(STATE_HELP);
			}

			if ( check_collision(exit, mouse ) == true )
			{
				next_state(STATE_EXIT);
			}

			if ( check_collision(level, mouse ) == true )
			{
				next_state(STATE_LEVEL_SELECT);
			}

		}

		if ( event.type == SDL_QUIT )
		{
			next_state(STATE_EXIT);
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

void Menu::logic()
{
	titleA++;
	titleA++;
	titleA++;

	if ( titleA > 250 )
	{
		titleA = 250;
	}

	SDL_SetAlpha(s_menu, SDL_SRCALPHA, titleA);
}

void Menu::render()
{
	apply_surface ( 0 ,0,s_black, screen, NULL );
	apply_surface( 0,0,s_menu,screen,NULL );

	SDL_Flip(screen);
}

Help::Help()
{
	s_help = load_image("data/gfx/help.png");
	alfa = 0;
}

Help::~Help()
{
	SDL_FreeSurface(s_help);
}

void Help::handle_events()
{
	while ( SDL_PollEvent(&event) )
	{
		if ( event.type == SDL_KEYUP )
		{
			next_state(STATE_GAME);
		}

		if ( event.type == SDL_QUIT )
		{
			next_state(STATE_EXIT);
		}

		if ( event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_ESCAPE )
		{
			next_state(STATE_MENU);
		}
	}
}

void Help::logic()
{
	alfa = alfa + 3;


	if ( alfa > 250 )
	{
		alfa = 250;
	}

	SDL_SetAlpha(s_help, SDL_SRCALPHA, alfa);
}


void Help::render()
{
	apply_surface ( 0 ,0,s_black, screen, NULL );
	apply_surface( 0, 0, s_help, screen, NULL);

	SDL_Flip(screen);
}

