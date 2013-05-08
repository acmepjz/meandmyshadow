/*
 * Copyright (C) 2013 Me and My Shadow
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

#include "SoundManager.h"
#include "TreeStorageNode.h"
#include "POASerializer.h"
#include "FileManager.h"
#include "Functions.h"
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <algorithm>
using namespace std;

SoundManager::SoundManager(){}

SoundManager::~SoundManager(){
	//We call destroy().
	destroy();
}

void SoundManager::destroy(){
	//Loop through the imageCollection and free them.
	std::map<std::string,Mix_Chunk*>::iterator i;
	for(i=sounds.begin();i!=sounds.end();++i){
		if(i->second!=NULL){
			Mix_FreeChunk(i->second);
		}
	}
	
	//And clear the collection.
	sounds.clear();
}

void SoundManager::loadSound(const std::string &file,const std::string &name){
	//Make sure the name isn't in use already.
	Mix_Chunk* sfx=sounds[name];
	if(sfx!=NULL){
		cerr<<"WARNING: There's already a sound registered for: "<<name<<endl;
		return;
	}

	//Now load the sound file.
	sfx=Mix_LoadWAV(file.c_str());
	if(sfx==NULL)
		cerr<<"ERROR: Can't load sound file "<<file<<", "<<SDL_GetError()<<endl;
	sounds[name]=sfx;
}


void SoundManager::playSound(const std::string &name){
	//Check if the music is in the collection.
	Mix_Chunk* sfx=sounds[name];
	if(sfx==NULL){
		cerr<<"WARNING: No such sound registered: "<<name<<endl;
		return;
	}
	
	//Make sure sound is enabled.
	if(getSettings()->getBoolValue("sound")==true){
		Mix_PlayChannel(-1,sfx,0);
	}
}
