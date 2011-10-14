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
#include "Classes.h"
#include "GameObjects.h"
#include "Timer.h"
#include "Player.h"
#include "GUIObject.h"
#include "GUIListBox.h"
#include "LevelSelect.h"
#include <curl/curl.h>
#include <archive.h>
#include <archive_entry.h>

class Addons :public GameState,public GUIEventCallback
{
private:
 
	struct Addon {
		string name;
		string type;
		string file;
		string author;
		int version;
		int installed_version;
		bool installed;
		bool uptodate;
	};

	SDL_Surface * s_background;
	
	std::vector<Addon> * addons;
	
	FILE *addon;
	CURL *curl;
	
	//String that should contain the error when something fails.
	string error;
	
	//The type of addon that is currently selected.
	string type;
	//The addon that is selected.
	Addon *selected;
	
	GUIListBox * list;
	GUIObject * actionButton;
	
	enum Action
	{
		NONE, INSTALL, UNINSTALL, UPDATE
	};
	Action action;
public:

	Addons();
	~Addons();
	
	static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream);

	bool get_addons_list(FILE *file);
	void fill_addon_list(std::vector<Addons::Addon> &list, TreeStorageNode &addons, TreeStorageNode &installed);
	void download_file(const string &path, const string &destination);
	void extract_file(const string &path, const string &destination);
	void copyData(archive *file, archive *dest);
	int removeDirectory(const char *path);
	
	void saveInstalledAddons();
	
	void update_actionButton();
	
	std::vector<std::string> addons_to_list(const string &type);
	
	void handle_events();
	void logic();
	void render();

	void check_mouse();

	void GUIEventCallback_OnEvent(std::string Name,GUIObject* obj,int nEventType);
};

#endif