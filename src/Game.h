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
#ifndef GAME_H
#define GAME_H

#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include <SDL/SDL_ttf.h>
#include <vector>
#include <map>
#include <string>
#include "Classes.h"
#include "GameObjects.h"
#include "Player.h"
#include "Shadow.h"

class Game : public GameState
{
private:
	bool b_reset;

protected:

	SDL_Surface *background,*bmTips[TYPE_MAX];

	std::vector<GameObject*> levelObjects;

	std::string LevelName;

	std::map<std::string,std::string> EditorData;

public:

	static const char* g_sBlockName[TYPE_MAX];
	static std::map<std::string,int> g_BlockNameMap;

	int GameTipIndex;

	Player o_player;
	Shadow o_shadow;

	//warning: weak reference only, may point to invalid location
	GameObject *objLastCheckPoint;

	Game(bool bLoadLevel=true);
	~Game();

	void Destroy();

	void handle_events();
	void logic();
	void render();
	
	virtual void load_level(std::string FileName);

	void BroadcastObjectEvent(int nEventType,int nObjectType=-1,const char* id=NULL);

	//new
	bool save_state();
	bool load_state();
	void reset();
	//end
};

#endif