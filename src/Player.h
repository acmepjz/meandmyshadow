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
#ifndef PLAYER_H
#define PLAYER_H

class Player
{
private:

	std::vector<bool> right_button;
	std::vector<bool> left_button;
	std::vector<bool> jump_button;

	std::vector<SDL_Rect> line;

	bool b_shadow_call;
	bool b_record;

protected:
	SDL_Rect box;

	int i_xVel, i_yVel;


	SDL_Surface * s_walking[4];
	SDL_Surface * s_standing[4];
	SDL_Surface * s_jumping[2];
	SDL_Surface * s_holding;
	SDL_Surface * s_line;

	Mix_Chunk * c_jump;
	Mix_Chunk * c_hit;

	bool b_inAir;
	bool b_jump;
	bool b_on_ground;
	bool b_can_move;
	bool b_dead;


	int i_frame;
	int i_animation;
	int i_direction;
	int i_state;
	int i_jump_time;
	bool b_shadow;

public:

	int i_fx, i_fy;
	bool b_reset;

	bool b_holding_other;

	Player();
	~Player();

	void set_position( int x, int y );

	void handle_input(class Shadow * shadow);
	void move(std::vector<GameObject*> &LevelObjects);
	void jump();
	void show();
	void shadow_set_state();
	void state_reset();
	void other_check(class Player * other);
	void set_mycamera();
	void reset();
	SDL_Rect get_box();

	void shadow_give_state(class Shadow * shadow);
};

class Shadow : public Player
{
protected:

	std::vector<bool> right_button;
	std::vector<bool> left_button;
	std::vector<bool> jump_button;

	bool b_called;

	friend class Player;

public:

	Shadow();

	void move_logic();
	void me_call();
	void reset();
};

#endif
