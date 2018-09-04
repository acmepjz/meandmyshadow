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
#include <string>
#include <map>
#include <vector>

//Builtin special level pack path which means collection of all individual levels.
//This contains question mark '?' which should be invalid for regular path,
//so it's unlikely that it will clash with other level pack paths.
#define LEVELS_PATH "?Levels/"

//Builtin special level pack path which means collection of all custom levels which can be edited in level editor.
//This contains question mark '?' which should be invalid for regular path,
//so it's unlikely that it will clash with other level pack paths.
#define CUSTOM_LEVELS_PATH "?Custom Levels/"

//Class for loading and managing levelpacks.
class LevelPackManager{
public:
	//Constructor.
	LevelPackManager(){}
	//Destructor.
	~LevelPackManager();
	
	//Load a levelpack and add it to the map.
	//path: The path of the levelpack files.
	void loadLevelPack(std::string path);
	
	//Insert a levelpack in the LevelPackManager.
	//levelpack: Pointer to the levelpack to add.
	void addLevelPack(LevelPack* levelpack);
	
	//Removes a levelpack from the LevelPackManager.
	//path: The path to the levelpack to remove.
	//del: If the corresponding LevelPack should also be deleted.
	void removeLevelPack(std::string path, bool del);
	
	//Method that will return a levelpack.
	//path: The path to the levelpack.
	//Returns: Pointer to the requested levelpack.
	LevelPack* getLevelPack(std::string path);
	
	//Method that will return a vector containing all (or a subset) of the levelpacks.
	//type: The list type, default is ALL_PACKS.
	std::vector<std::pair<std::string,std::string> > enumLevelPacks(int type=ALL_PACKS);

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

	//The tutorial level pack path.
	std::string tutorialLevelPackPath;

	//Get the tutorial level pack.
	LevelPack* getTutorialLevelPack(){
		return getLevelPack(tutorialLevelPackPath);
	};
private:
	//Map containing the levelpacks.
	//The key is the path to the levelpack and the value is a pointer to the levelpack.
	std::map<std::string,LevelPack*> levelpacks;
};

#endif
