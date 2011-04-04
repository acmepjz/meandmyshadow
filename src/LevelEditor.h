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
#ifndef LEVELEDITOR_H
#define LEVELEDITOR_H

#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include <SDL/SDL_ttf.h>
#include <vector>
#include <string>
#include "Classes.h"
#include "GameObjects.h"
#include "Player.h"
#include "Game.h"
#include "GUIObject.h"

class LevelEditor : public Game, private GUIEventCallback
{
private:

	SDL_Surface * s_blocks[TYPE_MAX+1];

	//std::vector<SDL_Rect> grid;

	int i_current_type;

	int i_current_object;

	std::string LevelName;

	void GUIEventCallback_OnEvent(std::string Name,GUIObject* obj,int nEventType);

public:

	LevelEditor(bool bLoadLevel=true);
	~LevelEditor();

	void handle_events();
	void logic();
	void render();

	void switch_currentObject(int next);
	void put_object( std::vector<GameObject*> &LevelObjects );
	void delete_object( std::vector<GameObject*> &LevelObjects );
	void show_current_object();
	void save_level(std::string FileName);
};

#endif