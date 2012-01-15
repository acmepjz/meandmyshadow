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
#include "Settings.h"
#include "Functions.h"
#include <stdlib.h>
#include <stdio.h>

InputManager inputMgr;

//the order must be the same as InputManagerKeys
static const char* keySettingNames[INPUTMGR_MAX]={
	"key_up","key_down","key_left","key_right","key_space",
	"key_escape","key_restart","key_save","key_load","key_swap",
	"key_teleport","key_shift"
};

//the order must be the same as InputManagerKeys
static const char* keySettingDescription[INPUTMGR_MAX]={
	"Up (Jump)","Down (Action)","Left","Right","Space (Record)",
	"Escape","Restart","Save game (in editor)","Load game","Swap (in editor)",
	"Teleport (in editor)","Shift (in editor)"
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
