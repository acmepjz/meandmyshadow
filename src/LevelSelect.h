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
#ifndef LEVELSELECT_H
#define LEVELSELECT_H

#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include <SDL/SDL_ttf.h>
#include <vector>
#include <string>
#include "Classes.h"
#include "GameObjects.h"
#include "Timer.h"
#include "Player.h"
#include "GUIObject.h"

class Number
{
private:
	SDL_Surface * s_level;
	SDL_Surface * s_image;
	int number;

public:
	SDL_Rect myBox;


	Number();
	~Number();

	void init( int number, SDL_Rect box );
	void update_lock();
	void show( int dy );
};

class LevelSelect :public GameState,public GUIEventCallback
{
private:

	SDL_Surface * s_background;
	std::vector<class Number> o_number;

public:

	LevelSelect();
	~LevelSelect();

	void refresh();

	void handle_events();
	void logic();
	void render();

	void check_mouse();

	void GUIEventCallback_OnEvent(std::string Name,GUIObject* obj,int nEventType);
};

#endif