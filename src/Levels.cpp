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
	levelCount=0;
	currentLevel=0;
	loaded=false;
	levelName.clear();
	levelFiles.clear();
	levelLocked.clear();
	levelpackDescription.clear();
	levelpackPath.clear();
	levelProgressFile.clear();
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

	if(!level){
		cerr<<"ERROR: Can't load level list "<<levelListNew<<endl;
		return false;
	}
	
	if(!levelProgressFile.empty()){
		this->levelProgressFile=levelProgressFile;
		levelProgress.open(processFileName(levelProgressFile).c_str());
	}

	TreeStorageNode obj;
	{
		POASerializer objSerializer;
		if(!objSerializer.readNode(level,&obj,true)){
			cerr<<"ERROR: Invalid file format of level list "<<levelListNew<<endl;
			return false;
		}
	}

	{
		vector<string> &v=obj.attributes["description"];
		if(v.size()>0) levelpackDescription=v[0];
	}

	for(unsigned int i=0;i<obj.subNodes.size();i++){
		TreeStorageNode* obj1=obj.subNodes[i];
		if(obj1==NULL) continue;
		if(obj1->value.size()>=2 && obj1->name=="levelfile"){
			levelFiles.push_back(obj1->value[0]);
			levelName.push_back(obj1->value[1]);
			//load level progress
			int a=1;
			if(levelProgress.is_open() && !levelProgress.eof()) levelProgress >> a;
			levelLocked.push_back( !( a==0 || levelCount==0 ) );
			//over
			levelCount++;
		}
	}

	loaded=true;
	return true;
}

void Levels::saveLevels(const std::string& levelListFile){
	string levelListNew=processFileName(levelListFile);

	ofstream level(levelListNew.c_str());

	if(!level){
		cerr<<"ERROR: Can't load level list "<<levelListNew<<endl;
		return;
	}
	
	TreeStorageNode obj;

	obj.attributes["description"].push_back(levelpackDescription);

	for(int i=0;i<levelCount;i++){
		TreeStorageNode* obj1=new TreeStorageNode;
		obj1->name="levelfile";
		obj1->value.push_back(levelFiles[i]);
		obj1->value.push_back(levelName[i]);
		obj.subNodes.push_back(obj1);
	}

	POASerializer objSerializer;
	objSerializer.writeNode(&obj,level,false,true);
}

void Levels::addLevel(const string& levelFileName,const string& levelName,int level){
	if(level<0 || level>=levelCount){
		levelFiles.push_back(levelFileName);
		Levels::levelName.push_back(levelName);
		levelLocked.push_back(levelCount>0?true:false);
		levelCount++;
	}else{
		levelFiles.insert(levelFiles.begin()+level,levelFileName);
		Levels::levelName.insert(Levels::levelName.begin()+level,levelName);
		levelLocked.insert(levelLocked.begin()+level,level>0?true:false);
		levelCount++;
	}
}

void Levels::saveLevelProgress(){
	//Check if the levels are loaded and a progress file is given.
	if(!loaded || levelProgressFile.empty()) return;

	//Open an output stream.
	ofstream levelProgress(processFileName(levelProgressFile).c_str());

	//Loop the levels and write their status to the progress file.
	for(int n=0; n<levelCount; n++){
		levelProgress<<(levelLocked[n]?1:0) << "\n";
	}
}

const string& Levels::getLevelName(int level){
	if(level<0) level=currentLevel;
	return levelName[level];
}

void Levels::setLevelName(int level,const std::string& name){
	if(level>=0&&level<levelCount) levelName[level]=name;
}

const string& Levels::getLevelFile(int level){
	if(level<0) level=currentLevel;
	return levelFiles[level];
}

const string& Levels::getLevelpackPath(){
	return levelpackPath;
}


void Levels::nextLevel(){
	currentLevel++;
}

bool Levels::getLocked(int level){
	return levelLocked[level];
}

void Levels::setLevel(int level){
	currentLevel = level;
}

void Levels::setLocked(int level,bool locked){
	levelLocked[level] = locked;
}

void Levels::swapLevel(int level1,int level2){
	if(level1>=0&&level1<levelCount&&level2>=0&&level2<levelCount){
		swap(levelFiles[level1],levelFiles[level2]);
		swap(levelName[level1],levelName[level2]);
		
		bool temp = levelLocked[level1];
		levelLocked[level1] = levelLocked[level2];
		levelLocked[level2] = temp;
	}
}

void Levels::removeLevel(int level){
	if(level>=0&&level<levelCount){
		levelFiles.erase(levelFiles.begin()+level);
		levelName.erase(levelName.begin()+level);
		levelLocked.erase(levelLocked.begin()+level);
		levelCount--;
	}
}