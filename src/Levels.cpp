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
#include "Levels.h"
#include "Functions.h"
#include "FileManager.h"
#include "TreeStorageNode.h"
#include "POASerializer.h"
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
using namespace std;

void Levels::clear(){
	currentLevel=0;
	loaded=false;
	levels.clear();
	levelpackDescription.clear();
	levelpackPath.clear();
	levelProgressFile.clear();
	congratulationText.clear();
}

bool Levels::loadLevels(const std::string& levelListFile,const std::string& levelProgressFile){
	//We're going to load a new levellist so first clean any existing levels.
	clear();

	//If the levelListFile is empty we have nothing to load so we return false.
	if(levelListFile.empty()){
		cerr<<"ERROR: No levellist file given."<<endl;
		return false;
	}
	
	//Process the levelListFile, create a new string since lecelListFile is constant.
	string levelListNew=levelListFile;
	levelpackPath=pathFromFileName(levelListNew);

	//Create two input streams, one for the levellist file and one for the levelprogress.
	ifstream level(levelListNew.c_str());
	ifstream levelProgress;
	if(!levelProgressFile.empty()){
		this->levelProgressFile=levelProgressFile;
		levelProgress.open(processFileName(this->levelProgressFile).c_str());
	}

	if(!level){
		cerr<<"ERROR: Can't load level list "<<levelListNew<<endl;
		return false;
	}
	
	//Load the level list file.
	TreeStorageNode obj;
	{
		POASerializer objSerializer;
		if(!objSerializer.readNode(level,&obj,true)){
			cerr<<"ERROR: Invalid file format of level list "<<levelListNew<<endl;
			return false;
		}
	}
	
	//Look for the description.
	{
		vector<string> &v=obj.attributes["description"];
		if(v.size()>0)
			levelpackDescription=v[0];
	}
	
	//Look for the congratulation text.
	{
		vector<string> &v=obj.attributes["congratulations"];
		if(v.size()>0)
			congratulationText=v[0];
	}
	
	//Loop through the level list entries.
	for(unsigned int i=0;i<obj.subNodes.size();i++){
		TreeStorageNode* obj1=obj.subNodes[i];
		if(obj1==NULL)
			continue;
		if(obj1->value.size()>=2 && obj1->name=="levelfile"){
			Level level;
			level.file=obj1->value[0];
			level.name=obj1->value[1];
			//The default for locked is true, unless it's the first one.
			level.locked=!levels.empty();
			
			//Add the level to the levels.
			levels.push_back(level);
		}
	}
	
	//Now load the progress/statistics.
	if(levelProgress){
		{
			POASerializer objSerializer;
			if(!objSerializer.readNode(levelProgress,&obj,true)){
				cerr<<"ERROR: Invalid file format of level progress file for "<<levelListNew<<endl;
				return false;
			}
		}
		//Loop through the entries.
		for(unsigned int i=0;i<obj.subNodes.size();i++){
			TreeStorageNode* obj1=obj.subNodes[i];
			if(obj1==NULL)
				continue;
			if(obj1->value.size()>=1 && obj1->name=="level"){
				//We've found an entry for a level, now search the correct level.
				Level* level=NULL;
				for(unsigned int o=0;o<levels.size();o++){
					if(obj1->value[0]==levels[o].file){
						level=&levels[o];
						break;
					}
				}
				
				//Check if we found the level.
				if(!level)
					continue;
				
				//Get the progress/statistics.
				for(map<string,vector<string> >::iterator i=obj1->attributes.begin();i!=obj1->attributes.end();i++){
					if(i->first=="locked"){
						level->locked=(i->second[0]=="1");
					}
					if(i->first=="won"){
						level->won=(i->second[0]=="1");
					}
				}
			}
		}
	}

	loaded=true;
	return true;
}

