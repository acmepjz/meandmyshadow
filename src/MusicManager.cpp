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

#include "MusicManager.h"
#include "TreeStorageNode.h"
#include "POASerializer.h"
#include "FileManager.h"
#include "Functions.h"
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <algorithm>
using namespace std;

MusicManager::MusicManager(){
	Mix_HookMusicFinished(musicStoppedHook);
	Mix_VolumeMusic(MIX_MAX_VOLUME);
	enabled=false;
	currentList="default";
	lastTime=0;
	playing=NULL;
}

MusicManager::~MusicManager(){
	//We call destroy().
	destroy();
}

void MusicManager::destroy(){
	//Loop through the imageCollection and free them.
	std::map<std::string,Music*>::iterator i;
	for(i=musicCollection.begin();i!=musicCollection.end();++i){
		if(i->second!=NULL){
			Mix_FreeMusic(i->second->music);
			if(i->second->loop)
				Mix_FreeMusic(i->second->loop);
			delete i->second;
		}
	}
	playing=NULL;
	
	//And clear the collection and the lists.
	musicCollection.clear();
	musicLists.clear();
}

void MusicManager::setEnabled(bool enable){
	//Set the new status.
	if(enabled!=enable)
		enabled=enable;
	else
		return;
  
	if(enable){
		//It got turned on, so start the menu music.
		playMusic("menu",false);
	}else{
		//Stop the current music.
		Mix_HaltMusic();
		Mix_VolumeMusic(atoi(getSettings()->getValue("music").c_str()));
	}
}

void MusicManager::setVolume(int volume){
	Mix_VolumeMusic(volume);
}

string MusicManager::loadMusic(const std::string &file){
	//Open the .music file.
	ifstream musicFile;
	musicFile.open(file.c_str());
	string returnString="";
	
	//Check if the file exists.
	if(musicFile){
		//Now parse the file.
		TreeStorageNode obj;
		{
			POASerializer objSerializer;
			if(!objSerializer.readNode(musicFile,&obj,true)){
				cerr<<"ERROR: Invalid file format of music description file."<<endl;
			}
		}
		
		//Loop through the entries.
		for(unsigned int i=0;i<obj.subNodes.size();i++){
			TreeStorageNode* obj1=obj.subNodes[i];
			if(obj1==NULL)
				continue;
			if(obj1->value.size()>=1 && obj1->name=="music"){
				//Make sure that this music isn't already loaded.
				map<string,Music*>::iterator it=musicCollection.find(obj1->value[0]);
				if(it==musicCollection.end()){
					//We've found an entry for a music file.
					Music* music=new Music;
					music->music=NULL;
					music->loop=NULL;
					
					//Load some data.
					for(map<string,vector<string> >::iterator i=obj1->attributes.begin();i!=obj1->attributes.end();++i){
						if(i->first=="file"){
							//Load the music file.
							music->music=Mix_LoadMUS((getDataPath()+"music/"+i->second[0]).c_str());
						}
						if(i->first=="loopfile"){
							//Load the loop file.
							music->loop=Mix_LoadMUS((getDataPath()+"music/"+i->second[0]).c_str());
						}
						if(i->first=="trackname"){
							music->trackName=i->second[0];
						}
						if(i->first=="author"){
							music->author=i->second[0];
						}
						if(i->first=="license"){
							music->license=i->second[0];
						}
						if(i->first=="start"){
							music->start=(atoi(i->second[0].c_str()));
						}
						if(i->first=="volume"){
							music->volume=(atoi(i->second[0].c_str()));
						}
						if(i->first=="loopstart"){
							music->loopStart=(atoi(i->second[0].c_str()));
						}
						if(i->first=="loopend"){
							music->loopEnd=(atoi(i->second[0].c_str()));
						}
					}
					
					//Set the default value for lastTime.
					music->lastTime=-1;
					music->name=obj1->value[0];
					
					//Now add it to the collection.
					musicCollection[obj1->value[0]]=music;
				}
				
				//Add the name of the music to the return string even if it's already loaded.
				//This is to allow music to be in multiple music lists.
				if(!returnString.empty())
					returnString+=',';
				returnString+=obj1->value[0];
			}
		}
	}
	
	//Return the return string.
	return returnString;
}

