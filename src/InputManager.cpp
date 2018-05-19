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

#include "InputManager.h"
#include "Globals.h"
#include "Settings.h"
#include "Functions.h"
#include "GUIObject.h"
#include "GUIListBox.h"
#include "GUIOverlay.h"
#include <stdlib.h>
#include <stdio.h>
#include <string>
using namespace std;

InputManager inputMgr;

//the order must be the same as InputManagerKeys
static const char* keySettingNames[INPUTMGR_MAX]={
	"key_up","key_down","key_left","key_right","key_jump","key_action","key_space","key_cancelRecording",
	"key_escape","key_restart","key_tab","key_save","key_load","key_swap",
	"key_teleport","key_suicide","key_shift","key_next","key_previous","key_select"
};

//the order must be the same as InputManagerKeys
static const char* keySettingDescription[INPUTMGR_MAX]={
	__("Up (in menu)"),__("Down (in menu)"),__("Left"),__("Right"),__("Jump"),__("Action"),__("Space (Record)"),__("Cancel recording"),
	__("Escape"),__("Restart"),__("Tab (View shadow/Level prop.)"),__("Save game (in editor)"),__("Load game"),__("Swap (in editor)"),
	__("Teleport (in editor)"),__("Suicide (in editor)"),__("Shift (in editor)"),__("Next block type (in Editor)"),
	__("Previous block type (in editor)"), __("Select (in menu)")
};

//A class that handles the gui events of the inputDialog.
class InputDialogHandler:public GUIEventCallback{
private:
	//the list box which contains keys.
	GUIListBox* listBox;
	//the parent object.
	InputManager* parent;

	//update specified key config item
    void updateConfigItem(SDL_Renderer& renderer,int index){
		//Get the description of the key.
		std::string s=_(keySettingDescription[index]);
		s+=": ";
		
		//Get the key code name.
		int keyCode=parent->getKeyCode((InputManagerKeys)index,false);
		s+=_(InputManager::getKeyCodeName(keyCode));

		//Add the alternative key if there is one.
		int keyCodeAlt=parent->getKeyCode((InputManagerKeys)index,true);
		if(keyCodeAlt!=0){
			s+=" ";
			s+=_("OR");
			s+=" ";
			s+=_(InputManager::getKeyCodeName(keyCodeAlt));
		}

		//Update item.
        listBox->updateItem(renderer, index,s);
	}
public:
	//Constructor.
    InputDialogHandler(SDL_Renderer& renderer,GUIListBox* listBox,InputManager* parent):listBox(listBox),parent(parent){
		//load the available keys to the list box.
		for(int i=0;i<INPUTMGR_MAX;i++){
			//Get the description of the key.
			std::string s=_(keySettingDescription[i]);
			s+=": ";
			
			//Get key code name.
			int keyCode=parent->getKeyCode((InputManagerKeys)i,false);
			s+=_(InputManager::getKeyCodeName(keyCode));
			
			//Add the alternative key if there is one.
			int keyCodeAlt=parent->getKeyCode((InputManagerKeys)i,true);
			if(keyCodeAlt!=0){
				s+=" ";
				s+=_("OR");
				s+=" ";
				s+=_(InputManager::getKeyCodeName(keyCodeAlt));
			}

			//Add item.
            listBox->addItem(renderer, s);
		}
	}

    virtual ~InputDialogHandler(){}
	
	//When a key is pressed call this to set the key to currently-selected item.
    void onKeyDown(SDL_Renderer& renderer,int keyCode){
		//Check if an item is selected.
		int index=listBox->value;
		if(index<0 || index>=INPUTMGR_MAX) return;
		
		//Update the key.
		//SDLK_BACKSPACE will erase the key.
		if(keyCode==SDLK_BACKSPACE){
			parent->setKeyCode((InputManagerKeys)index,0,true);
			parent->setKeyCode((InputManagerKeys)index,0,false);
            updateConfigItem(renderer,index);
		}else{
			//Update the main key if there isn't one. Otherwise update the alternative key if there isn't one.
			int key=parent->getKeyCode((InputManagerKeys)index,false);
			int altKey=parent->getKeyCode((InputManagerKeys)index,true);
			if(key==0){
				parent->setKeyCode((InputManagerKeys)index,keyCode,false);
			}else if((altKey==0)&&(keyCode!=key)){
				parent->setKeyCode((InputManagerKeys)index,keyCode,true);
			}
            updateConfigItem(renderer,index);
		}
	}

