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
#include "Classes.h"
#include "Functions.h"
#include "Globals.h"
#include "Objects.h"
#include "LevelSelect.h"
#include <sstream>
#include <SDL/SDL_ttf.h>
#include <SDL/SDL.h>
#include <iostream>

////////////////////NUMBER////////////////////////
Number::Number( )
{
	s_image = NULL;
	s_level = NULL;

	myBox.x = 0; myBox.y = 0; myBox.h = 50; myBox.w = 50;
}

Number::~Number()
{
	if(s_image) SDL_FreeSurface(s_image);
}

void Number::init(int number, SDL_Rect box )
{
	if ( o_mylevels.get_locked(number) == false )
	{
		s_level = load_image(DATA_PATH "data/gfx/level.png");
	}

	else { s_level = load_image(DATA_PATH "data/gfx/levellocked.png"); }
                
	std::stringstream text;
                
	text << number;
                
	SDL_Color black = { 0,0,0 };
                
	s_image = TTF_RenderText_Blended(font, text.str().c_str(), black);
                
    myBox.x = box.x; myBox.y = box.y; myBox.h = 50; myBox.w = 50; 
}

void Number::show()
{
     apply_surface( myBox.x,myBox.y, s_level, screen,NULL );
     
     apply_surface( (myBox.x + 25 - (s_image->w / 2)), (myBox.y + 25 - (s_image->h / 2)), s_image, screen, NULL );
}


/////////////////////LEVEL SELECT/////////////////////
LevelSelect::LevelSelect()
{
                          s_background = load_image(DATA_PATH "data/gfx/menu/levelselect.png");
                          

                                                   
                          for ( int n = 0; n < o_mylevels.get_level_number(); n++ )
                          {
								 o_number.push_back( Number () );
								
                              
                            
                          }     

						  lol = 0;

}

LevelSelect::~LevelSelect()
{
}

void LevelSelect::handle_events()
{
         if ( event.type == SDL_QUIT )
		 {
			 next_state(STATE_EXIT);
		 }

		 if ( event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT )
		 {
				check_mouse();
		 }

		 if ( event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_ESCAPE )
		 {
			 next_state(STATE_MENU);
		 }

		 if ( event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_s )
			{
				if ( Mix_PlayingMusic() == 1 )
				{
					Mix_HaltMusic();
				}

				else 
				{
					Mix_PlayMusic(music,-1);
				}				
			}
}

void LevelSelect::check_mouse()
{
	int x, y;

	SDL_GetMouseState(&x,&y);

	SDL_Rect mouse = { x,y,5,5};

	for ( int n = 0; n < (signed)o_mylevels.get_level_number(); n++ )
	{
		if ( o_mylevels.get_locked(n) == false )
		{
			if ( check_collision( mouse, o_number[n].myBox ) == true )
			{
				o_mylevels.set_level(n);
				next_state(STATE_GAME);
			}
		}
	}
}


void LevelSelect::logic()
{
     if ( lol == 0 )
	 {
		  SDL_Rect box;

          box.x= 60; box.y= 90; box.h=0; box.w=0;

		 for ( int n = 0; n < o_mylevels.get_level_number(); n++ )
                          {
								 o_number[n].init( n, box );
                              
                              box.x += 100;
                              
                              if ( box.x >= 750 ) { box.x= 60; box.y += 80; }
                          }     
		
		 lol = 1;
	 }


}

void LevelSelect::render()
{
     apply_surface( 0 , 0, s_background, screen, NULL );
     
     for ( int n = 0; n < (signed)o_number.size(); n++ )
     {
        o_number[n].show();
               
     }    
}
