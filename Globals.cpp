#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include <SDL/SDL_ttf.h>
#include "Globals.h"

//Globals
int LEVEL_HEIGHT = 0;
int LEVEL_WIDTH = 0;

bool NEXT_LEVEL = false;


int PLAYER_X_SPEED = 0;

Mix_Music * music = NULL;

//SLike
SDL_Surface * screen = NULL;
SDL_Surface * s_dark_block = NULL;
SDL_Surface * s_black = NULL;

TTF_Font *font = NULL;

//Game states
int stateID = STATE_NULL;
int nextState = STATE_NULL;

SDL_Rect camera = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };

SDL_Event event;
