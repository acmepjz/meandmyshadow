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
#include "Player.h"
#include "Game.h"
#include "Functions.h"
#include "FileManager.h"
#include "Globals.h"
#include "Objects.h"
#include <iostream>
#include <SDL/SDL_mixer.h>
#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
using namespace std;

Player::Player(Game* objParent):i_xVel_base(0),i_yVel_base(0),m_objParent(objParent){
	box.x = 0;
	box.y = 0;
	box.w = 21;
	box.h = 40;

	i_xVel = 0; //PLAYER_X_SPEED;
	i_yVel = 0;

	i_fx = 0;
	i_fy = 0;

	c_jump = Mix_LoadWAV((getDataPath()+"sfx/jump.wav").c_str());
	c_hit = Mix_LoadWAV((getDataPath()+"sfx/hit.wav").c_str());
	c_save = Mix_LoadWAV((getDataPath()+"sfx/checkpoint.wav").c_str());
	c_swap = Mix_LoadWAV((getDataPath()+"sfx/swap.wav").c_str());
	c_toggle = Mix_LoadWAV((getDataPath()+"sfx/toggle.wav").c_str());

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

Player::~Player(){
	Mix_FreeChunk(c_jump);
	Mix_FreeChunk(c_hit);
	Mix_FreeChunk(c_save);
	Mix_FreeChunk(c_swap);
	Mix_FreeChunk(c_toggle);
}

void Player::handleInput(class Shadow* shadow){
	Uint8 *lpKeyState=SDL_GetKeyState(NULL);
	i_xVel=0;
	if(lpKeyState[SDLK_RIGHT]){
		i_xVel += 7;
	}
	if(lpKeyState[SDLK_LEFT]){
		i_xVel -= 7;
	}

	if(event.type==SDL_KEYUP){
		bDownKeyPressed=false;
	}

	if(event.type==SDL_KEYDOWN){
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
		case SDLK_F2:
			if(!(b_dead || shadow->b_dead) && stateID == STATE_LEVEL_EDITOR){
				if(m_objParent) m_objParent->save_state();
			}
			break;
		case SDLK_F3:
			if(m_objParent) m_objParent->load_state();
			break;
		case SDLK_F4:
			if(!(b_dead || shadow->b_dead) && stateID == STATE_LEVEL_EDITOR){
				swapState(shadow);
			}
			break;
		case SDLK_F12:
			if(stateID == STATE_LEVEL_EDITOR){
				die();
				shadow->die();
			}
		//end
		default:
			break;
		}
	}

}

void Player::setPosition(int x,int y){
	box.x = x;
	box.y = y;
}

