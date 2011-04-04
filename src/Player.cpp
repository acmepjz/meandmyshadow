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
#include "Player.h"
#include "Game.h"
#include "Functions.h"
#include "Globals.h"
#include "Objects.h"
#include <iostream>
#include <SDL/SDL_mixer.h>
#include <SDL/SDL.h>
using namespace std;

Player::Player(Game* objParent,bool bLoadImage):m_objParent(objParent)
{
	box.x = 0;
	box.y = 0;
	box.w = 21;
	box.h = 40;

	i_xVel = 0; //PLAYER_X_SPEED;
	i_yVel = 0;

	i_fx = 0;
	i_fy = 0;

	if(bLoadImage){
		s_walking[0] = load_image("data/gfx/player/playerright1.png");
		s_walking[1] = load_image("data/gfx/player/playerright0.png");
		s_walking[2] = load_image("data/gfx/player/playerleft1.png");
		s_walking[3] = load_image("data/gfx/player/playerleft0.png");

		s_standing[0] = load_image("data/gfx/player/playerright0.png");
		s_standing[1] = load_image("data/gfx/player/playerright0.png");
		s_standing[2] = load_image("data/gfx/player/playerleft0.png");
		s_standing[3] = load_image("data/gfx/player/playerleft0.png");

		s_jumping[0] = load_image("data/gfx/player/jumpright.png");
		s_jumping[1] = load_image("data/gfx/player/jumpleft.png");

		s_holding = load_image("data/gfx/player/playerholdingright.png");
	}else{
		s_walking[0] = NULL;
		s_walking[1] = NULL;
		s_walking[2] = NULL;
		s_walking[3] = NULL;

		s_standing[0] = NULL;
		s_standing[1] = NULL;
		s_standing[2] = NULL;
		s_standing[3] = NULL;

		s_jumping[0] = NULL;
		s_jumping[1] = NULL;

		s_holding = NULL;
	}

	s_line = load_image("data/gfx/player/line.png");
	SDL_SetAlpha(s_line, SDL_SRCALPHA, 100);

	c_jump = Mix_LoadWAV("data/sfx/jump.wav");
	c_hit = Mix_LoadWAV("data/sfx/hit.wav");
	c_save = Mix_LoadWAV("data/sfx/checkpoint.wav");
	c_swap = Mix_LoadWAV("data/sfx/swap.wav");

	b_inAir = true;
	b_jump = false;
	b_on_ground = true;
	b_shadow_call = false;
	b_shadow = false;
	b_can_move = true;
	b_holding_other = false;
	b_dead = false;

	b_record = false;

	i_frame = 0;
	i_animation = 0;
	i_direction = 0;
	i_jump_time = 0;

	i_state = 0;

	//new
	i_xVel_saved = 0x80000000;
}

Player::~Player()
{
	Mix_FreeChunk(c_jump);
	Mix_FreeChunk(c_hit);
	Mix_FreeChunk(c_save);
	Mix_FreeChunk(c_swap);

	//PLAYER_X_SPEED = i_xVel;
}

void Player::handle_input(class Shadow * shadow)
{
	Uint8 *lpKeyState=SDL_GetKeyState(NULL);
	i_xVel=0;
	if(lpKeyState[SDLK_RIGHT]) i_xVel += 7;
	if(lpKeyState[SDLK_LEFT]) i_xVel -= 7;

	if ( event.type == SDL_KEYUP )
	{
		/*switch(event.key.keysym.sym)
		{
		case SDLK_RIGHT: i_xVel -= 7; break;
		case SDLK_LEFT: i_xVel += 7; break;
		default: break;
		}*/
		bDownKeyPressed=false; //???

	}

	if ( event.type == SDL_KEYDOWN )
	{
		switch(event.key.keysym.sym)
		{
		//case SDLK_RIGHT: i_xVel += 7; break;
		//case SDLK_LEFT: i_xVel -= 7; break;
		case SDLK_UP: if ( b_inAir == false ) {b_jump = true;} break;
		case SDLK_SPACE:
			if ( b_record == false ) {
				if( shadow->b_called == true) {
					b_shadow_call = false;

					shadow->b_called = false;
					shadow->player_button.clear();
				} else if(!b_dead) {
					b_record = true;
				}
			} else {
				b_record = false;
				b_shadow_call = true;
			}
			break;
		//new and TEST ONLY
		case SDLK_DOWN: bDownKeyPressed=true; break;
		/*case SDLK_F2:
			if(!(b_dead || shadow->b_dead)){
				if(m_objParent) m_objParent->save_state();
			}
			break;*/
		case SDLK_F3:
			if(m_objParent) m_objParent->load_state();
			break;
		/*case SDLK_F4:
			if(!(b_dead || shadow->b_dead)){
				swap_state(shadow);
			}
			break;*/
		//end
		default:
			break;
		}
	}

}

