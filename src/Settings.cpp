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

#include "Settings.h"
#include "FileManager.h"
#include <SDL.h>
#include <string>
#include <stdio.h>
using namespace std;

// Hardcode the addon url unless specified by command line arguments
#define DEFAULT_ADDON_URL "https://github.com/acmepjz/meandmyshadow-addons/raw/master/addons05"

Settings::Settings(const string& fileName): fileName(fileName){
	char s[32];
	settings["sound"]="128";
	settings["music"]="128";
	settings["musiclist"]="default";
	settings["fullscreen"]="0";
	settings["width"]="800";
	settings["height"]="600";
	settings["resizable"]="1";
	settings["theme"]="%DATA%/themes/Cloudscape";
	settings["internet"]="1";
	settings["lastlevelpack"]="tutorial";
	settings["internet-proxy"]="";
	settings["lang"]="";
	settings["addon_url"] = DEFAULT_ADDON_URL;

	//The record mode.
	settings["quickrecord"]="0";

	//Boolean if fading between states is enabled.
	settings["fading"]="1";

	//Key settings.
	sprintf(s,"%d",(int)SDLK_UP);
	settings["key_up"]=settings["key_jump"]=s;

	sprintf(s,"%d",(int)SDLK_DOWN);
	settings["key_down"]=settings["key_action"]=s; //SDLK_DOWN

	sprintf(s,"%d",(int)SDLK_LEFT);
	settings["key_left"]=s; //SDLK_LEFT

	sprintf(s,"%d",(int)SDLK_RIGHT);
	settings["key_right"]=s; //SDLK_RIGHT

	sprintf(s,"%d",(int)SDLK_SPACE);
	settings["key_space"]=s; //SDLK_SPACE

	settings["key_cancelRecording"]="0"; //not set by default

	sprintf(s,"%d",(int)SDLK_ESCAPE);
	settings["key_escape"]=s; //SDLK_ESCAPE

	sprintf(s,"%d",(int)SDLK_r);
	settings["key_restart"]=s; //SDLK_r

	sprintf(s,"%d",(int)SDLK_TAB);
	settings["key_tab"]=s;

	sprintf(s,"%d",(int)SDLK_F2);
	settings["key_save"]=s; //SDLK_F2

	sprintf(s,"%d",(int)SDLK_F3);
	settings["key_load"]=s; //SDLK_F3

	sprintf(s,"%d",(int)SDLK_F4);
	settings["key_swap"]=s; //SDLK_F4

	sprintf(s,"%d",(int)SDLK_F5);
	settings["key_teleport"]=s; //SDLK_F5

	sprintf(s,"%d",(int)SDLK_F12);
	settings["key_suicide"]=s;

	sprintf(s,"%d",(int)SDLK_RSHIFT);
	settings["key_shift"]=s; //SDLK_RSHIFT
	
	sprintf(s,"%d",(int)SDLK_PAGEUP);
	settings["key_next"]=s; //SDLK_PAGEUP
	
	sprintf(s,"%d",(int)SDLK_PAGEDOWN);
	settings["key_previous"]=s; //SDLK_PAGEDOWN
	
	sprintf(s,"%d",(int)SDLK_RETURN);
	settings["key_select"]=s; //SDLK_RETURN

	settings["key_up2"]=settings["key_down2"]=settings["key_left2"]=settings["key_right2"]=
		settings["key_jump2"]=settings["key_action2"]=
		settings["key_space2"]=settings["key_cancelRecording2"]=
		settings["key_escape2"]=settings["key_restart2"]=settings["key_tab2"]=
		settings["key_save2"]=settings["key_load2"]=settings["key_swap2"]=settings["key_teleport2"]=
		settings["key_suicide2"]=settings["key_shift2"]=settings["key_next2"]=settings["key_previous2"]=
		settings["key_select2"]="0";
}


void Settings::parseFile(int version){
	//Open the config file for reading.
	ifstream file;
	file.open(fileName.c_str());
	if(!file){
		//Check if the file exists.
		if(fileExists(fileName.c_str())){
			cerr<<"ERROR: Unable to read config file, default values will be used!"<<endl;
		}else{
			cout<<"Creating a new config file."<<endl;
			createFile();
		}
		//No need to parse the unreadable or newly created config file.
		return;
	}

	//Read the config file line by line.
	string line;
	while(getline(file,line)){
		string temp = line;

		unComment(temp);
		if(temp.empty() || empty(temp))
			continue;
		
		//The line is good so we parse it.
		parseLine(temp,version);
	}

	//And close the file.
	file.close();

	// Hardcode the addon url unless specified by command line arguments
	settings["addon_url"] = DEFAULT_ADDON_URL;
}

void Settings::parseLine(const string &line,int version){
	if((line.find('=') == line.npos) || !validLine(line))
		cerr<<"WARNING: illegal line in config file!"<<endl;
	
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
	
	//Check the version.
	//Currently in V0.5 we don't read key config saved in versions which is earlier than V0.5, since they are incompatible (SDL1.2 vs SDL2).
	if (version < 0x000500) {
		if (key.size() >= 4 && key.substr(0, 4) == "key_") {
			return;
		}
	}

	//Add the setting to the settings map.
	setValue(key,value);
}

bool Settings::validLine(const string &line){
	string temp = line;
	temp.erase(0, temp.find_first_not_of("\t "));
	if(temp[0] == '=')
		return false;

	for(size_t i = temp.find('=') + 1; i < temp.length(); i++)
		return true;
	return false;
}

void Settings::unComment(string &line){
	if(line.find('#') != line.npos)
		line.erase(line.find('#'));
}

bool Settings::empty(const string &line){
	return (line.find_first_not_of(' ')==line.npos);
}

string Settings::getValue(const string &key){
	if(settings.find(key) == settings.end()){
		cerr<<"WARNING: Key "<<key<<" couldn't be found!"<<endl;
		return "";
	}
	return settings[key];
}

bool Settings::getBoolValue(const string &key){
	if(settings.find(key) == settings.end()){
		cerr<<"WARNING: Key "<<key<<" couldn't be found!"<<endl;
		return false;
	}
	return (settings[key] != "0");
}

void Settings::setValue(const string &key, const string &value){
	settings[key]=value;
}

void Settings::createFile(){
	ofstream file;
	file.open(fileName.c_str());
	
	//Default Config file.
	file<<"# MeAndMyShadow config file"<<endl;
	
	map<string, string>::iterator iter;
	for(iter = settings.begin(); iter != settings.end(); ++iter){
		file << iter->first << " = " << iter->second << endl;
	}
	
	//And close the file.
	file.close();
}

bool Settings::save(){
	//Open the settings file.
	ofstream file;
	file.open(fileName.c_str());

	//Check if the file could be opened.
	if(!file)
		return false;

	//Now write it to the first line of the config file.
	file<<"# MeAndMyShadow config file"<<endl;
	
	//Loop through the settings and save them.
	map<string,string>::const_iterator iter;
	for(iter=settings.begin(); iter!=settings.end(); ++iter){
		if (iter->first == "addon_url") {
			// Hardcode the addon url
			file << iter->first << " = " DEFAULT_ADDON_URL << endl;
		} else {
			file << iter->first << " = " << iter->second << endl;
		}
	}
	file.close();

	//No errors so return true.
	return true;
}
