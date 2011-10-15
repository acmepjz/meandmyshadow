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
#ifndef TITLE_MENU_H
#define TITLE_MENU_H

#include <SDL/SDL.h>
#include "GameState.h"

//Included for the Options menu.
#include "GUIObject.h"
#include "GUIListBox.h"


//The Main menu.
class Menu : public GameState{
private:
	//The background of the main menu.
	//This will be rendered before anything else.
	SDL_Surface * menu;
public:
	//Constructor.
	Menu();
	//Destructor.
	~Menu();

	//Inherited from GameState.
	void handleEvents();
	void logic();
	void render();
};

//The Help menu.
//It does nothing more than displaying an image.
class Help : public GameState{
private:
	//The background of the help.
	//It also contains the text.
	SDL_Surface * help;

public:
	//Constructor.
	Help();
	//Destructor.
	~Help();

	//Inherited from GameState.
	//Note: only render() is used since all Help does is display an image.
	void handleEvents();
	void logic();
	void render();
};

//The Options menu.
class Options : public GameState, private GUIEventCallback{
private:
	//The background of the options menu.
	//It gets rendered before the GUI.
	SDL_Surface * options;

	//
	GUIObject *restartLabel;
	//
	GUISingleLineListBox *theme;
	
	 //GUI events are handled here.
	//name: The name of the element that invoked the event.
	//obj: Pointer to the object that invoked the event.
	//eventType: Integer containing the type of event.
	void GUIEventCallback_OnEvent(std::string name,GUIObject* obj,int eventType);

public:
	//Constructor.
	Options();
	//Destructor.
	~Options();
	
	//Inherited from GameState.
	void handleEvents();
	void logic();
	void render();
};
#endif