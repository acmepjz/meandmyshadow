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
#include "MusicManager.h"
#include "SoundManager.h"
#include "ThemeManager.h"
#include "WordWrapper.h"
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
#include <SDL_ttf_fontfallback.h>
#if defined(WIN32)
#define PRINTF_LONGLONG "%I64d"
#else
#define PRINTF_LONGLONG "%lld"
#endif

using namespace std;

StatisticsManager statsMgr;

static const int achievementDisplayTime=(FPS*4500)/1000;
static const int achievementIntervalTime=achievementDisplayTime+(FPS*500)/1000;

map<string, AchievementInfo*> StatisticsManager::avaliableAchievements;

//================================================================

StatisticsManager::StatisticsManager(){
	bmDropShadow=NULL;
	bmQuestionMark=NULL;
	bmAchievement=NULL;

	startTime=time(NULL);

	tutorialLevels=0;

	clear();
}

void StatisticsManager::clear(){
	playerTravelingDistance=shadowTravelingDistance=0.0f;
	playerJumps=shadowJumps
		=playerDies=shadowDies
		=playerSquashed=shadowSquashed
		=completedLevels=silverLevels=goldLevels=totalLevels
		=completedLevelpacks=silverLevelpacks=goldLevelpacks=totalLevelpacks
		=recordTimes=switchTimes=swapTimes=saveTimes=loadTimes
		=collectibleCollected
		=playTime=levelEditTime
		=createdLevels=tutorialCompleted=tutorialGold=0;

	completedLevelsByCategory.fill(0);
	silverLevelsByCategory.fill(0);
	goldLevelsByCategory.fill(0);
	totalLevelsByCategory.fill(0);
	completedLevelpacksByCategory.fill(0);
	silverLevelpacksByCategory.fill(0);
	goldLevelpacksByCategory.fill(0);
	totalLevelpacksByCategory.fill(0);

	achievements.clear();
	queuedAchievements.clear();

	achievementTime=0;
	currentAchievement=0;
	if(bmAchievement){
        bmAchievement.reset();
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
	LOAD_STATS(collectibleCollected,atoi);
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
				sscanf(s1.c_str(),PRINTF_LONGLONG,&n);

				t=(time_t)n;
			}

			map<string,AchievementInfo*>::iterator it=avaliableAchievements.find(s);
			if(it!=avaliableAchievements.end()){
				OwnedAchievement ach={t,it->second};
				achievements[it->first]=ach;
			}
		}
	}

	updateAchievementDisplayStyle();
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
			sprintf(s,PRINTF_LONGLONG,n);
			strm<<it->first<<";"<<s;

			v.push_back(strm.str());
		}
	}

	POASerializer serializer;
	serializer.writeNode(&node,file,true,true);
}

void StatisticsManager::loadPicture(SDL_Renderer& renderer, ImageManager& imageManager){
	//Load drop shadow picture
    bmDropShadow=imageManager.loadTexture(getDataPath()+"gfx/dropshadow.png", renderer);
    bmQuestionMark=imageManager.loadImage(getDataPath()+"gfx/menu/questionmark.png");
}

int StatisticsManager::getTotalAchievements() const {
	return avaliableAchievements.size();
}

int StatisticsManager::getCurrentNumberOfAchievements() const {
	return achievements.size();
}

void StatisticsManager::registerAchievements(ImageManager& imageManager){
	if(!avaliableAchievements.empty()) return;

	for(int i=0;achievementList[i].id!=NULL;i++){
		avaliableAchievements[achievementList[i].id]=&achievementList[i];
		if(achievementList[i].imageFile!=NULL){
            achievementList[i].imageSurface = imageManager.loadImage(getDataPath()+achievementList[i].imageFile);
		}
	}
}

