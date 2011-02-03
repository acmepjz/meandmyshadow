#include "GameObjects.h"
#include "Functions.h"
#include "Globals.h"
#include "Classes.h"
#include <iostream>

GameObject::GameObject()
{
	surface = NULL;
}

GameObject::~GameObject()
{

}


SDL_Rect GameObject::get_box()
{
	return box;
}





