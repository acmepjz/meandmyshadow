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

#include "GameState.h"
#include "GameObjects.h"
#include "Player.h"
#include "Game.h"
#include "GUIObject.h"
#include <vector>
#include <map>
#include <string>
#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include <SDL/SDL_ttf.h>

//The LevelEditor state, it's based on the Game state.
class LevelEditor: public Game, private GUIEventCallback{
private:
	//The selected type of block.
	int currentType;
	
	//GUI event handling is done here.
	void GUIEventCallback_OnEvent(std::string name,GUIObject* obj,int eventType);
public:
	//Constructor.
	LevelEditor();
	//Destructor.
	~LevelEditor();

	//Places the currently selected type of block.
	void putObject();
	//Removes the block under the cursor.
	void deleteObject();
	//Method that copies a block.
	//shouldDelete: Boolean if the block that is being copied should be deleted.
	void copyObject(bool shouldDelete);
	
	//Opens a popup with the configurable elements of the block underneath the mouse.
	void editObject();
	
	//Renders the current type of block at the location of the mouse.
	void showCurrentObject();
	
	//Saves the level to disk.
	//fileName: The filename to save the level in.
	void saveLevel(std::string fileName);

	
	//Inherited from Game(State).
	void handleEvents();
	void logic();
	void render();
};
#endif