void StatisticsManager::render(ImageManager&,SDL_Renderer &renderer){
    if(achievementTime==0 && !bmAchievement && currentAchievement<(int)queuedAchievements.size()){
		//create surface
        bmAchievement=createAchievementSurface(renderer, queuedAchievements[currentAchievement++]);

		//check if queue is empty
		if(currentAchievement>=(int)queuedAchievements.size()){
			queuedAchievements.clear();
			currentAchievement=0;
		}

		//play a sound
		getSoundManager()->playSound("achievement", 1, false, 32);
	}

	//check if we need to display achievements
	if(bmAchievement){
		achievementTime++;
		if(achievementTime<=0){
			return;
		}else if(achievementTime<=5){
            drawAchievement(renderer,achievementTime);
		}else if(achievementTime<=achievementDisplayTime-5){
            drawAchievement(renderer,5);
		}else if(achievementTime<achievementDisplayTime){
            drawAchievement(renderer,achievementDisplayTime-achievementTime);
		}else if(achievementTime>=achievementIntervalTime){
			if(bmAchievement){
                bmAchievement.reset();
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
		if (achievements.find(id) != achievements.end()) return;

		achievements[id] = OwnedAchievement{ time(NULL), it->second };

		//update achievement unlock
		for (int idx = 0; achievementUnlockList[idx].id; idx++) {
			if (achievementUnlockList[idx].id == id) {
				achievementDisplayStyle[achievementUnlockList[idx].unlockId] =
					std::max((int)achievementUnlockList[idx].displayStyle, achievementDisplayStyle[achievementUnlockList[idx].unlockId]);
			}
		}
	}

	//add it to queue
	queuedAchievements.push_back(it->second);
}

void StatisticsManager::updateAchievementDisplayStyle() {
	achievementDisplayStyle.clear();

	for (int idx = 0; achievementUnlockList[idx].id; idx++) {
		if (achievements.find(achievementUnlockList[idx].id) != achievements.end()) {
			achievementDisplayStyle[achievementUnlockList[idx].unlockId] =
				std::max((int)achievementUnlockList[idx].displayStyle, achievementDisplayStyle[achievementUnlockList[idx].unlockId]);
		}
	}
}

time_t StatisticsManager::achievedTime(const std::string& id) {
	auto it = achievements.find(id);

	if (it == achievements.end()) return 0;
	else return it->second.achievedTime;
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
	if(!strcmp(info->id,"create10")){
		return float(createdLevels)/10.0f*100.0f;
	}
	if (!strcmp(info->id, "jump100")){
		return float(playerJumps + shadowJumps) / 100.0f*100.0f;
	}
	if (!strcmp(info->id, "jump1k")){
		return float(playerJumps + shadowJumps) / 1000.0f*100.0f;
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
	if(!strcmp(info->id,"switch100")){
		return float(switchTimes)/100.0f*100.0f;
	}
	if(!strcmp(info->id,"switch1k")){
		return float(switchTimes)/1000.0f*100.0f;
	}
	if(!strcmp(info->id,"swap100")){
		return float(swapTimes)/100.0f*100.0f;
	}
	if (!strcmp(info->id, "mainBronze")) {
		if (totalLevelsByCategory[MAIN] > 0)
			return float(completedLevelsByCategory[MAIN]) / float(totalLevelsByCategory[MAIN])*100.0f;
		else
			return 0.0f;
	}
	if (!strcmp(info->id, "mainSilver")) {
		if (totalLevelsByCategory[MAIN] > 0)
			return float(silverLevelsByCategory[MAIN] + goldLevelsByCategory[MAIN]) / float(totalLevelsByCategory[MAIN])*100.0f;
		else
			return 0.0f;
	}
	if (!strcmp(info->id, "mainGold")) {
		if (totalLevelsByCategory[MAIN] > 0)
			return float(goldLevelsByCategory[MAIN]) / float(totalLevelsByCategory[MAIN])*100.0f;
		else
			return 0.0f;
	}

	//not found
	return 0.0f;
}

SharedTexture StatisticsManager::createAchievementSurface(SDL_Renderer& renderer, AchievementInfo* info,SDL_Rect* rect,bool showTip,const time_t *achievedTime){
	if(info==NULL || info->id==NULL) return NULL;

	//prepare text
    SurfacePtr title0(nullptr);
    SurfacePtr title1(nullptr);
	vector<SDL_Surface*> descSurfaces;
	SDL_Color fg = objThemes.getTextColor(true);
	int fontHeight=TTF_FontLineSkip(fontText);

	bool showDescription=false;
	bool showImage=false;
	float achievementProgress=0.0f;

	if(showTip){
        title0.reset(TTF_RenderUTF8_Blended(fontText,_("New achievement:"),fg));
        title1.reset(TTF_RenderUTF8_Blended(fontGUISmall,_(info->name),fg));
		showDescription=showImage=true;
	}else if(achievedTime){
		char s[256];
		strftime(s,sizeof(s),"%c",localtime(achievedTime));

        title0.reset(TTF_RenderUTF8_Blended(fontGUISmall,_(info->name),fg));
		title1.reset(TTF_RenderUTF8_Blended(fontText, tfm::format(_("Achieved on %s"), (char*)s).c_str(), fg));
		showDescription=showImage=true;
	}else if(info->displayStyle==ACHIEVEMENT_HIDDEN){
        title0.reset(TTF_RenderUTF8_Blended(fontGUISmall,_("Unknown achievement"),fg));
	}else{
		if(info->displayStyle==ACHIEVEMENT_PROGRESS){
			achievementProgress=getAchievementProgress(info);

			title1.reset(TTF_RenderUTF8_Blended(fontText, tfm::format(_("Achieved %1.0f%%"), achievementProgress).c_str(), fg));
		}else{
            title1.reset(TTF_RenderUTF8_Blended(fontText,_("Not achieved"),fg));
		}

        title0.reset(TTF_RenderUTF8_Blended(fontGUISmall,_(info->name),fg));

		showDescription= info->displayStyle==ACHIEVEMENT_ALL || info->displayStyle==ACHIEVEMENT_PROGRESS;
		showImage=true;
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
		if(!showTip && !achievedTime && info->displayStyle==ACHIEVEMENT_PROGRESS){
			h1+=4;
        }*/
	}

	const int preferredImageWidth = 50;
	const int preferredImageHeight = 50;

	if(showImage){
		if(info->imageSurface!=NULL){
			// NEW: we have the preferred image size
			const int width = std::max(info->r.w, preferredImageWidth);
			const int height = std::max(info->r.h, preferredImageHeight);
			w1+=width+8;
			w+=width+8;
			if(height>h1) h1=height;
		}
	}else{
        w1+=bmQuestionMark->w+8;
        w+=bmQuestionMark->w+8;
        if(bmQuestionMark->h>h1) h1=bmQuestionMark->h;
	}

	//we render the description here since we need to know the width of title
	if (info->description != NULL && showDescription){
		string description = _(info->description);

		WordWrapper wrapper;
		wrapper.font = fontText;
		wrapper.maxWidth = showTip ? std::max(int(SCREEN_WIDTH * 0.5f), w) : (rect ? (rect->w - 16) : -1);
		wrapper.wordWrap = wrapper.maxWidth > 0;
		wrapper.hyphen = "-";

		vector<string> lines;

		wrapper.addString(lines, description);

		int start, end;
		const int m = lines.size();

		for (start = 0; start < m; start++) {
			if (!lines[start].empty()) break;
		}
		for (end = m - 1; end >= start; end--) {
			if (!lines[end].empty()) break;
		}
		for (int i = start; i <= end; i++) {
			if (lines[i].empty()) {
				descSurfaces.push_back(TTF_RenderUTF8_Blended(fontText, " ", fg));
			} else {
				descSurfaces.push_back(TTF_RenderUTF8_Blended(fontText, lines[i].c_str(), fg));
			}
		}
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
        //NOTE: SDL2 port. This was never used.
/*		if(surface!=NULL){
			left=rect->x;
			top=rect->y;
        }*/
		if(rect->w>0) w=rect->w;
		else rect->w=w;
		rect->h=h;
	}

	//create surface if necessary
    SurfacePtr surface = createSurface(w, h);
    std::unique_ptr<SDL_Renderer,decltype(&SDL_DestroyRenderer)> surfaceRenderer(
                SDL_CreateSoftwareRenderer(surface.get()), &SDL_DestroyRenderer);

	//draw background
    const SDL_Rect r={left,top,w,h};
	if(showTip || achievedTime){
        SDL_FillRect(surface.get(),&r,SDL_MapRGB(surface->format,255,255,255));
	}else{
        SDL_FillRect(surface.get(),&r,SDL_MapRGB(surface->format,192,192,192));
	}

	//draw horizontal separator
	//FIXME: this is moved from StatisticsScreen::createGUI
	if (!showTip) {
		const SDL_Rect r0 = { left, top, w, 1 };
		const SDL_Rect r1 = { left, top + h - 2, w, 1 };
		const SDL_Rect r2 = { left, top + h - 1, w, 1 };
		Uint32 c0 = achievedTime ? SDL_MapRGB(surface->format, 224, 224, 224) : SDL_MapRGB(surface->format, 168, 168, 168);
		Uint32 c2 = achievedTime ? SDL_MapRGB(surface->format, 128, 128, 128) : SDL_MapRGB(surface->format, 96, 96, 96);
		SDL_FillRect(surface.get(), &r0, c0);
		SDL_FillRect(surface.get(), &r1, c0);
		SDL_FillRect(surface.get(), &r2, c2);
	}

	//draw picture
    if(showImage){
        if(info->imageSurface){
			// NEW: we have the preferred image size
			SDL_Rect r={left+8,top+8+(h1-info->r.h)/2,0,0};
			if (info->r.w < preferredImageWidth) r.x += (preferredImageWidth - info->r.w) / 2;
            SDL_BlitSurface(info->imageSurface,&info->r,surface.get(),&r);
        }
    }else{
        SDL_Rect r={left+8,top+8+(h1-bmQuestionMark->h)/2,0,0};
        SDL_BlitSurface(bmQuestionMark,NULL,surface.get(),&r);
	}

	//draw text
	h=8;
    if(title0){
		SDL_Rect r={left+w1,top+h,0,0};
        SDL_BlitSurface(title0.get(),NULL,surface.get(),&r);
		h+=title0->h;
	}
    if(title1){
		SDL_Rect r={left+w1,top+h,0,0};

		//Draw progress bar.
		if(!showTip && !achievedTime && info->displayStyle==ACHIEVEMENT_PROGRESS){
			//Draw borders.
			SDL_Rect r1={r.x,r.y,w-8-r.x,title1->h};
            drawGUIBox(r1.x,r1.y,r1.w,r1.h,*surfaceRenderer,0x1D);
			
			//Draw progress.
			r1.x++;
			r1.y++;
			r1.w=int(achievementProgress/100.0f*float(r1.w-2)+0.5f);
			r1.h-=2;

            SDL_SetRenderDrawColor(surfaceRenderer.get(),0,0,0,100);
            SDL_RenderFillRect(surfaceRenderer.get(),&r1);

			//shift the text a little bit (???)
			r.x+=2;
			r.y+=2;
		}
		
		//Draw text.
        SDL_BlitSurface(title1.get(),NULL,surface.get(),&r);
	}
	h=h1+16;
	for(unsigned int i=0;i<descSurfaces.size();i++){
		if(descSurfaces[i]!=NULL){
            SDL_Rect r={left+8,top+h+static_cast<int>(i)*fontHeight,0,0};
            SDL_BlitSurface(descSurfaces[i],NULL,surface.get(),&r);
		}
	}

	//clean up
	for(unsigned int i=0;i<descSurfaces.size();i++){
		if(descSurfaces[i]!=NULL){
			SDL_FreeSurface(descSurfaces[i]);
		}
	}
    //FIXME: Should we clear the vector here?



	//over
    return textureFromSurface(renderer, std::move(surface));
}

void StatisticsManager::drawAchievement(SDL_Renderer& renderer,int alpha){
    if(!bmAchievement || alpha<=0) {
        return;
    }
	if(alpha>5) alpha=5;

    SDL_Rect r = rectFromTexture(*bmAchievement);
    int w=0,h=0;
    SDL_GetRendererOutputSize(&renderer, &w, &h);
    r.x = w-32-r.w;
    r.y = 32;

	int a = alpha * 45;

    SDL_SetTextureAlphaMod(bmAchievement.get(), a);
    applyTexture(r.x, r.y, bmAchievement, renderer);

	//Draw the box.
	drawGUIBox(r.x, r.y, r.w, r.h, renderer, a, true, true);

    if(!bmDropShadow) {
        return;
    }

	//draw drop shadow - corner
	{
		int w1=r.w/2,w2=r.w-w1,h1=r.h/2,h2=r.h-h1;
		if(w1>16) w1=16;
		if(w2>16) w2=16;
		if(h1>16) h1=16;
		if(h2>16) h2=16;

        const int x=(5-alpha)*64;

		//top-left
        SDL_Rect r1={x,0,w1+16,h1+16};//),r2={r.x-16,r.y-16,0,0};
        SDL_Rect r2 ={r.x-16, r.y-16, r1.w, r1.h};
        SDL_RenderCopy(&renderer, bmDropShadow.get(), &r1, &r2);
		//top-right
        r1.x=x+48-w2;r2.w=r1.w =w2+16;r2.x=r.x+r.w-w2;
        SDL_RenderCopy(&renderer, bmDropShadow.get(), &r1, &r2);
		//bottom-right
        r1.y=48-h2;r2.h=r1.h=h2+16;r2.y=r.y+r.h-h2;
        SDL_RenderCopy(&renderer, bmDropShadow.get(), &r1, &r2);
		//bottom-left
        r1.x=x;r2.w=r1.w=w1+16;r2.x=r.x-16;
        SDL_RenderCopy(&renderer, bmDropShadow.get(), &r1, &r2);
	}
	//draw drop shadow - border
	int i=r.w-32;
	while(i>0){
        const int ii=i>128?128:i;

		//top
        SDL_Rect r1={0,256-alpha*16,ii,16};
        SDL_Rect r2={r.x+r.w-16-i,r.y-16,r1.w,r1.h};
        SDL_RenderCopy(&renderer, bmDropShadow.get(), &r1, &r2);
		//bottom
		r1.x=128;r2.y=r.y+r.h;
        SDL_RenderCopy(&renderer, bmDropShadow.get(), &r1, &r2);

		i-=ii;
	}
	i=r.h-32;
	while(i>0){
        const int ii=i>128?128:i;

		//top
        SDL_Rect r1={512-alpha*16,0,16,ii};
        SDL_Rect r2={r.x-16,r.y+r.h-16-i, r1.w, r1.h};
        SDL_RenderCopy(&renderer, bmDropShadow.get(), &r1, &r2);
		//bottom
		r1.y=128;r2.x=r.x+r.w;
        SDL_RenderCopy(&renderer, bmDropShadow.get(), &r1, &r2);

		i-=ii;
	}
}

void StatisticsManager::reloadCompletedLevelsAndAchievements(){
	completedLevels=silverLevels=goldLevels=totalLevels
		=completedLevelpacks=silverLevelpacks=goldLevelpacks=totalLevelpacks=0;

	completedLevelsByCategory.fill(0);
	silverLevelsByCategory.fill(0);
	goldLevelsByCategory.fill(0);
	totalLevelsByCategory.fill(0);
	completedLevelpacksByCategory.fill(0);
	silverLevelpacksByCategory.fill(0);
	goldLevelpacksByCategory.fill(0);
	totalLevelpacksByCategory.fill(0);

	LevelPackManager *lpm=getLevelPackManager();
	vector<pair<string,string> > v=lpm->enumLevelPacks();

	bool tutorialFinished=false,tutorialIsGold=false;

	for(unsigned int i=0;i<v.size();i++){
		string& s=v[i].first;
		LevelPack *levels=lpm->getLevelPack(s);
		levels->loadProgress();

		int category = (int)levels->type;
		if (category > CUSTOM) category = CUSTOM;

		int packMedal = 3;

		bool isTutorial=false;
		if(s==lpm->tutorialLevelPackPath){
			tutorialLevels=levels->getLevelCount();
			tutorialCompleted=tutorialGold=0;
			isTutorial=true;
		}

		for(int n=0,m=levels->getLevelCount();n<m;n++){
			int medal = levels->getLevel(n)->getMedal();
			if (packMedal > medal) packMedal = medal;
			if(medal){
				completedLevels++;
				completedLevelsByCategory[category]++;
				if(isTutorial) tutorialCompleted++;
				if (medal == 2) {
					silverLevels++;
					silverLevelsByCategory[category]++;
				} else if (medal == 3) {
					goldLevels++;
					goldLevelsByCategory[category]++;
					if (isTutorial) tutorialGold++;
				}
			}

			totalLevels++;
			totalLevelsByCategory[category]++;
		}

		if (isTutorial) {
			tutorialFinished = packMedal > 0;
			tutorialIsGold = packMedal == 3;
		} else if (levels->type != COLLECTION) {
			newAchievement("complete_levelpack");
		}

		if (levels->type != COLLECTION) {
			if (packMedal) {
				completedLevelpacks++;
				completedLevelpacksByCategory[category]++;
				if (packMedal == 2) {
					silverLevelpacks++;
					silverLevelpacksByCategory[category]++;
				} else if (packMedal == 3) {
					goldLevelpacks++;
					goldLevelpacksByCategory[category]++;
				}
			}
			totalLevelpacks++;
			totalLevelpacksByCategory[category]++;
		}
	}

	//upadte achievements
	updateLevelAchievements();
	updateTutorialAchievementsInternal((tutorialFinished?1:0)|(tutorialIsGold?2:0));
}

void StatisticsManager::reloadOtherAchievements(){
	int i;

	if(playTime>=7200) newAchievement("addicted");
	if(playTime>=86400) newAchievement("loyalFan");

	if(levelEditTime>=7200) newAchievement("constructor");
	if(levelEditTime>=28800) newAchievement("constructor2");

	if(createdLevels>=1) newAchievement("create1");
	if(createdLevels>=10) newAchievement("create10");

	i=playerJumps+shadowJumps;
	if (i >= 100) newAchievement("jump100");
	if (i >= 1000) newAchievement("jump1k");

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

	if(switchTimes>=100) newAchievement("switch100");
	if(switchTimes>=1000) newAchievement("switch1k");

	if(swapTimes>=100) newAchievement("swap100");

	if(saveTimes>=100) newAchievement("save100");

	if(loadTimes>=100) newAchievement("load100");

	if (collectibleCollected >= 100) newAchievement("collect100");
	if (collectibleCollected >= 1000) newAchievement("collect1k");

	if (version.find("Development") != string::npos
		|| version.find("Alpha") != string::npos
		|| version.find("Beta") != string::npos
		|| version.find("RC") != string::npos
		|| version.find("Candidate") != string::npos)
	{
		newAchievement("programmer");
	}
}

//Update level specified achievements.
//Make sure the completed level count is correct.
void StatisticsManager::updateLevelAchievements(){
	if(completedLevels>=1) newAchievement("newbie");
	if(goldLevels>=1) newAchievement("goodjob");
	if(completedLevels>=50) newAchievement("experienced");
	if(goldLevels>=50) newAchievement("expert");

	if (completedLevelsByCategory[MAIN] >= totalLevelsByCategory[MAIN]) newAchievement("mainBronze");
	if (silverLevelsByCategory[MAIN] + goldLevelsByCategory[MAIN] >= totalLevelsByCategory[MAIN]) newAchievement("mainSilver");
	if (goldLevelsByCategory[MAIN] >= totalLevelsByCategory[MAIN]) newAchievement("mainGold");
}

//Update tutorial specified achievements.
//Make sure the level progress of tutorial is correct.
void StatisticsManager::updateTutorialAchievements(){
	//find tutorial level pack
	LevelPackManager *lpm=getLevelPackManager();
	LevelPack *levels=lpm->getTutorialLevelPack();
	if(levels==NULL) return;

	bool tutorialFinished=true,tutorialIsGold=true;
	tutorialLevels=levels->getLevelCount();
	tutorialCompleted=tutorialGold=0;

	for(int n=0,m=levels->getLevelCount();n<m;n++){
		int medal = levels->getLevel(n)->getMedal();
		if(medal){
			tutorialCompleted++;

			if(medal!=3) tutorialIsGold=false;
			else tutorialGold++;
		}else{
			tutorialFinished=tutorialIsGold=false;
		}
	}

	//upadte achievements
	updateTutorialAchievementsInternal((tutorialFinished?1:0)|(tutorialIsGold?2:0));
}

//internal function
//flags: a bit-field value indicates which achievements we have.
void StatisticsManager::updateTutorialAchievementsInternal(int flags){
	if(flags&1) newAchievement("tutorial");
	if(flags&2) newAchievement("tutorialGold");
}
