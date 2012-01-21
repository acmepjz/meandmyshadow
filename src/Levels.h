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
#include "Player.h"


class Levels{
private:
	//Index of the current level.
	int currentLevel;
	
	//A level entry structure.
	struct Level{
		//The name of the level.
		string name;
		//The filename of the level.
		string file;
		
		//Boolean if the level is locked.
		bool locked;
		//Boolean if the level is won.
		bool won;
	};
	
	//Boolean if the levels are loaded.
	bool loaded;

	//Vector containing the filenames of the levels.
	std::vector<Level> levels;
	
	//The file name of the level progress.
	std::string levelProgressFile;

public:
	//The location the levelpack is stored.
	std::string levelpackPath;
	//A description of the levelpack.
	std::string levelpackDescription;
	
	//The text that will be displayed when the levels are finished.
	std::string congratulationText;
	
	//Constructor.
	Levels():currentLevel(0),loaded(false),levels(){};

	//Adds a level to the levels.
	//levelFileName: The filename of the level to add.
	//level: The index of the level to add.
	void addLevel(const std::string& levelFileName,int levelno=-1);
	//Removes a level from the levels.
	//level: The index of the level to remove.
	void removeLevel(int level);
	//Swaps two level.
	//level1: The first level to swap.
	//level2: The second level to swap.
	void swapLevel(int level1,int level2);

	//Get the levelFile for a given level.
	//level: The level index to get the levelFileName from.
	//Returns: String containing the levelFileName.
	const std::string& getLevelFile(int level=-1);
	//Get the levelpackPath of the levels.
	//Returns: String containing the levelpackPath.
	const std::string& getLevelpackPath();
	//Get the levelName for a given level.
	//level: The level index to get the levelName from.
	//Returns: String containing the levelName.
	const std::string& getLevelName(int level=-1);
	//Sets the levelName for a given level.
	//level: The level index to get the levelName from.
	//name: The new name of the level.
	void setLevelName(int level,const std::string& name);

	//Get the currentLevel.
	//Returns: The currentLevel.
	inline int getLevel(){return currentLevel;}
	//Get the levelCount.
	//Returns: The level count.
	inline int getLevelCount(){return levels.size();}
	
	//Check if a certain level is locked.
	//level: The index of the level to check.
	//Returns: True if the level is locked.
	bool getLocked(int level);
	//Set the currentLevel.
	//level: The new current level.
	void setLevel(int level);
	//Set a level locked or not.
	//level: The level to (un)lock.
	//locked: The new status of the level, default is unlocked (false).
	void setLocked(int level,bool locked=false);

	//Empties the levels.
	void clear();
	
	
	bool loadLevels(const std::string& levelListFile);
	bool loadProgress(const std::string& levelProgressFile);
	void saveLevels(const std::string& levelListFile);
	void saveLevelProgress();

	void nextLevel();

};
#endif