void Levels::saveLevels(const std::string& levelListFile){
	//Get the fileName.
	string levelListNew=processFileName(levelListFile);
	//Open an output stream.
	ofstream level(levelListNew.c_str());

	//Check if we can use the file.
	if(!level){
		cerr<<"ERROR: Can't save level list "<<levelListNew<<endl;
		return;
	}
	
	//Storage node that will contain the data that should be written.
	TreeStorageNode obj;

	//Make sure that there's a description.
	if(!levelpackDescription.empty())
		obj.attributes["description"].push_back(levelpackDescription);
	
	//Make sure that there's a congratulation text.
	if(!congratulationText.empty())
		obj.attributes["congratulations"].push_back(congratulationText);

	//Add the levels to the file.
	for(unsigned int i=0;i<levels.size();i++){
		TreeStorageNode* obj1=new TreeStorageNode;
		obj1->name="levelfile";
		obj1->value.push_back(fileNameFromPath(levels[i].file));
		obj1->value.push_back(levels[i].name);
		obj.subNodes.push_back(obj1);
		
		//We copy them to the levelpack folder
		//Check if the levelpath is relative or absolute.
		if(levels[i].file[0]=='%'){
			copyFile(processFileName(levels[i].file).c_str(),(pathFromFileName(levelListNew)+fileNameFromPath(levels[i].file)).c_str());
		}else{
			//Make sure we aren't copying to the same location.
			if((levelpackPath+levels[i].file)!=(pathFromFileName(levelListNew)+fileNameFromPath(levels[i].file))){
				copyFile((levelpackPath+levels[i].file).c_str(),(pathFromFileName(levelListNew)+fileNameFromPath(levels[i].file)).c_str());
			}
		}
	}

	//Write the it away.
	POASerializer objSerializer;
	objSerializer.writeNode(&obj,level,false,true);
}

void Levels::addLevel(const string& levelFileName,const string& levelName,int levelno){
	//Fill in the details.
	Level level;
	level.file=levelFileName;
	level.name=levelName;
	level.locked=levels.size()>0?true:false;
	
	//Check if the level should be at the end or somewhere in the middle.
	if(levelno<0 || levelno>=levels.size()){
		levels.push_back(level);
	}else{
		levels.insert(levels.begin()+levelno,level);
	}
}

void Levels::saveLevelProgress(){
	//Check if the levels are loaded and a progress file is given.
	if(!loaded || levelProgressFile.empty())
		return;
	
	//Open the progress file.
	ofstream levelProgress(processFileName(levelProgressFile).c_str());
	if(!levelProgress)
		return;
	
	//Open an output stream.
	TreeStorageNode node;
	
	//Loop through the levels.
	for(unsigned int o=0;o<levels.size();o++){
		TreeStorageNode* obj=new TreeStorageNode;
		node.subNodes.push_back(obj);
		
		//Set the name of the node.
		obj->name="level";
		obj->value.push_back(levels[o].file);
		
		//Set the values.
		obj->attributes["locked"].push_back(levels[o].locked?"1":"0");
		obj->attributes["won"].push_back(levels[o].won?"1":"0");
	}
	
	
	//Create a POASerializer and write away the leve node.
	POASerializer objSerializer;
	objSerializer.writeNode(&node,levelProgress,true,true);
}

const string& Levels::getLevelName(int level){
	if(level<0)
		level=currentLevel;
	return levels[level].name;
}

void Levels::setLevelName(int level,const std::string& name){
	if(level>=0&&level<levels.size()) 
		levels[level].name=name;
}

const string& Levels::getLevelFile(int level){
	if(level<0)
		level=currentLevel;
	return levels[level].file;
}

const string& Levels::getLevelpackPath(){
	return levelpackPath;
}


void Levels::nextLevel(){
	currentLevel++;
}

bool Levels::getLocked(int level){
	return levels[level].locked;
}

void Levels::setLevel(int level){
	currentLevel=level;
}

void Levels::setLocked(int level,bool locked){
	levels[level].locked=locked;
}

void Levels::swapLevel(int level1,int level2){
	if(level1>=0&&level1<levels.size()&&level2>=0&&level2<levels.size()){
		swap(levels[level1],levels[level2]);
	}
}

void Levels::removeLevel(int level){
	if(level>=0&&level<levels.size()){
		levels.erase(levels.begin()+level);
	}
}
