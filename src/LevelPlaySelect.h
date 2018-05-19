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

#ifndef LEVELPLAYSELECT_H
#define LEVELPLAYSELECT_H

#include "GameState.h"
#include "LevelSelect.h"
#include "LevelInfoRender.h"
#include "GameObjects.h"
#include "Player.h"
#include "GUIObject.h"
#include <vector>
#include <string>

//This is the LevelSelect state, here you can select levelpacks and levels.
class LevelPlaySelect : public LevelSelect{
private:
	//Pointer to the play button, it is only shown when a level is selected.
	GUIObject* play;
	
	//Image of a play icon used as button to start replays.
    SharedTexture playButtonImage;

    //Textures to display level info.
    LevelInfoRender levelInfoRender;

    std::string bestTimeFilePath;
    std::string bestRecordingFilePath;
	
	//Method that will create the GUI elements.
	//initial: Boolean if it is the first time the gui is created.
    void createGUI(ImageManager& imageManager, SDL_Renderer& renderer, bool initial);
	
	//display level info.
    void displayLevelInfo(ImageManager &imageManager, SDL_Renderer &renderer, int number);

	//Check where and if the mouse clicked on a number.
	//If so it will start the level.
    void checkMouse(ImageManager &imageManager, SDL_Renderer &renderer) override;
public:
	//Constructor.
    LevelPlaySelect(ImageManager &imageManager, SDL_Renderer &renderer);
	//Destructor.
	~LevelPlaySelect();

	//Inherited from LevelSelect.
    void refresh(ImageManager &imageManager, SDL_Renderer &renderer, bool change=true) override;
    void selectNumber(ImageManager &imageManager, SDL_Renderer &renderer, unsigned int number, bool selected) override;
	
	//Inherited from GameState.
    void render(ImageManager&imageManager, SDL_Renderer& renderer) override;
	
	//Inherited from GameState.
    void resize(ImageManager &imageManager, SDL_Renderer& renderer) override;

	//Inherited from LevelSelect.
    void renderTooltip(SDL_Renderer& renderer,unsigned int number,int dy) override;
	
	//GUI events will be handled here.
	void GUIEventCallback_OnEvent(ImageManager& imageManager, SDL_Renderer& renderer, std::string name,GUIObject* obj,int eventType);
};

#endif
