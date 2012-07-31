/*
 * Copyright (C) 2012 Me and My Shadow
 *
 * This file is part of Me and My Shadow.
 *
 * Me and My Shadow is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Me and My Shadow is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Me and My Shadow.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "StatisticsManager.h"
#include "FileManager.h"
#include "TreeStorageNode.h"
#include "POASerializer.h"
#include "Functions.h"
#include "LevelPackManager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>

using namespace std;

StatisticsManager statsMgr;

static const int achievementDisplayTime=100;
static const int achievementIntervalTime=120;

//internal struct for achievement info
struct AchievementInfo{
	//achievement id for save to statistics file
	const char* id;
	//achievement name for display
	const char* name;
	//achievement image. NULL for no image. will be loaded at getDataPath()+imageFile
	const char* imageFile;
	//SDL_Surface of achievement image.
	SDL_Surface* imageSurface;
	//image offset and size.
	SDL_Rect r;
	//achievement description. supports multi-line text
	const char* description;
};

static AchievementInfo achievementList[]={
	{"newbie",__("Newbie"),"themes/Cloudscape/player.png",NULL,{0,0,23,40},__("Congratulations, you completed one level!")},
	{"experienced",__("Experienced player"),"themes/Cloudscape/player.png",NULL,{0,0,23,40},__("Completed 50 levels.")},
	{"goodjob",__("Good job!"),"gfx/medals.png",NULL,{60,0,30,30},__("Get your first gold medal.")},
	{"expert",__("Expert"),"gfx/medals.png",NULL,{60,0,30,30},__("Earned 50 gold medal.")},

	{"tutorial",__("Graduate"),"gfx/medals.png",NULL,{60,0,30,30},__("Complete the tutorial level pack.")},
	{"tutorialGold",__("Outstanding graduate"),"gfx/medals.png",NULL,{60,0,30,30},__("Complete the tutorial level pack with all levels gold medal.")},

	{"addicted",__("Addicted"),"themes/Cloudscape/player.png",NULL,{0,0,23,40},__("Played Me and My Shadow for more than 2 hours.")},
	{"loyalFan",__("Me and My Shadow loyal fan"),"themes/Cloudscape/player.png",NULL,{0,0,23,40},__("Played Me and My Shadow for more than 24 hours.")},

	{"constructor",__("Constructor"),"gfx/gui.png",NULL,{112,16,16,16},__("Use the level editor for more than 2 hours.")},
	{"constructor2",__("The creator"),"gfx/gui.png",NULL,{112,16,16,16},__("Use the level editor for more than 24 hours.")},

	{"create1",__("Look, cute level!"),"gfx/gui.png",NULL,{112,16,16,16},__("Created your first level.")},
	{"create50",__("The level museum"),"gfx/gui.png",NULL,{112,16,16,16},__("Created 50 levels.")},

	{"frog",__("Frog"),"themes/Cloudscape/player.png",NULL,{0,0,23,40},__("Jump for 1000 times.")},

	{"die1",__("Be careful!"),"themes/Cloudscape/deathright.png",NULL,{0,14,23,40},__("The first death.")},
	{"die50",__("It doesn't matter..."),"themes/Cloudscape/deathright.png",NULL,{0,14,23,40},__("Died for 50 times.")},
	{"die1000",__("Expert of trial and error"),"themes/Cloudscape/deathright.png",NULL,{0,14,23,40},__("Died for 1000 times.")},

	{"squash1",__("Keep an eye for moving walls!"),"themes/Cloudscape/deathright.png",NULL,{0,14,23,40},__("First time being squashed.")},
	{"suqash50",__("Potato masher"),"themes/Cloudscape/deathright.png",NULL,{0,14,23,40},__("Squashed for 50 times.")},

	{"doubleKill",__("Double kill"),"themes/Cloudscape/deathright.png",NULL,{0,14,23,40},__("Make both player and shadow die.")},

	//test only
	{"programmer",__("Programmer"),"gfx/gui.png",NULL,{112,16,16,16},__("Played the development version of Me and My Shadow.")},

	//end of achievements
	{NULL,NULL,NULL,NULL,{0,0,0,0},NULL}
};

static map<string,AchievementInfo*> avaliableAchievements;

//================================================================

StatisticsManager::StatisticsManager(){
	bmDropShadow=NULL;
	bmAchievement=NULL;

	startTime=time(NULL);

	clear();
}

StatisticsManager::~StatisticsManager(){
	if(bmAchievement){
		SDL_FreeSurface(bmAchievement);
		bmAchievement=NULL;
	}
}

void StatisticsManager::clear(){
	playerTravelingDistance=shadowTravelingDistance=0.0f;
	playerJumps=shadowJumps
		=playerDies=shadowDies
		=playerSquashed=shadowSquashed
		=completedLevels=silverLevels=goldLevels
		=recordTimes=switchTimes=swapTimes
		=playTime=levelEditTime
		=createdLevels=0;

	achievements.clear();
	queuedAchievements.clear();

	achievementTime=0;
	currentAchievement=0;
	if(bmAchievement){
		SDL_FreeSurface(bmAchievement);
		bmAchievement=NULL;
	}
}

#define LOAD_STATS(var,func) { \
	vector<string> &v=node.attributes[ #var ]; \
	if(!v.empty() && !v[0].empty()) \
		var=func(v[0].c_str()); \
}

void StatisticsManager::loadFile(const std::string& fileName){
	clear();

	ifstream file(fileName.c_str());
	if(!file) return;

	TreeStorageNode node;
	POASerializer serializer;
	if(!serializer.readNode(file,&node,true)) return;

	//load statistics
	LOAD_STATS(playerTravelingDistance,atof);
	LOAD_STATS(shadowTravelingDistance,atof);
	LOAD_STATS(playerJumps,atoi);
	LOAD_STATS(shadowJumps,atoi);
	LOAD_STATS(playerDies,atoi);
	LOAD_STATS(shadowDies,atoi);
	LOAD_STATS(playerSquashed,atoi);
	LOAD_STATS(shadowSquashed,atoi);
	LOAD_STATS(recordTimes,atoi);
	LOAD_STATS(switchTimes,atoi);
	LOAD_STATS(swapTimes,atoi);
	LOAD_STATS(playTime,atoi);
	LOAD_STATS(levelEditTime,atoi);
	LOAD_STATS(createdLevels,atoi);

	//load achievements.
	{
		vector<string> &v=node.attributes["achievements"];
		for(unsigned int i=0;i<v.size();i++){
			map<string,AchievementInfo*>::iterator it=avaliableAchievements.find(v[i]);
			if(it!=avaliableAchievements.end()){
				achievements[it->first]=it->second;
			}
		}
	}
}

//Call when level edit is start
void StatisticsManager::startLevelEdit(){
	levelEditStartTime=time(NULL);
}

//Call when level edit is end
void StatisticsManager::endLevelEdit(){
	levelEditTime+=time(NULL)-levelEditStartTime;
}

//update in-game time
void StatisticsManager::updatePlayTime(){
	time_t endTime=time(NULL);
	playTime+=endTime-startTime;
	startTime=endTime;
}

#define SAVE_STATS(var,pattern) { \
	sprintf(s,pattern,var); \
	node.attributes[ #var ].push_back(s); \
}

void StatisticsManager::saveFile(const std::string& fileName){
	char s[64];

	//update in-game time
	updatePlayTime();

	ofstream file(fileName.c_str());
	if(!file) return;

	TreeStorageNode node;

	//save statistics
	SAVE_STATS(playerTravelingDistance,"%.2f");
	SAVE_STATS(shadowTravelingDistance,"%.2f");
	SAVE_STATS(playerJumps,"%d");
	SAVE_STATS(shadowJumps,"%d");
	SAVE_STATS(playerDies,"%d");
	SAVE_STATS(shadowDies,"%d");
	SAVE_STATS(playerSquashed,"%d");
	SAVE_STATS(shadowSquashed,"%d");
	SAVE_STATS(recordTimes,"%d");
	SAVE_STATS(switchTimes,"%d");
	SAVE_STATS(swapTimes,"%d");
	SAVE_STATS(playTime,"%d");
	SAVE_STATS(levelEditTime,"%d");
	SAVE_STATS(createdLevels,"%d");

	//save achievements.
	{
		vector<string>& v=node.attributes["achievements"];

		for(map<string,AchievementInfo*>::iterator it=achievements.begin();it!=achievements.end();++it){
			v.push_back(it->first);
		}
	}

	POASerializer serializer;
	serializer.writeNode(&node,file,true,true);
}

void StatisticsManager::loadPicture(){
	//Load drop shadow picture
	bmDropShadow=loadImage(getDataPath()+"gfx/dropshadow.png");
}

void StatisticsManager::registerAchievements(){
	if(!avaliableAchievements.empty()) return;

	for(int i=0;achievementList[i].id!=NULL;i++){
		avaliableAchievements[achievementList[i].id]=&achievementList[i];
		if(achievementList[i].imageFile!=NULL){
			achievementList[i].imageSurface=loadImage(getDataPath()+achievementList[i].imageFile);
		}
	}
}

void StatisticsManager::render(){
	//debug
	if(achievementTime==0){
		if(SDL_GetKeyState(NULL)[SDLK_1]) newAchievement("hello",false);
		if(SDL_GetKeyState(NULL)[SDLK_2]) newAchievement("123",false);
	} 

	if(achievementTime==0 && bmAchievement==NULL && currentAchievement<(int)queuedAchievements.size()){
		//create surface
		bmAchievement=createAchievementSurface(queuedAchievements[currentAchievement++]);

		//check if queue is empty
		if(currentAchievement>=(int)queuedAchievements.size()){
			queuedAchievements.clear();
			currentAchievement=0;
		}

		//play a sound
		if(getSettings()->getBoolValue("sound")){
			Mix_PlayChannel(-1,achievementSound,0);
		}
	}

	//check if we need to display achievements
	if(bmAchievement){
		achievementTime++;
		if(achievementTime<=0){
			return;
		}else if(achievementTime<=5){
			drawAchievement(achievementTime);
		}else if(achievementTime<=achievementDisplayTime-5){
			drawAchievement(5);
		}else if(achievementTime<achievementDisplayTime){
			drawAchievement(achievementDisplayTime-achievementTime);
		}else if(achievementTime>=achievementIntervalTime){
			if(bmAchievement){
				SDL_FreeSurface(bmAchievement);
				bmAchievement=NULL;
			}
			achievementTime=0;
		}
	}
}

void StatisticsManager::newAchievement(const std::string& id,bool save){
	//check avaliable achievements
	map<string,AchievementInfo*>::iterator it=avaliableAchievements.find(id);
	if(it==avaliableAchievements.end()) return;

	//check if already have this achievement
	if(save){
		map<string,AchievementInfo*>::iterator it2=achievements.find(id);
		if(it2!=achievements.end()) return;
		achievements[id]=it->second;
	}

	//add it to queue
	queuedAchievements.push_back(it->second);
}

SDL_Surface* StatisticsManager::createAchievementSurface(AchievementInfo* info,SDL_Surface* surface,SDL_Rect* rect,bool showTip){
	if(info==NULL || info->id==NULL) return NULL;

	//prepare text
	SDL_Surface *title0=NULL,*title1=NULL;
	vector<SDL_Surface*> descSurfaces;
	SDL_Color fg={0,0,0};
	int fontHeight=TTF_FontLineSkip(fontText);

	if(showTip) title0=TTF_RenderUTF8_Blended(fontText,_("New achievement:"),fg);
	title1=TTF_RenderUTF8_Blended(fontGUISmall,_(info->name),fg);

	if(info->description!=NULL){
		string description=_(info->description);
		string::size_type lps=0,lpe;
		for(;;){
			lpe=description.find('\n',lps);
			if(lpe==string::npos){
				descSurfaces.push_back(TTF_RenderUTF8_Blended(fontText,(description.substr(lps)+' ').c_str(),fg));
				break;
			}else{
				descSurfaces.push_back(TTF_RenderUTF8_Blended(fontText,(description.substr(lps,lpe-lps)+' ').c_str(),fg));
				lps=lpe+1;
			}
		}
	}

	//calculate the size
	int w=0,h=0,w1=8,h1=0;

	if(title0!=NULL){
		if(title0->w>w) w=title0->w;
		h1+=title0->h;
	}
	if(title1!=NULL){
		if(title1->w>w) w=title1->w;
		h1+=title1->h;
	}
	if(info->imageSurface!=NULL){
		w1+=info->r.w+8;
		w+=info->r.w+8;
		if(info->r.h>h1) h1=info->r.h;
	}
	h=h1+8;
	for(unsigned int i=0;i<descSurfaces.size();i++){
		if(descSurfaces[i]!=NULL){
			if(descSurfaces[i]->w>w) w=descSurfaces[i]->w;
		}
	}
	h+=descSurfaces.size()*fontHeight;
	w+=16;
	h+=16;

	//check if size is specified
	int left=0,top=0;
	if(rect!=NULL){
		if(surface!=NULL){
			left=rect->x;
			top=rect->y;
		}
		if(rect->w>0) w=rect->w;
		else rect->w=w;
		rect->h=h;
	}

	//create surface if necessary
	if(surface==NULL){
		surface=SDL_CreateRGBSurface(SDL_HWSURFACE,w,h,
			screen->format->BitsPerPixel,screen->format->Rmask,screen->format->Gmask,screen->format->Bmask,0);
	}

	//draw background
	drawGUIBox(left,top,w,h,surface,0xFFFFFFFFU);

	//draw picture
	if(info->imageSurface!=NULL){
		SDL_Rect r={left+8,top+8+(h1-info->r.h)/2,0,0};
		SDL_BlitSurface(info->imageSurface,&info->r,surface,&r);
	}

	//draw text
	h=8;
	if(title0!=NULL){
		SDL_Rect r={left+w1,top+h,0,0};
		SDL_BlitSurface(title0,NULL,surface,&r);
		h+=title0->h;
	}
	if(title1!=NULL){
		SDL_Rect r={left+w1,top+h,0,0};
		SDL_BlitSurface(title1,NULL,surface,&r);
	}
	h=h1+16;
	for(unsigned int i=0;i<descSurfaces.size();i++){
		if(descSurfaces[i]!=NULL){
			SDL_Rect r={left+8,top+h+i*fontHeight,0,0};
			SDL_BlitSurface(descSurfaces[i],NULL,surface,&r);
		}
	}

	//clean up
	if(title0) SDL_FreeSurface(title0);
	if(title1) SDL_FreeSurface(title1);
	for(unsigned int i=0;i<descSurfaces.size();i++){
		if(descSurfaces[i]!=NULL){
			SDL_FreeSurface(descSurfaces[i]);
		}
	}

	//over
	return surface;
}

void StatisticsManager::drawAchievement(int alpha){
	if(bmAchievement==NULL) return;
	if(alpha<=0) return;
	if(alpha>5) alpha=5;

	SDL_Rect r={screen->w-32-bmAchievement->w,32,
		bmAchievement->w,bmAchievement->h};

	//draw the surface
	SDL_SetAlpha(bmAchievement,SDL_SRCALPHA,alpha*40);
	SDL_BlitSurface(bmAchievement,NULL,screen,&r);

	//draw drop shadow - corner
	{
		int w1=r.w/2,w2=r.w-w1,h1=r.h/2,h2=r.h-h1;
		if(w1>16) w1=16;
		if(w2>16) w2=16;
		if(h1>16) h1=16;
		if(h2>16) h2=16;

		int x=(5-alpha)*64;
		//top-left
		SDL_Rect r1={x,0,w1+16,h1+16},r2={r.x-16,r.y-16,0,0};
		SDL_BlitSurface(bmDropShadow,&r1,screen,&r2);
		//top-right
		r1.x=x+48-w2;r1.w=w2+16;r2.x=r.x+r.w-w2;
		SDL_BlitSurface(bmDropShadow,&r1,screen,&r2);
		//bottom-right
		r1.y=48-h2;r1.h=h2+16;r2.y=r.y+r.h-h2;
		SDL_BlitSurface(bmDropShadow,&r1,screen,&r2);
		//bottom-left
		r1.x=x;r1.w=w1+16;r2.x=r.x-16;
		SDL_BlitSurface(bmDropShadow,&r1,screen,&r2);
	}
	//draw drop shadow - border
	int i=r.w-32;
	while(i>0){
		int ii=i>128?128:i;

		//top
		SDL_Rect r1={0,256-alpha*16,ii,16},r2={r.x+r.w-16-i,r.y-16,0,0};
		SDL_BlitSurface(bmDropShadow,&r1,screen,&r2);
		//bottom
		r1.x=128;r2.y=r.y+r.h;
		SDL_BlitSurface(bmDropShadow,&r1,screen,&r2);

		i-=ii;
	}
	i=r.h-32;
	while(i>0){
		int ii=i>128?128:i;

		//top
		SDL_Rect r1={512-alpha*16,0,16,ii},r2={r.x-16,r.y+r.h-16-i,0,0};
		SDL_BlitSurface(bmDropShadow,&r1,screen,&r2);
		//bottom
		r1.y=128;r2.x=r.x+r.w;
		SDL_BlitSurface(bmDropShadow,&r1,screen,&r2);

		i-=ii;
	}
}

void StatisticsManager::reloadCompletedLevelsAndAchievements(){
	completedLevels=silverLevels=goldLevels=0;

	LevelPackManager *lpm=getLevelPackManager();
	vector<string> v=lpm->enumLevelPacks();

	bool tutorial=false,tutorialGold=false;

	for(unsigned int i=0;i<v.size();i++){
		string& s=v[i];
		LevelPack *levels=lpm->getLevelPack(s);
		levels->loadProgress(getUserPath(USER_DATA)+"progress/"+s+".progress");

		bool b=false;
		if(s=="tutorial"){
			b=tutorial=tutorialGold=true;
		}

		for(int n=0,m=levels->getLevelCount();n<m;n++){
			LevelPack::Level *lv=levels->getLevel(n);
			int medal=lv->won;
			if(medal){
				if(lv->targetTime<0 || lv->time<=lv->targetTime)
					medal++;
				if(lv->targetRecordings<0 || lv->recordings<=lv->targetRecordings)
					medal++;

				completedLevels++;
				if(medal==2) silverLevels++;
				if(medal==3) goldLevels++;

				if(medal!=3 && b) tutorialGold=false;
			}else if(b){
				tutorial=tutorialGold=false;
			}
		}
	}

	//upadte achievements
	updateLevelAchievements();
	updateTutorialAchievementsInternal((tutorial?1:0)|(tutorialGold?2:0));
}

void StatisticsManager::reloadOtherAchievements(){
	int i;

	if(playTime>=7200) newAchievement("addicted");
	if(playTime>=86400) newAchievement("loyalFan");

	if(levelEditTime>=7200) newAchievement("constructor");
	if(levelEditTime>=86400) newAchievement("constructor2");

	if(createdLevels>=1) newAchievement("create1");
	if(createdLevels>=50) newAchievement("create50");

	i=playerJumps+shadowJumps;
	if(i>=1000) newAchievement("frog");

	i=playerDies+shadowDies;
	if(i>=1) newAchievement("die1");
	if(i>=50) newAchievement("die50");
	if(i>=1000) newAchievement("die1000");

	i=playerSquashed+shadowSquashed;
	if(i>=1) newAchievement("squash1");
	if(i>=50) newAchievement("squash50");

	if(version.find("Development")!=string::npos) newAchievement("programmer");
}

//Update level specified achievements.
//Make sure the completed level count is correct.
void StatisticsManager::updateLevelAchievements(){
	if(completedLevels>=1) newAchievement("newbie");
	if(goldLevels>=1) newAchievement("goodjob");
	if(completedLevels>=50) newAchievement("experienced");
	if(goldLevels>=50) newAchievement("expert");
}

//Update tutorial specified achievements.
//Make sure the level progress of tutorial is correct.
void StatisticsManager::updateTutorialAchievements(){
	//find tutorial level pack
	LevelPackManager *lpm=getLevelPackManager();
	LevelPack *levels=lpm->getLevelPack("tutorial");
	if(levels==NULL) return;

	bool tutorial=true,tutorialGold=true;

	for(int n=0,m=levels->getLevelCount();n<m;n++){
		LevelPack::Level *lv=levels->getLevel(n);
		int medal=lv->won;
		if(medal){
			if(lv->targetTime<0 || lv->time<=lv->targetTime)
				medal++;
			if(lv->targetRecordings<0 || lv->recordings<=lv->targetRecordings)
				medal++;

			if(medal!=3) tutorialGold=false;
		}else{
			tutorial=tutorialGold=false;
			break;
		}
	}

	//upadte achievements
	updateTutorialAchievementsInternal((tutorial?1:0)|(tutorialGold?2:0));
}

//internal function
//flags: a bit-field value indicates which achievements we have.
void StatisticsManager::updateTutorialAchievementsInternal(int flags){
	if(flags&1) newAchievement("tutorial");
	if(flags&2) newAchievement("tutorialGold");
}
