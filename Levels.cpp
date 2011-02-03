#include "Classes.h"
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
using namespace std;

Level::Level()
{
	i_level_number = 0;
	i_current_level = 0;

	ifstream level ( "data/level/levellist.txt" );

	while ( !(level.eof()) )
	{
		level_name.push_back(string());
		level_locked.push_back(bool());

		level >> level_name[i_level_number];
		
		int a;
		level >> a;
		if ( a == 1 )
		{
			level_locked[i_level_number] = true;
		}
		else
		{
			level_locked[i_level_number] = false;
		}

		i_level_number++;

	}

	level_name.pop_back();
	i_level_number--;

}

void Level::save_levels()
{
	ofstream levelu ( "data/level/levellist.txt" );

	for ( int n = 0; n < i_level_number; n++ )
	{
			levelu << level_name[n] << " " << level_locked[n] << "\n";
	}

}

int Level::get_level()
{
	return i_current_level;
}

string Level::give_level_name()
{
	return level_name[i_current_level];
}

int Level::get_level_number()
{
	return i_level_number;
}

void Level::next_level()
{
	i_current_level++;
}

bool Level::get_locked( int level )
{
	return level_locked[level];
}

void Level::set_level(int lvl)
{
	i_current_level = lvl;
}

void Level::set_locked(int lvl)
{
	level_locked[lvl] = false;
}