	void GUIEventCallback_OnEvent(ImageManager& imageManager, SDL_Renderer& renderer, std::string name,GUIObject* obj,int eventType){
		//Do nothing...
	}
};

//Event handler.
static InputDialogHandler* handler;

//A GUIObject that is used to create events for key presses.
class GUIKeyListener:public GUIObject{
public:

    GUIKeyListener(ImageManager& imageManager, SDL_Renderer& renderer)
        :GUIObject(imageManager,renderer){}
private:
	//Leave empty.
	~GUIKeyListener(){}

    bool handleEvents(SDL_Renderer& renderer,int x,int y,bool enabled,bool visible,bool processed){
		if(enabled && handler){
			if(event.type==SDL_KEYDOWN){
                handler->onKeyDown(renderer,event.key.keysym.sym);
			}
			//Joystick
			else if(event.type==SDL_JOYAXISMOTION){
				if(event.jaxis.value>3200){
                    handler->onKeyDown(renderer,0x00010001 | (int(event.jaxis.axis)<<8));
				}else if(event.jaxis.value<-3200){
                    handler->onKeyDown(renderer,0x000100FF | (int(event.jaxis.axis)<<8));
				}
			}
			else if(event.type==SDL_JOYBUTTONDOWN){
                handler->onKeyDown(renderer,0x00020000 | (int(event.jbutton.button)<<8));
			}
			else if(event.type==SDL_JOYHATMOTION){
				if(event.jhat.value & SDL_HAT_LEFT){
                    handler->onKeyDown(renderer,0x00030000 | (int(event.jhat.hat)<<8) | SDL_HAT_LEFT);
				}else if(event.jhat.value & SDL_HAT_RIGHT){
                    handler->onKeyDown(renderer,0x00030000 | (int(event.jhat.hat)<<8) | SDL_HAT_RIGHT);
				}else if(event.jhat.value & SDL_HAT_UP){
                    handler->onKeyDown(renderer,0x00030000 | (int(event.jhat.hat)<<8) | SDL_HAT_UP);
				}else if(event.jhat.value & SDL_HAT_DOWN){
                    handler->onKeyDown(renderer,0x00030000 | (int(event.jhat.hat)<<8) | SDL_HAT_DOWN);
				}
			}
		}
		return false;
	}

	//Nothing to do.
	void render(){}
};

int InputManager::getKeyCode(InputManagerKeys key,bool isAlternativeKey){
	if(isAlternativeKey) return alternativeKeys[key];
	else return keys[key];
}

void InputManager::setKeyCode(InputManagerKeys key,int keyCode,bool isAlternativeKey){
	if(isAlternativeKey) alternativeKeys[key]=keyCode;
	else keys[key]=keyCode;
}

void InputManager::loadConfig(){
	for(int i=0;i<INPUTMGR_MAX;i++){
		string s=keySettingNames[i];
		
		keys[i]=atoi(getSettings()->getValue(s).c_str());
		
		s+="2";
		alternativeKeys[i]=atoi(getSettings()->getValue(s).c_str());
		
		//Move the alternative key to main key if the main key is empty.
		if(keys[i]==0&&alternativeKeys[i]!=0){
			keys[i]=alternativeKeys[i];
			alternativeKeys[i]=0;
		}
		
		//Remove duplicate.
		if(keys[i]==alternativeKeys[i])
			alternativeKeys[i]=0;
	}
}

