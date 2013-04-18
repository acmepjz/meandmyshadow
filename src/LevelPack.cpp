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

#include "LevelPack.h"
#include "Functions.h"
#include "FileManager.h"
#include "TreeStorageNode.h"
#include "POASerializer.h"
#include "MD5.h"
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
#include <iostream>
using namespace std;

LevelPack::LevelPack():currentLevel(0),loaded(false),levels(),customTheme(false){
	//We need to set the pointer to the dictionaryManager to NULL.
	dictionaryManager=NULL;
}

LevelPack::~LevelPack(){
	//We call clear, since that already takes care of the deletion, including the dictionaryManager.
	clear();
}

void LevelPack::clear(){
	currentLevel=0;
	loaded=false;
	levels.clear();
	levelpackDescription.clear();
	levelpackPath.clear();
	levelProgressFile.clear();
	congratulationText.clear();
	
	//Also delete the dictionaryManager if it isn't null.
	if(dictionaryManager){
		delete dictionaryManager;
		dictionaryManager=NULL;
	}
}

bool LevelPack::loadLevels(const std::string& levelListFile){
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
	
	//Check for folders inside the levelpack folder.
	{
		//Get all the sub directories.
		vector<string> v;
		v=enumAllDirs(pathFromFileName(levelListNew),false);
		
		//Check if there's a locale folder containing translations.
		if(std::find(v.begin(),v.end(),"locale")!=v.end()){
			//Folder is present so configure the levelDictionaryManager.
			dictionaryManager=new tinygettext::DictionaryManager();
			dictionaryManager->add_directory(pathFromFileName(levelListNew)+"locale/");
			dictionaryManager->set_charset("UTF-8");
			dictionaryManager->set_language(tinygettext::Language::from_name(language));
		}else{
			dictionaryManager=NULL;
		}

		//Check for a theme folder.
		if(std::find(v.begin(),v.end(),"theme")!=v.end()){
			customTheme=true;
		}
	}
	
	//Look for the name.
	{
		vector<string> &v=obj.attributes["name"];
		if(!v.empty()){
			levelpackName=v[0];
		}else{
			//Name is not defined so take the folder name.
			levelpackName=pathFromFileName(levelListFile);
			//Remove the last character '/'
			levelpackName=levelpackName.substr(0,levelpackName.size()-1);
			levelpackName=fileNameFromPath(levelpackName);
		}
	}
	
	//Look for the description.
	{
		vector<string> &v=obj.attributes["description"];
		if(!v.empty())
			levelpackDescription=v[0];
	}
	
	//Look for the congratulation text.
	{
		vector<string> &v=obj.attributes["congratulations"];
		if(!v.empty())
			congratulationText=v[0];
	}
	
	//Loop through the level list entries.
	for(unsigned int i=0;i<obj.subNodes.size();i++){
		TreeStorageNode* obj1=obj.subNodes[i];
		if(obj1==NULL)
			continue;
		if(!obj1->value.empty() && obj1->name=="levelfile"){
			Level level;
			level.file=obj1->value[0];
			level.targetTime=0;
			level.targetRecordings=0;
			
			//Open the level file to retrieve the name and target time/recordings.
			TreeStorageNode obj;
			POASerializer objSerializer;
			if(objSerializer.loadNodeFromFile((levelpackPath+level.file).c_str(),&obj,true)){
				//Calc the MD5 FIRST because query obj.attributes will modify internal structure.
				obj.name.clear();
				obj.calcMD5(level.md5Digest);

				//Get the name of the level.
				vector<string>& v=obj.attributes["name"];
				if(!v.empty())
					level.name=v[0];
				//If the name is empty then we set it to the file name.
				if(level.name.empty())
					level.name=fileNameFromPath(level.file);
				
				//Get the target time of the level.
				v=obj.attributes["time"];
				if(!v.empty())
					level.targetTime=atoi(v[0].c_str());
				else
					level.targetTime=-1;
				//Get the target recordings of the level.
				v=obj.attributes["recordings"];
				if(!v.empty())
					level.targetRecordings=atoi(v[0].c_str());
				else
					level.targetRecordings=-1;
			}
			
			//The default for locked is true, unless it's the first one.
			level.locked=!levels.empty();
			level.won=false;
			level.time=-1;
			level.recordings=-1;
			
			//Add the level to the levels.
			levels.push_back(level);
		}
	}
	
	loaded=true;
	return true;
}

