#include <SDL/SDL.h>
#include "Timer.h"

void Timer::start()
{
	ticks = SDL_GetTicks();
}

int Timer::get_ticks()
{
	return ( SDL_GetTicks() - ticks );
}