void Player::set_position( int x, int y )
{
	box.x = x;
	box.y = y;
}

void Player::move(vector<GameObject*> &LevelObjects)
{
	GameObject *objCheckPoint=NULL,*objSwap=NULL;

	if ( b_dead == false )
	{
		//Add gravity
		if ( b_inAir == true )
		{
			i_yVel += 1;	
			if ( i_yVel > 13 )
			{
				i_yVel = 13;
			}
		}

		//~ bool costumy = false;
		bool costumx = false;

		if ( b_can_move == true )
		{
			//Test x
			SDL_Rect testbox; testbox.x = box.x; testbox.y = box.y;
			testbox.x += i_xVel;
			testbox.w = box.w;
			testbox.h = box.h;
			int i_xmove = 0;
			//~ int i_ymove = 0;
			if ( i_xVel > 0 ) { i_direction = 0 ; b_on_ground = false; }
			else if ( i_xVel < 0 ) { i_direction = 1; b_on_ground = false; }
			else if ( i_xVel == 0 ) { b_on_ground = true; }

			for ( int o = 0; o < (signed)LevelObjects.size(); o++ )
			{
				if ( LevelObjects[o]->QueryProperties(GameObjectProperty_PlayerCanWalkOn,this) )
				{
					if ( check_collision( testbox, LevelObjects[o]->get_box() ) )
					{
						if ( box.x + box.w <= LevelObjects[o]->get_box().x )
						{
							i_xmove = LevelObjects[o]->get_box().x - (box.x + box.w );	
							costumx = true;
							break;
						}

						if ( box.x >= LevelObjects[o]->get_box().x + LevelObjects[o]->get_box().w )
						{
							i_xmove = -(box.x - (LevelObjects[o]->get_box().x + LevelObjects[o]->get_box().w));
							costumx = true;
							break;
						}

					}
				}

				//save game?
				if ( LevelObjects[o]->i_type == TYPE_CHECKPOINT )
				{
					if ( check_collision( testbox, LevelObjects[o]->get_box() ) )
					{
						objCheckPoint=LevelObjects[o];
					}
				}

				//can swap?
				if ( LevelObjects[o]->i_type == TYPE_SWAP )
				{
					if ( check_collision( testbox, LevelObjects[o]->get_box() ) ) objSwap=LevelObjects[o];
				}

				if ( LevelObjects[o]->i_type == TYPE_EXIT && stateID != STATE_LEVEL_EDITOR )
				{
					if ( check_collision ( testbox, LevelObjects[o]->get_box() ) )
					{
						o_mylevels.next_level();

						if ( o_mylevels.get_level() < o_mylevels.get_level_number() )
						{
							o_mylevels.set_locked(o_mylevels.get_level());
							next_state(STATE_GAME);
						}
					}
				}

				if ( LevelObjects[o]->i_type == TYPE_SPIKES )
				{
					if ( check_collision ( testbox, LevelObjects[o]->get_box() ) )
					{
						b_dead = true;
						Mix_PlayChannel(-1, c_hit, 0);
					}
				}
			}	
			
			//////////////////////
			if ( !costumx )
			{	box.x += i_xVel;}
			else
			{
				box.x += i_xmove;
			}
		}

		box.y += i_yVel;

		GameObject *objLastStand=NULL;

		for ( int o = 0; o < (signed)LevelObjects.size(); o++ )
		{
			if ( LevelObjects[o]->QueryProperties(GameObjectProperty_PlayerCanWalkOn,this) )
			{
				if ( check_collision ( LevelObjects[o]->get_box(), box ) )
				{
					box.y -= i_yVel;
					b_inAir = false;


					if ( i_yVel > 0 )
					{
						box.y += LevelObjects[o]->get_box().y - ( box.y + box.h );
						i_yVel = 1;
						//printf("%08X hit %08X\n",this,LevelObjects[o]);
						objLastStand=LevelObjects[o];
					}
					else if ( i_yVel < 0 ) { 
						i_yVel = 0; 
						box.y -= box.y  - ( LevelObjects[o]->get_box().y + LevelObjects[o]->get_box().h );
					}
					break;
				}

				else {b_inAir = true; b_can_move = true; }
			}

			if ( LevelObjects[o]->i_type == TYPE_SPIKES )
			{
				if ( check_collision ( box, LevelObjects[o]->get_box() ) )
				{
					b_dead = true;
					Mix_PlayChannel(-1, c_hit, 0);
				}
			}
		}

		if(objLastStand!=m_objLastStand){
			m_objLastStand=objLastStand;
			if(objLastStand) objLastStand->OnEvent(GameObjectEvent_PlayerWalkOn);
		}
	}

	//check checkpoint
	if(m_objParent!=NULL && bDownKeyPressed && objCheckPoint!=NULL){
		if(m_objParent->save_state()) m_objParent->objLastCheckPoint=objCheckPoint;
	}
	if(objSwap!=NULL && bDownKeyPressed && m_objParent!=NULL){
		if(b_shadow){
			if(!(b_dead || m_objParent->o_player.b_dead)){
				m_objParent->o_player.swap_state(this);
				Mix_PlayChannel(-1, c_swap, 0);
				objSwap->play_animation(1);
			}
		}else{
			if(!(b_dead || m_objParent->o_shadow.b_dead)){
				swap_state(&m_objParent->o_shadow);
				Mix_PlayChannel(-1, c_swap, 0);
				objSwap->play_animation(1);
			}
		}
	}
	bDownKeyPressed=false;

}