void LevelPack::loadProgress(const std::string& levelProgressFile){
	//Open the levelProgress file.
	ifstream levelProgress;
	if(!levelProgressFile.empty()){
		this->levelProgressFile=levelProgressFile;
		levelProgress.open(processFileName(this->levelProgressFile).c_str());
	}
	
	//Check if the file exists.
	if(levelProgress){
		//Now load the progress/statistics.
		TreeStorageNode obj;
		{
			POASerializer objSerializer;
			if(!objSerializer.readNode(levelProgress,&obj,true)){
				cerr<<"ERROR: Invalid file format of level progress file."<<endl;
			}
		}
		
		//Loop through the entries.
		for(unsigned int i=0;i<obj.subNodes.size();i++){
			TreeStorageNode* obj1=obj.subNodes[i];
			if(obj1==NULL)
				continue;
			if(!obj1->value.empty() && obj1->name=="level"){
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
				for(map<string,vector<string> >::iterator i=obj1->attributes.begin();i!=obj1->attributes.end();++i){
					if(i->first=="locked"){
					level->locked=(i->second[0]=="1");
					}
					if(i->first=="won"){
						level->won=(i->second[0]=="1");
					}
					if(i->first=="time"){
						level->time=(atoi(i->second[0].c_str()));
					}
					if(i->first=="recordings"){
						level->recordings=(atoi(i->second[0].c_str()));
					}
				}
			}
		}
	}
}

void LevelPack::saveLevels(const std::string& levelListFile){
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
	}

	//Write the it away.
	POASerializer objSerializer;
	objSerializer.writeNode(&obj,level,false,true);
}

void LevelPack::updateLanguage(){
	if(dictionaryManager!=NULL)
		dictionaryManager->set_language(tinygettext::Language::from_name(language));
}

void LevelPack::addLevel(const string& levelFileName,int levelno){
	//Fill in the details.
	Level level;
	if(!levelpackPath.empty() && levelFileName.compare(0,levelpackPath.length(),levelpackPath)==0){
		level.file=fileNameFromPath(levelFileName);
	}else{
		level.file=levelFileName;
	}
	level.targetTime=0;
	level.targetRecordings=0;
	
	//Get the name of the level.
	TreeStorageNode obj;
	POASerializer objSerializer;
	if(objSerializer.loadNodeFromFile(levelFileName.c_str(),&obj,true)){
		//Calc the MD5 FIRST because query obj.attributes will modify internal structure.
		obj.name.clear();
		obj.calcMD5(level.md5Digest);

		//Get the name of the level.
		vector<string>& v=obj.attributes["name"];
		if(!v.empty())
			level.name=v[0];
		//If the name is empty then we set it to the file name.
		if(level.name.empty())
			level.name=fileNameFromPath(levelFileName);
		
		//Get the target time of the level.
		v=obj.attributes["time"];
		if(!v.empty())
			level.targetTime=atoi(v[0].c_str());
		else
			level.targetTime=-1;
		//Get the target recordings of the level.
		v=obj.attributes["recordings"];
		if(!v.empty())
			level.targetRecordings=atoi(v[0].c_str());
		else
			level.targetRecordings=-1;
	}
	//Set if it should be locked or not.
	level.won=false;
	level.time=-1;
	level.recordings=-1;
	level.locked=levels.empty()?false:true;
	
	//Check if the level should be at the end or somewhere in the middle.
	if(levelno<0 || levelno>=int(levels.size())){
		levels.push_back(level);
	}else{
		levels.insert(levels.begin()+levelno,level);
	}
	
	//NOTE: We set loaded to true.
	loaded=true;
}

