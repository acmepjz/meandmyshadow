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
