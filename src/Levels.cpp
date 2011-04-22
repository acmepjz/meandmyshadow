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
#include "Levels.h"
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
using namespace std;

Level::Level()
{
	i_level_number = 0;
	i_current_level = 0;

	ifstream level ( DATA_PATH "data/level/levellist.txt" );

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
	ofstream levelu ( DATA_PATH "data/level/levellist.txt" );

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
