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
#include "GameState.h"
#include "GameObjects.h"
#include "Player.h"
#include "Shadow.h"

//This structure contains variables that make a GameObjectEvent.
struct typeGameObjectEvent{
	//The type of event.
	int eventType;
	//The type of object that should react to the event.
	int objectType;
	//Flags, 0x1 means use the id.
	int flags; 
	//Blocks with this id should react to the event.
	std::string id;
};

class ThemeManager;
class ThemeBackground;

class Game : public GameState{
private:
	//Boolean if the game should reset.
	bool isReset;

protected:
	//Array containing "tooltips" for certain block types.
	//It will be shown in the topleft corner of the screen.
	SDL_Surface* bmTips[TYPE_MAX];
	
	//SDL_Surface containing the action images (record, play, etc..)
	SDL_Surface* action;

	//Vector containing all the levelObjects in the current game.
	std::vector<GameObject*> levelObjects;

	//The name of the current level.
	std::string levelName;
	//The path + file of the current level.
	std::string levelFile;

	//Editor data containing information like name, size, etc...
	std::map<std::string,std::string> editorData;

	//Vector used to queue the gameObjectEvents.
	std::vector<typeGameObjectEvent> eventQueue;

	//The themeManager.
	ThemeManager* customTheme;
	//The themeBackground.
	ThemeBackground* background;

public:
	//Array used to convert GameObject type->string.
	static const char* blockName[TYPE_MAX];
	//Map used to convert GameObject string->type.
	static std::map<std::string,int> blockNameMap;

	//Integer containing the current tip index.
	int gameTipIndex;
	
	//Boolean if tab is pressed, this key is used to move the camera to the shadow.
	bool tab;

	//The player...
	Player player;
	//... and his shadow.
	Shadow shadow;

	//warning: weak reference only, may point to invalid location
	GameObject *objLastCheckPoint;

	//Constructor.
	//loadLevel: Boolean if the GameState should load the level.
	Game(bool loadLevel=true);
	//Destructor.
	//It will call destroy();
	~Game();

	//Method used to clean up the GameState.
	void destroy();

	//Inherited from GameState.
	void handleEvents();
	void logic();
	void render();
	
	//This method will load a level.
	//fileName: The fileName of the level.
	virtual void loadLevel(std::string fileName);

	//Method used to broadcast a GameObjectEvent.
	//eventType: The type of event.
	//objectType: The type of object that should react to the event.
	//id: The id of the blocks that should react.
	void broadcastObjectEvent(int eventType,int objectType=-1,const char* id=NULL);

	//Method used to store the current state.
	//This is used for checkpoints.
	//Returns: True if it succeeds without problems.
	bool saveState();
	//Method used to load the stored state.
	//This is used for checkpoints.
	//Returns: True if it succeeds without problems.
	bool loadState();
	//Method that will reset the GameState to it's initial state.
	//save: Boolean if the saved state should also be delted.
	void reset(bool save);
};

#endif