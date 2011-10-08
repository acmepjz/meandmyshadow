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
#include "TitleMenu.h"
#include "GUIObject.h"
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

//#define RECORD_PICUTRE_SEQUENCE

#ifdef RECORD_PICUTRE_SEQUENCE
bool m_bRecordPictureSequence=false;
int m_nRecordPictureIndex=0;
#endif

#if 0

//test only

#else

int main ( int argc, char ** argv )
{
	if(!ParseCommandLines(argc,argv)){
		printf("Usage: %s [OPTIONS] ...\n",argv[0]);
		printf("Avaliable options:\n");
		printf("    %-20s  %s\n","--data-dir <dir>","Specifies the data directory.");
		printf("    %-20s  %s\n","--help","Display this help.");
		printf("    %-20s  %s\n","--user-dir <dir>","Specifies the user preferences directory.");
		return 0;
	}

	if ( init() == false )
	{
		fprintf(stderr,"FATAL ERROR: Failed to initalize game\n");
		return 1;
	}

	if ( load_files() == false )
	{
		fprintf(stderr,"FATAL ERROR: Failed to load necessary files\n");
		return 1;
	}
	
	if(load_settings() == false)
	{
		fprintf(stderr,"FATAL ERROR: Failed to load config file.\n");
		return 1;
	}

	//IGRA/////
	stateID = STATE_MENU;
	currentState = new Menu();

	//////LEVEL EDITOR////////
	/*stateID = STATE_LEVEL_EDITOR;
	currentState = new LevelEditor();*/

	delta.start();

	srand((unsigned)time(NULL));

	if(get_settings()->getBoolValue("sound"))
		Mix_PlayMusic(music, -1);

	if(get_settings()->getBoolValue("fullscreen"))
		SDL_SetVideoMode(screen->w,screen->h,screen->format->BitsPerPixel, SDL_FULLSCREEN | SDL_HWSURFACE);
	s_temp = SDL_CreateRGBSurface(SDL_HWSURFACE|SDL_SRCALPHA,
		screen->w,screen->h,screen->format->BitsPerPixel,
		screen->format->Rmask,screen->format->Gmask,screen->format->Bmask,0);
	int nFadeIn=0;

	while ( stateID != STATE_EXIT)
	{
		FPS.start();

		while(SDL_PollEvent(&event)){
#ifdef RECORD_PICUTRE_SEQUENCE
			if(event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_F10){
				m_bRecordPictureSequence=!m_bRecordPictureSequence;
				printf("Record Picture Sequence %s\n",m_bRecordPictureSequence?"ON":"OFF");
			}
#endif
			currentState->handle_events();
			GUIObjectHandleEvents();
		}

		currentState->logic();

		delta.start();

		set_camera();

		currentState->render();
		if(GUIObjectRoot) GUIObjectRoot->render();
		if(nFadeIn>0&&nFadeIn<255){
			SDL_BlitSurface(screen,NULL,s_temp,NULL);
			SDL_FillRect(screen,NULL,0);
			SDL_SetAlpha(s_temp, SDL_SRCALPHA, nFadeIn);
			SDL_BlitSurface(s_temp,NULL,screen,NULL);
			nFadeIn+=17;
		}
#ifdef RECORD_PICUTRE_SEQUENCE
		if(m_bRecordPictureSequence){
			char s[64];
			m_nRecordPictureIndex++;
			sprintf(s,"pic%08d.bmp",m_nRecordPictureIndex);
			printf("Save screen to %s\n",s);
			SDL_SaveBMP(screen,(GetUserPath()+s).c_str());
		}
#endif
		SDL_Flip(screen);

		if(nextState!=STATE_NULL) nFadeIn=17;
		change_state();

		int t=FPS.getTicks();
		t=( 1000 / g_FPS ) - t;
		if ( t>0 )
		{
			SDL_Delay( t );
		}

	}

	SDL_FreeSurface(s_temp);

	o_mylevels.save_level_progress();

	clean();
	return 0;
}

#endif