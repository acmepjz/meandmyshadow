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

#include "Functions.h"
#include "Settings.h"
#include "GameState.h"
#include "TitleMenu.h"
#include "ThemeManager.h"
#include "InputManager.h"
#include <iostream>

using namespace std;

/////////////////////////MAIN_MENU//////////////////////////////////

Menu::Menu(ImageManager &imageManager, SDL_Renderer& renderer){
	animation=highlight=0;
	
	//Load the title image.
    titleTexture=imageManager.loadTexture(getDataPath()+"gfx/menu/title.png",renderer);


    auto tft = [&](const char* text){
		return titleTextureFromText(renderer, text, objThemes.getTextColor(false), SCREEN_WIDTH - 100);
    };

    //Now render the five entries.
    entries[0]=tft(_("Play"));
    entries[1]=tft(_("Options"));
    entries[2]=tft(_("Map Editor"));
    entries[3]=tft(_("Addons"));
    entries[4]=tft(_("Quit"));
    entries[5]=tft(">");
    entries[6]=tft("<");

    //Load the textures for the credits and statistics buttons and their tooltips.
    creditsIcon=imageManager.loadTexture(getDataPath()+"gfx/menu/credits.png", renderer);
    creditsTooltip=textureFromText(renderer, *fontText, _("Credits"), objThemes.getTextColor(true));
    statisticsIcon=imageManager.loadTexture(getDataPath()+"gfx/menu/statistics.png", renderer);
    statisticsTooltip=textureFromText(renderer, *fontText, _("Achievements and Statistics"), objThemes.getTextColor(true));

    // Check if textures were loaded.
    //TODO: Handle this better.
    if(!titleTexture || !creditsIcon || !statisticsIcon) {
        std::cerr << "Failed to load one or more images for the main menu, exiting. : " << SDL_GetError() << std::endl;
        std::terminate();
    }
}

Menu::~Menu(){
}


void Menu::handleEvents(ImageManager& imageManager, SDL_Renderer& renderer){
	//Get the x and y location of the mouse.
	int x,y;
	SDL_GetMouseState(&x,&y);

	//Calculate which option is highlighted using the location of the mouse.
	//Only if mouse is 'doing something'
	if(event.type==SDL_MOUSEMOTION || event.type==SDL_MOUSEBUTTONDOWN){
		highlight=0;

		if(x>=200&&x<SCREEN_WIDTH-200&&y>=(SCREEN_HEIGHT-250)/2&&y<(SCREEN_HEIGHT-200)/2+320){
			highlight=(y-((SCREEN_HEIGHT-200)/2-64))/64;
			if(highlight>5) highlight=0;
		}

		//Also check the icons.
		if(y>=SCREEN_HEIGHT-56&&y<SCREEN_HEIGHT-8){
			if(x>=SCREEN_WIDTH-8){
				//do nothing
			}else if(x>=SCREEN_WIDTH-56){
				highlight=7;
			}else if(x>=SCREEN_WIDTH-104){
				highlight=6;
			}
		}
	}
	
	//Down/Up -arrows move highlight
	int mod = SDL_GetModState();
	if (inputMgr.isKeyDownEvent(INPUTMGR_DOWN) || inputMgr.isKeyDownEvent(INPUTMGR_RIGHT) || (inputMgr.isKeyDownEvent(INPUTMGR_TAB) && (mod & KMOD_SHIFT) == 0)) {
		isKeyboardOnly = true;
		highlight++;
		if(highlight>7)
			highlight=1;
	} else if (inputMgr.isKeyDownEvent(INPUTMGR_UP) || inputMgr.isKeyDownEvent(INPUTMGR_LEFT) || (inputMgr.isKeyDownEvent(INPUTMGR_TAB) && (mod & KMOD_SHIFT) != 0)) {
		isKeyboardOnly = true;
		highlight--;
		if(highlight<1)
			highlight=7;
	}
	
	//Check if there's a press event.
	if((event.type==SDL_MOUSEBUTTONUP && event.button.button==SDL_BUTTON_LEFT) ||
		(inputMgr.isKeyDownEvent(INPUTMGR_SELECT))){
		//We have one so check which selected/highlighted option needs to be done.
		switch(highlight){
		case 1:
			//Enter the levelSelect state.
			setNextState(STATE_LEVEL_SELECT);
			break;
		case 2:
			//Enter the options state.
			setNextState(STATE_OPTIONS);
			break;
		case 3:
			//Enter the levelEditor, but first set the level to a default leveledit map.
			levelName="";
			setNextState(STATE_LEVEL_EDIT_SELECT);
			break;
		case 4:
			//Check if internet is enabled.
			if(!getSettings()->getBoolValue("internet")){
                msgBox(imageManager,renderer,_("Enable internet in order to install addons."),MsgBoxOKOnly,_("Internet disabled"));
				break;
			}
			
			//Enter the addons state.
			setNextState(STATE_ADDONS);
			break;
		case 5:
			//We quit, so we enter the exit state.
			setNextState(STATE_EXIT);
			break;
		case 6:
			//Show the credits screen.
			setNextState(STATE_CREDITS);
			break;
		case 7:
			//Show the statistics screen.
			setNextState(STATE_STATISTICS);
			break;
		}
	}
	
	//We also need to quit the menu when escape is pressed.
	if(inputMgr.isKeyDownEvent(INPUTMGR_ESCAPE)){
		setNextState(STATE_EXIT);
	}
	
	//Check if we need to quit, if so we enter the exit state.
	if(event.type==SDL_QUIT){
		setNextState(STATE_EXIT);
	}
}

