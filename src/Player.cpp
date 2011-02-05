#include "Classes.h"
#include "Player.h"
#include "Functions.h"
#include "Globals.h"
#include "Objects.h"
#include <iostream>
#include <SDL/SDL_mixer.h>
#include <SDL/SDL.h>
using namespace std;

Player::Player()
{
	box.x = 0;
	box.y = 0;
	box.w = 21;
	box.h = 40;

	i_xVel = PLAYER_X_SPEED;
	i_yVel = 0;

	i_fx = 0;
	i_fy = 0;

	s_walking[0] = load_image("data/gfx/playerright1.png");
	s_walking[1] = load_image("data/gfx/playerright0.png");
	s_walking[2] = load_image("data/gfx/playerleft1.png");
	s_walking[3] = load_image("data/gfx/playerleft0.png");

	s_standing[0] = load_image("data/gfx/playerright0.png");
	s_standing[1] = load_image("data/gfx/playerright0.png");
	s_standing[2] = load_image("data/gfx/playerleft0.png");
	s_standing[3] = load_image("data/gfx/playerleft0.png");

	s_jumping[0] = load_image("data/gfx/jumpright.png");
	s_jumping[1] = load_image("data/gfx/jumpleft.png");

	s_holding = load_image("data/gfx/playerholdingright.png");

	s_line = load_image("data/gfx/line.png");
	SDL_SetAlpha(s_line, SDL_SRCALPHA, 100);

	c_jump = Mix_LoadWAV("data/sfx/jump.wav");

	c_hit = Mix_LoadWAV("data/sfx/hit.wav");

	b_inAir = true;
	b_jump = false;
	b_on_ground = true;
	b_shadow_call = false;
	b_shadow = false;
	b_can_move = true;
	b_holding_other = false;
	b_reset = false;
	b_dead = false;

	b_record = false;

	i_frame = 0;
	i_animation = 0;
	i_direction = 0;
	i_jump_time = 0;

	i_state = 0;

}

Player::~Player()
{
	SDL_FreeSurface(s_walking[0]);
	SDL_FreeSurface(s_walking[1]);
	SDL_FreeSurface(s_walking[2]);
	SDL_FreeSurface(s_walking[3]);

	SDL_FreeSurface(s_standing[0]);
	SDL_FreeSurface(s_standing[1]);
	SDL_FreeSurface(s_standing[2]);
	SDL_FreeSurface(s_standing[3]);

	SDL_FreeSurface(s_jumping[0]);
	SDL_FreeSurface(s_jumping[1]);

	SDL_FreeSurface(s_holding);

	Mix_FreeChunk(c_jump);
	Mix_FreeChunk(c_hit);

	PLAYER_X_SPEED = i_xVel;
}

