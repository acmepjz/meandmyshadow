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

Block::Block( int x, int y, int type, Game *objParent):GameObject(objParent),surface2(NULL),
m_t(0),m_t_save(0),m_x_save(0),m_y_save(0)
{
	box.x = x; box.y = y;
	box.w = 50; box.h = 50;

	box_base.x = x; box_base.y = y;
	
	i_type = type;
	if ( type == TYPE_BLOCK || (type == TYPE_MOVING_BLOCK && stateID != STATE_LEVEL_EDITOR))
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
	}
	else if ( type == TYPE_SHADOW_BLOCK || (type == TYPE_MOVING_SHADOW_BLOCK && stateID != STATE_LEVEL_EDITOR))
	{
		surface = load_image("data/gfx/blocks/shadowblock.png");
	}	
	else if ( type == TYPE_SPIKES || (type == TYPE_MOVING_SPIKES && stateID != STATE_LEVEL_EDITOR))
	{
		surface = load_image("data/gfx/blocks/spikes.png");
	}
	else if ( type == TYPE_EXIT )
	{
		surface = load_image("data/gfx/blocks/exit.png");
	}
	else if ( type == TYPE_CHECKPOINT )
	{
		surface = load_image("data/gfx/blocks/checkpoint.png");
		surface2 = load_image("data/gfx/blocks/checkpoint_1.png");
	}
	else if ( type == TYPE_SWAP )
	{
		surface = load_image("data/gfx/blocks/swap.png");
	}
	else if ( type == TYPE_FRAGILE )
	{
		surface = load_image("data/gfx/blocks/fragile.png");
	}
	else if ( type == TYPE_MOVING_BLOCK )
	{
		surface = load_image("data/gfx/blocks/moving_block.png");
	}
	else if ( type == TYPE_MOVING_SHADOW_BLOCK )
	{
		surface = load_image("data/gfx/blocks/moving_shadowblock.png");
	}
	else if ( type == TYPE_MOVING_SPIKES )
	{
		surface = load_image("data/gfx/blocks/moving_spikes.png");
	}
}

Block::~Block()
{
}

void Block::show()
{
	if ( check_collision(camera, box) == true || (stateID==STATE_LEVEL_EDITOR && check_collision(camera, box_base) == true))
	{
		SDL_Rect r={0,0,50,50};
		switch(i_type){
		case TYPE_CHECKPOINT:
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
			break;
		case TYPE_SWAP:
			if(m_t>0){
				r.x=(m_t%12)*50;
				m_t++;
				if(m_t>=24) m_t=0;
			}
			break;
		case TYPE_FRAGILE:
			if(m_t>=3){
				if(stateID==STATE_LEVEL_EDITOR) r.x=150;
				else return;
			}
			r.x=m_t*50;
			break;
		case TYPE_MOVING_BLOCK:
		case TYPE_MOVING_SHADOW_BLOCK:
		case TYPE_MOVING_SPIKES:
			if(stateID==STATE_LEVEL_EDITOR){
				SDL_Rect r1={50,0,50,50};
				apply_surface( box_base.x - camera.x, box_base.y - camera.y, surface, screen, &r1 ); 
			}
		}
		apply_surface( box.x - camera.x, box.y - camera.y, surface, screen, &r ); 
	}
}

void Block::reset(){
	m_t=0;
	m_t_save=0;
}

void Block::save_state(){
	m_t_save=m_t;
	m_x_save=box.x-box_base.x;
	m_y_save=box.y-box_base.y;
}