void Player::move(vector<GameObject*> &LevelObjects){
	GameObject *objCheckPoint=NULL,*objSwap=NULL;
	bool bCanTeleport=true;
	m_objCurrentStand=NULL;

	if(b_dead==false){
		//Add gravity
		if(b_inAir==true){
			i_yVel += 1;	
			if ( i_yVel > 13 )
			{
				i_yVel = 13;
			}
		}

		//~ bool costumy = false;
		//~ bool costumx = false;

		if(b_can_move==true){
			//Test x
			//SDL_Rect testbox; testbox.x = box.x; testbox.y = box.y;
			//testbox.x += i_xVel;
			//testbox.w = box.w;
			//testbox.h = box.h;
			if ( i_xVel > 0 ) { 
				i_direction = 0;
				b_on_ground = false;
			}else if ( i_xVel < 0 ) { 
				i_direction = 1;
				b_on_ground = false;
			}else if ( i_xVel == 0 ) { b_on_ground = true; }

			box.x += i_xVel;

			for(unsigned int o=0; o<LevelObjects.size(); o++){
				if(LevelObjects[o]->queryProperties(GameObjectProperty_PlayerCanWalkOn,this)){
					SDL_Rect r=LevelObjects[o]->getBox();
					if(checkCollision(box,r)){
						SDL_Rect v=LevelObjects[o]->getBox(BoxType_Delta);  //???
						if(box.x + box.w/2 <= r.x + r.w/2 ){
							if(i_xVel+i_xVel_base>v.x){ //???
								if(box.x > r.x - box.w) box.x = r.x - box.w;	
								//~ costumx = true;
								//if(!b_shadow) printf("left ");
							}
						}else{
							if(i_xVel+i_xVel_base<v.x){ //???
								if(box.x < r.x + r.w) box.x = r.x + r.w;
								//~ costumx = true;
								//if(!b_shadow) printf("right ");
							}
						}

					}
				}
			}	
			
			//////////////////////
			//if ( !costumx )
			//{	box.x += i_xVel;}
		}

		box.y += i_yVel;

		GameObject *objLastStand=NULL;

		b_inAir = true; b_can_move = true; //???

		for(unsigned int o=0; o<LevelObjects.size(); o++){
			//Test y
			if( /*objLastStand==NULL &&*/ LevelObjects[o]->queryProperties(GameObjectProperty_PlayerCanWalkOn,this)){
				SDL_Rect r=LevelObjects[o]->getBox();
				if(checkCollision(r,box)==true){ //TODO:fix some bug
					SDL_Rect v=LevelObjects[o]->getBox(BoxType_Delta);
					//box.y -= i_yVel; //???

					if(box.y+box.h/2 <= r.y + r.h/2 ){
						if(i_yVel >= v.y || i_yVel>=0){
							b_inAir = false;
							box.y = r.y - box.h;
							i_yVel = 1; //???
							//if(!b_shadow) printf("over ");
							objLastStand=LevelObjects[o];
							objLastStand->onEvent(GameObjectEvent_PlayerIsOn);
						}
					}else{
						if(i_yVel <= v.y + 1){ 
							i_yVel = v.y>0?v.y:0; 
							//if(!b_shadow) printf("under ");
							if(box.y < r.y + r.h) box.y = r.y + r.h;
						}
					}
				}

				//else {b_inAir = true; b_can_move = true; }
			}

			//save game?
			if ( LevelObjects[o]->type == TYPE_CHECKPOINT && checkCollision( box, LevelObjects[o]->getBox() ))
			{
				if(!b_shadow && m_objParent!=NULL) m_objParent->GameTipIndex=TYPE_CHECKPOINT;
				objCheckPoint=LevelObjects[o];
			}

			//can swap?
			if ( LevelObjects[o]->type == TYPE_SWAP && checkCollision( box, LevelObjects[o]->getBox() ))
			{
				if(!b_shadow && m_objParent!=NULL) m_objParent->GameTipIndex=TYPE_SWAP;
				objSwap=LevelObjects[o];
			}

			if(LevelObjects[o]->type==TYPE_EXIT && stateID!=STATE_LEVEL_EDITOR && checkCollision(box,LevelObjects[o]->getBox())){
				levels.nextLevel();

				if(levels.getLevel() < levels.getLevelCount()){
					levels.setLocked(levels.getLevel());
					setNextState(STATE_GAME);
				}else{
					msgBox("You have finished the level!",MsgBoxOKOnly,"Congratulations");
					setNextState(STATE_MENU);
				}
			}

			//teleport?
			if ( LevelObjects[o]->type == TYPE_PORTAL && checkCollision( box, LevelObjects[o]->getBox() ) )
			{
				if(!b_shadow && m_objParent!=NULL) m_objParent->GameTipIndex=TYPE_PORTAL;
				if(bCanTeleport && (bDownKeyPressed || (LevelObjects[o]->queryProperties(GameObjectProperty_Flags,this)&1))){
					bCanTeleport=false;
					if(bDownKeyPressed || LevelObjects[o]!=m_objLastTeleport){
						bDownKeyPressed=false;
						for ( unsigned int oo = o+1; ; ){
							if(oo>=LevelObjects.size()) oo-=(int)LevelObjects.size();
							if(oo==o) break;
							//---
							if(LevelObjects[oo]->type == TYPE_PORTAL){
								if((dynamic_cast<Block*>(LevelObjects[o]))->id == (dynamic_cast<Block*>(LevelObjects[oo]))->id){
									LevelObjects[o]->onEvent(GameObjectEvent_OnToggle);
									m_objLastTeleport=LevelObjects[oo];
									SDL_Rect r=LevelObjects[oo]->getBox();
									box.x=r.x+5;
									box.y=r.y+2;
									Mix_PlayChannel(-1, c_swap, 0);
									break;
								}
							}
							//---
							oo++;
						}
					}
				}
			}

			//press switch?
			if ( LevelObjects[o]->type == TYPE_SWITCH && checkCollision( box, LevelObjects[o]->getBox() ) )
			{
				if(!b_shadow && m_objParent!=NULL) m_objParent->GameTipIndex=TYPE_SWITCH;
				if(bDownKeyPressed){
					LevelObjects[o]->playAnimation(1);
					Mix_PlayChannel(-1,c_toggle,0);
					if(m_objParent!=NULL){
						m_objParent->BroadcastObjectEvent(0x10000 | (LevelObjects[o]->queryProperties(GameObjectProperty_Flags,this)&3),
							-1,(dynamic_cast<Block*>(LevelObjects[o]))->id.c_str());
					}
				}
			}

			//die?
			if ( LevelObjects[o]->queryProperties(GameObjectProperty_IsSpikes,this) )
			{
				SDL_Rect r=LevelObjects[o]->getBox();
				//TODO:pixel-accuracy hit test
				r.x+=2;
				r.y+=2;
				r.w-=4;
				r.h-=4;
				//
				if ( checkCollision ( box, r ) ) die();
			}
		}

		if(box.y>LEVEL_HEIGHT) die();

		m_objCurrentStand=objLastStand;
		if(objLastStand!=m_objLastStand){
			m_objLastStand=objLastStand;
			if(objLastStand) objLastStand->onEvent(GameObjectEvent_PlayerWalkOn);
		}

		if(bCanTeleport) m_objLastTeleport=NULL;

		//check checkpoint
		if(m_objParent!=NULL && bDownKeyPressed && objCheckPoint!=NULL){
			if(m_objParent->save_state()) m_objParent->objLastCheckPoint=objCheckPoint;
		}
		if(objSwap!=NULL && bDownKeyPressed && m_objParent!=NULL){
			if(b_shadow){
				if(!(b_dead || m_objParent->player.b_dead)){
					m_objParent->player.swapState(this);
					objSwap->playAnimation(1);
				}
			}else{
				if(!(b_dead || m_objParent->shadow.b_dead)){
					swapState(&m_objParent->shadow);
					objSwap->playAnimation(1);
				}
			}
		}
	}

	bDownKeyPressed=false;
	i_xVel_base=0;
	i_yVel_base=0;
}


