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
#include "ThemeManager.h"
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <iostream>

#include "SDL_ttf_fontfallback.h"

#include "libs/tinyformat/tinyformat.h"

using namespace std;

InputManager inputMgr;

//the order must be the same as InputManagerKeys
static const char* keySettingNames[INPUTMGR_MAX]={
	"key_up","key_down","key_left","key_right","key_jump","key_action","key_space","key_cancelRecording",
	"key_escape","key_restart","key_tab","key_save","key_load","key_swap",
	"key_teleport","key_suicide","key_shift","key_next","key_previous","key_select",
	"key_pause",
};

//the order must be the same as InputManagerKeys
static const char* keySettingDescription[INPUTMGR_MAX]={
	__("Up"),__("Down"),__("Left"),__("Right"),__("Jump"),__("Action"),__("Record"),__("Cancel recording"),
	__("Back to previous screen"),__("Restart"),__("Toggle shadow camera"),__("Save game"),__("Load game"),__("Swap the player and shadow"),
	__("Teleport player to cursor"),__("Suicide"),__("Move selected object precisely"),__("Next block type"),
	__("Previous block type"), __("Select"), __("Pause game"),
};

InputManagerKeyCode::InputManagerKeyCode(int sym_, int mod_)
	: type(KEYBOARD), sym(sym_), mod(mod_)
{
	// normalize a bit
	if (sym == 0) {
		mod = 0;
	} else {
		mod = ((mod & KMOD_CTRL) ? KMOD_CTRL : 0)
			| ((mod & KMOD_ALT) ? KMOD_ALT : 0)
			| ((mod & KMOD_SHIFT) ? KMOD_SHIFT : 0);

		if (sym == SDLK_LCTRL || sym == SDLK_RCTRL) mod &= ~KMOD_CTRL;
		if (sym == SDLK_LALT || sym == SDLK_RALT) mod &= ~KMOD_ALT;
		if (sym == SDLK_LSHIFT || sym == SDLK_RSHIFT) mod &= ~KMOD_SHIFT;
	}
}


InputManagerKeyCode::InputManagerKeyCode(InputType type_, int buttonIndex_, int buttonValue_)
	: type(type_), buttonIndex(buttonIndex_), buttonValue(buttonValue_)
{
}

InputManagerKeyCode InputManagerKeyCode::createFromString(const std::string& s) {
	int i, j, lp;

	if (s.find("JoystickAxis;") == 0) {
		lp = s.find(';');
		if (sscanf(s.c_str() + (lp + 1), "%d;%d", &i, &j) == 2) {
			return InputManagerKeyCode(JOYSTICK_AXIS, i, j);
		}
	} else if (s.find("JoystickButton;") == 0) {
		lp = s.find(';');
		if (sscanf(s.c_str() + (lp + 1), "%d", &i) == 1) {
			return InputManagerKeyCode(JOYSTICK_BUTTON, i, 0);
		}
	} else if (s.find("JoystickHat;") == 0) {
		lp = s.find(';');
		if (sscanf(s.c_str() + (lp + 1), "%d;%d", &i, &j) == 2) {
			return InputManagerKeyCode(JOYSTICK_HAT, i, j);
		}
	} else {
		i = atoi(s.c_str());
		j = 0;
		if (i) {
			if (s.find(";Ctrl") != std::string::npos) j |= KMOD_CTRL;
			if (s.find(";Alt") != std::string::npos) j |= KMOD_ALT;
			if (s.find(";Shift") != std::string::npos) j |= KMOD_SHIFT;
		}
		return InputManagerKeyCode(i, j);
	}

	fprintf(stderr, "ERROR: Can't parse '%s' as InputManagerKeyCode\n", s.c_str());

	return InputManagerKeyCode();
}

