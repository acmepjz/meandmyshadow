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
#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include <SDL/SDL_ttf.h>
#include "Globals.h"

//Globals
int LEVEL_HEIGHT = 0;
int LEVEL_WIDTH = 0;

bool NEXT_LEVEL = false;

//int PLAYER_X_SPEED = 0;

Mix_Music * music = NULL;

//SLike
SDL_Surface * screen = NULL;
SDL_Surface * s_dark_block = NULL;
SDL_Surface * s_black = NULL;
SDL_Surface * s_temp = NULL;

TTF_Font *font = NULL, *font_small = NULL;

//Game states
int stateID = STATE_NULL;
int nextState = STATE_NULL;

std::string m_sLevelName;

SDL_Rect camera = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };

SDL_Event event;
