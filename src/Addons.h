/*
 * Copyright (C) 2011-2012 Me and My Shadow
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

#ifndef ADDONS_H
#define ADDONS_H

#include "GameState.h"
#include "GameObjects.h"
#include "GUIObject.h"
#include "GUIListBox.h"
#include <vector>
#include <string>
#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include <SDL/SDL_ttf.h>

//The addons menu.
class Addons: public GameState,public GUIEventCallback{
private:
	//An addon entry.
	struct Addon{
		//The name of the addon.
		string name;
		//The type of addon. (Level, Levelpack, Theme)
		string type;
		//The link to the addon file.
		string file;
		//The folder to place the addon in, only for themes and levelpacks.
		string folder;
		//The name of the author.
		string author;
		
		//The latest version of the addon.
		int version;
		//The version that the user has installed, if installed.
		int installedVersion;
		
		//Boolean if the addon is installed.
		bool installed;
		//Boolean if the addon is upToDate. (installedVersion==version)
		bool upToDate;
	};

	//The title.
	SDL_Surface* title;
	
	//Vector containing all the addons.
	std::vector<Addon>* addons;
	
	//File pointing to the addon file in the userpath.
	FILE* addon;
	
	//String that should contain the error when something fails.
	string error;
	
	//The type of addon that is currently selected.
	string type;
	//Pointer to the addon that is selected.
	Addon* selected;
	
	//The list used for listing the addons.
	GUIListBox* list;
	//The button that does install/uninstall.
	GUIObject* actionButton;
	//The button used for the updates
	GUIObject* updateButton;
	
	//The possible actions for the action button.
	//Note: Update was moved to an extra button.
	enum Action{
		NONE, INSTALL, UNINSTALL
	};
	//The current action.
	Action action;
public:
	//Constructor.
	Addons();
	//Destructor.
	~Addons();
	
	//Method that will create the GUI.
	void createGUI();
	
	//Method that loads that downloads the addons list.
	//file: Pointer to the file to download the list to.
	//Returns: True if the file is downloaded successfuly.
	bool getAddonsList(FILE* file);
	//
	void fillAddonList(std::vector<Addons::Addon> &list,TreeStorageNode &addons,TreeStorageNode &installed);
	//Put all the addons of a given type in a vector.
	//type: The type the addons must be.
	//Returns: Vector containing the addons.
	std::vector<std::string> addonsToList(const string &type);
	
	//Method that will save the installed addons to the installed_addons file.
	//Returns: True if the file is saved successfuly.
	bool saveInstalledAddons();
	
	//Inherited from GameState.
	void handleEvents();
	void logic();
	void render();
	void resize();
	
	//Method used for GUI event handling.
	//name: The name of the callback.
	//obj: Pointer to the GUIObject that caused the event.
	//eventType: The type of event: click, change, etc..
	void GUIEventCallback_OnEvent(std::string name,GUIObject* obj,int eventType);
	
	//Updates the text on the action button to the current action.
	void updateActionButton();
	//Switches enabled/disables of the update button
	void updateUpdateButton();
};
#endif
