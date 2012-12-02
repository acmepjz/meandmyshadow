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
#include <sstream>
#include <vector>
#include <map>
#include "libs/tinyformat/tinyformat.h"

using namespace std;

StatisticsManager statsMgr;

static const int achievementDisplayTime=100;
static const int achievementIntervalTime=120;

#include "AchievementList.h"

static map<string,AchievementInfo*> avaliableAchievements;

//================================================================

StatisticsManager::StatisticsManager(){
	bmDropShadow=NULL;
	bmQuestionMark=NULL;
	bmAchievement=NULL;

	startTime=time(NULL);

	tutorialLevels=0;

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
		=recordTimes=switchTimes=swapTimes=saveTimes=loadTimes
		=playTime=levelEditTime
		=createdLevels=tutorialCompleted=tutorialGold=0;

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
	LOAD_STATS(saveTimes,atoi);
	LOAD_STATS(loadTimes,atoi);
	LOAD_STATS(playTime,atoi);
	LOAD_STATS(levelEditTime,atoi);
	LOAD_STATS(createdLevels,atoi);

	//load achievements.
	//format is: name;time,name;time,...
	{
		vector<string> &v=node.attributes["achievements"];
		for(unsigned int i=0;i<v.size();i++){
			string s=v[i];
			time_t t=0;

			string::size_type lps=s.find(';');
			if(lps!=string::npos){
				string s1=s.substr(lps+1);
				s=s.substr(0,lps);

				long long n;
				sscanf(s1.c_str(),
#ifdef WIN32
					"%I64d",
#else
					"%Ld",
#endif
					&n);

				t=(time_t)n;
			}

			map<string,AchievementInfo*>::iterator it=avaliableAchievements.find(s);
			if(it!=avaliableAchievements.end()){
				OwnedAchievement ach={t,it->second};
				achievements[it->first]=ach;
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
	SAVE_STATS(saveTimes,"%d");
	SAVE_STATS(loadTimes,"%d");
	SAVE_STATS(playTime,"%d");
	SAVE_STATS(levelEditTime,"%d");
	SAVE_STATS(createdLevels,"%d");

	//save achievements.
	//format is: name;time,name;time,...
	{
		vector<string>& v=node.attributes["achievements"];

		for(map<string,OwnedAchievement>::iterator it=achievements.begin();it!=achievements.end();++it){
			stringstream strm;
			char s[32];

			long long n=it->second.achievedTime;
			sprintf(s,
#ifdef WIN32
				"%I64d",
#else
				"%Ld",
#endif
				n);
			strm<<it->first<<";"<<s;

			v.push_back(strm.str());
		}
	}

	POASerializer serializer;
	serializer.writeNode(&node,file,true,true);
}

void StatisticsManager::loadPicture(){
	//Load drop shadow picture
	bmDropShadow=loadImage(getDataPath()+"gfx/dropshadow.png");
	bmQuestionMark=loadImage(getDataPath()+"gfx/menu/questionmark.png");
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
	if(achievementTime==0 && bmAchievement==NULL && currentAchievement<(int)queuedAchievements.size()){
		//create surface
		bmAchievement=createAchievementSurface(queuedAchievements[currentAchievement++]);
		drawGUIBox(0,0,bmAchievement->w,bmAchievement->h,bmAchievement,0xFFFFFF00);

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
		map<string,OwnedAchievement>::iterator it2=achievements.find(id);
		if(it2!=achievements.end()) return;

		OwnedAchievement ach={time(NULL),it->second};
		achievements[id]=ach;
	}

	//add it to queue
	queuedAchievements.push_back(it->second);
}

float StatisticsManager::getAchievementProgress(AchievementInfo* info){
	if(!strcmp(info->id,"experienced")){
		return float(completedLevels)/50.0f*100.0f;
	}
	if(!strcmp(info->id,"expert")){
		return float(goldLevels)/50.0f*100.0f;
	}
	if(!strcmp(info->id,"tutorial")){
		if(tutorialLevels>0)
			return float(tutorialCompleted)/float(tutorialLevels)*100.0f;
		else
			return 0.0f;
	}
	if(!strcmp(info->id,"tutorialGold")){
		if(tutorialLevels>0)
			return float(tutorialGold)/float(tutorialLevels)*100.0f;
		else
			return 0.0f;
	}
	if(!strcmp(info->id,"create50")){
		return float(createdLevels)/50.0f*100.0f;
	}
	if(!strcmp(info->id,"frog")){
		return float(playerJumps+shadowJumps)/1000.0f*100.0f;
	}
	if(!strcmp(info->id,"die50")){
		return float(playerDies+shadowDies)/50.0f*100.0f;
	}
	if(!strcmp(info->id,"die1000")){
		return float(playerDies+shadowDies)/1000.0f*100.0f;
	} 
	if(!strcmp(info->id,"suqash50")){
		return float(playerSquashed+shadowSquashed)/50.0f*100.0f;
	}
	if(!strcmp(info->id,"travel100")){
		return (playerTravelingDistance+shadowTravelingDistance)/100.0f*100.0f;
	}
	if(!strcmp(info->id,"travel1k")){
		return (playerTravelingDistance+shadowTravelingDistance)/1000.0f*100.0f;
	}
	if(!strcmp(info->id,"travel10k")){
		return (playerTravelingDistance+shadowTravelingDistance)/10000.0f*100.0f;
	}
	if(!strcmp(info->id,"travel42k")){
		return (playerTravelingDistance+shadowTravelingDistance)/42195.0f*100.0f;
	}
	if(!strcmp(info->id,"record100")){
		return float(recordTimes)/100.0f*100.0f;
	}
	if(!strcmp(info->id,"record1k")){
		return float(recordTimes)/1000.0f*100.0f;
	}

	//not found
	return 0.0f;
}

SDL_Surface* StatisticsManager::createAchievementSurface(AchievementInfo* info,SDL_Surface* surface,SDL_Rect* rect,bool showTip,const time_t *achievedTime){
	if(info==NULL || info->id==NULL) return NULL;

	//prepare text
	SDL_Surface *title0=NULL,*title1=NULL;
	vector<SDL_Surface*> descSurfaces;
	SDL_Color fg={0,0,0};
	int fontHeight=TTF_FontLineSkip(fontText);

	bool showDescription=false;
	bool showImage=false;
	float achievementProgress=0.0f;

	if(showTip){
		title0=TTF_RenderUTF8_Blended(fontText,_("New achievement:"),fg);
		title1=TTF_RenderUTF8_Blended(fontGUISmall,_(info->name),fg);
		showDescription=showImage=true;
	}else if(achievedTime){
		char s[128];
		strftime(s,sizeof(s),"%c",localtime(achievedTime));

		stringstream strm;
		tinyformat::format(strm,_("Achieved at %s"),s);
		
		title1=TTF_RenderUTF8_Blended(fontText,strm.str().c_str(),fg);
		title0=TTF_RenderUTF8_Blended(fontGUISmall,_(info->name),fg);
		showDescription=showImage=true;
	}else if(info->displayStyle==ACHIEVEMT_HIDDEN){
		title0=TTF_RenderUTF8_Blended(fontGUISmall,_("Unknown achievement"),fg);
	}else{
		if(info->displayStyle==ACHIEVEMT_PROGRESS){
			achievementProgress=getAchievementProgress(info);

			stringstream strm;
			tinyformat::format(strm,_("Achieved %0.1f%%"),achievementProgress);

			title1=TTF_RenderUTF8_Blended(fontText,strm.str().c_str(),fg);
		}else{
			title1=TTF_RenderUTF8_Blended(fontText,_("Not achieved"),fg);
		}

		title0=TTF_RenderUTF8_Blended(fontGUISmall,_(info->name),fg);

		showDescription= info->displayStyle==ACHIEVEMT_ALL || info->displayStyle==ACHIEVEMT_PROGRESS;
		showImage=true;
	}

	if(info->description!=NULL && showDescription){
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
		/*//calc progress bar size
		if(!showTip && !achievedTime && info->displayStyle==ACHIEVEMT_PROGRESS){
			h1+=4;
		}*/
	}

	if(showImage){
		if(info->imageSurface!=NULL){
			w1+=info->r.w+8;
			w+=info->r.w+8;
			if(info->r.h>h1) h1=info->r.h;
		}
	}else{
		w1+=bmQuestionMark->w+8;
		w+=bmQuestionMark->w+8;
		if(bmQuestionMark->h>h1) h1=bmQuestionMark->h;
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
	SDL_Rect r={left,top,w,h};
	if(showTip || achievedTime){
		SDL_FillRect(surface,&r,SDL_MapRGB(surface->format,255,255,255));
	}else{
		SDL_FillRect(surface,&r,SDL_MapRGB(surface->format,192,192,192));
	}

	//draw picture
	if(showImage){
		if(info->imageSurface!=NULL){
			SDL_Rect r={left+8,top+8+(h1-info->r.h)/2,0,0};
			SDL_BlitSurface(info->imageSurface,&info->r,surface,&r);
		}
	}else{
		SDL_Rect r={left+8,top+8+(h1-bmQuestionMark->h)/2,0,0};
		SDL_BlitSurface(bmQuestionMark,NULL,surface,&r);
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

		//draw progress bar
		if(!showTip && !achievedTime && info->displayStyle==ACHIEVEMT_PROGRESS){
			SDL_Rect r1={r.x,r.y,w-8-r.x,title1->h};
			SDL_FillRect(surface,&r1,SDL_MapRGB(surface->format,96,96,96));
			r1.x++;
			r1.y++;
			r1.w-=2;
			r1.h-=2;
			SDL_FillRect(surface,&r1,SDL_MapRGB(surface->format,216,216,216));
			r1.w=int(achievementProgress/100.0f*float(r1.w));
			SDL_FillRect(surface,&r1,SDL_MapRGB(surface->format,144,144,144));

			//???
			r.x+=2;
			r.y+=2;
		}

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

	bool tutorial=false,tutorialIsGold=false;

	for(unsigned int i=0;i<v.size();i++){
		string& s=v[i];
		LevelPack *levels=lpm->getLevelPack(s);
		levels->loadProgress(getUserPath(USER_DATA)+"progress/"+s+".progress");

		bool b=false;
		if(s=="tutorial"){
			tutorialLevels=levels->getLevelCount();
			tutorialCompleted=tutorialGold=0;
			b=tutorial=tutorialIsGold=true;
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
				if(b) tutorialCompleted++;
				if(medal==2) silverLevels++;
				if(medal==3){
					goldLevels++;
					if(b) tutorialGold++;
				}

				if(medal!=3 && b) tutorialIsGold=false;
			}else if(b){
				tutorial=tutorialIsGold=false;
			}
		}
	}

	//upadte achievements
	updateLevelAchievements();
	updateTutorialAchievementsInternal((tutorial?1:0)|(tutorialIsGold?2:0));
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

	float d=playerTravelingDistance+shadowTravelingDistance;
	if(d>=100.0f) newAchievement("travel100");
	if(d>=1000.0f) newAchievement("travel1k");
	if(d>=10000.0f) newAchievement("travel10k");
	if(d>=42195.0f) newAchievement("travel42k");

	if(recordTimes>=100) newAchievement("record100");
	if(recordTimes>=1000) newAchievement("record1k");

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

	bool tutorial=true,tutorialIsGold=true;
	tutorialLevels=levels->getLevelCount();
	tutorialCompleted=tutorialGold=0;

	for(int n=0,m=levels->getLevelCount();n<m;n++){
		LevelPack::Level *lv=levels->getLevel(n);
		int medal=lv->won;
		if(medal){
			if(lv->targetTime<0 || lv->time<=lv->targetTime)
				medal++;
			if(lv->targetRecordings<0 || lv->recordings<=lv->targetRecordings)
				medal++;

			tutorialCompleted++;

			if(medal!=3) tutorialIsGold=false;
			else tutorialGold++;
		}else{
			tutorial=tutorialIsGold=false;
			break;
		}
	}

	//upadte achievements
	updateTutorialAchievementsInternal((tutorial?1:0)|(tutorialIsGold?2:0));
}

//internal function
//flags: a bit-field value indicates which achievements we have.
void StatisticsManager::updateTutorialAchievementsInternal(int flags){
	if(flags&1) newAchievement("tutorial");
	if(flags&2) newAchievement("tutorialGold");
}
