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

#include "LevelPackManager.h"
#include "LevelPack.h"
#include "FileManager.h"
#include <stdio.h>

void LevelPackManager::loadLevelPack(std::string path){
	//Load the levelpack.
	LevelPack* levelpack=new LevelPack();
	levelpack->loadLevels(path+"/levels.lst");
	
	//Check if the entry doesn't already exist.
	if(levelpacks.find(levelpack->levelpackPath)!=levelpacks.end()){
		cerr<<"WARNING: Levelpack entry \""+levelpack->levelpackPath+"\" already exist."<<endl;
		return;
	}
	
	//It doesn't exist so add it.
	levelpacks[levelpack->levelpackPath]=levelpack;

	//Check if it's the tutorial level pack.
	//FIXME: If the folder name contains "tutorial" then it doesn't work :|
	if(levelpack->type==MAIN
		&& levelpack->levelpackPath.find("tutorial")!=string::npos)
	{
		tutorialLevelPackPath=levelpack->levelpackPath;
	}
}

void LevelPackManager::addLevelPack(LevelPack* levelpack){
	//Check if the entry doesn't already exist.
	if(levelpacks.find(levelpack->levelpackPath)!=levelpacks.end()){
		cerr<<"WARNING: Levelpack entry \""+levelpack->levelpackPath+"\" already exist."<<endl;
		return;
	}
	
	//It doesn't exist so add it.
	levelpacks[levelpack->levelpackPath]=levelpack;
}

void LevelPackManager::removeLevelPack(std::string path){
	std::map<std::string,LevelPack*>::iterator it=levelpacks.find(path);
	
	//Check if the entry exists.
	if(it!=levelpacks.end()){
		levelpacks.erase(it);
	}else{
		cerr<<"WARNING: Levelpack entry \""+path+"\" doesn't exist."<<endl;
	}
}

LevelPack* LevelPackManager::getLevelPack(std::string path){
	std::map<std::string,LevelPack*>::iterator it=levelpacks.find(path);
	
	//Check if the entry exists.
	if(it!=levelpacks.end()){
		return it->second;
	}else{
		cerr<<"WARNING: Levelpack entry \""+path+"\" doesn't exist."<<endl;
		return NULL;
	}
}

vector<pair<string,string> > LevelPackManager::enumLevelPacks(int type){
	//The vector that will be returned.
	vector<pair<string,string> > v;
	
	//Now do the type dependent adding.
	switch(type){
		case ALL_PACKS:
		{
			std::map<std::string,LevelPack*>::iterator i;
			for(i=levelpacks.begin();i!=levelpacks.end();++i){
				//We add everything except the "Custom Levels" pack since that's also in "Levels".
				if(i->second->levelpackName!="Custom Levels")
					v.push_back(pair<string,string>(i->first,i->second->levelpackName));
			}
			break;
		}
		case CUSTOM_PACKS:
		{
			std::map<std::string,LevelPack*>::iterator i;
			for(i=levelpacks.begin();i!=levelpacks.end();++i){
				//Only add levelpacks that are of the custom type, one exception is the "Custom Levels" pack.
				if(i->second->type==CUSTOM || i->second->levelpackName=="Custom Levels")
					v.push_back(pair<string,string>(i->first,i->second->levelpackName));
			}
			break;
		}
	}
	
	//And return the vector.
	return v;
}

void LevelPackManager::updateLanguage(){
	std::map<std::string,LevelPack*>::iterator i;
	for(i=levelpacks.begin();i!=levelpacks.end();++i){
		i->second->updateLanguage();
	}
}

LevelPackManager::~LevelPackManager(){
	//We call destroy().
	destroy();
}

void LevelPackManager::destroy(){
	//Loop through the levelpacks and delete them.
	std::map<std::string,LevelPack*>::iterator i;
	for(i=levelpacks.begin();i!=levelpacks.end();++i){
		delete i->second;
	}
	levelpacks.clear();
	tutorialLevelPackPath.clear();
}