std::string InputManagerKeyCode::toString() const {
	std::ostringstream str;

	switch (type) {
	default:
		if (sym == 0) {
			return std::string();
		} else {
			str << sym;
			if (mod & KMOD_CTRL) str << ";Ctrl";
			if (mod & KMOD_ALT) str << ";Alt";
			if (mod & KMOD_SHIFT) str << ";Shift";
			return str.str();
		}
		break;
	case JOYSTICK_AXIS:
		str << "JoystickAxis;" << buttonIndex << ";" << buttonValue;
		return str.str();
		break;
	case JOYSTICK_BUTTON:
		str << "JoystickButton;" << buttonIndex;
		return str.str();
		break;
	case JOYSTICK_HAT:
		str << "JoystickHat;" << buttonIndex << ";" << buttonValue;
		return str.str();
		break;
	}
}

std::string InputManagerKeyCode::describe() const {
	switch (type) {
	default:
		if (sym == 0) {
			return std::string();
		} else {
			std::ostringstream str;

			if (mod & KMOD_CTRL) str << "CTRL+";
			if (mod & KMOD_ALT) str << "ALT+";
			if (mod & KMOD_SHIFT) str << "SHIFT+";

			const char* s = SDL_GetKeyName(sym);
			if (s != NULL && s[0] != '\0'){
				std::string keyCode = s;
				std::transform(keyCode.begin(), keyCode.end(), keyCode.begin(), [](char c)->char {
					if (c >= 'a' && c <= 'z') {
						return c + ('A' - 'a');
					} else {
						return c;
					}
				});
				str << (dictionaryManager != NULL ? dictionaryManager->get_dictionary().translate_ctxt("keys", keyCode) : keyCode);
			} else{
				/// TRANSLAOTRS: This is used when the name of the key code is not found.
				str << tfm::format(_("(KEY %d)"), sym);
			}

			return str.str();
		}
		break;
	case JOYSTICK_AXIS:
		return tfm::format(_("JOYSTICK AXIS %d %s"), buttonIndex, buttonValue > 0 ? "+" : "-");
		break;
	case JOYSTICK_BUTTON:
		return tfm::format(_("JOYSTICK BUTTON %d"), buttonIndex);
		break;
	case JOYSTICK_HAT:
		switch (buttonValue){
		case SDL_HAT_LEFT:
			return tfm::format(_("JOYSTICK HAT %d LEFT"), buttonIndex);
			break;
		case SDL_HAT_RIGHT:
			return tfm::format(_("JOYSTICK HAT %d RIGHT"), buttonIndex);
			break;
		case SDL_HAT_UP:
			return tfm::format(_("JOYSTICK HAT %d UP"), buttonIndex);
			break;
		case SDL_HAT_DOWN:
			return tfm::format(_("JOYSTICK HAT %d DOWN"), buttonIndex);
			break;
		default:
			fprintf(stderr, "ERROR: Invalid joystick hat value %d\n", buttonValue);
			/// TRANSLAOTRS: This is used when the JOYSTICK_HAT value is invalid.
			return tfm::format(_("JOYSTICK HAT %d %d"), buttonIndex, buttonValue);
			break;
		}
		break;
	}
}

bool InputManagerKeyCode::empty() const {
	return type == KEYBOARD && sym == 0;
}

void InputManagerKeyCode::prependToDescription(std::string& description) const {
	if (empty()) return;

	if (description.empty()) {
		description = describe();
	} else {
		description.insert(0, describe() + " " + _("OR") + " ");
	}
}

void InputManagerKeyCode::appendToDescription(std::string& description) const {
	if (empty()) return;

	if (description.empty()) {
		description = describe();
	} else {
		description += std::string(" ") + _("OR") + " " + describe();
	}
}

void InputManagerKeyCode::prependToDescription(std::string& description, InputManagerKeys keyCode) {
	inputMgr.getKeyCode(keyCode, true).prependToDescription(description);
	inputMgr.getKeyCode(keyCode, false).prependToDescription(description);
}

void InputManagerKeyCode::appendToDescription(std::string& description, InputManagerKeys keyCode) {
	inputMgr.getKeyCode(keyCode, true).appendToDescription(description);
	inputMgr.getKeyCode(keyCode, false).appendToDescription(description);
}

