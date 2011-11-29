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
#ifndef LEVELEDITOR_H
#define LEVELEDITOR_H

#include "GameState.h"
#include "GameObjects.h"
#include "Player.h"
#include "Game.h"
#include "GUIObject.h"
#include <vector>
#include <map>
#include <string>
#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include <SDL/SDL_ttf.h>

//The LevelEditor state, it's based on the Game state.
class LevelEditor: public Game, private GUIEventCallback{
private:
	//Boolean if the user isplaying/testing the level.
	bool playMode;
	
	//Enumaration containing the tools.
	//SELECT: The select tool, for selecting/dragging blocks.
	//ADD: For adding blocks.
	//DELETE: For removing blocks.
	//CONFIGURE: Used to configure special blocks.
	enum Tools{
		SELECT,
		ADD,
		DELETE,
		CONFIGURE,
		
		NUMBER_TOOLS
	};
	//The tool the user has selected.
	Tools tool;
	//The toolbar surface.
	SDL_Surface* toolbar;
	//Rectangle the size and location of the toolbar on screen.
	SDL_Rect toolbarRect;
	
	//Vector containing pointers to the selected GameObjects.
	vector<GameObject*> selection;
	//The selection square.
	SDL_Surface* selectionMark;
	
	//Surface used for drawing transparent selection/dragging.
	SDL_Surface* placement;
	
	//The current type of block to place in Add mode.
	int currentType;
	
	//Boolean if the shift button is pressed.
	bool pressedShift;
	//Boolean if the left mouse button is pressed.
	bool pressedLeftMouse;
	//Boolean if the mouse is dragged. (Left button pressed and moved)
	bool dragging;
	
	//The camera x velocity.
	int cameraXvel;
	int cameraYvel;
	//SDL_Rect used to store the camera's location when entering playMode.
	SDL_Rect cameraSave;
	
	//Boolean if the selection is dragged.
	bool selectionDrag;
	//Pointer to the gameobject that's the center of the drag.
	GameObject* dragCenter;
	
	//Pointer to a GUIObject for a property of the object.
	//Only used in the configure tool.
	GUIObject* objectProperty;
	//Pointer to the object that is being configured.
	GameObject* configuredObject;
  
	//GUI event handling is done here.
	void GUIEventCallback_OnEvent(std::string name,GUIObject* obj,int eventType);
	
	//Methos used to save the level.
	//fileName: Thge filename to write the level to.
	void saveLevel(string fileName);
public:
	//Constructor.
	LevelEditor();
	//Destructor.
	~LevelEditor();

	//Inherited from Game(State).
	void handleEvents();
	void logic();
	void render();
	
	//Method used to draw the currentType on the placement surface.
	//This will only be called when the tool is ADD.
	void showCurrentObject();
	//Method used to draw the selection that's being dragged.
	void showSelectionDrag();
	
	//Event that is invoked when there's a mouse click on an object.
	//obj: Pointer to the GameObject clicked on.
	//selected: Boolean if the GameObject that has been clicked on was selected.
	void onClickObject(GameObject* obj,bool selected);
	//Event that is invoked when there's a mouse click but not on any object.
	//x: The x location of the click on the game field (+= camera.x). 
	//y: The y location of the click on the game field (+= camera.y).
	void onClickVoid(int x,int y);
	//Event that is invoked when the dragging starts.
	//x: The x location the drag started. (mouse.x+camera.x)
	//y: The y location the drag started. (mouse.y+camera.y)
	void onDragStart(int x,int y);
	//Event that is invoked when the mouse is dragged.
	//dx: The relative x distance the mouse dragged.
	//dy: The relative y distance the mouse dragged.
	void onDrag(int dx,int dy);
	//Event that is invoked when the mouse stopped dragging.
	//x: The x location the drag stopped. (mouse.x+camera.x)
	//y: The y location the drag stopped. (mouse.y+camera.y)
	void onDrop(int x,int y);
	//Event that is invoked when the camera is moved.
	//dx: The relative x distance the camera moved.
	//dy: The relative y distance the camera moved.
	void onCameraMove(int dx,int dy);

};
#endif