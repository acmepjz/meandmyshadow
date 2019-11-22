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

#ifndef STATISTICSMANAGER_H
#define STATISTICSMANAGER_H

#include <SDL.h>
#include <string>
#include <vector>
#include <array>
#include <map>
#include <time.h>

#include "Render.h"
#include "AchievementList.h"

struct OwnedAchievement{
	time_t achievedTime;
	AchievementInfo* info;
};

class StatisticsScreen;

class StatisticsManager{
	friend class StatisticsScreen;
public:
	//Player and shadow traveling distance (m), 1 block = 1 meter
	float playerTravelingDistance,shadowTravelingDistance;
	//Player and shadow pushing distance (m), 1 block = 1 meter
	float playerPushingDistance, shadowPushingDistance;
	//Player and shadow jumps
	int playerJumps,shadowJumps;
	//Player and shadow carries
	int playerCarries,shadowCarries;
	//Player and shadow dies
	int playerDies,shadowDies;
	//Player and shadow squashed
	int playerSquashed,shadowSquashed;
	//Completed levels. NOTE: this is dynamically calculated, and doesn't save to file.
	int completedLevels,silverLevels,goldLevels,totalLevels;
	//Completed levels by category. NOTE: this is dynamically calculated, and doesn't save to file.
	std::array<int, 3> completedLevelsByCategory, silverLevelsByCategory, goldLevelsByCategory, totalLevelsByCategory;
	//Completed levelpacks (excluding individual levels). NOTE: this is dynamically calculated, and doesn't save to file.
	int completedLevelpacks, silverLevelpacks, goldLevelpacks, totalLevelpacks;
	//Completed levelpacks by category. NOTE: this is dynamically calculated, and doesn't save to file.
	std::array<int, 3> completedLevelpacksByCategory, silverLevelpacksByCategory, goldLevelpacksByCategory, totalLevelpacksByCategory;
	//Record times
	int recordTimes;
	//number of switched pulled
	int playerSwitchTimes, shadowSwitchTimes;
	//swap times
	int playerSwapTimes, shadowSwapTimes;
	//save and load times
	int playerSaveTimes, shadowSaveTimes, loadTimes;
	//collectible collected
	int playerCollectibleCollected, shadowCollectibleCollected;
	//fragile blocks broken
	int playerFragileBlocksBroken, shadowFragileBlocksBroken;
	//pushable blocks broken
	int pushableBlocksBroken;
	//play time (seconds)
	int playTime;
	//level edit time (seconds)
	int levelEditTime;
	//created levels
	int createdLevels;
	//cheat times
	int cheatTimes;

	//achievement display style obtained from unlock info
	std::map<std::string, int> achievementDisplayStyle;
private:
	//current achievement displayed time
	int achievementTime;
	//some picture
    SharedTexture bmDropShadow;
    SDL_Surface* bmQuestionMark;
	//SDL_Surface for current achievement (excluding drop shadow)
    //SDL_Surface *bmAchievement;
    SharedTexture bmAchievement;
	//currently owned achievements
	std::map<std::string,OwnedAchievement> achievements;
	//available achievements
	static std::map<std::string, AchievementInfo*> avaliableAchievements;
	//queued achievements for display
	std::vector<std::pair<AchievementInfo*,bool> > queuedAchievements;
	//currently displayed achievement
	int currentAchievement;
	//starting time
	time_t startTime;
	//level edit starting time
	time_t levelEditStartTime;
	//statistics for tutorial level pack
	int tutorialLevels,tutorialCompleted,tutorialGold;
public:
	StatisticsManager();

	int getTotalAchievements() const;
	int getCurrentNumberOfAchievements() const;

	//clear the statistics and achievements.
	void clear();
	//load needed picture
    void loadPicture(SDL_Renderer &renderer, ImageManager &imageManager);
	//register avaliable achievements
    static void registerAchievements(ImageManager& imageManager);
	//load statistics file.
	void loadFile(const std::string& fileName);
	//save statistics file.
	void saveFile(const std::string& fileName);
	//add or display a new achievement.
	//name: the achievement id. if can't find it in avaliable achievement, nothing happens.
	//save: if true then save to currently owned achievements. if it already exists in
	//currently owned achievements, nothing happens.
	//if false then just added it to queue, including duplicated achievements.
	void newAchievement(const std::string& id,bool save=true);
	//if there are new achievements, draw it on the screen,
	//otherwise do nothing.
    void render(ImageManager&,SDL_Renderer& renderer);

	//get the achieved time of an achievement. 0 means not achieved yet.
	time_t achievedTime(const std::string& id);

	//Update the achievement display style from unlock info.
	void updateAchievementDisplayStyle();

	//Call this function to update completed levels.
	//NOTE: Level progress files are reloaded, so it's slow.
	void reloadCompletedLevelsAndAchievements();

	//Call this function to update other achievements at game startup.
	void reloadOtherAchievements();

	//Update level specified achievements.
	//Make sure the completed level count is correct.
	void updateLevelAchievements();

	//Update tutorial specified achievements.
	//Make sure the level progress of tutorial is correct.
	void updateTutorialAchievements();

	//Call when level edit is start
	void startLevelEdit();

	//Call when level edit is end
	void endLevelEdit();

	//update in-game time
	void updatePlayTime();

    //create a SharedTexture contains specified achievements or draw to existing surface.
    //renderer: renderer to create the texture on.
	//info: achievement info.
	//rect [in, out, optional]: specifies position and optionally width to draw on. height will be returned.
	//  if NULL then will be drawn on top-left corner. if surface is NULL then rect->x and rect->y are ignored.
	//isPopup: if it's the popup notification show on the top right corner of the screen
	//achievedTime: if we should show achieved time (and progress bar if AchievementInfo specifies) and when is it.
	//  NOTE: if isPopup=true then this argument does nothing.
	//showTip: shows "New achievement" tip
	//  NOTE: if isPopup=false then this argument does nothing.
	//return value: A texture that contains the specified achievements or NULL if any error occured.
    SharedTexture createAchievementSurface(SDL_Renderer& renderer, AchievementInfo* info,SDL_Rect* rect=NULL,bool isPopup=true,const time_t *achievedTime=NULL,bool showTip=true);
private:
	//internal function
	//flags: a bit-field value indicates which achievements we have.
	void updateTutorialAchievementsInternal(int flags);
	//internal function. alpha should be 1-5, 5 means fully opaque (not really)
    void drawAchievement(SDL_Renderer& renderer, int alpha);
	//internal function for get progress (in percent, 0-100)
	float getAchievementProgress(AchievementInfo* info);
};

extern StatisticsManager statsMgr;

#endif
