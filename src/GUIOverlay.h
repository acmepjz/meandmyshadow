 
/****************************************************************************
** Copyright (C) 2011 Luka Horvat <redreaper132 at gmail.com>
** Copyright (C) 2011 Edward Lii <edward_iii at myway.com>
** Copyright (C) 2011 O. Bahri Gordebak <gordebak at gmail.com>
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
#ifndef GUI_OVERLAY_H
#define GUI_OVERLAY_H

#include <SDL/SDL.h>
#include "GameState.h"
#include "GUIObject.h"


//The GUIOverlay state, this is a special state since it doens't appear in the states list.
//It is used to properly handle GUIs that overlay the current state, dialogs for example.
class GUIOverlay : public GameState{
private:
	//A pointer to the current state to put back when needed.
	GameState* parentState;

	//Pointer to the GUI root of the overlay.
	GUIObject* root;
	//Pointer to the previous GUIObjectRoot.
	GUIObject* tempGUIObjectRoot;

	//Boolean if the screen should be dimmed.
	bool dim;
public:
	//Constructor.
	//root: Pointer to the new GUIObjectRoot.
	//dim: Boolean if the parent state should be dimmed.
	GUIOverlay(GUIObject* root,bool dim=true);
	//Destructor.
	~GUIOverlay();

	//Method that can be used to create a "sub gameloop".
	//This is usefull in case the GUI that is overlayed is used for userinput which the function needs to return.
	//NOTE: This loop should be kept similar to the main loop.
	//skip: Boolean if this GUIOverlay can be "skipped", meaning it can be exited quickly by pressing escape or return.
	void enterLoop(bool skip=false);

	//Inherited from GameState.
	void handleEvents();
	void logic();
	void render();
	void resize();
};

#endif
