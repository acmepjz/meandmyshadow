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
#include "Game.h"
#include "Player.h"
#include "Block.h"
#include "Functions.h"
#include "Globals.h"
#include <iostream>
#include <cstdlib>
using namespace std;

Block::Block( int x, int y, int type, Game *objParent):GameObject(objParent),surface2(NULL),m_t(0),m_t_save(0)
{
	box.x = x; box.y = y;
	box.w = 50; box.h = 50;
	
	if ( type == TYPE_BLOCK )
	{
		switch ( rand() % 10 )
		{
		case 0:
			surface = load_image("data/gfx/blocks/block2.png");
			break;

		case 1:
			surface = load_image("data/gfx/blocks/block3.png");
			break;

		default:
			surface = load_image("data/gfx/blocks/block.png");
			break;
		}

		i_type = TYPE_BLOCK;
	}
	else if ( type == TYPE_SHADOW_BLOCK )
	{
		surface = load_image("data/gfx/blocks/shadowblock.png");
		i_type = TYPE_SHADOW_BLOCK;
	}
	
	else if ( type == TYPE_SPIKES )
	{
		surface = load_image("data/gfx/blocks/spikes.png");
		i_type = TYPE_SPIKES;
	}
	else if ( type == TYPE_EXIT )
	{
		surface = load_image("data/gfx/blocks/exit.png");
		i_type = TYPE_EXIT;
	}
	else if ( type == TYPE_CHECKPOINT )
	{
		surface = load_image("data/gfx/blocks/checkpoint.png");
		surface2 = load_image("data/gfx/blocks/checkpoint_1.png");
		i_type = TYPE_CHECKPOINT;
	}
	else if ( type == TYPE_SWAP )
	{
		surface = load_image("data/gfx/blocks/swap.png");
		i_type = TYPE_SWAP;
	}
	else if ( type == TYPE_FRAGILE )
	{
		surface = load_image("data/gfx/blocks/fragile.png");
		i_type = TYPE_FRAGILE;
	}
}

Block::~Block()
{
}

void Block::show()
{
	if ( check_collision(camera, box) == true )
	{
		if(i_type==TYPE_CHECKPOINT){
			if(m_objParent!=NULL && m_objParent->objLastCheckPoint == this){
				int i=m_t;
				if(i>=4&&i<12) i=8-i;
				else if(i>=12) i-=16;
				apply_surface( box.x - camera.x, box.y - camera.y + i*2, surface2, screen, NULL ); 
				m_t=(m_t+1)&0xF;
				return;
			}else{
				m_t=0;
			}
		}
		SDL_Rect r={0,0,50,50};
		if(i_type==TYPE_SWAP && m_t>0){
			r.x=(m_t%12)*50;
			m_t++;
			if(m_t>=24) m_t=0;
		}
		if(i_type==TYPE_FRAGILE){
			if(m_t>=3) return;
			r.x=m_t*50;
		}
		apply_surface( box.x - camera.x, box.y - camera.y, surface, screen, &r ); 
	}
}

void Block::reset(){
	switch(i_type){
	case TYPE_FRAGILE:
		m_t=0;
		m_t_save=0;
		break;
	}
}

void Block::save_state(){
	switch(i_type){
	case TYPE_FRAGILE:
		m_t_save=m_t;
		break;
	}
}

void Block::load_state(){
	switch(i_type){
	case TYPE_FRAGILE:
		m_t=m_t_save;
		break;
	}
}

void Block::play_animation(int flags){
	switch(i_type){
	case TYPE_SWAP:
		m_t=1;
		break;
	}
}

void Block::OnEvent(int nEventType){
	switch(nEventType){
	case GameObjectEvent_PlayerWalkOn:
		switch(i_type){
		case TYPE_FRAGILE:
			m_t++;
			break;
		}
		break;
	}
}

int Block::QueryProperties(int nPropertyType,Player* obj){
	switch(nPropertyType){
	case GameObjectProperty_PlayerCanWalkOn:
		if(i_type == TYPE_BLOCK || (i_type == TYPE_SHADOW_BLOCK && obj!=NULL && obj->is_shadow())) return 1;
		if(i_type == TYPE_FRAGILE && m_t<3) return 1;
		break;
	default:
		break;
	}
	return 0;
}
