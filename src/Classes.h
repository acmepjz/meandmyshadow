#ifndef CLASSES_H
#define CLASSES_H

#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include <SDL/SDL_ttf.h>
#include <vector>
#include <string>
#include "GameObjects.h"
#include "Timer.h"


class Player
{
private:

	std::vector<bool> right_button;
	std::vector<bool> left_button;
	std::vector<bool> jump_button;

	std::vector<SDL_Rect> line;

	bool b_shadow_call;

protected:
	SDL_Rect box;

	int i_xVel, i_yVel;


	SDL_Surface * s_walking[4];
	SDL_Surface * s_standing[4];
	SDL_Surface * s_jumping[2];
	SDL_Surface * s_holding;
	SDL_Surface * s_line;

	Mix_Chunk * c_jump;
	Mix_Chunk * c_hit;

	bool b_inAir;
	bool b_jump;
	bool b_on_ground;
	bool b_can_move;
	bool b_dead;
	

	int i_frame;
	int i_animation;
	int i_direction;
	int i_state;
	int i_jump_time;
	bool b_shadow; 

public:

	int i_fx, i_fy;
	bool b_reset;

	bool b_holding_other;

	Player();
	~Player();

	void set_position( int x, int y );

	void handle_input(class Shadow * shadow);
	void move(std::vector<GameObject*> &LevelObjects);
	void jump();
	void show();
	void shadow_set_state();
	void state_reset();
	void other_check(class Player * other);
	void set_mycamera();
	void reset();
	SDL_Rect get_box();
	
	void shadow_give_state(class Shadow * shadow);
};

class Shadow : public Player
{
protected:

	std::vector<bool> right_button;
	std::vector<bool> left_button;
	std::vector<bool> jump_button;

	bool b_called;

	friend class Player;

public:

	Shadow();

	void move_logic();
	void me_call();
	void reset();
};

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


class Title : public GameState
{
private:

	SDL_Surface * title;

	Timer time;

	int titleA;

public:

	Title();
	~Title();

	void handle_events();
	void logic();
	void render();
};

class Menu : public GameState
{
private:

	SDL_Surface * s_menu;

	int titleA;

	SDL_Rect play, level, exit;

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

	int alfa;

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

	int alfa;

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
		int titleA;
               
public:
       
       LevelSelect();
       ~LevelSelect();
       
       void handle_events();
       void logic();
       void render();

	   void check_mouse();
};


#endif
