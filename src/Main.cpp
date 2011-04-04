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
#include <SDL/SDL.h>
#include "Functions.h"
#include "Timer.h"
#include "Objects.h"
#include "Globals.h"
#include "Title_Menu.h"
#include "GUIObject.h"
#include <cstdlib>
#include <ctime>

int main ( int argc, char * args[] )
{
	if ( init() == false )
	{
		return 1;
	}

	if ( load_files() == false )
	{
		return 1;
	}

	//IGRA/////
	stateID = STATE_MENU;
	currentState = new Menu();

	//////LEVEL EDITOR////////
	/*stateID = STATE_LEVEL_EDITOR;
	currentState = new LevelEditor();*/

	delta.start();

	srand(time(NULL));

	Mix_PlayMusic(music, -1);

	while ( stateID != STATE_EXIT)
	{
		FPS.start();

		while(SDL_PollEvent(&event)){
			currentState->handle_events();
			GUIObjectHandleEvents();
		}

		currentState->logic();

		delta.start();

		set_camera();

		currentState->render();
		if(GUIObjectRoot) GUIObjectRoot->render();
		SDL_Flip(screen);

		change_state();

		if ( FPS.get_ticks() < 1000 / g_FPS )
		{
			SDL_Delay( ( 1000 / g_FPS ) - FPS.get_ticks() );
		}

	}

	o_mylevels.save_levels();

	clean();
	return 0;
}
