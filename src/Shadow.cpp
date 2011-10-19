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
#include "Functions.h"
#include "FileManager.h"
#include "Game.h"
#include "Player.h"
#include "Shadow.h"
#include <vector>
#include <iostream>
using namespace std;

Shadow::Shadow(Game* objParent):Player(objParent){
	b_called = false;
	b_shadow = true;

	i_xVel = 0;
}

void Shadow::move_logic(){
	if(b_called && i_state < (signed)player_button.size()){
		int nCurrentKey=player_button[i_state];

		i_xVel = 0;
		if ( nCurrentKey & PlayerButtonRight ) i_xVel = 7;
		if ( nCurrentKey & PlayerButtonLeft ) i_xVel = -7;

		if ( (nCurrentKey & PlayerButtonJump) && !b_inAir ) b_jump = true;
		else b_jump = false;

		if ( nCurrentKey & PlayerButtonDown ) bDownKeyPressed = true;
		else bDownKeyPressed = false;

		i_state++;

	}else{
		b_called = false;
		i_state = 0;
		i_xVel = 0;
	}
}

void Shadow::stateReset(){
	i_state = 0;
	b_called = false;
}

void Shadow::me_call(){
	b_called = true;
}

void Shadow::reset(){
	box.x = i_fx;
	box.y = i_fy;

	i_xVel = 0;
	i_yVel = 0;

	b_inAir = true;
	b_jump = false;
	b_on_ground = true;
	b_can_move = true;
	b_holding_other = false;
	b_dead = false;

	i_frame = 0;
	i_animation = 0;
	i_direction = 0;

	i_state = 0;
	
	player_button.clear();
}

void Shadow::loadState(){
	Player::loadState();
	b_called = false;
	player_button.clear();
}