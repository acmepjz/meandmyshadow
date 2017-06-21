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

//Included for the Options menu.
#include "Render.h"
#include "GUIObject.h"
#include "GUIListBox.h"
#include "GUISlider.h"
#include "GUITextArea.h"
#include "InputManager.h"
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

//The Options menu.
class Options : public GameState, private GUIEventCallback{
private:
	//The title of the options menu.
    TexturePtr title;

	//Icon.
    SharedTexture clearIcon;
	bool clearIconHower;
    TexturePtr clearTooltip;

	//Slider used to set the music volume
	GUISlider* musicSlider;
	//Slider used to set the sound volume
	GUISlider* soundSlider;
	
	//Integer to keep track of the time passed since last playing the test sound.
	int lastJumpSound;
	
	//ListBox containing the themes the user can choose out.
	GUISingleLineListBox* theme;
	
	//Available languages
	GUISingleLineListBox* langs;
	
	//Resolution list
	GUISingleLineListBox* resolutions;
	
	//Containers for different tabs.
	GUIObject* tabGeneral;
	GUIObject* tabControls;
	
	//Keys.
	int tmpKeys[INPUTMGR_MAX];
	int tmpAlternativeKeys[INPUTMGR_MAX];
	
	//GUI events are handled here.
	//name: The name of the element that invoked the event.
	//obj: Pointer to the object that invoked the event.
	//eventType: Integer containing the type of event.
	void GUIEventCallback_OnEvent(ImageManager& imageManager, SDL_Renderer& renderer, std::string name,GUIObject* obj,int eventType);

public:
	//Constructor.
    Options(ImageManager& imageManager,SDL_Renderer& renderer);
	//Destructor.
	~Options();
	
	//Method that will create the GUI for the options menu.
    void createGUI(ImageManager &imageManager, SDL_Renderer &renderer);
	
	//Inherited from GameState.
    void handleEvents(ImageManager& imageManager, SDL_Renderer& renderer) override;
    void logic(ImageManager&, SDL_Renderer&) override;
    void render(ImageManager&, SDL_Renderer& renderer) override;
    void resize(ImageManager &imageManager, SDL_Renderer&renderer) override;
};

//The Credits menu.
class Credits : public GameState, private GUIEventCallback{
private:
	//The title of the credits menu.
    TexturePtr title;

	//Widgets.
	GUITextArea* textArea;
	GUIObject* backButton;
	
	//GUI events are handled here.
	//name: The name of the element that invoked the event.
	//obj: Pointer to the object that invoked the event.
	//eventType: Integer containing the type of event.
	void GUIEventCallback_OnEvent(ImageManager& imageManager, SDL_Renderer& renderer, std::string name,GUIObject* obj,int eventType);
	
public:
	//Constructor.
    Credits(ImageManager& imageManager, SDL_Renderer &renderer);
	//Destructor.
	~Credits();
	
	//Inherited from GameState.
    void handleEvents(ImageManager&, SDL_Renderer&) override;
    void logic(ImageManager&, SDL_Renderer&) override;
    void render(ImageManager&, SDL_Renderer& renderer) override;
    void resize(ImageManager &imageManager, SDL_Renderer& renderer) override;
};

#endif
