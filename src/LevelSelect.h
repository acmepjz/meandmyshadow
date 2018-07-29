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

#ifndef LEVELSELECT_H
#define LEVELSELECT_H

#include "GameState.h"
#include "GameObjects.h"
#include "GUIObject.h"
#include "LevelPackManager.h"
#include <vector>
#include <string>

class GUIScrollBar;
class GUISingleLineListBox;

//Class that represents a level in the levelselect menu.
class Number{
private:
	//The background image of the number.
	ThemeBlockInstance block;
	//The background image of the number when it's locked.
	ThemeBlockInstance blockLocked;
	//The (text) image of the number.
    SharedTexture image;
	
	//Image containing the three stars a player can earn.
    SharedTexture medals;

	//The number (or text).
	int number;
	//Integer containing the medal the player got.
	//0 = none, 1 = bronze, 2 = silver, 3 = gold
	int medal;
	
	//Boolean if the number is locked or not.
	bool locked;
public:
	//The location and size of the number.
	SDL_Rect box;

	//If the Number is selected then we draw something indicates it.
	bool selected;

	//Constructor.
    Number(ImageManager &imageManager, SDL_Renderer &renderer);

	//Method used for initialising the number.
	//number: The number.
	//box: The location and size of the number.
    void init(SDL_Renderer &renderer, int number, SDL_Rect box);
	
	//Method used for initialising the number.
	//text: The caption of the number.
	//box: The location and size of the number.
    void init(SDL_Renderer& renderer,std::string text,SDL_Rect box,int number_=-1);

	//get current number.
	inline int getNumber(){return number;}
	
	//Method used to set the locked status of the number.
	//locked: Boolean if it should be locked or not.
	void setLocked(bool locked=true);
	//Method used to retrieve the locked status of the number.
	//Returns: True if the number is locked.
	inline bool getLocked(){return locked;}
	
	//Method used to set the medal for this number.
	//medal: The new medal for this number.
	void setMedal(int medal);
	
	//Method that is used to draw the number.
	//dy: The y offset.
    void show(SDL_Renderer &renderer, int dy);
};

struct ToolTip {
    TexturePtr name;
    TexturePtr time;
    TexturePtr recordings;
    size_t number;
};

//This is the LevelSelect state, here you can select levelpacks and levels.
class LevelSelect : public GameState,public GUIEventCallback{
protected:
	//Surface containing the title.
    TexturePtr title;

    ToolTip toolTip;
	
	//Vector containing the numbers.
	std::vector<Number> numbers;
	
	//Contains selected level number (displayed at bottom left corner).
	//If it's NULL then nothing selected.
	Number* selectedNumber;
	
	//Pointer to the scrollbar.
	GUIScrollBar* levelScrollBar;
	//Pointer to the description.
	GUIObject* levelpackDescription;
	
	//Pointer to the levelpack list.
	GUISingleLineListBox* levelpacks;
	
	//Check where and if the mouse clicked on a number.
	//If so select that number.
    virtual void checkMouse(ImageManager& imageManager, SDL_Renderer& renderer);
	
	//Selected section for keyboard/gamepad control
	//1=level pack selection 2=level selection 3=LevelPlaySelect/LevelEditSelect specific
	int section;

	//LevelPlaySelect/LevelEditSelect specific selected section. Only used when section==3.
	int section2;

	//The number of blocks in a row.
	int LEVELS_PER_ROW;
	//The number of levels displayed on screen at once.
	int LEVELS_DISPLAYED_IN_SCREEN;
public:
	//Constructor.
	//titleText: The title that is shown at the top of the screen.
	//packType: The type of levelpacks that should be listed (See LevelPackManager.h).
    LevelSelect(ImageManager& imageManager, SDL_Renderer &renderer, const char* titleText, LevelPackManager::LevelPackLists packType=LevelPackManager::ALL_PACKS);
	//Destructor.
	virtual ~LevelSelect();

	//Method that will calculate the number of rows and the number of levels per row.
	void calcRows();
	
	//Method used to update the numbers and the scrollbar.
	//change: Boolean if the levelpack changed, if not only the numbers need to be replaced.
    virtual void refresh(ImageManager& imageManager, SDL_Renderer& renderer, bool change=true)=0;
	
	//Method that is called when a number is selected.
	//number: The selected number.
	//selected: Boolean if the number was already selected.
    virtual void selectNumber(ImageManager &imageManager, SDL_Renderer &renderer, unsigned int number,bool selected)=0;
	
	//Used for keyboard/gamepad navigation
    void selectNumberKeyboard(ImageManager &imageManager, SDL_Renderer &renderer, int x, int y);

    //Inherited from GameState.
    void handleEvents(ImageManager& imageManager, SDL_Renderer& renderer) override;
    void logic(ImageManager&, SDL_Renderer&) override;
    void render(ImageManager&, SDL_Renderer& renderer) override;
    void resize(ImageManager &imageManager, SDL_Renderer& renderer) override;
	
	//Method that is called to render the tooltip.
	//number: The number that the tooltip should be drawn for.
	//dy: The y offset of the number, used to draw the tooltip in the right place.
    virtual void renderTooltip(SDL_Renderer& renderer,unsigned int number,int dy)=0;

	//GUI events will be handled here.
	virtual void GUIEventCallback_OnEvent(ImageManager& imageManager, SDL_Renderer& renderer, std::string name,GUIObject* obj,int eventType)=0;
};

#endif
