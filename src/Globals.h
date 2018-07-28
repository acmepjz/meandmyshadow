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

#ifndef GLOBALS_H
#define GLOBALS_H

#include <SDL.h>
#include <string>
#include "libs/tinygettext/tinygettext.hpp"
#include "LevelPack.h"
#include "Render.h"

#if defined (WIN32) || defined (__APPLE__)
//#define DATA_PATH
#else
#include "config.h"
#endif

#define TITLE_FONT_RAISE 19
#define GUI_FONT_RAISE 5

class GameState;

//Global constants
//The width of the screen.
extern int SCREEN_WIDTH;
//The height of the screen.
extern int SCREEN_HEIGHT;
//The depth of the screen.
#if defined(ANDROID)
//TODO: change other surface creating code to make the game runs faster
const int SCREEN_BPP=16; //??? 24?? 32??
//const int SCREEN_FLAGS=SDL_HWSURFACE;
#else
const int SCREEN_BPP=32;
//const int SCREEN_FLAGS=SDL_HWSURFACE;
#endif
const int SCREEN_FLAGS = 0;

//SDL interprets each pixel as a 32-bit number,
// so our masks must depend on the endianness (byte order) of the machine.
//NOTE: We define them here so we only have to do it once.
/*#if SDL_BYTEORDER == SDL_BIG_ENDIAN
const Uint32 BMASK=0xFF000000;
const Uint32 GMASK=0x00FF0000;
const Uint32 RMASK=0x0000FF00;
const Uint32 AMASK=0x000000FF;
#else*/
// NOTE: Changed to ARGB for SDL2.
const Uint32 BMASK=0x000000FF;
const Uint32 GMASK=0x0000FF00;
const Uint32 RMASK=0x00FF0000;
const Uint32 AMASK=0xFF000000;
/*#endif*/

//String containing the version, used in the titelbar.
const std::string version="V0.5 Development version";

//The height of the current level.
extern int LEVEL_HEIGHT;
//The width of the current level.
extern int LEVEL_WIDTH;

//The target frames per seconds.
const int FPS=40;

//The language that in which the game should be translated.
extern std::string language;
//The DictionaryManager that is used to translate the game itself.
extern tinygettext::DictionaryManager* dictionaryManager;

//SDL Window and renderer
extern SDL_Window* sdlWindow;

//Font that is used for titles.
//Knewave large.
extern TTF_Font* fontTitle;
//Font that is used for captions of buttons and other GUI elements.
//Knewave small.
extern TTF_Font* fontGUI;
//Font that is used for long captions of buttons and other GUI elements.
//Knewave smaller.
extern TTF_Font* fontGUISmall;
//Font that is used for (long) text.
//Blokletter-Viltstift small.
extern TTF_Font* fontText;
//Font used for scripting editor.
//Monospace.
extern TTF_Font* fontMono;

//Small arrows used for GUI widgets.
//2 directions and 2 different/same colors depending on theme.
extern TexturePtr arrowLeft1;
extern TexturePtr arrowRight1;
extern TexturePtr arrowLeft2;
extern TexturePtr arrowRight2;

//Event, used for event handling.
extern SDL_Event event;

//GUI
class GUIObject;
extern GUIObject *GUIObjectRoot;

//The state id of the current state.
extern int stateID;
//Integer containing what the next state will be.
extern int nextState;
//The currentState.
extern GameState* currentState;

//Pointer to the current levelpack.
extern LevelPack* levels;
//String containing the name of the current level.
extern std::string levelName;

//SDL rectangle used to store the camera.
//x is the x location of the camera.
//y is the y location of the camera.
//w is the width of the camera. (equal to SCREEN_WIDTH)
//h is the height of the camera. (equal to SCREEN_HEIGHT)
extern SDL_Rect camera;

//Themable colors
const SDL_Color BLACK = SDL_Color{0,0,0,255};

//Enumeration containing the different cursor types there are.
enum CursorType{
	//The default pointer.
	CURSOR_POINTER,

	//The vertical ibeam, used to indicate text input.
	CURSOR_CARROT,

	//A closed hand, used for indicating a drag action.
	CURSOR_DRAG,

	//The different (window) size cursor icons.
	CURSOR_SIZE_HOR,
	CURSOR_SIZE_VER,
	CURSOR_SIZE_FDIAG,
	CURSOR_SIZE_BDIAG,
	
	//Remove cursor used in level editor
	CURSOR_REMOVE,

	//Pointing hand cursor, for hyperlinks.
	CURSOR_POINTING_HAND,

	//The number of cursor types there are.
	CURSOR_MAX
};
//Currently used cursor type.
extern CursorType currentCursor;
//Array containing the SDL_Cursors.
extern SDL_Cursor* cursors[CURSOR_MAX];

//Enumeration containing the ids of the game states.
enum GameStates{
	//State null is a special state used to indicate no state.
	//This is used when no next state is defined.
	STATE_NULL,

	//This state is before the actual leveleditor used to make levelpacks.
	STATE_LEVEL_EDIT_SELECT,
	//This state is for the level editor.
	STATE_LEVEL_EDITOR,
	//This state is for the main menu.
	STATE_MENU,
	//This state is for the actual game.
	STATE_GAME,
	//Special state used when exiting meandmyshadow.

	STATE_EXIT,
	//This state is for the help screen.

	STATE_LEVEL_SELECT,
	//This state is for the options screen.
	STATE_OPTIONS,
	//This state is for the addon screen.
	STATE_ADDONS,
	//This state is for credits screen
	STATE_CREDITS,
	//This state is for statistics screen
	STATE_STATISTICS,
};

//Enumeration containing the ids of the different block types.
enum GameTileType{
	//The normal solid block.
	TYPE_BLOCK=0,
	//Block representing the start location of the player.
	TYPE_START_PLAYER,
	//Block representing the start location of the shadow.
	TYPE_START_SHADOW,
	//The exit of the level.
	TYPE_EXIT,
	//The shadow block which is only solid for the shadow.
	TYPE_SHADOW_BLOCK,
	//Block that can kill both the player and the shadow.
	TYPE_SPIKES,

	//Special point where the player can save.
	TYPE_CHECKPOINT,
	//Block that will switch the location of the player and the shadow when invoked.
	TYPE_SWAP,
	//Block that will crumble to dust when stepped on it for the third time.
	TYPE_FRAGILE,

	//Normal block that moves along a path.
	TYPE_MOVING_BLOCK,
	//Shadow block that moves along a path.
	TYPE_MOVING_SHADOW_BLOCK,
	//A spike block that moves along a path.
	TYPE_MOVING_SPIKES,

	//Special block which, once entered, moves the player/shadow to a different portal.
	TYPE_PORTAL,
	//A block with a button which can activate or stop moving blocks, converyor belts
	TYPE_BUTTON,
	//A switch which can activate or stop moving blocks, converyor belts
	TYPE_SWITCH,

	//Solid block which works like
	TYPE_CONVEYOR_BELT,
	TYPE_SHADOW_CONVEYOR_BELT,

	//Block that contains a message that can be read.
	TYPE_NOTIFICATION_BLOCK,

	//A collectable that is able to open locked doors
	TYPE_COLLECTABLE,

	//Block that can be pushed by the player and the shadow.
	//Pushable blocks can push other pushable blocks.
	TYPE_PUSHABLE,

	//The (max) number of tiles.
	TYPE_MAX
};

#endif
