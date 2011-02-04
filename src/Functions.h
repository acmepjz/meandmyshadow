#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <SDL/SDL.h>
#include <string>

SDL_Surface * load_image( std::string file );

void apply_surface ( int x, int y, SDL_Surface * src, SDL_Surface * dst, SDL_Rect *clip );

bool init();

bool load_files();

void clean();

void next_state ( int newstate );

bool check_collision( SDL_Rect A, SDL_Rect B );

void change_state();

void set_camera();


#endif