void InputManager::saveConfig(){
	int i;
	char c[32];
	for(i=0;i<INPUTMGR_MAX;i++){
		string s=keySettingNames[i];
		
		//Remove duplicate.
		if(keys[i]==alternativeKeys[i])
			alternativeKeys[i]=0;

		sprintf(c,"%d",keys[i]);
		getSettings()->setValue(s,c);

		s+="2";
		sprintf(c,"%d",alternativeKeys[i]);
		getSettings()->setValue(s,c);
	}
}

GUIObject* InputManager::showConfig(ImageManager& imageManager, SDL_Renderer& renderer,int height){
	//Create the new GUI.
    GUIObject* root=new GUIObject(imageManager,renderer,0,0,SCREEN_WIDTH,height);

	//Instruction label.
    GUIObject* obj=new GUILabel(imageManager,renderer,0,6,root->width,36,_("Select an item and press a key to change it."),0,true,true,GUIGravityCenter);
	root->addChild(obj);
	
    obj=new GUILabel(imageManager,renderer,0,30,root->width,36,_("Press backspace to clear the selected item."),0,true,true,GUIGravityCenter);
	root->addChild(obj);
	
	//The listbox for keys.
    GUIListBox *listBox=new GUIListBox(imageManager,renderer,SCREEN_WIDTH*0.15,72,SCREEN_WIDTH*0.7,height-72-8);
	root->addChild(listBox);
	
	//Create the event handler.
	if(handler)
		delete handler;
    handler=new InputDialogHandler(renderer,listBox,this);

    obj=new GUIKeyListener(imageManager,renderer);
	root->addChild(obj);

	//Return final widget.
	return root;
}

//Get key name from key code.
std::string InputManager::getKeyCodeName(int keyCode){
	char c[64];
//	if(keyCode>0 && keyCode <0x1000){
		//Keyboard.
		const char* s=SDL_GetKeyName(static_cast<SDL_Keycode>(keyCode));
		if(s!=NULL){
			return s;
		}else{
            snprintf(c,64,"(Key %d)",keyCode);
			return c;
		}
		//TODO Key code numbers seems to have changed in SDL2
/*	}else if(keyCode>0x1000){
		//Joystick. First set it to invalid value.
		sprintf(c,"(Joystick 0x%08X)",keyCode);
		//Check the input type.
		switch((keyCode & 0x00FF0000)>>16){
		case 1:
			//Axis.
			switch(keyCode & 0xFF){
			case 1:
				sprintf(c,"Joystick axis %d +",(keyCode & 0x0000FF00)>>8);
				break;
			case 0xFF:
				sprintf(c,"Joystick axis %d -",(keyCode & 0x0000FF00)>>8);
				break;
			}
			break;
		case 2:
			//Button.
			sprintf(c,"Joystick button %d",(keyCode & 0x0000FF00)>>8);
			break;
		case 3:
			//Hat.
			switch(keyCode & 0xFF){
			case SDL_HAT_LEFT:
				sprintf(c,"Joystick hat %d left",(keyCode & 0x0000FF00)>>8);
				break;
			case SDL_HAT_RIGHT:
				sprintf(c,"Joystick hat %d right",(keyCode & 0x0000FF00)>>8);
				break;
			case SDL_HAT_UP:
				sprintf(c,"Joystick hat %d up",(keyCode & 0x0000FF00)>>8);
				break;
			case SDL_HAT_DOWN:
				sprintf(c,"Joystick hat %d down",(keyCode & 0x0000FF00)>>8);
				break;
			}
			break;
		}
		return c;
	}else{
		//Disabled or unknown.
		return "-";
	}*/
}

InputManager::InputManager(){
	//Clear the arrays.
	for(int i=0;i<INPUTMGR_MAX;i++){
		keys[i]=alternativeKeys[i]=keyFlags[i]=0;
	}
}

InputManager::~InputManager(){
	closeAllJoysticks();
}

