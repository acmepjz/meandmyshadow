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

#ifndef CREDITS_MENU_H
#define CREDITS_MENU_H

#include <SDL.h>
#include "GameState.h"

//Included for the Credits menu.
#include "Render.h"
#include "GUIObject.h"
#include "ImageManager.h"

class GUITextArea;

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
