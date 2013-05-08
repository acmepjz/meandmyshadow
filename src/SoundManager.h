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

#include <SDL/SDL.h>
#ifdef __APPLE__
#include <SDL_mixer/SDL_mixer.h>
#else
#include <SDL/SDL_mixer.h>
#endif
#include <string>
#include <map>
#include <vector>

//Class for loading and playing sound effects.
class SoundManager{
private:
	std::map<std::string,Mix_Chunk*> sounds;
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
	void playSound(const std::string &name);
};

#endif
