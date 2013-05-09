/*
 * Copyright (C) 2011-2012 Me and My Shadow
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

#ifndef LEVELPACK_H
#define LEVELPACK_H

#ifdef __APPLE__
#include <SDL_mixer/SDL_mixer.h>
#include <SDL_ttf/SDL_ttf.h>
#else
#include <SDL/SDL_mixer.h>
#include <SDL/SDL_ttf.h>
#endif
#include <vector>
#include <string>
#include "GameObjects.h"
#include "Player.h"

#include "libs/tinygettext/tinygettext.hpp"

enum LevelPackType{
	//Main levelpacks are distibuted along with the game and located in the data path.
	MAIN,
	//Addon levelpacks are downloaded/added packs which reside in the user data path.
	ADDON,
	//Custom levelpacks are user made and are located in the 
	CUSTOM,

	//Collection levelpacks can contain levels from different locations.
	//This type is used for the Levels and Custom Levels levelpacks.
	//NOTE: The levelpackPath is ignored for these type of levelpacks since levels can be anywhere.
	COLLECTION
};

class LevelPack{
public:
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
		
		//Integer containing the number of ticks (40 = 1s) it took to finish the level.
		//If there's no time the value will be -1.
		int time;
		//Integer containing the target time to get a medal.
		int targetTime;
		
		//Integer containing the number of recordings used to finish the level.
		//When not won the value is -1.
		int recordings;
		//Integer containing the target recordings to get a medal.
		int targetRecordings;

		//MD5 of level node. :/
		unsigned char md5Digest[16];
	};
private:
	//Index of the current level.
	int currentLevel;
	
	//Boolean if the levels are loaded.
	bool loaded;

	//Vector containing the filenames of the levels.
	std::vector<Level> levels;
	
	//The file name of the level progress.
	std::string levelProgressFile;

public:
	//The name of the levelpack.
	std::string levelpackName;
	//The location the levelpack is stored.
	std::string levelpackPath;
	//A description of the levelpack.
	std::string levelpackDescription;

	//The type of levelpack.
	LevelPackType type;
	
	//The text that will be displayed when the levels are finished.
	std::string congratulationText;

	//The preferred music list to be used with this levelpack.
	std::string levelpackMusicList;
	
	//The dictionaryManager of the levelpack, used to translate strings.
	tinygettext::DictionaryManager* dictionaryManager;

	//Boolean if the levelpack has a custom theme/partial theme bundled with it.
	//NOTE: Themes can't be preloaded since they would be destroyed by the ThemeStack.
	bool customTheme;
	
	//Constructor.
	LevelPack();
	//Destructor.
	~LevelPack();

	//gettext function
	inline tinygettext::DictionaryManager* getDictionaryManager() const{
		return dictionaryManager;
	}

	//Method for updating the language to the configured one.
	//NOTE: This is called when changing the translation in the Options menu.
	void updateLanguage();

	//Adds a level to the levels.
	//levelFileName: The filename of the level to add.
	//level: The index of the level to add.
	void addLevel(const std::string& levelFileName,int levelno=-1);
	//Removes a level from the levels.
	//level: The index of the level to remove.
	void removeLevel(unsigned int level);
	//Moves the level to a given index.
	//level1: The level to move.
	//level2: The destination.
	void moveLevel(unsigned int level1,unsigned int level2);
	//Swaps two level.
	//level1: The first level to swap.
	//level2: The second level to swap.
	void swapLevel(unsigned int level1,unsigned int level2);

	//Get the levelFile for a given level.
	//level: The level index to get the levelFile from.
	//Returns: String containing the levelFileName (full path to the file).
	const std::string getLevelFile(int level=-1);
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
	void setLevelName(unsigned int level,const std::string& name);
	//Get the MD5 for a given level.
	//level: The level index.
	//Returns: const unsigned char[16] containing the digest.
	const unsigned char* getLevelMD5(int level=-1);

	//get level's auto-save record path,
	//using level's MD5, file name and other information.
	void getLevelAutoSaveRecordPath(int level,std::string &bestTimeFilePath,std::string &bestRecordingFilePath,bool createPath);

	//Method for getting the path to the progress file.
	//Returns: The path + filename to the progress file.
	std::string getLevelProgressPath();

	//Set the currentLevel.
	//level: The new current level.
	void setCurrentLevel(unsigned int level);
	//Get the currentLevel.
	//Returns: The currentLevel.
	inline int getCurrentLevel(){return currentLevel;}
	//Get the levelCount.
	//Returns: The level count.
	inline int getLevelCount(){return levels.size();}
	
	//Method that will return the requested level.
	//level: The index of the level, default is the current level.
	//Returns: Pointer to the requested level structure.
	struct Level* getLevel(int level=-1);
	
	//Method that will reset any progress/statistics for a given level.
	//level: The index of the level to reset, default is currentLevel.
	void resetLevel(int level=-1);
	
	//Check if a certain level is locked.
	//level: The index of the level to check.
	//Returns: True if the level is locked.
	bool getLocked(unsigned int level);
	//Set a level locked or not.
	//level: The level to (un)lock.
	//locked: The new status of the level, default is unlocked (false).
	void setLocked(unsigned int level,bool locked=false);

	//Empties the levels.
	void clear();
	
	
	bool loadLevels(const std::string& levelListFile);
	void loadProgress();
	void saveLevels(const std::string& levelListFile);
	void saveLevelProgress();

	void nextLevel();

};
#endif
