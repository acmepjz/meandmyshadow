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

#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H

enum InputManagerKeys{
	INPUTMGR_UP,
	INPUTMGR_DOWN,
	INPUTMGR_LEFT,
	INPUTMGR_RIGHT,
	INPUTMGR_SPACE,
	INPUTMGR_ESCAPE,
	INPUTMGR_RESTART,
	INPUTMGR_SAVE,
	INPUTMGR_LOAD,
	INPUTMGR_SWAP,
	INPUTMGR_TELEPORT,
	INPUTMGR_SHIFT,
	INPUTMGR_MAX
};

class InputManager{
public:
	//get and set key code of each key.
	int getKeyCode(InputManagerKeys key);
	void setKeyCode(InputManagerKeys key,int keyCode);
	//load and save key settings from config file.
	void loadConfig();
	void saveConfig();
private:
	//the key code of each key.
	// - note of key code:
	//   0 means this key is disabled (??)
	//   1 to 4095 (0xFFF) means keyboard keys, 
	//     currently SDLKey is less than 4095
	//   >= 4096: bit field value means joystick.
	//     TBA
	int keys[INPUTMGR_MAX];
};

extern InputManager inputMgr;

#endif INPUTMANAGER_H
