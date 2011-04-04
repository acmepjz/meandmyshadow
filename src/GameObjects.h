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
#ifndef GAME_OBJECTS_H
#define GAME_OBJECTS_H

#include <SDL/SDL.h>
#include "Globals.h"

class Game;
class Player;

const int GameObjectEvent_PlayerWalkOn = 1;

const int GameObjectProperty_PlayerCanWalkOn = 1;

class GameObject
{
protected:
	SDL_Rect box;

	SDL_Surface *surface;
	
public:

	int i_type;
	Game *m_objParent;

	GameObject(Game *objParent);
	~GameObject();

	SDL_Rect get_box();

	virtual void show() = 0;
	virtual void save_state();
	virtual void load_state();
	virtual void reset();
	//
	virtual void play_animation(int flags);
	virtual void OnEvent(int nEventType);
	virtual int QueryProperties(int nPropertyType,Player* obj);
	
};

#include "StartObjects.h"
#include "Block.h"

#endif
