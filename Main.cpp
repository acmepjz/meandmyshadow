#include <SDL/SDL.h>
#include "Functions.h"
#include "Timer.h"
#include "Objects.h"
#include "Globals.h"
#include <cstdlib>
#include <ctime>

int main ( int argc, char * args[] )
{
	if ( init() == false )
	{
		return 1;
	}

	if ( load_files() == false )
	{
		return 1;
	}

	//IGRA/////
	stateID = STATE_INTRO;
	currentState = new Title();

	//////LEVEL EDITOR////////
	/*stateID = STATE_LEVEL_EDITOR;
	currentState = new LevelEditor();*/

	delta.start();

	srand(time(NULL));

	Mix_PlayMusic(music, -1);

	while ( stateID != STATE_EXIT)
	{
		FPS.start();

		currentState->handle_events();

		currentState->logic();

		delta.start();

		set_camera();

		currentState->render();

		change_state();

		if ( FPS.get_ticks() < 1000 / g_FPS )
		{
			SDL_Delay( ( 1000 / g_FPS ) - FPS.get_ticks() );
		}

	}

	o_mylevels.save_levels();

	clean();
	return 0;
}