int InputManager::getKeyState(int keyCode,int oldState,bool hasEvent){
	int state=0;
//	if(keyCode>0 && keyCode<0x1000){
		//Keyboard.
		if(hasEvent){
			if(event.type==SDL_KEYDOWN && event.key.keysym.sym==keyCode){
				state|=0x2;
			}
			if(event.type==SDL_KEYUP && event.key.keysym.sym==keyCode){
				state|=0x4;
			}
		}
		//Get keyboard state for key code
		//SDL_GetKeyboardState needs a scankey rather than keycode, so we convert
		int numKeys = 0;
		const Uint8* keyStates = SDL_GetKeyboardState(&numKeys);
		SDL_Scancode scanCode = SDL_GetScancodeFromKey(keyCode);
		if(scanCode<numKeys && keyStates[scanCode]){
			state|=0x1;
		}
		//TODO - key code numbers seems to have changed in SDL2, so disabling joystick for now
/*	}else if(keyCode>0x1000){
		//Joystick.
		int index=(keyCode & 0x0000FF00)>>8;
		int value=keyCode & 0xFF;
		int i,v;
		switch((keyCode & 0x00FF0000)>>16){
		case 1:
			//Axis.
			if(hasEvent){
				if(event.type==SDL_JOYAXISMOTION && event.jaxis.axis==index){
					if((value==1 && event.jaxis.value>3200) || (value==0xFF && event.jaxis.value<-3200)){
						if((oldState & 0x1)==0) state|=0x2;
					}else{
						if(oldState & 0x1) state|=0x4;
					}
				}
			}
			for(i=0;i<(int)joysticks.size();i++){
				v=SDL_JoystickGetAxis(joysticks[i],index);
				if((value==1 && v>3200) || (value==0xFF && v<-3200)){
					state|=0x1;
					break;
				}
			}
			break;
		case 2:
			//Button.
			if(hasEvent){
				if(event.type==SDL_JOYBUTTONDOWN && event.jbutton.button==index){
					state|=0x2;
				}
				if(event.type==SDL_JOYBUTTONUP && event.jbutton.button==index){
					state|=0x4;
				}
			}
			for(i=0;i<(int)joysticks.size();i++){
				v=SDL_JoystickGetButton(joysticks[i],index);
				if(v){
					state|=0x1;
					break;
				}
			}
			break;
		case 3:
			//Hat.
			if(hasEvent){
				if(event.type==SDL_JOYHATMOTION && event.jhat.hat==index){
					if(event.jhat.value & value){
						if((oldState & 0x1)==0) state|=0x2;
					}else{
						if(oldState & 0x1) state|=0x4;
					}
				}
			}
			for(i=0;i<(int)joysticks.size();i++){
				v=SDL_JoystickGetHat(joysticks[i],index);
				if(v & value){
					state|=0x1;
					break;
				}
			}
			break;
		}
	}*/
	return state;
}

//Update the key state, according to current SDL event, etc.
void InputManager::updateState(bool hasEvent){
	for(int i=0;i<INPUTMGR_MAX;i++){
		keyFlags[i]=getKeyState(keys[i],keyFlags[i],hasEvent)|getKeyState(alternativeKeys[i],keyFlags[i],hasEvent);
	}
}

//Check if there is KeyDown event.
bool InputManager::isKeyDownEvent(InputManagerKeys key){
	return keyFlags[key]&0x2;
}

//Check if there is KeyUp event.
bool InputManager::isKeyUpEvent(InputManagerKeys key){
	return keyFlags[key]&0x4;
}

//Check if specified key is down.
bool InputManager::isKeyDown(InputManagerKeys key){
	return keyFlags[key]&0x1;
}

//Open all joysticks.
void InputManager::openAllJoysitcks(){
	int i,m;
	//First close previous joysticks.
	closeAllJoysticks();

	//Open all joysticks.
	m=SDL_NumJoysticks();
	for(i=0;i<m;i++){
		SDL_Joystick *j=SDL_JoystickOpen(i);
		if(j==NULL){
			printf("ERROR: Couldn't open Joystick %d\n",i);
		}else{
			joysticks.push_back(j);
		}
	}
}

//Close all joysticks.
void InputManager::closeAllJoysticks(){
	for(int i=0;i<(int)joysticks.size();i++){
		SDL_JoystickClose(joysticks[i]);
	}
	joysticks.clear();
}