void Player::jump(){
	if(b_dead==true){
		b_jump=false;
	}

	if(b_jump==true && b_inAir==false){
		i_yVel = -13;
		b_inAir = true;
		b_jump = false;
		i_jump_time++;
		Mix_PlayChannel(-1, c_jump, 0 );
	}
}

void Player::show(){
	if(b_shadow==false && b_record==true){
		line.push_back(SDL_Rect());
		line[line.size() - 1].x = box.x + 11;
		line[line.size() - 1].y = box.y + 20;

		for(int l=0; l<(signed)line.size(); l++){
			Appearance.drawState("line", screen, line[l].x - camera.x, line[l].y - camera.y,NULL );
		}
	}

	if(b_dead==false){
		i_frame++;
		if(i_frame>=5){
			i_animation++;
			if(i_animation>=2){
				i_animation = 0;
			}

			i_frame = 0;
		}


		if(b_inAir==false){
			if(i_direction==0){
				if(b_on_ground==false){
					char state[64];
					sprintf(state,"%s%d","walkright",i_animation);
					Appearance.drawState(state, screen, box.x-camera.x, box.y-camera.y, NULL);
					//applySurface(box.x-camera.x, box.y-camera.y, s_walking[0+i_animation], screen, NULL);
				}else{ 
					if(b_holding_other==true){
						Appearance.drawState("holding", screen, box.x-camera.x, box.y-camera.y, NULL);
						//applySurface(box.x - camera.x, box.y - camera.y, s_holding, screen, NULL);
					}else{ 
						Appearance.drawState("standright", screen, box.x-camera.x, box.y-camera.y, NULL);
						//applySurface(box.x - camera.x, box.y - camera.y, s_standing[0+i_animation], screen, NULL); 
					}
				}
			}else if(i_direction==1){
				if(b_on_ground==false){
					char state[64];
					sprintf(state,"%s%d","walkleft",i_animation);
					Appearance.drawState(state, screen, box.x-camera.x, box.y-camera.y, NULL);
					//applySurface( box.x - camera.x, box.y - camera.y, s_walking[2+i_animation], screen, NULL );
				}else{ 
					if(b_holding_other==true){
						Appearance.drawState("holding", screen, box.x-camera.x, box.y-camera.y, NULL);
						//applySurface( box.x - camera.x, box.y - camera.y, s_holding, screen, NULL);
					}else {
						Appearance.drawState("standleft", screen, box.x-camera.x, box.y-camera.y, NULL);
						//applySurface( box.x - camera.x, box.y - camera.y, s_standing[2+i_animation], screen, NULL ); 
					} 
				}
			}
		}else{
			if(i_direction==0){
				Appearance.drawState("jumpright", screen, box.x-camera.x, box.y-camera.y, NULL);
				//applySurface( box.x - camera.x, box.y - camera.y, s_jumping[0], screen, NULL);
			}

			if(i_direction==1){
				Appearance.drawState("jumpleft", screen, box.x-camera.x, box.y-camera.y, NULL);
				//applySurface( box.x - camera.x, box.y - camera.y, s_jumping[1], screen, NULL);
			}
		}
	}

}