std::string InputManagerKeyCode::describe(InputManagerKeys keyCode) {
	return describe(inputMgr.getKeyCode(keyCode, false), inputMgr.getKeyCode(keyCode, true));
}

bool InputManagerKeyCode::operator == (const InputManagerKeyCode& rhs) const {
	return type == rhs.type && sym == rhs.sym && ((type == KEYBOARD && sym == 0) || mod == rhs.mod);
}

bool InputManagerKeyCode::contains(const InputManagerKeyCode& rhs) const {
	if (type == KEYBOARD && rhs.type == KEYBOARD) {
		if ((mod & KMOD_CTRL) == 0 && (rhs.mod & KMOD_CTRL) != 0) return false;
		if ((mod & KMOD_ALT) == 0 && (rhs.mod & KMOD_ALT) != 0) return false;
		if ((mod & KMOD_SHIFT) == 0 && (rhs.mod & KMOD_SHIFT) != 0) return false;

		if ((mod & KMOD_CTRL) != 0 && (rhs.mod & KMOD_CTRL) == 0) {
			if (rhs.sym == SDLK_LCTRL || rhs.sym == SDLK_RCTRL) return true;
		}
		if ((mod & KMOD_ALT) != 0 && (rhs.mod & KMOD_ALT) == 0) {
			if (rhs.sym == SDLK_LALT || rhs.sym == SDLK_RALT) return true;
		}
		if ((mod & KMOD_SHIFT) != 0 && (rhs.mod & KMOD_SHIFT) == 0) {
			if (rhs.sym == SDLK_LSHIFT || rhs.sym == SDLK_RSHIFT) return true;
		}
	}

	return false;
}

//A class that handles the gui events of the inputDialog.
class InputDialogHandler:public GUIEventCallback{
private:
	//the list box which contains keys.
	GUIListBox* listBox;
	//the parent object.
	InputManager* parent;

	//create the key config item
	SharedTexture createConfigItem(SDL_Renderer& renderer, int keyIndex) {
		SDL_Color fg = objThemes.getTextColor(true);

		SurfacePtr surf1(TTF_RenderUTF8_Blended(fontText, _(keySettingDescription[keyIndex]), fg));
		SurfacePtr surf2(TTF_RenderUTF8_Blended(fontText, InputManagerKeyCode::describe((InputManagerKeys)keyIndex).c_str(), fg));

		const int w0 = int(SCREEN_WIDTH * 0.4);
		int w = w0;
		if (surf2) w += surf2->w;

		SurfacePtr surf(createSurface(w, surf1->h));

		SDL_Rect dstrect = { 4, 0, surf1->w, surf1->h };
		SDL_BlitSurface(surf1.get(), NULL, surf.get(), &dstrect);

		if (surf2) {
			dstrect = SDL_Rect{ w0, 0, surf2->w, surf2->h };
			SDL_BlitSurface(surf2.get(), NULL, surf.get(), &dstrect);
		}

		return textureFromSurface(renderer, std::move(surf));
	}

