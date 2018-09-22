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
#include "ThemeManager.h"
#include <vector>
#include <SDL.h>

class Scenery: public GameObject{
private:
	//Save variables for the current location of the scenery.
	int xSave,ySave;

	//Delta variables, if the scenery moves these must be set to the delta movement.
	int dx,dy;
public:
	//The ThemeBlock, kept so it can be deleted later on.
	ThemeBlock internalThemeBlock;
	// The pointer points to the real ThemeBlock, either point to internalThemeBlock, or a ThemeBlock in ThemeManager, or NULL.
	ThemeBlock* themeBlock;
	//The Appearance of the scenery.
	//NOTE: We use a ThemeBlockInstance since it allows for all sorts of things like animations.
	ThemeBlockInstance appearance, appearanceSave, appearanceInitial;

	// The scenery name. "" means custom scenery, in this case themeBlock is pointing to internalThemeBlock
	std::string sceneryName_;

	// The custom scenery description, which is the text dump of the TreeStorageNode.
	std::string customScenery_;

	// The repeat mode.
	enum RepeatMode {
		DEFAULT, // Starts or ends at the position of this block (default)
		NEGATIVE_INFINITY, // Starts at negative infinity
		ZERO, // Starts or ends at 0
		LEVEL_SIZE, // Starts or ends at level size
		POSITIVE_INFINITY, // Ends at positive infinity
		REPEAT_MODE_MAX,
	};

	// The repeat mode of this block. The value is Scenery::RepeatMode left shifted by appropriate value
	// bit 0-7: x start
	// bit 8-15: x end
	// bit 16-23: y start
	// bit 24-31: y end
	unsigned int repeatMode;
	
	//Constructor.
	//objParent: Pointer to the Game object.
	Scenery(Game* objParent);
	//Constructor.
	//objParent: Pointer to the Game object.
	//x: the x coordinate
	//y: the y coordinate
	//w: the width
	//h: the height
	//sceneryName: the scenery name, "" means custom scenery block
	Scenery(Game* objParent, int x, int y, int w, int h, const std::string& sceneryName);
	//Desturctor.
	~Scenery();

	//Method to load custom scenery from customScenery_ member variable.
	bool updateCustomScenery(ImageManager& imageManager, SDL_Renderer& renderer);

	//Method used to draw the scenery.
	//NOTE: To enable parallax scrolling, etc. use showScenery() instead.
    void show(SDL_Renderer& renderer) override;

	//Method used to draw the scenery.
	//offsetX/Y: the offset apply to the scenery block before considering camera position.
	void showScenery(SDL_Renderer& renderer, int offsetX, int offsetY);

	//Returns the box of a given type.
	//boxType: The type of box that should be returned.
	//See GameObjects.h for the types.
	//Returns: The box.
	virtual SDL_Rect getBox(int boxType=BoxType_Current) override;
	
	//Method used to set the location of the scenery.
	//NOTE: The new location isn't stored as base location.
	//x: The new x location.
	//y: The new y location.
	virtual void setLocation(int x,int y) override;

	//Save the state of the scenery so we can load it later on.
	void saveState();
	//Load the saved state of the scenery.
	void loadState();
	//Reset the scenery.
	//save: Boolean if the saved state should also be deleted.
	void reset(bool save);
	
	//Play an animation.
	virtual void playAnimation() override;
	
	//Method called when there's an event.
	//eventType: The type of event.
	//See GameObjects.h for the eventtypes.
	virtual void onEvent(int eventType) override;
	
	//Method used to retrieve a property from the scenery.
	//propertyType: The type of property requested.
	//See GameObjects.h for the properties.
	//obj: Pointer to the player.
	//Returns: Integer containing the value of the property.
	virtual int queryProperties(int propertyType,Player* obj) override;
	
	//Get the editor data of the scenery.
	//obj: The vector that will be filled with the editorData.
	virtual void getEditorData(std::vector<std::pair<std::string,std::string> >& obj) override;
	//Set the editor data of the scenery.
	//obj: The new editor data.
	virtual void setEditorData(std::map<std::string,std::string>& obj) override;

	//Get a single property of the scenery.
	//property: The property to return.
	//Returns: The value for the requested property.
	virtual std::string getEditorProperty(std::string property) override;
	//Set a single property of the scenery.
	//property: The property to set.
	//value: The new value for the property.
	virtual void setEditorProperty(std::string property,std::string value) override;

	//Method for loading the Scenery object from a node.
	//objNode: Pointer to the storage node to load from.
	//Returns: True if it succeeds without errors.
    virtual bool loadFromNode(ImageManager& imageManager,SDL_Renderer& renderer,TreeStorageNode* objNode) override;

	//Method used for updating any animations.
	virtual void move() override;
};

#endif
