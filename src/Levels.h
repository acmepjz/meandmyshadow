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
#ifndef LEVELS_H
#define LEVELS_H

#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include <SDL/SDL_ttf.h>
#include <vector>
#include <string>
#include "GameObjects.h"
#include "Timer.h"
#include "Player.h"


class Level
{
private:
	int i_level_count;
	int i_current_level;
	bool m_bLoaded;

	std::vector<std::string> level_files,level_name;
	std::vector<bool> level_locked;
	std::string m_sLevelProgressFile;

public:

	std::string LevelPackName;
	//Boolean if the levels are located in the UserPath.
	bool m_bAddon;

	Level():i_level_count(0),i_current_level(0),m_bLoaded(false),m_bAddon(false){};

	void add_level(const std::string& level_file_name,const std::string& level_name,int level=-1);
	void remove_level(int lvl);
	void swap_level(int lvl1,int lvl2);

	const std::string& get_level_file(int level=-1);
	const std::string& get_level_name(int level=-1);
	void set_level_name(int lvl,const std::string& s);

	inline int get_level(){return i_current_level;}
	inline int get_level_count(){return i_level_count;}
	bool get_locked( int level );
	void set_level(int lvl);
	void set_locked(int lvl,bool bLocked=false);

	void clear();
	bool load_levels(const std::string& level_list_file,const std::string& level_progress_file);
	void save_levels(const std::string& level_list_file);
	void save_level_progress();

	void next_level();

};

#endif