	//update specified key config item
    void updateConfigItem(SDL_Renderer& renderer,int index){
		if (!listBox->item[index].empty()) {
			int keyIndex = SDL_atoi(listBox->item[index].c_str());
			if (keyIndex >= 0 && keyIndex < INPUTMGR_MAX) {
				//Update item.
				listBox->updateItem(renderer, index, listBox->item[index], createConfigItem(renderer, keyIndex));
			}
		}
	}
public:
	//Constructor.
    InputDialogHandler(SDL_Renderer& renderer,GUIListBox* listBox,InputManager* parent):listBox(listBox),parent(parent){
		auto addItem = [&](int keyIndex) {
			char s[16];
			SDL_itoa(keyIndex, s, 10);

			listBox->addItem(renderer, s, createConfigItem(renderer, keyIndex));
		};

		auto addTitle = [&](const char* title) {
			listBox->addItem(renderer, "", textureFromText(renderer, *fontGUISmall, title, objThemes.getTextColor(true)), false);
			listBox->itemOffset.back() = SDL_Point{ 4, 0 };
		};

		//load the available keys to the list box.
		addTitle(_("In menu:"));
		addItem(INPUTMGR_UP);
		addItem(INPUTMGR_DOWN);
		addItem(INPUTMGR_SELECT);
		addItem(INPUTMGR_ESCAPE);

		addTitle(_("In game:"));
		addItem(INPUTMGR_LEFT);
		addItem(INPUTMGR_RIGHT);
		addItem(INPUTMGR_JUMP);
		addItem(INPUTMGR_ACTION);
		addItem(INPUTMGR_SPACE);
		addItem(INPUTMGR_CANCELRECORDING);
		addItem(INPUTMGR_PAUSE);
		addItem(INPUTMGR_RESTART);
		addItem(INPUTMGR_TAB);
		addItem(INPUTMGR_LOAD);

		addTitle(_("In level editor:"));
		addItem(INPUTMGR_SHIFT);
		addItem(INPUTMGR_NEXT);
		addItem(INPUTMGR_PREVIOUS);

		addTitle(_("In test play:"));
		addItem(INPUTMGR_SAVE);
		addItem(INPUTMGR_SWAP);
		addItem(INPUTMGR_TELEPORT);
		addItem(INPUTMGR_SUICIDE);
	}

    virtual ~InputDialogHandler(){}
	
	//When a key is pressed call this to set the key to currently-selected item.
    void onKeyDown(SDL_Renderer& renderer,const InputManagerKeyCode& keyCode){
		//Check if an item is selected.
		int index=listBox->value;
		if (index < 0 || index >= (int)listBox->item.size() || listBox->item[index].empty()) return;

		InputManagerKeys keyIndex = (InputManagerKeys)SDL_atoi(listBox->item[index].c_str());
		if (keyIndex < 0 || keyIndex >= INPUTMGR_MAX) return;

		//Get the existing keys.
		auto key = parent->getKeyCode(keyIndex, false);
		auto altKey = parent->getKeyCode(keyIndex, true);

		//SDLK_BACKSPACE will erase the last key if there are keys.
		if (keyCode == InputManagerKeyCode(SDLK_BACKSPACE) && (!key.empty() || !altKey.empty())) {
			if (!altKey.empty()) {
				parent->setKeyCode(keyIndex, InputManagerKeyCode(), true);
			} else {
				parent->setKeyCode(keyIndex, InputManagerKeyCode(), false);
			}
			updateConfigItem(renderer, index);
		} else if (keyCode == key || keyCode == altKey) {
			//Do nothing if keyCode is equal to one of the existing keys.
		} else if (key.empty() || (altKey.empty() && keyCode.contains(key))) {
			//Update the main key if there isn't one.
			parent->setKeyCode(keyIndex, keyCode, false);
			updateConfigItem(renderer, index);
		} else if (altKey.empty() || keyCode.contains(altKey)) {
			//Otherwise update the alternative key if there isn't one.
			parent->setKeyCode(keyIndex, keyCode, true);
			updateConfigItem(renderer, index);
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
				handler->onKeyDown(renderer, InputManagerKeyCode(event.key.keysym.sym, event.key.keysym.mod));
			}
			//Joystick
			else if(event.type==SDL_JOYAXISMOTION){
				if(event.jaxis.value>3200){
					handler->onKeyDown(renderer, InputManagerKeyCode(InputManagerKeyCode::JOYSTICK_AXIS, event.jaxis.axis, 1));
				}else if(event.jaxis.value<-3200){
					handler->onKeyDown(renderer, InputManagerKeyCode(InputManagerKeyCode::JOYSTICK_AXIS, event.jaxis.axis, -1));
				}
			}
			else if(event.type==SDL_JOYBUTTONDOWN){
				handler->onKeyDown(renderer, InputManagerKeyCode(InputManagerKeyCode::JOYSTICK_BUTTON, event.jbutton.button, 0));
			}
			else if(event.type==SDL_JOYHATMOTION){
				if (event.jhat.value & SDL_HAT_LEFT){
					handler->onKeyDown(renderer, InputManagerKeyCode(InputManagerKeyCode::JOYSTICK_HAT, event.jhat.hat, SDL_HAT_LEFT));
				} else if (event.jhat.value & SDL_HAT_RIGHT){
					handler->onKeyDown(renderer, InputManagerKeyCode(InputManagerKeyCode::JOYSTICK_HAT, event.jhat.hat, SDL_HAT_RIGHT));
				} else if (event.jhat.value & SDL_HAT_UP){
					handler->onKeyDown(renderer, InputManagerKeyCode(InputManagerKeyCode::JOYSTICK_HAT, event.jhat.hat, SDL_HAT_UP));
				} else if (event.jhat.value & SDL_HAT_DOWN){
					handler->onKeyDown(renderer, InputManagerKeyCode(InputManagerKeyCode::JOYSTICK_HAT, event.jhat.hat, SDL_HAT_DOWN));
				}
			}
		}
		return false;
	}

	//Nothing to do.
	void render(){}
};

