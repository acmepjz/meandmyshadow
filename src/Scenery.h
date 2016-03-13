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

#ifndef SCENERY_H
#define SCENERY_H

#include "GameObjects.h"
#include "Globals.h"
#include "ThemeManager.h"
#include <vector>
#include <SDL2/SDL.h>

class Scenery: public GameObject{
private:
	//Save variables for the current location of the scenery.
	int xSave,ySave;

	//Delta variables, if the scenery moves these must be set to the delta movement.
	int dx,dy;
public:
	//The ThemeBlock, kept so it can be deleted later on.
	ThemeBlock themeBlock;
	//The Appearance of the scenery.
	//NOTE: We use a ThemeBlockInstance since it allows for all sorts of things like animations.
	ThemeBlockInstance appearance;
	
	//Constructor.
	//objParent: Pointer to the Game object.
	Scenery(Game* objParent);
	//Desturctor.
	~Scenery();

	//Method used to draw the scenery.
    void show(SDL_Renderer& renderer) override;

	//Returns the box of a given type.
	//boxType: The type of box that should be returned.
	//See GameObjects.h for the types.
	//Returns: The box.
	virtual SDL_Rect getBox(int boxType=BoxType_Current);
	
	//Method used to set the location of the scenery.
	//NOTE: The new location isn't stored as base location.
	//x: The new x location.
	//y: The new y location.
	virtual void setLocation(int x,int y);

	//Save the state of the scenery so we can load it later on.
	virtual void saveState();
	//Load the saved state of the scenery.
	virtual void loadState();
	//Reset the scenery.
	//save: Boolean if the saved state should also be deleted.
	virtual void reset(bool save);
	
	//Play an animation.
	virtual void playAnimation();
	
	//Method called when there's an event.
	//eventType: The type of event.
	//See GameObjects.h for the eventtypes.
	virtual void onEvent(int eventType);
	
	//Method used to retrieve a property from the scenery.
	//propertyType: The type of property requested.
	//See GameObjects.h for the properties.
	//obj: Pointer to the player.
	//Returns: Integer containing the value of the property.
	virtual int queryProperties(int propertyType,Player* obj);
	
	//Get the editor data of the scenery.
	//obj: The vector that will be filled with the editorData.
	virtual void getEditorData(std::vector<std::pair<std::string,std::string> >& obj);
	//Set the editor data of the scenery.
	//obj: The new editor data.
	virtual void setEditorData(std::map<std::string,std::string>& obj);

	//Get a single property of the scenery.
	//property: The property to return.
	//Returns: The value for the requested property.
	virtual std::string getEditorProperty(std::string property);
	//Set a single property of the scenery.
	//property: The property to set.
	//value: The new value for the property.
	virtual void setEditorProperty(std::string property,std::string value);

	//Method for loading the Scenery object from a node.
	//objNode: Pointer to the storage node to load from.
	//Returns: True if it succeeds without errors.
    virtual bool loadFromNode(ImageManager& imageManager,SDL_Renderer& renderer,TreeStorageNode* objNode) override;

	//Method used for resetting the dx/dy and xVel/yVel variables.
	virtual void prepareFrame();
	//Method used for updating any animations.
	virtual void move();
};

#endif
