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
#ifndef ADDONS_H
#define ADDONS_H

#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include <SDL/SDL_ttf.h>
#include <vector>
#include <string>
#include "GameState.h"
#include "GameObjects.h"
#include "Player.h"
#include "GUIObject.h"
#include "GUIListBox.h"
#include "LevelSelect.h"

//The addons menu.
class Addons: public GameState,public GUIEventCallback{
private:
	//An addon entry.
	struct Addon {
		//The name of the addon.
		string name;
		//The type of addon. (Level, Levelpack, Theme)
		string type;
		//The link to the file containing the addon.
		string file;
		//The name of the author.
		string author;
		
		//The latest version of the addon.
		int version;
		//The version that the user has installed.
		int installedVersion;
		
		//Boolean if the addon is installed.
		bool installed;
		//Boolean if the addon is upToDate. (installedVersion==version)
		bool upToDate;
	};

	//The background image.
	SDL_Surface* background;
	
	//Vector containing all the addons.
	std::vector<Addon>* addons;
	
	FILE *addon;
	
	//String that should contain the error when something fails.
	string error;
	
	//The type of addon that is currently selected.
	string type;
	//The addon that is selected.
	Addon* selected;
	
	GUIListBox* list;
	GUIObject* actionButton;
	
	enum Action{
		NONE, INSTALL, UNINSTALL, UPDATE
	};
	Action action;
public:
	//Constructor.
	Addons();
	//Destructor.
	~Addons();
	
	bool getAddonsList(FILE *file);
	void fillAddonList(std::vector<Addons::Addon> &list, TreeStorageNode &addons, TreeStorageNode &installed);
			
	void saveInstalledAddons();
	
	void updateActionButton();
	
	std::vector<std::string> addonsToList(const string &type);
	
	void handleEvents();
	void logic();
	void render();

	void GUIEventCallback_OnEvent(std::string Name,GUIObject* obj,int nEventType);
};

#endif