void Player::jump()
{
	if ( b_dead == true )
	{
		b_jump = false;
	}

	if ( b_jump == true && b_inAir == false  )
	{
		i_yVel = -13;
		b_inAir = true;
		b_jump = false;
		i_jump_time++;
		Mix_PlayChannel(-1, c_jump, 0 );
	}

}

void Player::show()
{
	if ( b_shadow == false && b_record == true)
	{
		line.push_back(SDL_Rect());
		line[line.size() - 1].x = box.x + 11;
		line[line.size() - 1].y = box.y + 20;

		for ( int l = 0; l < (signed)line.size(); l++ )
		{
			apply_surface( line[l].x - camera.x, line[l].y - camera.y, s_line, screen, NULL );
		}
	}

	if ( b_dead == false )
	{
		i_frame++;
		if ( i_frame >= 5 )
		{
			i_animation++;
			if ( i_animation >= 2 )
			{
				i_animation = 0;
			}

			i_frame = 0;
		}


		if ( b_inAir == false )
		{
			if ( i_direction == 0 )
			{
				if ( b_on_ground == false )
				{
					apply_surface( box.x - camera.x, box.y - camera.y, s_walking[0+i_animation], screen, NULL );

				}
				else
				{ 
					if ( b_holding_other == true )
					{
						apply_surface( box.x - camera.x, box.y - camera.y, s_holding, screen, NULL);
					}
					else { apply_surface( box.x - camera.x, box.y - camera.y, s_standing[0+i_animation], screen, NULL ); }
				}
			}
			else if ( i_direction == 1 )
			{
				if ( b_on_ground == false )
				{
					apply_surface( box.x - camera.x, box.y - camera.y, s_walking[2+i_animation], screen, NULL );}
				else { 
					if ( b_holding_other == true )
					{
						apply_surface( box.x - camera.x, box.y - camera.y, s_holding, screen, NULL);
					}
					else {apply_surface( box.x - camera.x, box.y - camera.y, s_standing[2+i_animation], screen, NULL ); } }
			}
		}

		else
		{
			if ( i_direction == 0 )
			{
				apply_surface( box.x - camera.x, box.y - camera.y, s_jumping[0], screen, NULL);
			}

			if ( i_direction == 1 )
			{
				apply_surface( box.x - camera.x, box.y - camera.y, s_jumping[1], screen, NULL);
			}
		}
	}

}

void Player::shadow_set_state()
{
	if(b_record) {
		int nCurrentButton=0;

		if ( i_direction == 0 && !b_on_ground ) nCurrentButton |= PlayerButtonRight;

		if ( i_direction == 1 && !b_on_ground ) nCurrentButton |= PlayerButtonLeft;

		if ( b_jump ) nCurrentButton |= PlayerButtonJump;
		
		if ( bDownKeyPressed ) nCurrentButton |= PlayerButtonDown;

		player_button.push_back(nCurrentButton);
		i_state++;
	}
}

