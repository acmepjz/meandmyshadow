/*
 * Copyright (C) 2012 Me and My Shadow
 *
 * This file is part of Me and My Shadow.
 *
 * Me and My Shadow is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Me and My Shadow is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Me and My Shadow.  If not, see <http://www.gnu.org/licenses/>.
 */

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

	//Method that will update the translation of the levelpacks.
	//NOTE: This is called when changing the language in the Options menu.
	void updateLanguage();
	
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
