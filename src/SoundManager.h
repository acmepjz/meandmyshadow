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

#ifndef SOUNDMANAGER_H
#define SOUNDMANAGER_H

#include <SDL.h>
#include <SDL_mixer.h>
#include <string>
#include <map>
#include <vector>

//Class for loading and playing sound effects.
class SoundManager{
private:
	//Map containing the sfx chunk by name.
	std::map<std::string,Mix_Chunk*> sounds;

	//Array that holds the name of the sfx that is playing on each channel.
	std::string playing[MIX_CHANNELS];
public:
	//Constructor.
	SoundManager();
	//Destructor.
	~SoundManager();

	//Destroys the sound chunks.
	void destroy();
	
	//This method will load one sfx file and add it to the collection.
	//file: The filename of the sfx file.
	//name: The name the sfx should have.
	void loadSound(const std::string &file,const std::string &name);
	
	//This method will start playing a sfx.
	//name: The name of the sfx to play.
	//concurrent: Integer containing the number of times the same sfx can be played at once, -1 is unlimited.
	//force: Boolean if the sound must get 
	void playSound(const std::string &name,const int concurrent=1,const bool force=false);

	//Method that is called when a sfx is done playing.
	//channel: The channel number.
	void channelFinished(int channel);

};

#endif
