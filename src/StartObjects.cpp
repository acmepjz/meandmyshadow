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
#include "GameObjects.h"
#include "Functions.h"
#include "Globals.h"
#include "Classes.h"

StartObject::StartObject(int x, int y, class Player * player)
{
	box.x = x;
	box.y = y;
	box.w = 50;
	box.h = 50;

	i_type = TYPE_START_PLAYER;

	surface = load_image("data/gfx/blocks/playerstart.png");
	player->set_position(box.x, box.y);
	player->i_fx = box.x;
	player->i_fy = box.y;
}

void StartObject::show()
{
	if ( stateID == STATE_LEVEL_EDITOR )
	{
		apply_surface( box.x - camera.x, box.y - camera.y, surface, screen, NULL );
	}
}

StartObjectShadow::StartObjectShadow( int x, int y, Shadow * player )
{
	box.x = x;
	box.y = y;
	box.w = 50;
	box.h = 50;

	i_type = TYPE_START_SHADOW;

	surface = load_image("data/gfx/blocks/shadowstart.png");
	player->set_position(box.x, box.y);
	player->i_fx = box.x;
	player->i_fy = box.y;
}

void StartObjectShadow::show()
{
	if ( stateID == STATE_LEVEL_EDITOR )
	{
		apply_surface( box.x - camera.x, box.y - camera.y, surface, screen, NULL );
	}
}

/*

Exit::Exit(int x, int y)
{
	box.x = x;
	box.y = y;
	box.w = 50;
	box.h = 50;

	surface = load_image("data/gfx/blocks/exit.png");
	i_type = TYPE_EXIT;
}

void Exit::show()
{
	if ( check_collision( box, camera ) == true )
	{
		apply_surface( box.x - camera.x, box.y - camera.y, surface, screen, NULL);
	}
}

*/