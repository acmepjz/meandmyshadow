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
#ifndef GLOBALS_H
#define GLOBALS_H

#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include <SDL/SDL_ttf.h>

//Global constants
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int SCREEN_BPP = 32;
extern int LEVEL_HEIGHT;
extern int LEVEL_WIDTH;

extern bool NEXT_LEVEL;

//Player saving speed
extern int PLAYER_X_SPEED;

const int g_FPS = 40;

//Slike
extern SDL_Surface * screen;
extern SDL_Surface * s_dark_block;
extern SDL_Surface * s_black;

extern Mix_Music * music;

extern TTF_Font *font;

//Event
extern SDL_Event event;

//Game states
extern int stateID;
extern int nextState;

extern SDL_Rect camera;

enum GameStates
{
	STATE_NULL, STATE_LEVEL_EDITOR, STATE_MENU, STATE_GAME, STATE_EXIT, STATE_HELP, STATE_LEVEL_SELECT };


const int TYPE_BLOCK = 0;
const int TYPE_START_PLAYER = 1;
const int TYPE_START_SHADOW = 2;
const int TYPE_EXIT = 3;  
const int TYPE_SHADOW_BLOCK = 4;
const int TYPE_SPIKES = 5;

#endif
