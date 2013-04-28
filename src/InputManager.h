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

#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H

#include <SDL/SDL.h>
#include <vector>
#include <string>

#include "GUIObject.h"

enum InputManagerKeys{
	INPUTMGR_UP,
	INPUTMGR_DOWN,
	INPUTMGR_LEFT,
	INPUTMGR_RIGHT,
	INPUTMGR_JUMP,
	INPUTMGR_ACTION,
	INPUTMGR_SPACE,
	INPUTMGR_CANCELRECORDING,
	INPUTMGR_ESCAPE,
	INPUTMGR_RESTART,
	INPUTMGR_TAB,
	INPUTMGR_SAVE,
	INPUTMGR_LOAD,
	INPUTMGR_SWAP,
	INPUTMGR_TELEPORT,
	INPUTMGR_SUICIDE,
	INPUTMGR_SHIFT,
	INPUTMGR_NEXT,
	INPUTMGR_PREVIOUS,
	INPUTMGR_SELECT,
	INPUTMGR_MAX
};

class InputManager{
public:
	InputManager();
	~InputManager();

	//Get and set key code of each key.
	int getKeyCode(InputManagerKeys key,bool isAlternativeKey);
	void setKeyCode(InputManagerKeys key,int keyCode,bool isAlternativeKey);

	//Load and save key settings from config file.
	void loadConfig();
	void saveConfig();

	//Show the config screen.
	GUIObject* showConfig(int height);

	//Get key name from key code
	static std::string getKeyCodeName(int keyCode);

	//Update the key state, according to current SDL event, etc.
	void updateState(bool hasEvent);

	//Check if there is KeyDown event.
	bool isKeyDownEvent(InputManagerKeys key);
	//Check if there is KeyUp event.
	bool isKeyUpEvent(InputManagerKeys key);
	//Check if specified key is down.
	bool isKeyDown(InputManagerKeys key);

	//Open all joysticks.
	void openAllJoysitcks();
	//Close all joysticks.
	void closeAllJoysticks();
private:
	//The key code of each key.
	// - note of key code:
	//   0 means this key is disabled
	//   1 to 4095 (0xFFF) means keyboard keys, 
	//     currently SDLKey is less than 4095
	//   >= 4096: bit field value means joystick.
	//     0xWWXXYYZZ
	//     WW = joystick index. currently unused, should be 0.
	//     XX = joystick button type: 1-axis 2-button 3-hat, currently ball is unsupported
	//     YY = joystick button index. (we assume joystick has at most 256 buttons)
	//     ZZ = value. if type=axis then value should be 1 or 0xFF.
	//                 if type=button then it's unused.
	//                 if type=hat then it's SDL_HAT_LEFT, SDL_HAT_RIGHT, SDL_HAT_UP or SDL_HAT_DOWN.
	int keys[INPUTMGR_MAX];
	int alternativeKeys[INPUTMGR_MAX];
	//The bit-field flag array saves the key states.
	// 0x1 means the key is down.
	// 0x2 means KeyDown event.
	// 0x4 means KeyUp event.
	int keyFlags[INPUTMGR_MAX];
	//Contains all joysticks.
	std::vector<SDL_Joystick*> joysticks;
	//Internal function.
	int getKeyState(int keyCode,int oldState,bool hasEvent);
};

extern InputManager inputMgr;

#endif //INPUTMANAGER_H
