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
#ifndef BLOCK_H
#define BLOCK_H

#include <SDL/SDL.h>
#include "Globals.h"
#include "GameObjects.h"
#include <vector>

class Game;

class Block : public GameObject
{
private:
	SDL_Surface *custom_surface,*surface2;
	int m_t,m_t_save,m_flags,m_flags_save;
	/*
	flags:
	moving object 0x1=disabled
	button bit0-1=behavior 0x4=pressed
	switch bit0-1=behavior
	*/

	//for moving objects
	SDL_Rect box_base;
	std::vector<SDL_Rect> MovingPos;
	int m_dx,m_x_save,m_dy,m_y_save;
	//over

	int m_editor_flags;
	/*
	flags:
	moving object 0x1=disabled
	portal 0x1=automatic
	*/

public:

	std::string id;
	std::string sImageFile;

	Block(int x, int y, int type, Game *objParent);
	~Block();

	virtual SDL_Rect get_box(int nBoxType=0);

	void show();

	virtual void save_state();
	virtual void load_state();
	virtual void reset();
	virtual void play_animation(int flags);
	virtual void OnEvent(int nEventType);
	virtual int QueryProperties(int nPropertyType,Player* obj);
	virtual void GetEditorData(std::vector<std::pair<std::string,std::string> >& obj);
	virtual void SetEditorData(std::map<std::string,std::string>& obj);
	virtual void move();
};

#endif