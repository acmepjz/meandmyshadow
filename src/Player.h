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

#include "ThemeManager.h"
#include <vector>
#include <SDL/SDL_mixer.h>
#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>

class GameObject;
class Game;

const int PlayerButtonRight=0x01;
const int PlayerButtonLeft=0x02;
const int PlayerButtonJump=0x04;
const int PlayerButtonDown=0x08;

class Player{
protected:

	std::vector<int> playerButton;

private:
	std::vector<SDL_Rect> line;

	bool shadowCall;
	bool record;
	//new
	SDL_Rect boxSaved;
	bool inAirSaved;
	bool isJumpSaved;
	bool onGroundSaved;
	bool canMoveSaved;
	bool holdingOtherSaved;
	int xVelSaved, yVelSaved;
	//end
	
protected:
	SDL_Rect box;

	int xVel, yVel;
	int xVelBase, yVelBase;

	Mix_Chunk* jumpSound;
	Mix_Chunk* hitSound;
	Mix_Chunk* saveSound;
	Mix_Chunk* swapSound;
	Mix_Chunk* toggleSound;

	bool inAir;
	bool isJump;
	bool onGround;
	bool canMove;
	bool dead;


	int frame;
	int animation;
	int direction;
	int state;
	int jumpTime;
	bool shadow;

	friend class Game;
	Game* objParent;

	//new
	bool downKeyPressed;
	GameObject* objCurrentStand; //always be valid pointer
	GameObject* objLastStand; //warning: weak reference only
	GameObject* objLastTeleport; //warning: weak reference only
	//end

public:

	int fx, fy;
	ThemeCharacterInstance appearance;
	bool holdingOther;

	Player(Game* objParent);
	~Player();

	void setPosition(int x,int y);

	void handleInput(class Shadow* shadow);
	void move(std::vector<GameObject*> &levelObjects);
	void jump();
	void show();
	void shadowSetState();
	virtual void stateReset();
	void otherCheck(class Player* other);
	void setMyCamera();
	void reset();
	SDL_Rect getBox();

	void shadowGiveState(class Shadow* shadow);
	//new
	virtual void saveState();
	virtual void loadState();
	virtual bool canSaveState();
	virtual bool canLoadState();
	void swapState(Player * other);
	inline bool isShadow(){return shadow;}
	void die();
	//end
};

#endif
