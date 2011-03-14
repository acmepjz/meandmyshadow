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

#include "StartObjects.h"
#include "Block.h"

#endif
