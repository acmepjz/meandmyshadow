/*
 * Copyright (C) 2011-2013 Me and My Shadow
 *
 * This file is part of Me and My Shadow.
 *
 * Me and My Shadow is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Me and My Shadow is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Me and My Shadow.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Globals.h"
#ifdef __APPLE__
#include <SDL_mixer/SDL_mixer.h>
#include <SDL_ttf/SDL_ttf.h>
#else
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#endif
#include "libs/tinygettext/tinygettext.hpp"

//Set the defautl value for the screen width and height.
int SCREEN_WIDTH=800;
int SCREEN_HEIGHT=600;

//Set the default value for the level width and height.
int LEVEL_HEIGHT=0;
int LEVEL_WIDTH=0;

//The language that in which the game should be translated.
std::string language;
//The DictionaryManager that is used to translate the game itself.
tinygettext::DictionaryManager* dictionaryManager=NULL;

//SDL Window and renderer
SDL_Window* sdlWindow=NULL;

//Font that is used for titles.
//Knewave large.
TTF_Font* fontTitle=NULL;
//Font that is used for captions of buttons and other GUI elements.
//Knewave small.
TTF_Font* fontGUI=NULL;
//Font that is used for long captions of buttons and other GUI elements.
//Knewave small.
TTF_Font* fontGUISmall=NULL;
//Font that is used for (long) text.
//Blokletter-Viltstift small.
TTF_Font* fontText=NULL;
//Font used for scripting editor.
//Monospace.
TTF_Font* fontMono=NULL;

//Small arrows used for GUI widgets.
//2 directions and 2 different/same colors depending on theme.
TexturePtr arrowLeft1=nullptr;
TexturePtr arrowRight1=nullptr;
TexturePtr arrowLeft2=nullptr;
TexturePtr arrowRight2=nullptr;

//Set the current stateID and the nextState.
int stateID=STATE_NULL;
int nextState=STATE_NULL;
GameState* currentState=NULL;

LevelPack* levels=NULL;
//The name of the current level.
std::string levelName;

//Initialise the camera.
//Start location is 0, size is the same as the screen size.
SDL_Rect camera={0,0,SCREEN_WIDTH,SCREEN_HEIGHT};

//The SDL_Event object.
SDL_Event event;

//Themable colors
SDL_Color themeTextColor={0,0,0};
SDL_Color themeTextColorDialog={0,0,0};

//The current cursor type.
CursorType currentCursor=CURSOR_POINTER;
//Array containing the SDL_Cursors.
SDL_Cursor* cursors[CURSOR_MAX];
