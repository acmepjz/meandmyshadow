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

#ifndef TITLE_MENU_H
#define TITLE_MENU_H

#include <array>

#include <SDL.h>
#include "GameState.h"

#include "Render.h"
#include "ImageManager.h"

//The Main menu.
class Menu : public GameState{
private:
    //The title of the main menu.
    SharedTexture titleTexture;
	
    //Array containg pointers the five main menu entries.
	//The last two are the '>' and '<' characters.
    std::array<TexturePtr, 7> entries;

    //The icon and for the statistics menu.
    SharedTexture statisticsIcon;
    TexturePtr statisticsTooltip;

	//The icon for the credits menu.
    SharedTexture creditsIcon;
    TexturePtr creditsTooltip;
	
	//Integer used for animations.
	int animation;
	
	//Integer containing the highlighted/selected menu option.
	int highlight;
public:
	//Constructor.
    Menu(ImageManager& imageManager, SDL_Renderer &renderer);
	//Destructor.
	~Menu();

	//Inherited from GameState.
    void handleEvents(ImageManager& imageManager, SDL_Renderer& renderer) override;
    void logic(ImageManager&, SDL_Renderer&) override;
    void render(ImageManager&, SDL_Renderer& renderer) override;
    void resize(ImageManager&, SDL_Renderer&) override;
};

#endif
