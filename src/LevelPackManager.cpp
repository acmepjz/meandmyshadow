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

#include "LevelPackManager.h"
#include "LevelPack.h"
#include "FileManager.h"
#include <stdio.h>

void LevelPackManager::loadLevelPack(std::string path){
	//Load the levelpack.
	LevelPack* levelpack=new LevelPack();
	levelpack->loadLevels(path+"/levels.lst");
	
	//Check if the entry doesn't already exist.
	if(levelpacks.find(levelpack->levelpackName)!=levelpacks.end()){
		cerr<<"WARNING: Levelpack entry \""+levelpack->levelpackName+"\" already exist."<<endl;
		return;
	}
	
	//It doesn't exist so add it.
	levelpacks[levelpack->levelpackName]=levelpack;
}

void LevelPackManager::addLevelPack(LevelPack* levelpack){
	//Check if the entry doesn't already exist.
	if(levelpacks.find(levelpack->levelpackName)!=levelpacks.end()){
		cerr<<"WARNING: Levelpack entry \""+levelpack->levelpackName+"\" already exist."<<endl;
		return;
	}
	
	//It doesn't exist so add it.
	levelpacks[levelpack->levelpackName]=levelpack;
}

void LevelPackManager::removeLevelPack(std::string name){
	std::map<std::string,LevelPack*>::iterator it=levelpacks.find(name);
	
	//Check if the entry exists.
	if(it!=levelpacks.end()){
		levelpacks.erase(it);
	}else{
		cerr<<"WARNING: Levelpack entry \""+name+"\" doesn't exist."<<endl;
	}
}

LevelPack* LevelPackManager::getLevelPack(std::string name){
	return levelpacks[name];
}

vector<string> LevelPackManager::enumLevelPacks(int type){
	//The vector that will be returned.
	vector<string> v;
	
	//Now do the type dependent adding.
	switch(type){
		case ALL_PACKS:
		{
			std::map<std::string,LevelPack*>::iterator i;
			for(i=levelpacks.begin();i!=levelpacks.end();++i){
				//We add everything except the "Custom Levels" pack since that's also in "Levels".
				if(i->first!="Custom Levels")
					v.push_back(i->first);
			}
			break;
		}
		case CUSTOM_PACKS:
		{
			std::map<std::string,LevelPack*>::iterator i;
			for(i=levelpacks.begin();i!=levelpacks.end();++i){
				//Only add levelpacks that are under the custom folder OR if it's the "Custom Levels" levelpack.
				if(i->second->levelpackPath.find(getUserPath(USER_DATA)+"custom/")==0 || i->first=="Custom Levels"){
					v.push_back(i->first);
				}
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
}
