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
#ifndef LEVELPACKMANAGER_H
#define LEVELPACKMANAGER_H

#include "LevelPack.h"
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <string>
#include <map>
#include <vector>

//Class for loading and managing levelpacks.
class LevelPackManager{
public:
	//Constructor.
	LevelPackManager(){}
	//Destructor.
	~LevelPackManager();
	
	//Load a levelpack and add it to the map.
	//name: The name of the levelpack files.
	void loadLevelPack(std::string name);
	
	//Insert a levelpack in the LevelPackManager.
	//levelpack: Pointer to the levelpack to add.
	void addLevelPack(LevelPack* levelpack);
	
	//Removes a levelpack from the LevelPackManager.
	//name: The name of the levelpack to remove.
	void removeLevelPack(std::string name);
	
	//Method that will return a levelpack.
	//name: The name of the levelpack.
	//Returns: Pointer to the requested levelpack.
	LevelPack* getLevelPack(std::string name);
	
	//Method that will return a vector containing all (or a subset) of the levelpacks.
	//type: The list type, default is ALL_PACKS.
	std::vector<std::string> enumLevelPacks(int type=ALL_PACKS);
	
	//Destroys the levelpacks.
	void destroy();
	
	//Enumeration containing the different types of levelpack lists.
	enum LevelPackLists
	{
		//This list contains every levelpack.
		ALL_PACKS,
		//This list contains all the custom levelpacks (and Levels).
		CUSTOM_PACKS
		
	};
private:
	//Map containing the levelpacks.
	//The key is the name of the levelpack and the value is a pointer to the levelpack.
	std::map<std::string,LevelPack*> levelpacks;
};

#endif
