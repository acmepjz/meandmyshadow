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
#include "Functions.h"
#include "TreeStorageNode.h"
#include "POASerializer.h"
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
using namespace std;

bool Level::load_levels(const std::string& level_list_file,const std::string& level_progress_file){
	i_level_count = 0;
	i_current_level = 0;
	m_bLoaded = false;
	level_name.clear();
	level_files.clear();
	level_locked.clear();

	string level_list_new=ProcessFileName(level_list_file);

	ifstream level ( level_list_new.c_str() );
	ifstream level_progress ( ProcessFileName(level_progress_file).c_str() );

	if(!level){
		cerr<<"Error: Can't load level list "<<level_list_new<<endl;
		return false;
	}

	TreeStorageNode obj;
	{
		POASerializer objSerializer;
		if(!objSerializer.ReadNode(level,&obj,true)){
			cerr<<"Error: Invalid file format of level list "<<level_list_new<<endl;
			return false;
		}
	}

	for(unsigned int i=0;i<obj.SubNodes.size();i++){
		TreeStorageNode* obj1=obj.SubNodes[i];
		if(obj1==NULL) continue;
		if(obj1->Value.size()>=2 && obj1->Name=="levelfile"){
			level_files.push_back(obj1->Value[0]);
			level_name.push_back(obj1->Value[1]);
			//load level progress
			int a=1;
			if(level_progress.is_open() && !level_progress.eof()) level_progress >> a;
			level_locked.push_back( !( a==0 || i_level_count==0 ) );
			//over
			i_level_count++;
		}
	}

	m_bLoaded=true;
	return true;
}

void Level::save_levels(const std::string& level_progress_file)
{
	if(!m_bLoaded) return;

	ofstream level_progress ( ProcessFileName(level_progress_file).c_str() );

	for ( int n = 0; n < i_level_count; n++ )
	{
		level_progress << (level_locked[n]?1:0) << "\n";
	}
}

const string& Level::get_level_name(int level)
{
	if(level<0) level=i_current_level;
	return level_name[level];
}

const string& Level::get_level_file(int level)
{
	if(level<0) level=i_current_level;
	return level_files[level];
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

void Level::set_locked(int lvl,bool bLocked)
{
	level_locked[lvl] = bLocked;
}
