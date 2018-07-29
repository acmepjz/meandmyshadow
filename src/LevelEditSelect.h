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

#ifndef LEVELEDITSELECT_H
#define LEVELEDITSELECT_H

#include "LevelSelect.h"
#include "GameState.h"
#include "GUIObject.h"
#include <vector>
#include <string>

//This is the LevelEditSelect state, here you can select levelpacks and levels.
class LevelEditSelect :public LevelSelect{
private:
	//Pointer to the GUIObjectRoot of the levelselect main gui.
	GUIObject* levelEditGUIObjectRoot;
  
	//Pointer to the new levelpack textfield.
	GUIObject* levelpackName;
	
	//Pointer to the levelpack properties button.
	GUIButton* propertiesPack;
	//Pointer to the remove levelpack button.
	GUIButton* removePack;
	
	//Pointer to the move map button.
	GUIButton* move;
	//Pointer to the remove map button.
	GUIButton* remove;
	//Pointer to the edit map button.
	GUIButton* edit;
	
	//String that contains the name of the current levelpack.
	std::string packName;
	
	//Method that will create the GUI elements.
	//initial: Boolean if it is the first time the gui is created.
    void createGUI(ImageManager& imageManager, SDL_Renderer& renderer, bool initial);
	
	//Method that should be called when changing the current levelpack in an abnormal way.
	void changePack();
	
	//This method will show a popup with levelpack specific settings.
	//newPack: Boolean if it's a new levelpack.
    void packProperties(ImageManager& imageManager, SDL_Renderer &renderer, bool newPack);
	
	//This method will show an add level dialog.
    void addLevel(ImageManager& imageManager, SDL_Renderer &renderer);
	
	//This method will show an move level dialog.
    void moveLevel(ImageManager& imageManager, SDL_Renderer &renderer);
public:
	//Constructor.
    LevelEditSelect(ImageManager &imageManager, SDL_Renderer& renderer);
	//Destructor.
	~LevelEditSelect();
	
	//Inherited from LevelSelect.
	//change: Boolean if the levelpack changed, if not we only have to rearrange the numbers.
    void refresh(ImageManager &imageManager, SDL_Renderer &renderer, bool change=true) override;
    void selectNumber(ImageManager &imageManager, SDL_Renderer &renderer, unsigned int number,bool selected) override;
	void handleEvents(ImageManager& imageManager, SDL_Renderer& renderer) override;

	//Inherited from GameState.
    void render(ImageManager&imageManager, SDL_Renderer& renderer) override;
	
	//Inherited from GameState.
    void resize(ImageManager &imageManager, SDL_Renderer& renderer) override;

	//Inherited from LevelSelect.
    void renderTooltip(SDL_Renderer& renderer,unsigned int number,int dy) override;
	
	//GUI events will be handled here.
    void GUIEventCallback_OnEvent(ImageManager& imageManager, SDL_Renderer& renderer, std::string name,GUIObject* obj,int eventType) override;
};

#endif