InputManagerKeys InputManager::getKeyFromName(const std::string& name) {
	for (int i = 0; i < INPUTMGR_MAX; i++) {
		if (name == keySettingNames[i]) {
			return (InputManagerKeys)i;
		}
	}
	return INPUTMGR_MAX;
}

InputManagerKeyCode InputManager::getKeyCode(InputManagerKeys key, bool isAlternativeKey){
	if(isAlternativeKey) return alternativeKeys[key];
	else return keys[key];
}

void InputManager::setKeyCode(InputManagerKeys key, const InputManagerKeyCode& keyCode, bool isAlternativeKey){
	if(isAlternativeKey) alternativeKeys[key]=keyCode;
	else keys[key]=keyCode;
}

void InputManager::loadConfig(){
	for(int i=0;i<INPUTMGR_MAX;i++){
		string s=keySettingNames[i];
		
		keys[i] = InputManagerKeyCode::createFromString(getSettings()->getValue(s));
		
		s+="2";
		alternativeKeys[i] = InputManagerKeyCode::createFromString(getSettings()->getValue(s));
		
		//Move the alternative key to main key if the main key is empty.
		if(keys[i].empty() && !alternativeKeys[i].empty()){
			keys[i]=alternativeKeys[i];
			alternativeKeys[i] = InputManagerKeyCode();
		}
		
		//Remove duplicate.
		if(keys[i]==alternativeKeys[i])
			alternativeKeys[i] = InputManagerKeyCode();
	}
}