void Menu::logic(ImageManager&, SDL_Renderer&){
	animation++;
	if(animation>10)
		animation=-10;
}

void Menu::render(ImageManager&, SDL_Renderer& renderer){
	//Draw background.
    objThemes.getBackground(true)->draw(renderer);
	objThemes.getBackground(true)->updateAnimation();
	
	//Draw the title.
    {
        int titleWidth, titleHeight = 0;
        SDL_QueryTexture(titleTexture.get(), NULL, NULL, &titleWidth, &titleHeight);
        const SDL_Rect rect = SDL_Rect{(SCREEN_WIDTH-titleWidth)/2, 40, titleWidth, titleHeight};
        SDL_RenderCopy(&renderer, titleTexture.get(), NULL, &rect);
    }

    // Position where we start drawing the menu entries from.
    const int menuStartY = std::max(SCREEN_HEIGHT - 200, 0) / 2;

	//Draw the menu entries.
	for(unsigned int i=0;i<5;i++){
        SDL_Rect dstRect = rectFromTexture(0, 0, *entries[i]);
        dstRect.x = std::max(SCREEN_WIDTH - dstRect.w, 0) / 2;
        dstRect.y = menuStartY + 64*i+(64-dstRect.h) / 2;
        SDL_RenderCopy(&renderer, entries[i].get(), NULL, &dstRect);
	}
	
	//Check if an option is selected/highlighted.
	if(highlight>0 && highlight<=5){
        // Width of highlighted entry.
        const int highlightWidth = rectFromTexture(0, 0, *entries[highlight - 1]).w;
        const int leftOfHighlight = (SCREEN_WIDTH-highlightWidth)/2;
        const SDL_Rect leftSize = rectFromTexture(0, 0, *entries[5]);
        const int rightHeight = rectFromTexture(0, 0, *entries[6]).h;
        // How much to offset the arrows to create the animation.
        const int animationX = (25-abs(animation)/2);
        // The common value of both arrow's y positions.
        const int yCommon = menuStartY-64+64*highlight;

		//Draw the '>' sign, which is entry 5.
        int x=leftOfHighlight-animationX-leftSize.w;
        int y=yCommon+(64-leftSize.h)/2;
        applyTexture(x, y, *entries[5], renderer);

		//Draw the '<' sign, which is entry 6.
        x=leftOfHighlight+highlightWidth+animationX;
        y=yCommon+(64-rightHeight)/2;
        applyTexture(x, y, *entries[6], renderer);
	}

	//Check if an icon is selected/highlighted and draw tooltip
	if(highlight==6 || highlight==7){
        SDL_Texture* texture;
        if(highlight==6) {
            texture = creditsTooltip.get();
        } else {
            texture = statisticsTooltip.get();
        }
        const SDL_Rect textureSize = rectFromTexture(*texture);
        drawGUIBox(-2,SCREEN_HEIGHT-textureSize.h-2,textureSize.w+4,textureSize.h+4,renderer,0xFFFFFFFF);
        applyTexture(0, SCREEN_HEIGHT - textureSize.h, *texture, renderer);
	}

	//Draw border of icon if it's keyboard only mode.
	if (isKeyboardOnly) {
		if (highlight == 6) {
			drawGUIBox(SCREEN_WIDTH - 100, SCREEN_HEIGHT - 52, 40, 40, renderer, 0xFFFFFF40);
		}
		if (highlight == 7) {
			drawGUIBox(SCREEN_WIDTH - 52, SCREEN_HEIGHT - 52, 40, 40, renderer, 0xFFFFFF40);
		}
	}

	//Draw icons.
    applyTexture(SCREEN_WIDTH-96,SCREEN_HEIGHT-48,*creditsIcon,renderer);
    applyTexture(SCREEN_WIDTH-48,SCREEN_HEIGHT-48,*statisticsIcon,renderer);
}

void Menu::resize(ImageManager &, SDL_Renderer&){}

