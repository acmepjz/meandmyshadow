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
#include "GameState.h"
#include "CreditsMenu.h"
#include "ThemeManager.h"
#include "GUITextArea.h"
#include "InputManager.h"
#include "MusicManager.h"
#include <iostream>

using namespace std;

/////////////////////////CREDITS_MENU//////////////////////////////////

Credits::Credits(ImageManager& imageManager,SDL_Renderer& renderer){
	//Render the title.
    title=textureFromText(renderer, *fontTitle,_("Credits"),objThemes.getTextColor(false));

	//Vector that will hold every line of the credits.
	vector<string> credits;

	//Open the AUTHORS file and read every line.
	{
		ifstream fin((getDataPath()+"/../AUTHORS").c_str());
		if(!fin.is_open()) {
			cerr<<"ERROR: Unable to open the AUTHORS file."<<endl;
			credits.push_back("ERROR: Unable to open the AUTHORS file.");
			credits.push_back("");
		}

		//Loop the lines of the file.
		string line;
		while(getline(fin,line)){
			credits.push_back(line);
		}
	}

	//Enter a new line between the two files.
	credits.push_back("");
	
	//Open the Credits.text file and read every line.
	{
		ifstream fin((getDataPath()+"/Credits.txt").c_str());
		if(!fin.is_open()) {
			cerr<<"ERROR: Unable to open the Credits.txt file."<<endl;
			credits.push_back("ERROR: Unable to open the Credits.txt file.");
			credits.push_back("");
		}

		//Loop the lines of the file.
		string line;
		while(getline(fin,line)){
			credits.push_back(line);
			
			//NOTE: Some sections point to other credits files.
			if(line=="music/") {
				vector<string> musicCredits=getMusicManager()->createCredits();
				credits.insert(credits.end(),musicCredits.begin(),musicCredits.end());
			}
		}
	}
	
	//Create the root element of the GUI.
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}
    GUIObjectRoot=new GUIObject(imageManager,renderer,0,0,SCREEN_WIDTH,SCREEN_HEIGHT);
	
	//Create back button.
    backButton=new GUIButton(imageManager,renderer,SCREEN_WIDTH*0.5,SCREEN_HEIGHT-60,-1,36,_("Back"),0,true,true,GUIGravityCenter);
	backButton->name="cmdBack";
	backButton->eventCallback=this;
	GUIObjectRoot->addChild(backButton);
	
	//Create a text area for credits.
    textArea=new GUITextArea(imageManager,renderer,SCREEN_WIDTH*0.05,114,SCREEN_WIDTH*0.9,SCREEN_HEIGHT-200);
	textArea->setFont(fontMono);
    textArea->setStringArray(renderer, std::move(credits));
	textArea->editable=false;
	textArea->extractHyperlinks();
	GUIObjectRoot->addChild(textArea);
}

Credits::~Credits(){
	//Delete the GUI.
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}
}

void Credits::GUIEventCallback_OnEvent(ImageManager& imageManager, SDL_Renderer& renderer, std::string name,GUIObject* obj,int eventType){
	//Check what type of event it was.
	if(eventType==GUIEventClick){
		if(name=="cmdBack"){
			//Goto the main menu.
			setNextState(STATE_MENU);
		}
	}
}

void Credits::handleEvents(ImageManager&, SDL_Renderer&){
	//Check if we need to quit, if so enter the exit state.
	if(event.type==SDL_QUIT){
		setNextState(STATE_EXIT);
	}

	//Check movement
	if (inputMgr.isKeyDownEvent(INPUTMGR_RIGHT)){
		isKeyboardOnly = true;
		textArea->scrollScrollbar(20, 0);
	} else if (inputMgr.isKeyDownEvent(INPUTMGR_LEFT)){
		isKeyboardOnly = true;
		textArea->scrollScrollbar(-20, 0);
	} else if (inputMgr.isKeyDownEvent(INPUTMGR_UP)){
		isKeyboardOnly = true;
		textArea->scrollScrollbar(0, -1);
	} else if (inputMgr.isKeyDownEvent(INPUTMGR_DOWN)){
		isKeyboardOnly = true;
		textArea->scrollScrollbar(0, 1);
	}

	//Check if the escape button is pressed, if so go back to the main menu.
	if(inputMgr.isKeyDownEvent(INPUTMGR_ESCAPE)){
		setNextState(STATE_MENU);
	}
}

void Credits::logic(ImageManager&, SDL_Renderer&){}

void Credits::render(ImageManager&,SDL_Renderer &renderer){
	//Draw background.
    objThemes.getBackground(true)->draw(renderer);
	objThemes.getBackground(true)->updateAnimation();
	
	//Now render the title.
    drawTitleTexture(SCREEN_WIDTH, *title, renderer);

	//NOTE: The rendering of the GUI is done in Main.
}

void Credits::resize(ImageManager&, SDL_Renderer&){
	//Resize and position widgets.
	GUIObjectRoot->width=SCREEN_WIDTH;
	GUIObjectRoot->height=SCREEN_HEIGHT;
	
	backButton->left=SCREEN_WIDTH/2;
	backButton->top=SCREEN_HEIGHT-60;
	
	textArea->left=SCREEN_WIDTH*0.05;
	textArea->width=SCREEN_WIDTH*0.9;
	textArea->height=SCREEN_HEIGHT-200;
	textArea->onResize();
}
