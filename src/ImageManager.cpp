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

#include "ImageManager.h"
#include <stdio.h>
#include <SDL_image.h>

SDL_Surface* ImageManager::loadImage(const std::string &file){
	SDL_Surface* opt=NULL;

	//First check if the image isn't already in the imageCollection.
	opt=imageCollection[file];
	if(opt!=NULL)
		return opt;

	//Now load the image to load.
    opt=IMG_Load(file.c_str());

	//Check if loading didn't fail.
    if(opt!=NULL){
		//Check if there's an alpha mask.
        if(!opt->format->Amask){
			//We use cyan for alpha, so set the alpha color key.
            SDL_SetColorKey(opt,SDL_TRUE,SDL_MapRGB(opt->format,0,0xFF,0xFF));
		}
	}else{
		//We couldn't load the image.
        fprintf(stderr,"ERROR: Can't open image file %s\n. Error:%s\n",file.c_str(),IMG_GetError());
		return NULL;
	}

	//Add a pointer to the imageCollection.
	imageCollection[file]=opt;
	//And return the SDL_Surface*.
	return opt;
}

SharedTexture ImageManager::loadTexture(const std::string &file, SDL_Renderer& renderer) {
    //First check if the image isn't already in the textureCollection.
    SharedTexture opt=textureCollection[file];
    if(opt.get()) {
        return opt;
    }

    //Now try to load the image into a texture.
    opt=SharedTexture(IMG_LoadTexture(&renderer, file.c_str()), SDL_DestroyTexture);
    if(opt.get()) {
        //Add a pointer to the textureCollection.
        textureCollection[file]=opt;
        return opt;
    } else {
        fprintf(stderr,"ERROR: Can't open image file %s\n. Error:%s\n",file.c_str(),IMG_GetError());
        return opt;
    }
}

ImageManager::~ImageManager(){
	//We call destroy().
	destroy();
}

void ImageManager::destroy(){
    {
        //Loop through the imageCollection and free them.
        std::map<std::string,SDL_Surface*>::iterator i;
        for(i=imageCollection.begin();i!=imageCollection.end();++i){
            SDL_FreeSurface(i->second);
        }
        imageCollection.clear();
    }
    {
        textureCollection.clear();
    }
}
