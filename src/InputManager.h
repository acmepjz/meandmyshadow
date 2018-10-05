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

#include <SDL.h>
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

// Represents a key code or a joystick input code.
struct InputManagerKeyCode {
	// The input type. Currently joystick ball is unsupported.
	enum InputType {
		KEYBOARD,
		JOYSTICK_AXIS,
		JOYSTICK_BUTTON,
		JOYSTICK_HAT,
	};

	// The input type.
	InputType type;

	union {
		struct {
			// The keycode if the input type is KEYBOARD.
			// NOTE: if type == KEYBOARD and sym == 0 then this key is disabled at all.
			int sym;

			// The modifier if the input type is KEYBOARD.
			// NOTE: if modifier == 0 then we don't check the modifier at all.
			int mod;
		};

		struct {
			// Joystick button index.
			int buttonIndex;

			// Joystick button value.
			// If type == JOYSTICK_AXIS then value should be 1 or -1.
			// If type == JOYSTICK_BUTTON then it's unused.
			// If type == JOYSTICK_HAT then it's SDL_HAT_LEFT, SDL_HAT_RIGHT, SDL_HAT_UP or SDL_HAT_DOWN.
			int buttonValue;
		};
	};

	// Constructor for a keyboard input.
	explicit InputManagerKeyCode(int sym_ = 0, int mod_ = 0);

	// Constructor for a joystick input.
	explicit InputManagerKeyCode(InputType type_, int buttonIndex_, int buttonValue_);

	// Create a key code from string.
	static InputManagerKeyCode createFromString(const std::string& s);

	// Convert a key code to a machine-readable string.
	std::string toString() const;

	// Convert a key code to a human-readable string.
	std::string describe() const;

	// Convert two key codes to a human-readable string.
	static std::string describeTwo(const InputManagerKeyCode& keyCode, const InputManagerKeyCode& keyCodeAlt);

	// Check if the key code is empty.
	bool empty() const;

	// Internal function to check if the key corresponding to this key code is pressed.
	// oldState: The bit-field flag saves the key states. 0x1=key is down, 0x2=KeyDown event,0x4=KeyUp event.
	// hasEvent: current SDL event is present.
	// joysticks: List of joystick to detect.
	// deadZone: Joystick dead zone for JOYSTICK_AXIS detection.
	// Return value: The bit-field flag with the same meaning as oldState.
	int getKeyState(int oldState, bool hasEvent, std::vector<SDL_Joystick*>& joysticks, int deadZone = 3200) const;

	bool operator==(const InputManagerKeyCode& rhs) const;
	bool operator!=(const InputManagerKeyCode& rhs) const { return !(*this == rhs); }

	// Check if a keyboard input contains another keyboard input (for example, Ctrl+A contains Ctrl but doesn't contain A).
	bool contains(const InputManagerKeyCode& rhs) const;
};

class InputManager{
public:
	InputManager();
	~InputManager();

	//Get and set key code of each key.
	InputManagerKeyCode getKeyCode(InputManagerKeys key, bool isAlternativeKey);
	void setKeyCode(InputManagerKeys key, const InputManagerKeyCode& keyCode, bool isAlternativeKey);

	//Load and save key settings from config file.
	void loadConfig();
	void saveConfig();

	//Show the config screen.
    GUIObject* showConfig(ImageManager& imageManager, SDL_Renderer& renderer, int height);

	//Update the key state, according to current SDL event, etc.
	void updateState(bool hasEvent);

	//Check if there is KeyDown event.
	bool isKeyDownEvent(InputManagerKeys key, bool excludePrintable = false);
	//Check if there is KeyUp event.
	bool isKeyUpEvent(InputManagerKeys key, bool excludePrintable = false);
	//Check if specified key is down.
	bool isKeyDown(InputManagerKeys key, bool excludePrintable = false);

	//Open all joysticks.
	void openAllJoysitcks();
	//Close all joysticks.
	void closeAllJoysticks();
private:
	//The key code of each key.
	InputManagerKeyCode keys[INPUTMGR_MAX], alternativeKeys[INPUTMGR_MAX];

	//The bit-field flag array saves the key states.
	// 0x1 means the key is down.
	// 0x2 means KeyDown event.
	// 0x4 means KeyUp event.
	int keyFlags[INPUTMGR_MAX];

	//Contains all joysticks.
	std::vector<SDL_Joystick*> joysticks;
};

extern InputManager inputMgr;

#endif //INPUTMANAGER_H
