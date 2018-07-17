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
#include "MusicManager.h"
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <algorithm>
using namespace std;

SoundManager::SoundManager(){
	Mix_ChannelFinished(channelFinishedHook);

	//Assume the channel count is default number
	playing.resize(MIX_CHANNELS);
	playingFlags.resize(MIX_CHANNELS, 0);
}

SoundManager::~SoundManager(){
	//We call destroy().
	destroy();
}

void SoundManager::setNumberOfChannels(int channels) {
	//in case of the allocate channels is different from what we requested
	channels = Mix_AllocateChannels(channels);

	playing.resize(channels);
	playingFlags.resize(channels, 0);
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


int SoundManager::playSound(const std::string &name, int concurrent, bool force, int fadeMusic){
	//Make sure sound is enabled.
	if(!getSettings()->getBoolValue("sound"))
		return -1;
	
	//Check if the music is in the collection.
	Mix_Chunk* sfx=sounds[name];
	if(sfx==NULL){
		cerr<<"WARNING: No such sound registered: "<<name<<endl;
		return -1;
	}

	//Check if there's a limit to the number of times the sfx should be played at the same time.
	if(concurrent!=-1){
		//There is so check the number of currently playing.
		int currentPlaying=0;
		for(int i=0;i<(int)playing.size();i++){
			if(playing[i]==name)
				currentPlaying++;
		}

		//Check if there are too many of the same effect playing.
		if(currentPlaying>=concurrent)
			return -1;
	}
	
	//Try to play the sfx on a free channel.
	int channel=Mix_PlayChannel(-1,sfx,0);
	//Check if there was an error.
	if(channel==-1){
		if(force){
			//We need to clear a channel for the sfx since it has to be played.
			//FIXME: Always clear the first channel?
			Mix_HaltChannel(0);
			channel=Mix_PlayChannel(0,sfx,0);
			if(channel==-1)
				cerr<<"WARNING: Unable to forcefully play sound '"<<name<<"'!"<<endl;
		}else{
			cerr<<"WARNING: Unable to play sound '"<<name<<"', out of channels."<<endl;
		}
	}

	if (channel >= 0) {
		int flags = 0;

		if (fadeMusic >= 0 && getSettings()->getBoolValue("music")) {
			flags |= 0x1;
			getMusicManager()->setVolume(fadeMusic*float(atoi(getSettings()->getValue("music").c_str()) / float(MIX_MAX_VOLUME)));
		}

		playing[channel] = name;
		playingFlags[channel] = flags;
	}

	return channel;
}

void SoundManager::channelFinished(int channel){
	int flags = playingFlags[channel];

	if (flags & 0x1) {
		getMusicManager()->setVolume(atoi(getSettings()->getValue("music").c_str()));
	}

	playing[channel].clear();
	playingFlags[channel] = 0;
}
