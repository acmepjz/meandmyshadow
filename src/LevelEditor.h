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
#include <map>
#include <string>
#include "GameState.h"
#include "GameObjects.h"
#include "Player.h"
#include "Game.h"
#include "GUIObject.h"

class LevelEditor : public Game, private GUIEventCallback{
private:
	int currentType;
	void GUIEventCallback_OnEvent(std::string Name,GUIObject* obj,int nEventType);
public:
	//Constructor.
	LevelEditor();
	//Destructor.
	~LevelEditor();

	//Inherited from Game(State).
	void handleEvents();
	void logic();
	void render();

	void putObject();
	void deleteObject();
	void copyObject(bool bDelete);
	void editObject();
	void showCurrentObject();
	void saveLevel(std::string fileName);
};
#endif