void LevelPack::moveLevel(unsigned int level1,unsigned int level2){
	if(level1>=levels.size())
		return;
	if(level2>=levels.size())
		return;
	if(level1==level2)
		return;
	
	levels.insert(levels.begin()+level2,levels[level1]);
	if(level2<=level1)
		levels.erase(levels.begin()+level1+1);
	else
		levels.erase(levels.begin()+level1);
}

void LevelPack::saveLevelProgress(){
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
		
		char s[64];
		
		//Set the name of the node.
		obj->name="level";
		obj->value.push_back(levels[o].file);
		
		//Set the values.
		obj->attributes["locked"].push_back(levels[o].locked?"1":"0");
		obj->attributes["won"].push_back(levels[o].won?"1":"0");
		sprintf(s,"%d",levels[o].time);
		obj->attributes["time"].push_back(s);
		sprintf(s,"%d",levels[o].recordings);
		obj->attributes["recordings"].push_back(s);
	}
	
	
	//Create a POASerializer and write away the leve node.
	POASerializer objSerializer;
	objSerializer.writeNode(&node,levelProgress,true,true);
}

const string& LevelPack::getLevelName(int level){
	if(level<0)
		level=currentLevel;
	return levels[level].name;
}

const unsigned char* LevelPack::getLevelMD5(int level){
	if(level<0)
		level=currentLevel;
	return levels[level].md5Digest;
}

void LevelPack::getLevelAutoSaveRecordPath(int level,std::string &bestTimeFilePath,std::string &bestRecordingFilePath,bool createPath){
	if(level<0)
		level=currentLevel;

	bestTimeFilePath.clear();
	bestRecordingFilePath.clear();

	//get level pack path.
	string levelpackPath=LevelPack::levelpackPath;
	string s=levels[level].file;

	//process level pack name
	for(;;){
		string::size_type lps=levelpackPath.find_last_of("/\\");
		if(lps==string::npos){
			break;
		}else if(lps==levelpackPath.size()-1){
			levelpackPath.resize(lps);
		}else{
			levelpackPath=levelpackPath.substr(lps+1);
			break;
		}
	}

	//profess file name
	{
		string::size_type lps=s.find_last_of("/\\");
		if(lps!=string::npos) s=s.substr(lps+1);
	}

	//check if it's custom level
	{
		string path="%USER%/records/autosave/";
		if(!levelpackPath.empty()){
			path+=levelpackPath;
			path+='/';
		}
		path=processFileName(path);
		if(createPath) createDirectory(path.c_str());
		s=path+s;
	}

	//calculate MD5
	s+='-';
	s+=Md5::toString(levels[level].md5Digest);

	//over
	bestTimeFilePath=s+"-best-time.mnmsrec";
	bestRecordingFilePath=s+"-best-recordings.mnmsrec";
}

void LevelPack::setLevelName(unsigned int level,const std::string& name){
	if(level<levels.size()) 
		levels[level].name=name;
}

const string& LevelPack::getLevelFile(int level){
	if(level<0)
		level=currentLevel;
	return levels[level].file;
}

const string& LevelPack::getLevelpackPath(){
	return levelpackPath;
}

struct LevelPack::Level* LevelPack::getLevel(int level){
	if(level<0)
		return &levels[currentLevel];
	return &levels[level];
}

void LevelPack::resetLevel(int level){
	if(level<0)
		level=currentLevel;
	
	//Set back to default.
	levels[level].locked=(level!=0);
	levels[level].won=false;
	levels[level].time=-1;
	levels[level].recordings=-1;
}

void LevelPack::nextLevel(){
	currentLevel++;
}

bool LevelPack::getLocked(unsigned int level){
	return levels[level].locked;
}

void LevelPack::setCurrentLevel(unsigned int level){
	currentLevel=level;
}

void LevelPack::setLocked(unsigned int level,bool locked){
	levels[level].locked=locked;
}

void LevelPack::swapLevel(unsigned int level1,unsigned int level2){
	if(level1<levels.size()&&level2<levels.size()){
		swap(levels[level1],levels[level2]);
	}
}

void LevelPack::removeLevel(unsigned int level){
	if(level<levels.size()){
		levels.erase(levels.begin()+level);
	}
}
