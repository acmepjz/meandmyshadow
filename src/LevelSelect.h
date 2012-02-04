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
#ifndef LEVELSELECT_H
#define LEVELSELECT_H

#include "GameState.h"
#include "GameObjects.h"
#include "Player.h"
#include "GUIObject.h"
#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include <SDL/SDL_ttf.h>
#include <vector>
#include <string>

//Class that represents a level in the levelselect menu.
class Number{
private:
	//The background image of the number.
	SDL_Surface* background;
	//The (text) image of the number.
	SDL_Surface* image;
	
	//Image containing the three stars a player can earn.
	SDL_Surface* medals;
	
	//The number.
	int number;
	//Integer containing the medal the player got.
	//0 = none, 1 = bronze, 2 = silver, 3 = gold
	int medal;
public:
	//The location and size of the number.
	SDL_Rect box;

	//If the Number is selected then we draw something indicates it.
	bool selected;

	//Constructor.
	Number();
	//Destructor.
	~Number();

	//Method used for initialising the number.
	//number: The number.
	//box: The location and size of the number.
	void init(int number, SDL_Rect box);

	//get current number.
	inline int getNumber(){return number;}
	
	//This will update the number. (locked and medal)
	void update();
	
	void show(int dy);
};

//This is the LevelSelect state, here you can select levelpacks and levels.
class LevelSelect :public GameState,public GUIEventCallback{
private:
	//The background image which is drawn before the rest.
	SDL_Surface* background;
	//Surface containing the title.
	SDL_Surface* title;
	
	//Vector containing the numbers.
	std::vector<Number> numbers;
	
	//This hashmap is used to get the path to the levelpack by giving the name of the levelpack.
	std::map<std::string,std::string> levelpackLocations;

	//Contains selected level number (displayed at bottom left corner).
	//If it's NULL then nothing selected.
	Number* selectedNumber;
	
	//Pointer to the play button, it is only shown when a level is selected.
	GUIObject* play;
	
	//display level info.
	void displayLevelInfo(int number);

	//Check where and if the mouse clicked on a number.
	//If so it will start the level.
	void checkMouse();
public:
	//Constructor.
	LevelSelect();
	//Destructor.
	~LevelSelect();

	//Method used to update the numbers and the scrollbar.
	void refresh();

	//Inherited from GameState.
	void handleEvents();
	void logic();
	void render();

	//GUI events will be handled here.
	void GUIEventCallback_OnEvent(std::string name,GUIObject* obj,int eventType);
};

#endif
