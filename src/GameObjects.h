#ifndef GAME_OBJECTS_H
#define GAME_OBJECTS_H

#include <SDL/SDL.h>
#include "Globals.h"

class GameObject
{
protected:
	SDL_Rect box;

	SDL_Surface *surface;

	

public:

	int i_type;

	GameObject();
	~GameObject();

	SDL_Rect get_box();

	virtual void show() = 0;
	
};

class Block : public GameObject
{
private:

public:

	Block(int x, int y, int type = TYPE_BLOCK);
	~Block();

	void show();
};

class Exit : public GameObject
{

public:
	Exit( int x, int y );

	void test_player( class Player * player );
	void show();
};



#endif
