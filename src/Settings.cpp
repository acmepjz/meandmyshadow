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

#include "Settings.h"
#include <string>
using namespace std;

const char* Settings::settingNames[]={
	"sound","1",
	"fullscreen","0",
	"theme","default",
	"leveltheme","1",
	"internet","1",
	"lastlevelpack","default",
	"internet-proxy","", /* 2011-10-30 new */
};

const int Settings::maxSettingNames = sizeof(Settings::settingNames)/sizeof(Settings::settingNames[0]);

Settings::Settings(const string fileName):
	fileName(fileName){
		for(int i=0; i<maxSettingNames; i+=2){
			settings.insert(pair<string, string>(settingNames[i],settingNames[i+1]));
		}
	};


void Settings::parseFile(){
	//We open the settings file.
	ifstream file;
	file.open(fileName.c_str());
	if(!file){
		cout<<"Can't find config file!"<<endl;
		createFile();
	}

	//Now we're going to walk throught the file line by line.
	string line;
	while(getline(file,line)){
		string temp = line;

		unComment(temp);
		if(temp.empty() || empty(temp))
			continue;
		
		//The line is good so we parse it.
		parseLine(temp);
	}

	//And close the file.
	file.close();
}

void Settings::parseLine(const string &line){
	if((line.find('=') == line.npos) || !validLine(line))
		cout<<"Warning illegal line in config file!"<<endl;
	
	string temp = line;
	temp.erase(0, temp.find_first_not_of("\t "));
	int seperator = temp.find('=');

	//Get the key and trim it.
	string key, value;
	key = line.substr(0, seperator);
	if(key.find('\t')!=line.npos || key.find(' ')!=line.npos)
		key.erase(key.find_first_of("\t "));
	
	//Get the value and trim it.
	value = line.substr(seperator + 1);
	value.erase(0, value.find_first_not_of("\t "));
	value.erase(value.find_last_not_of("\t ") + 1);
	
	//Add the setting to the settings map.
	setValue(key,value);
}

bool Settings::validLine(const string &line){
	string temp = line;
	temp.erase(0, temp.find_first_not_of("\t "));
	if(temp[0] == '=')
		return false;

	for(int i = temp.find('=') + 1; i < temp.length(); i++)
		if(temp[i] != ' ')
			return true;
	return false;
}

void Settings::unComment(string &line){
	if (line.find('#') != line.npos)
		line.erase(line.find('#'));
}

bool Settings::empty(const string &line){
	return (line.find_first_not_of(' ')==line.npos);
}

string Settings::getValue(const string &key){
	if(settings.find(key) == settings.end()){
		cout<<"Key "<<key<<" couldn't be found!"<<endl;
		return "";
	}
	return settings[key];
}

bool Settings::getBoolValue(const string &key){
	if(settings.find(key) == settings.end()){
		cout<<"Key "<<key<<" couldn't be found!"<<endl;
		return false;
	}
	return (settings[key] == "1");
}

void Settings::setValue(const string &key, const string &value){
	if(settings.find(key) == settings.end()){
		cout<<"Key "<<key<<" couldn't be found!"<<endl;
		return;
	}
	settings[key]=value;
}

void Settings::createFile(){
	ofstream file;
	file.open(fileName.c_str());
	
	//Default Config file.
	file<<"#MeAndMyShadow config file. Created on "<<endl;
	
	for(int i=0; i<maxSettingNames;i+=2){
		file<<settingNames[i]<<" = "<<settingNames[i+1]<<endl;
	}

	//And close the file.
	file.close();
}

void Settings::save(){
	ofstream file;
	file.open(fileName.c_str());
	
	//Default Config file.
	file<<"#MeAndMyShadow config file. Created on "<<endl;
	map<string,string>::const_iterator iter;
	for(iter=settings.begin(); iter!=settings.end(); ++iter){
		file<<iter->first<<" = "<<iter->second<<endl;
	}
	file.close();
}