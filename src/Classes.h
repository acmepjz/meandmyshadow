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
#ifndef CLASSES_H
#define CLASSES_H

#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include <SDL/SDL_ttf.h>
#include <vector>
#include <string>
#include "GameObjects.h"
#include "Timer.h"
#include "Player.h"


class Level
{
private:
	int i_level_number;
	int i_current_level;

	std::vector<std::string> level_name;
	std::vector<bool> level_locked;

public:

	Level();

	std::string give_level_name();

	int get_level();
	int get_level_number();
	bool get_locked( int level );
	void set_level(int lvl);
	void set_locked(int lvl);

	void save_levels();

	void next_level();

};

class StartObject : public GameObject
{

public:

	StartObject(int x, int y, class Player * player);

	void show();
};

class StartObjectShadow : public GameObject
{

public:

	StartObjectShadow(int x, int y, class Shadow * player);

	void show();
};


class GameState
{
public:
	virtual void handle_events() = 0;
	virtual void logic() = 0;
	virtual void render() = 0;
	virtual ~GameState(){};
};


class Menu : public GameState
{
private:

	SDL_Surface * s_menu;
	SDL_Rect play, help, exit;

public:

	Menu();
	~Menu();

	void handle_events();
	void logic();
	void render();
};

class Help : public GameState
{
private:
	SDL_Surface * s_help;

public:

	Help();
	~Help();

	void handle_events();
	void logic();
	void render();
};


class LevelEditor : public GameState
{
private:

	SDL_Surface * test;
	//Objekti slike
	SDL_Surface * s_block;
	SDL_Surface * s_playerstart;
	SDL_Surface * s_shadowstart;
	SDL_Surface * s_exit;
	SDL_Surface * s_shadowblock;
	SDL_Surface * s_spikes;

	std::vector<GameObject*> levelObjects;
	std::vector<SDL_Rect> grid;

	Player o_player;
	Shadow o_shadow;

	int i_current_type;

	int i_current_object;

public:

	LevelEditor();
	~LevelEditor();

	void handle_events();
	void logic();
	void render();

	void switch_currentObject(int next);
	void put_object( std::vector<GameObject*> &LevelObjects );
	void delete_object( std::vector<GameObject*> &LevelObjects );
	void show_current_object();
	void save_level();
	void load_level();
};

class Game : public GameState
{
private:

	SDL_Surface *background;

	std::vector<GameObject*> levelObjects;

	Player o_player;
	Shadow o_shadow;

public:

	Game();
	~Game();

	void handle_events();
	void logic();
	void render();
	
	void load_level();
};

class Number
{
private:
        SDL_Surface * s_level;
        SDL_Surface * s_image;
        
        
public:
	SDL_Rect myBox;

       
       Number();
       ~Number();

	   void init( int number, SDL_Rect box );
       
       void show();
};

class LevelSelect :public GameState
{
private:
        
        SDL_Surface * s_background;
     
        std::vector<class Number> o_number;

		int lol;
               
public:
       
       LevelSelect();
       ~LevelSelect();
       
       void handle_events();
       void logic();
       void render();

	   void check_mouse();
};


#endif
