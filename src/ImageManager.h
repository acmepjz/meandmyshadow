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

#ifndef IMAGEMANAGER_H
#define IMAGEMANAGER_H

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <string>
#include <map>

//Class for loading images.
class ImageManager{
public:
	//Constructor.
	ImageManager(){}
	//Destructor.
	~ImageManager();
	
	//Loads an image.
	//file: The image file to load.
	//Returns: The SDL_Surface containing the image.
	SDL_Surface* loadImage(std::string file);
	
	//Destroys the images
	void destroy();
private:
	//Map containing the images.
	//The key is the name of the image and the value is a pointer to the SDL_Surface.
	std::map<std::string,SDL_Surface*> imageCollection;
};

#endif
