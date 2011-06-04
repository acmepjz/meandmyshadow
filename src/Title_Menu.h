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
#ifndef TITLE_MENU_H
#define TITLE_MENU_H

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

class Menu : public GameState
{
private:

	SDL_Surface * s_menu;

public:

	Menu();
	~Menu();

	void handle_events();
	void logic();
	void render();
};

class Help : public GameState
{
private:
	SDL_Surface * s_help;

public:

	Help();
	~Help();

	void handle_events();
	void logic();
	void render();
};

class Options : public GameState, private GUIEventCallback
{
private:
	SDL_Surface * s_options;

	void GUIEventCallback_OnEvent(std::string Name,GUIObject* obj,int nEventType);

public:
	Options();
	~Options();
	
	void handle_events();
	void logic();
	void render();
};

#endif