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

#include <SDL2/SDL.h>
#ifdef __APPLE__
#include <SDL_image/SDL_image.h>
#else
#include <SDL2/SDL_image.h>
#endif
#include <string>
#include <map>
#include <memory>

using SharedTexture = std::shared_ptr<SDL_Texture>;

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
    SDL_Surface* loadImage(const std::string& file);
    //Load an image directly to a texture. Returns NULL on failure.
    //This does not support color keyed textures.
    //SDL_Texture* loadTexture(const std::string& file, SDL_Renderer& renderer);
    //Load an image directly to a texture. Terminates on failure.
    //This does not support color keyed textures.
    SharedTexture loadTexture(const std::string& file, SDL_Renderer& renderer);
	
	//Destroys the images
	void destroy();
private:
    //Forbid copying
    ImageManager(const ImageManager&) = delete;
    ImageManager& operator=(ImageManager const&) = delete;

	//Map containing the images.
	//The key is the name of the image and the value is a pointer to the SDL_Surface.
    std::map<std::string, SDL_Surface*> imageCollection;
    std::map<std::string, SharedTexture> textureCollection;
};

#endif
