/*
 * Copyright (C) 2011-2013 Me and My Shadow
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

#ifndef SCENERYLAYER_H
#define SCENERYLAYER_H

#include <SDL.h>
#include <array>
#include <vector>
#include <map>
#include <string>

#include "Scenery.h"

class ImageManager;
class TreeStorageNode;
class Game;

//The class representing a scenery layer.
//NOTE: The name of the layer is not stored inside the class.
class SceneryLayer {
public:
	//Vector containing all the objects in the current game.
	std::vector<Scenery*> objects;

	//Float containing the speed the layer moves (in pixel/frame).
	float speedX, speedY;

	//Float containing the speed the layer will have when moving the camera.
	float cameraX, cameraY;
private:
	//Float with the current position.
	float currentX, currentY;

	//Stored location for when loading a state.
	float savedX, savedY;
public:
	//Constructor.
	SceneryLayer();

	SceneryLayer(const SceneryLayer& other) = delete;

	//Destructor, which will delete objects in "objects" automatically.
	~SceneryLayer();

	//Add objects load from the node to the object list. NOTE: The name of the layer is not loaded here.
	void loadFromNode(Game *parent, ImageManager& imageManager, SDL_Renderer& renderer, TreeStorageNode* obj);

	//Save all objects to the node.
	void saveToNode(TreeStorageNode* obj);

	//Method that will update the animation of all the objects.
	void updateAnimation();

	//Method that will reset the animation of all the objects.
	//save: Boolean if the saved state should be deleted.
	void resetAnimation(bool save);

	//Method that will save the animation of all the objects.
	void saveAnimation();

	//Method that will load the animation of all the objects.
	void loadAnimation();

	//Method used to draw the scenery.
	void show(SDL_Renderer& renderer);
};

#endif