void Block::load_state(){
	m_t=m_t_save;
	switch(i_type){
	case TYPE_MOVING_BLOCK:
	case TYPE_MOVING_SHADOW_BLOCK:
		box.x=box_base.x+m_x_save;
		box.y=box_base.y+m_y_save;
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
		switch(i_type){
		case TYPE_BLOCK:
		case TYPE_MOVING_BLOCK:
			return 1;
		case TYPE_SHADOW_BLOCK:
		case TYPE_MOVING_SHADOW_BLOCK:
			if(obj!=NULL && obj->is_shadow()) return 1;
			break;
		case TYPE_FRAGILE:
			if(m_t<3) return 1;
			break;
		}
		break;
	case GameObjectProperty_ApplySpeedToPlayer:
		switch(i_type){
		case TYPE_MOVING_BLOCK:
		case TYPE_MOVING_SHADOW_BLOCK:
			{
				SDL_Rect r=obj->get_box();
				obj->set_position(r.x+m_dx,r.y+m_dy);
			}
			break;
		}
	case GameObjectProperty_IsSpikes:
		switch(i_type){
		case TYPE_SPIKES:
		case TYPE_MOVING_SPIKES:
			return 1;
		}
		break;
	default:
		break;
	}
	return 0;
}

void Block::GetEditorData(std::vector<std::pair<std::string,std::string> >& obj){
	switch(i_type){
	case TYPE_MOVING_BLOCK:
	case TYPE_MOVING_SHADOW_BLOCK:
	case TYPE_MOVING_SPIKES:
		{
			char s[64],s0[64];
			sprintf(s,"%d",(int)MovingPos.size());
			obj.push_back(pair<string,string>("MovingPosCount",s));
			for(unsigned int i=0;i<MovingPos.size();i++){
				sprintf(s0+1,"%d",i);
				sprintf(s,"%d",MovingPos[i].x);
				s0[0]='x';
				obj.push_back(pair<string,string>(s0,s));
				sprintf(s,"%d",MovingPos[i].y);
				s0[0]='y';
				obj.push_back(pair<string,string>(s0,s));
				sprintf(s,"%d",MovingPos[i].w);
				s0[0]='t';
				obj.push_back(pair<string,string>(s0,s));
			}
		}
		break;
	}
}

void Block::SetEditorData(std::map<std::string,std::string>& obj){
	switch(i_type){
	case TYPE_MOVING_BLOCK:
	case TYPE_MOVING_SHADOW_BLOCK:
	case TYPE_MOVING_SPIKES:
		{
			char s0[64];
			int m=0;
			m=atoi(obj["MovingPosCount"].c_str());
			MovingPos.clear();
			for(int i=0;i<m;i++){
				SDL_Rect r={0,0,0,0};
				sprintf(s0+1,"%d",i);
				s0[0]='x';
				r.x=atoi(obj[s0].c_str());
				s0[0]='y';
				r.y=atoi(obj[s0].c_str());
				s0[0]='t';
				r.w=atoi(obj[s0].c_str());
				MovingPos.push_back(r);
			}
		}
		break;
	}
}

void Block::move(){
	switch(i_type){
	case TYPE_MOVING_BLOCK:
	case TYPE_MOVING_SHADOW_BLOCK:
	case TYPE_MOVING_SPIKES:
		{
			int t=(++m_t);
			SDL_Rect r0={0,0,0,0},r1;
			m_dx=0;
			m_dy=0;
			for(unsigned int i=0;i<MovingPos.size();i++){
				r1.x=MovingPos[i].x;
				r1.y=MovingPos[i].y;
				r1.w=MovingPos[i].w;
				if(t>=0 && t<(int)r1.w){
					int new_x=box_base.x+(int)(float(r0.x)+(float(r1.x)-float(r0.x))*float(t)/float(r1.w)+0.5f);
					int new_y=box_base.y+(int)(float(r0.y)+(float(r1.y)-float(r0.y))*float(t)/float(r1.w)+0.5f);
					m_dx=new_x-box.x;
					m_dy=new_y-box.y;
					box.x=new_x;
					box.y=new_y;
					return;
				}
				t-=r1.w;
				r0.x=r1.x;
				r0.y=r1.y;
			}
			m_t=0;
			if(MovingPos.size()>0 && MovingPos.back().x==0 && MovingPos.back().y==0){
				m_dx=box_base.x-box.x;
				m_dy=box_base.y-box.y;
			}
			box.x=box_base.x;
			box.y=box_base.y;
		}
		break;
	}
}

SDL_Rect Block::get_box_base(){
	return box_base;
}