void Player::handle_input(class Shadow * shadow)
{
	if ( event.type == SDL_KEYUP )
	{
		switch(event.key.keysym.sym)
		{
		case SDLK_RIGHT: i_xVel -= 7; break;
		case SDLK_LEFT: i_xVel += 7; break;
		}

	}

	if ( event.type == SDL_KEYDOWN )
	{
		switch(event.key.keysym.sym)
		{
		case SDLK_RIGHT: i_xVel += 7; break;
		case SDLK_LEFT: i_xVel -= 7; break;
		case SDLK_UP: if ( b_inAir == false ) {b_jump = true;} break;
		case SDLK_SPACE:
			if ( b_record == false ) {
				if( shadow->b_called == true) {
					b_shadow_call = false;

					shadow->b_called = false;
					shadow->right_button.clear();
					shadow->left_button.clear();
					shadow->jump_button.clear();
				} else {
					b_record = true;
				}
			} else {
				b_record = false;
				b_shadow_call = true;
			}
			break;
		case SDLK_r: b_reset = true; shadow->b_reset = true; break;
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

		bool costumy = false;
		bool costumx = false;

		if ( b_can_move == true )
		{
			//Test x
			SDL_Rect testbox; testbox.x = box.x; testbox.y = box.y;
			testbox.x += i_xVel;
			testbox.w = box.w;
			testbox.h = box.h;
			int i_xmove = 0;
			int i_ymove = 0;
			if ( i_xVel > 0 ) { i_direction = 0 ; b_on_ground = false; }
			else if ( i_xVel < 0 ) { i_direction = 1; b_on_ground = false; }
			else if ( i_xVel == 0 ) { b_on_ground = true; }

			for ( int o = 0; o < (signed)LevelObjects.size(); o++ )
			{
				if ( LevelObjects[o]->i_type == TYPE_BLOCK || (b_shadow == true && LevelObjects[o]->i_type == TYPE_SHADOW_BLOCK) )
				{
					if ( check_collision( testbox, LevelObjects[o]->get_box() ) == true )
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

				if ( LevelObjects[o]->i_type == TYPE_EXIT )
				{
					if ( check_collision ( testbox, LevelObjects[o]->get_box() ) == true )
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
					if ( check_collision ( testbox, LevelObjects[o]->get_box() ) == true )
					{
						b_dead = true;
						Mix_PlayChannel(-1, c_hit, 0);
					}
				}
			}	

			//////////////////////
			if ( costumx == false )
			{	box.x += i_xVel;}
			else if ( costumx == true )
			{
				box.x += i_xmove;}
		}

		box.y += i_yVel;

		for ( int o = 0; o < (signed)LevelObjects.size(); o++ )
		{
			if ( LevelObjects[o]->i_type == TYPE_BLOCK || (b_shadow == true && LevelObjects[o]->i_type == TYPE_SHADOW_BLOCK) )
			{
				if ( check_collision ( LevelObjects[o]->get_box(), box ) == true )
				{
					box.y -= i_yVel;
					b_inAir = false;


					if ( i_yVel > 0 )
					{
						box.y += LevelObjects[o]->get_box().y - ( box.y + box.h );
						i_yVel = 1;
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
				if ( check_collision ( box, LevelObjects[o]->get_box() ) == true )
				{
					b_dead = true;
					Mix_PlayChannel(-1, c_hit, 0);
				}
			}
		}
	}

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
		right_button.push_back(bool());

		if ( i_direction == 0 && b_on_ground == false )
		{
			right_button[i_state] = true;
		}
		else { right_button[i_state] = false; }



		left_button.push_back(bool());

		if ( i_direction == 1 && b_on_ground == false )
		{
			left_button[i_state] = true;
		}
		else { left_button[i_state] = false; }

		jump_button.push_back(bool());

		if ( b_jump == true )
		{
			jump_button[i_state] = true;
		}
		else {	jump_button[i_state] = false; }

		i_state++;
	}
}

void Player::shadow_give_state(Shadow *shadow)
{
	if ( b_shadow_call == true )
	{
		//Zbrisi vse vectore shadow
		shadow->right_button.clear();
		shadow->left_button.clear();
		shadow->jump_button.clear();

		//Napisi nove
		for ( unsigned int s = 0; s < right_button.size(); s++ )
		{
			shadow->right_button.push_back(right_button[s]);
			shadow->left_button.push_back(left_button[s]);
			shadow->jump_button.push_back(jump_button[s]);
		}

		//Resetiraj state
		state_reset();
		shadow->state_reset();

		//brisi vse vectore svoje
		right_button.clear();
		left_button.clear();
		jump_button.clear();
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
	if ( b_reset == true )
	{
		box.x = i_fx;
		box.y = i_fy;

		b_inAir = true;
		b_jump = false;
		b_on_ground = true;
		b_shadow_call = false;
		b_can_move = true;
		b_holding_other = false;
		b_reset = false;
		b_dead = false;
		b_record = false;

		i_frame = 0;
		i_animation = 0;
		i_direction = 0;

		i_state = 0;	
		i_yVel = 0;


		line.clear();
		right_button.clear();
		left_button.clear();
		jump_button.clear();
	}
}