void Player::shadowSetState(){
	if(b_record) {
		int nCurrentKey=0;

		//if ( i_direction == 0 && !b_on_ground ) nCurrentKey |= PlayerButtonRight;
		//if ( i_direction == 1 && !b_on_ground ) nCurrentKey |= PlayerButtonLeft;

		if ( i_xVel>0 ) nCurrentKey |= PlayerButtonRight;
		if ( i_xVel<0 ) nCurrentKey |= PlayerButtonLeft;

		if ( b_jump ) nCurrentKey |= PlayerButtonJump;
		
		if ( bDownKeyPressed ) nCurrentKey |= PlayerButtonDown;

		player_button.push_back(nCurrentKey);

		i_state++;
	}
}

void Player::shadowGiveState(Shadow* shadow){
	if(b_shadow_call==true){
		//Zbrisi vse vectore shadow
		shadow->player_button.clear();

		//Napisi nove
		for ( unsigned int s = 0; s < player_button.size(); s++ )
		{
			shadow->player_button.push_back(player_button[s]);
		}

		//Resetiraj state
		stateReset();
		shadow->stateReset();

		//brisi vse vectore svoje
		player_button.clear();
		line.clear();

		b_shadow_call = false;
		shadow->me_call();
	}
}

void Player::stateReset(){
	i_state = 0;
}

void Player::otherCheck(class Player* other){
	if ( !b_dead ){
		if(m_objCurrentStand!=NULL){
			SDL_Rect v=m_objCurrentStand->getBox(BoxType_Velocity);
			i_xVel_base=v.x;
			i_yVel_base=v.y;
			box.x+=v.x;
			box.y+=v.y;
		}

		if(!other->b_dead){
			SDL_Rect box_shadow = other->getBox();

			if ( checkCollision(box, box_shadow) == true )
			{
				if ( box.y + box.h <= box_shadow.y + 13 && !other->b_inAir )
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
	m_objCurrentStand=NULL;
}

SDL_Rect Player::getBox(){
	return box;
}

void Player::setMyCamera(){
	if(b_dead || stateID==STATE_LEVEL_EDITOR) return;

	if ( box.x > camera.x + 450 )
	{
		camera.x += (box.x - camera.x - 400)>>4;//+= 7;
		if(box.x < camera.x + 450) camera.x=box.x-450;
	}

	if ( box.x < camera.x + 350 )
	{
		camera.x += (box.x - camera.x - 400)>>4;//-= 7;
		if(box.x > camera.x + 350) camera.x=box.x-350;
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
		camera.y += (box.y - camera.y - 300)>>4;//+= 7;
		if(box.y < camera.y + 350) camera.y=box.y-350;
	}

	if ( box.y < camera.y + 250 )
	{
		camera.y += (box.y - camera.y - 300)>>4;//-= 7;
		if(box.y > camera.y + 250) camera.y=box.y-250;
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

void Player::reset(){
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

	m_objCurrentStand=NULL;

	line.clear();
	player_button.clear();

	//new
	i_xVel_saved = 0x80000000;
}

//new

void Player::saveState(){
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
		if(!b_shadow) Mix_PlayChannel(-1, c_save, 0);
	}
}

void Player::loadState(){
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
	stateReset();
	//???
	line.clear();
	player_button.clear();
}

void Player::swapState(Player* other){
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
	stateReset();
	//???
	line.clear();
	player_button.clear();
	//????????
	other->stateReset();
	//play sound
	Mix_PlayChannel(-1, c_swap, 0);
}

bool Player::canSaveState(){
	return !b_dead;
}

bool Player::canLoadState(){
	return i_xVel_saved != 0x80000000;
}

void Player::die(){
	if(!b_dead){
		b_dead = true;
		Mix_PlayChannel(-1, c_hit, 0);
	}
}

//end