void InputManager::saveConfig(){
	int i;
	for(i=0;i<INPUTMGR_MAX;i++){
		string s=keySettingNames[i];
		
		//Remove duplicate.
		if(keys[i]==alternativeKeys[i])
			alternativeKeys[i] = InputManagerKeyCode();

		getSettings()->setValue(s, keys[i].toString());

		s+="2";
		getSettings()->setValue(s, alternativeKeys[i].toString());
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
    GUIListBox *listBox=new GUIListBox(imageManager,renderer,SCREEN_WIDTH*0.1,72,SCREEN_WIDTH*0.8,height-72-8);
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

InputManager::InputManager(){
	//Clear the arrays.
	for(int i=0;i<INPUTMGR_MAX;i++){
		keyFlags[i]=0;
	}
}

InputManager::~InputManager(){
	closeAllJoysticks();
}

int InputManagerKeyCode::getKeyState(int oldState, bool hasEvent, std::vector<SDL_Joystick*>& joysticks, int deadZone) const {
	int state = 0;

	switch (type) {
	default: {
		//Keyboard.
		if (sym == 0) return 0;
		const bool isPrintable = (mod & ~KMOD_SHIFT) == 0 && sym >= 32 && sym <= 126;
		if (hasEvent && (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP)) {
			const bool isModCorrect = (mod == 0) ? true :
				(mod == InputManagerKeyCode(event.key.keysym.sym, event.key.keysym.mod).mod);

			if (event.type == SDL_KEYDOWN && event.key.keysym.sym == sym && isModCorrect) {
				state |= 0x3;
				if (!isPrintable) state |= 0x30;
			} else if (event.type == SDL_KEYUP && event.key.keysym.sym == sym && isModCorrect) {
				state |= 0x4;
				if (!isPrintable) state |= 0x40;
			}
		}
		{
			//Get keyboard state for key code
			//SDL_GetKeyboardState needs a scankey rather than keycode, so we convert
			int numKeys = 0;
			const Uint8* keyStates = SDL_GetKeyboardState(&numKeys);
			SDL_Scancode scanCode = SDL_GetScancodeFromKey(sym);
			if (scanCode < numKeys && keyStates[scanCode]) {
				const int m = SDL_GetModState();
				const bool isModCorrect = (mod == 0) ? true :
					(mod == InputManagerKeyCode(sym, m).mod);
				if (isModCorrect) {
					state |= 0x1;
					if (!isPrintable) state |= 0x10;
				}
			}
		}
		break;
	}
	case JOYSTICK_AXIS:
		//Axis.
		if (hasEvent && event.type == SDL_JOYAXISMOTION && event.jaxis.axis == buttonIndex) {
			if ((buttonValue > 0 && event.jaxis.value > deadZone) || (buttonValue < 0 && event.jaxis.value < -deadZone)) {
				if ((oldState & 0x1) == 0) state |= 0x3;
				if ((oldState & 0x10) == 0) state |= 0x30;
			} else {
				if (oldState & 0x1) state |= 0x4;
				if (oldState & 0x10) state |= 0x40;
			}
		}
		for (auto j : joysticks) {
			Sint16 v = SDL_JoystickGetAxis(j, buttonIndex);
			if ((buttonValue > 0 && v > deadZone) || (buttonValue < 0 && v < -deadZone)){
				state |= 0x11;
				break;
			}
		}
		break;
	case JOYSTICK_BUTTON:
		//Button.
		if (hasEvent) {
			if (event.type == SDL_JOYBUTTONDOWN && event.jbutton.button == buttonIndex){
				state |= 0x33;
			} else if (event.type == SDL_JOYBUTTONUP && event.jbutton.button == buttonIndex){
				state |= 0x44;
			}
		}
		for (auto j : joysticks) {
			Uint8 v = SDL_JoystickGetButton(j, buttonIndex);
			if (v) {
				state |= 0x11;
				break;
			}
		}
		break;
	case JOYSTICK_HAT:
		//Hat.
		if (hasEvent && event.type == SDL_JOYHATMOTION && event.jhat.hat == buttonIndex) {
			if (event.jhat.value & buttonValue){
				if ((oldState & 0x1) == 0) state |= 0x3;
				if ((oldState & 0x10) == 0) state |= 0x30;
			} else{
				if (oldState & 0x1) state |= 0x4;
				if (oldState & 0x10) state |= 0x40;
			}
		}
		for (auto j : joysticks) {
			Uint8 v = SDL_JoystickGetHat(j, buttonIndex);
			if (v & buttonValue) {
				state |= 0x11;
				break;
			}
		}
		break;
	}

	return state;
}

//Update the key state, according to current SDL event, etc.
void InputManager::updateState(bool hasEvent){
	for(int i=0;i<INPUTMGR_MAX;i++){
		keyFlags[i] = keys[i].getKeyState(keyFlags[i], hasEvent, joysticks) | alternativeKeys[i].getKeyState(keyFlags[i], hasEvent, joysticks);
	}
}

//Check if there is KeyDown event.
bool InputManager::isKeyDownEvent(InputManagerKeys key, bool excludePrintable) {
	return (keyFlags[key] & (excludePrintable ? 0x20 : 0x2)) != 0;
}

//Check if there is KeyUp event.
bool InputManager::isKeyUpEvent(InputManagerKeys key, bool excludePrintable) {
	return (keyFlags[key] & (excludePrintable ? 0x40 : 0x4)) != 0;
}

//Check if specified key is down.
bool InputManager::isKeyDown(InputManagerKeys key, bool excludePrintable) {
	return (keyFlags[key] & (excludePrintable ? 0x10 : 0x1)) != 0;
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