bool MusicManager::loadMusicList(const std::string &file){
	//Open the .list file.
	ifstream musicFile;
	musicFile.open(file.c_str());
	
	//Check if the file exists.
	if(musicFile){
		//Now parse the file.
		TreeStorageNode obj;
		{
			POASerializer objSerializer;
			if(!objSerializer.readNode(musicFile,&obj,true)){
				cerr<<"ERROR: Invalid file format of music list file."<<endl;
				return false;
			}
		}
		
		//Get the name of the list.
		std::string name;
		{
			map<string,vector<string> >::iterator it=obj.attributes.find("name");
			if(it!=obj.attributes.end()){
				name=obj.attributes["name"][0];
			}else{
				cerr<<"ERROR: No name for music list "<<file<<endl;
				return false;
			}
		}
		
		//Check if the list isn't already loaded.
		std::map<std::string,std::vector<std::string> >::iterator it=musicLists.find(name);
		if(it!=musicLists.end())
			return true;
		
		//Loop through the entries.
		for(unsigned int i=0;i<obj.subNodes.size();i++){
			TreeStorageNode* obj1=obj.subNodes[i];
			if(obj1==NULL)
				continue;
			if(obj1->value.size()>=1 && obj1->name=="musicfile"){
				//Load the music file.
				string result=loadMusic(getDataPath()+"music/"+obj1->value[0]);
				if(!result.empty()){
					if(result.find(',')!=string::npos){
						size_t pos=result.find(',');
						while(pos!=string::npos){
							musicLists[name].push_back(result.substr(pos,result.find(',',pos+1)));
						}
					}else{
						musicLists[name].push_back(result);
					}
				}
			}
		}
	}else{
		cerr<<"ERROR: Unable to open music list file "<<file<<endl;
		return false;
	}
	
	//Nothing went wrong so return true.
	return true;
}

void MusicManager::playMusic(const std::string &name,bool fade){
	//Make sure music is enabled.
	if(!enabled)
		return;
	
	//Check if the music is in the collection.
	Music* music=musicCollection[name];
	if(music==NULL){
		cerr<<"ERROR: Unable to play music "<<name<<endl;
		return;
	}
	
	//Now check if we should fade the previous one out.
	if(fade){
	  	Mix_FadeOutMusic(375);
		//Set the next music.
		nextMusic=name;
	}else{
		if(music->loopStart<=0 && music->loop==NULL){
			Mix_FadeInMusicPos(music->music,-1,0,music->start);
		}else{
			Mix_FadeInMusicPos(music->music,0,0,music->start);
		}
		Mix_VolumeMusic(music->volume*float(atoi(getSettings()->getValue("music").c_str())/float(MIX_MAX_VOLUME)));
		
		//Set the playing pointer.
		playing=music;
	}
}

void MusicManager::pickMusic(){
	//Make sure the currentList exists.
	vector<std::string> list=musicLists[currentList];
	if(currentList.empty()){
		cerr<<"ERROR: Unkown music list "<<currentList<<endl;
		return;
	}
	
	//Shuffle the list.
	random_shuffle(list.begin(),list.end());
	
	//Now loop through the music and search the oldest.
	Music* oldest=NULL;
	for(unsigned int i=0;i<list.size();i++){
		//Check if oldest is set.
		if(oldest==NULL){
			//It isn't so pick the first music.
			oldest=musicCollection[list[i]];
			continue;
		}
		
		//Check if this song is null.
		if(musicCollection[list[i]]==NULL)
			continue;
		
		//Check if this music is never played.
		if(musicCollection[list[i]]->lastTime==-1){
			oldest=musicCollection[list[i]];
			//And break out.
			break;
		}
		
		//Check if this music is older.
		if(musicCollection[list[i]]->lastTime<oldest->lastTime){
			oldest=musicCollection[list[i]];
		}
	}
	
	//Check if oldest ins't null.
	if(oldest!=NULL){
		playMusic(oldest->name);
		//Set the lastTime and increase it.
		oldest->lastTime=lastTime;
		lastTime++;
	}
}

void MusicManager::musicStopped(){
	//Check if there's a music to play.
	if(!nextMusic.empty()){
		//Check if the music is in the collection.
		Music* music=musicCollection[nextMusic];
		if(music==NULL){
			cerr<<"ERROR: Unable to play music "<<nextMusic<<endl;
			return;
		}
		
		if(music->loopStart<=0){
			Mix_FadeInMusicPos(music->music,-1,375,music->start);
		}else{
			Mix_FadeInMusicPos(music->music,0,375,music->start);
		}
		
		Mix_VolumeMusic(music->volume*float(atoi(getSettings()->getValue("music").c_str())/float(MIX_MAX_VOLUME)));
		
		//Set playing.
		playing=music;
		
		//Now reset nextMusic.
		nextMusic.clear();
	}else{
		//Check what kind of loop.
		if(playing->loop!=NULL){
			Mix_FadeInMusicPos(playing->loop,-1,0,playing->loopStart);
		}else{
			//This is for looping the end of music.
			Mix_FadeInMusicPos(playing->music,0,0,playing->loopStart);
		}
	}
}

void MusicManager::setMusicList(const string &list){
	//Check if the list exists.
	
}

vector<string> MusicManager::createCredits(){
	//Vector that will be returned.
	vector<string> result;

	//Loop through the music tracks.
	std::map<std::string,Music*>::iterator it;
	for(it=musicCollection.begin();it!=musicCollection.end();it++){
		result.push_back("    - "+it->second->trackName);
		result.push_back("        License: "+it->second->license);
		result.push_back("        Attribution: "+it->second->author);
	}

	//And return the result.
	return result;
}
