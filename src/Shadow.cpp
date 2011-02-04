#include "Classes.h"
#include "Functions.h"
#include <vector>
#include <iostream>
using namespace std;

Shadow::Shadow()
{
	s_walking[0] = load_image("data/gfx/shadowright1.png");
	s_walking[1] = load_image("data/gfx/shadowright0.png");
	s_walking[2] = load_image("data/gfx/shadowleft1.png");
	s_walking[3] = load_image("data/gfx/shadowleft0.png");

	s_standing[0] = load_image("data/gfx/shadowright0.png");
	s_standing[1] = load_image("data/gfx/shadowright0.png");
	s_standing[2] = load_image("data/gfx/shadowleft0.png");
	s_standing[3] = load_image("data/gfx/shadowleft0.png");

	s_jumping[0] = load_image("data/gfx/jumprightshadow.png");
	s_jumping[1] = load_image("data/gfx/jumpleftshadow.png");

	s_holding = load_image("data/gfx/shadowholdingright.png");

	b_called = false;
	b_shadow = true;

	i_xVel = 0;


}

void Shadow::move_logic()
{
	if ( b_called == true )
	{
		if ( right_button[i_state] == true )
		{
			i_xVel = 7;
		}

		if ( left_button[i_state] == true )
		{
			i_xVel = -7;}

		if ( left_button[i_state] == false && right_button[i_state] == false )
		{
			i_xVel = 0;
		}

		if ( jump_button[i_state] == true && b_inAir == false )
		{
			b_jump = true;
		}

		i_state++;

		if ( i_state >= (signed)right_button.size() )
		{
			b_called = false;
			i_xVel = 0;
		}
	}
}

void Shadow::me_call()
{
	b_called = true;
}

void Shadow::reset()
{
	if ( b_reset == true )
	{
		box.x = i_fx;
		box.y = i_fy;

		b_inAir = true;
		b_jump = false;
		b_on_ground = true;
		b_can_move = true;
		b_holding_other = false;
		b_reset = false;
		b_dead = false;

		i_frame = 0;
		i_animation = 0;
		i_direction = 0;

		i_state = 0;	

		
		right_button.clear();
		right_button.push_back(false);

		left_button.clear();
		left_button.push_back(false);

		jump_button.clear();
		jump_button.push_back(false);

	}
}
