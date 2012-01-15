/****************************************************************************
** Copyright (C) 2012 me and my shadow developers
**
**
** This file may be used under the terms of the GNU General Public
** License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
**  You should have received a copy of the GNU General Public License
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/

#include "InputManager.h"
#include "Globals.h"
#include "Settings.h"
#include "Functions.h"
#include "GUIObject.h"
#include "GUIListBox.h"
#include <stdlib.h>
#include <stdio.h>
#include <string>

InputManager inputMgr;

//the order must be the same as InputManagerKeys
static const char* keySettingNames[INPUTMGR_MAX]={
	"key_up","key_down","key_left","key_right","key_space",
	"key_escape","key_restart","key_tab","key_save","key_load","key_swap",
	"key_teleport","key_suicide","key_shift"
};

//the order must be the same as InputManagerKeys
static const char* keySettingDescription[INPUTMGR_MAX]={
	"Up (Jump)","Down (Action)","Left","Right","Space (Record)",
	"Escape","Restart","Tab (View shadow/Level properties)","Save game (in editor)","Load game","Swap (in editor)",
	"Teleport (in editor)","Suicide (in editor)","Shift (in editor)"
};

class InputDialogHandler:public GUIEventCallback{
private:
	//the list box which contains keys.
	GUIListBox* listBox;
	//the parent object.
	InputManager* parent;

	//update specified key config item
	void updateConfigItem(int index){
		//get the description
		std::string s=keySettingDescription[index];
		s+='\t';
		
		//get key code name
		int keyCode=parent->getKeyCode((InputManagerKeys)index);
		s+=InputManager::getKeyCodeName(keyCode);

		//show it
		listBox->item[index]=s;
	}
public:
	//Constructor.
	InputDialogHandler(GUIListBox* listBox,InputManager* parent):listBox(listBox),parent(parent){
		//load the avaliable keys to the list box.
		for(int i=0;i<INPUTMGR_MAX;i++){
			//get the description
			std::string s=keySettingDescription[i];
			s+='\t';
			
			//get key code name
			int keyCode=parent->getKeyCode((InputManagerKeys)i);
			s+=InputManager::getKeyCodeName(keyCode);

			//add item
			listBox->item.push_back(s);
		}
	}
	
	//when a key is pressed call this to set the key to currently-selected item
	void onKeyDown(int keyCode){
		//check if an item is selected.
		int index=listBox->value;
		if(index<0 || index>=INPUTMGR_MAX) return;

		//set it.
		parent->setKeyCode((InputManagerKeys)index,keyCode);
		updateConfigItem(index);
	}

	void GUIEventCallback_OnEvent(std::string name,GUIObject* obj,int eventType){
		//Make sure it's a click event.
		if(eventType==GUIEventClick){
			if(name=="cmdOK"){
				//config is done, exiting
				delete GUIObjectRoot;
				GUIObjectRoot=NULL;
			}else if(name=="cmdEscape"){
				//set a key to Escape
				//simply call onKeyDown().
				onKeyDown(SDLK_ESCAPE);
			}
		}
	}
};

int InputManager::getKeyCode(InputManagerKeys key){
	return keys[key];
}

void InputManager::setKeyCode(InputManagerKeys key,int keyCode){
	keys[key]=keyCode;
}

void InputManager::loadConfig(){
	int i;
	for(i=0;i<INPUTMGR_MAX;i++){
		keys[i]=atoi(getSettings()->getValue(keySettingNames[i]).c_str());
	}
}

void InputManager::saveConfig(){
	int i;
	char s[32];
	for(i=0;i<INPUTMGR_MAX;i++){
		sprintf(s,"%d",keys[i]);
		getSettings()->setValue(keySettingNames[i],s);
	}
}

void InputManager::showConfig(){
	//Save the old GUI.
	GUIObject* tmp=GUIObjectRoot;

	//Create the new GUI.
	GUIObjectRoot=new GUIObject((SCREEN_WIDTH-600)/2,(SCREEN_HEIGHT-400)/2,600,400,GUIObjectFrame,"Config Keys");
	GUIObject* obj;

	obj=new GUIObject(20,20,560,36,GUIObjectLabel,"Select an item and press a key to config it.");
	GUIObjectRoot->childControls.push_back(obj);

	//The list box.
	GUIListBox *listBox=new GUIListBox(20,60,560,280);

	//Event handler.
	InputDialogHandler handler(listBox,this);
	//listBox->name="lstKeys";
	//listBox->eventCallback=handler;
	GUIObjectRoot->childControls.push_back(listBox);

	//two buttons
	obj=new GUIObject(20,350,240,36,GUIObjectButton,"Set to ESCAPE key");
	obj->name="cmdEscape";
	obj->eventCallback=&handler;
	GUIObjectRoot->childControls.push_back(obj);
	obj=new GUIObject(500,350,80,36,GUIObjectButton,"OK");
	obj->name="cmdOK";
	obj->eventCallback=&handler;
	GUIObjectRoot->childControls.push_back(obj);

	//Now we keep rendering and updating the GUI.
	SDL_FillRect(tempSurface,NULL,0);
	SDL_SetAlpha(tempSurface,SDL_SRCALPHA,155);
	SDL_BlitSurface(tempSurface,NULL,screen,NULL);
	while(GUIObjectRoot){
		while(SDL_PollEvent(&event)){
			//check if some key is down.
			if(event.type==SDL_KEYDOWN){
				handler.onKeyDown(event.key.keysym.sym);
			}
			//TODO: Joystick

			//now process GUI events.
			GUIObjectHandleEvents(true);
		}
		if(GUIObjectRoot)
			GUIObjectRoot->render();
		SDL_Flip(screen);
		SDL_Delay(30);
	}

	//Restore the old GUI.
	GUIObjectRoot=tmp;
}


//get key name from key code
std::string InputManager::getKeyCodeName(int keyCode){
	if(keyCode>0 && keyCode <0x1000){
		//keyboard
		char* s=SDL_GetKeyName((SDLKey)keyCode);
		if(s!=NULL){
			return s;
		}else{
			char c[32];
			sprintf(c,"(key %d)",keyCode);
			return c;
		}
	}else if(keyCode>0x1000){
		//TODO: Joystick
		return "(TODO:Joystick)";
	}else{
		//unknown??
		return "(Not set)";
	}
}

InputManager::InputManager(){
	//clear the array.
	for(int i=0;i<INPUTMGR_MAX;i++){
		keys[i]=keyFlags[i]=0;
	}
}

static int getKeyState(int keyCode,bool hasEvent){
	int state=0;
	if(keyCode>0 && keyCode<0x1000){
		//keyboard
		if(hasEvent){
			if(event.type==SDL_KEYDOWN && event.key.keysym.sym==keyCode){
				state|=0x2;
			}
			if(event.type==SDL_KEYUP && event.key.keysym.sym==keyCode){
				state|=0x4;
			}
		}
		if(keyCode<SDLK_LAST && SDL_GetKeyState(NULL)[keyCode]){
			state|=0x1;
		}
	}else{
		//TODO:Joystick, etc.
	}
	return state;
}

//update the key state, according to current SDL event, etc.
void InputManager::updateState(bool hasEvent){
	for(int i=0;i<INPUTMGR_MAX;i++){
		keyFlags[i]=getKeyState(keys[i],hasEvent);
	}
}

//check if there is KeyDown event.
bool InputManager::isKeyDownEvent(InputManagerKeys key){
	return keyFlags[key]&0x2;
}

//check if there is KeyUp event.
bool InputManager::isKeyUpEvent(InputManagerKeys key){
	return keyFlags[key]&0x4;
}

//check if specified key is down.
bool InputManager::isKeyDown(InputManagerKeys key){
	return keyFlags[key]&0x1;
}