void Player::shadow_give_state(Shadow *shadow)
{
	if ( b_shadow_call == true )
	{
		//Zbrisi vse vectore shadow
		shadow->player_button.clear();

		//Napisi nove
		for ( unsigned int s = 0; s < player_button.size(); s++ )
		{
			shadow->player_button.push_back(player_button[s]);
		}

		//Resetiraj state
		state_reset();
		shadow->state_reset();

		//brisi vse vectore svoje
		player_button.clear();
		line.clear();

		b_shadow_call = false;
		shadow->me_call();
	}
}

void Player::state_reset()
{
	i_state = 0;
}

void Player::other_check(class Player * other)
{
	if ( b_dead == false && other->b_dead == false )
	{
		SDL_Rect box_shadow = other->get_box();

		if ( check_collision(box, box_shadow) == true )
		{
			if ( box.y + box.h <= box_shadow.y + 13 )
			{
				int yVel = i_yVel - 1;
				if ( yVel > 0 )
				{
					box.y -= i_yVel;
					box.y += box_shadow.y - ( box.y + box.h );
					b_inAir = false;
					b_can_move = false;
					b_on_ground = true;
					other->b_holding_other = true;
				}
			}
		}

		else { other->b_holding_other = false; }
	}
}

SDL_Rect Player::get_box()
{
	return box;
}

void Player::set_mycamera()
{
	if ( box.x > camera.x + 450 )
	{
		camera.x += 7;
	}

	if ( box.x < camera.x + 350 )
	{
		camera.x -= 7;
	}

	if ( camera.x < 0 )
	{
		camera.x = 0;
	}

	if ( camera.x + camera.w > LEVEL_WIDTH)
	{
		camera.x = LEVEL_WIDTH - camera.w;
	}

	if ( box.y > camera.y + 350 )
	{
		camera.y += 7;
	}

	if ( box.y < camera.y + 250 )
	{
		camera.y -= 7;
	}

	if ( camera.y < 0 )
	{
		camera.y = 0;
	}

	if ( camera.y + camera.h > LEVEL_HEIGHT )
	{
		camera.y = LEVEL_HEIGHT - camera.h;
	}
}

void Player::reset()
{
	box.x = i_fx;
	box.y = i_fy;

	b_inAir = true;
	b_jump = false;
	b_on_ground = true;
	b_shadow_call = false;
	b_can_move = true;
	b_holding_other = false;
	b_dead = false;
	b_record = false;

	i_frame = 0;
	i_animation = 0;
	i_direction = 0;

	i_state = 0;	
	i_yVel = 0;


	line.clear();
	player_button.clear();

	//new
	i_xVel_saved = 0x80000000;
}

//new

void Player::save_state(){
	if(!b_dead){
		box_saved.x=box.x;
		box_saved.y=box.y;
		i_xVel_saved=i_xVel;
		i_yVel_saved=i_yVel;
		b_inAir_saved=b_inAir;
		b_jump_saved=b_jump;
		b_on_ground_saved=b_on_ground;
		b_can_move_saved=b_can_move;
		b_holding_other_saved=b_holding_other;
		if(b_shadow) Mix_PlayChannel(-1, c_save, 0);
	}
}

void Player::load_state(){
	if(i_xVel_saved == 0x80000000){
		reset();
		return;
	}
	box.x=box_saved.x;
	box.y=box_saved.y;
	i_xVel=0; //i_xVel_saved;
	i_yVel=i_yVel_saved; //0;
	b_inAir=b_inAir_saved;
	b_jump=b_jump_saved;
	b_on_ground=b_on_ground_saved;
	b_can_move=b_can_move_saved;
	b_holding_other=b_holding_other_saved;
	b_dead=false;
	b_record=false;
	b_shadow_call=false;
	state_reset();
	//???
	line.clear();
	player_button.clear();
}

void Player::swap_state(Player * other){
	swap(box.x,other->box.x);
	swap(box.y,other->box.y);
	swap(i_yVel,other->i_yVel);
	swap(b_inAir,other->b_inAir);
	swap(b_jump,other->b_jump);
	swap(b_on_ground,other->b_on_ground);
	swap(b_can_move,other->b_can_move);
	swap(b_holding_other,other->b_holding_other);
	swap(b_dead,other->b_dead);
	b_record=false;
	b_shadow_call=false;
	state_reset();
	//???
	line.clear();
	player_button.clear();
	//????????
	other->state_reset();
}

bool Player::can_save_state(){
	return !b_dead;
}

bool Player::can_load_state(){
	return i_xVel_saved != 0x80000000;
}

//end
