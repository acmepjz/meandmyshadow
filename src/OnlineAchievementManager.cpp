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

#include "OnlineAchievementManager.h"
#include "AchievementList.h"
#include "FileManager.h"
#include "string.h"
#include <algorithm>

#ifdef GAMERZILLA
#include "gamerzilla.h"
#endif

OnlineAchievementManager::OnlineAchievementManager(){
#ifdef GAMERZILLA
	game_id = -1;
#endif
}

OnlineAchievementManager::~OnlineAchievementManager(){
#ifdef GAMERZILLA
	GamerzillaQuit();
#endif
}

void OnlineAchievementManager::registerAchievements(){
#ifdef GAMERZILLA
    GamerzillaStart(false, (getUserPath(USER_DATA) + "/gamerzilla/").c_str());
    Gamerzilla g;
    std::string path = getDataPath() + "/gamerzilla/";
    std::string main_image = path + "meandmyshadow.png";
    std::string true_image = path + "achievement1.png";
    std::string false_image = path + "notachieved.png";
    GamerzillaInitGame(&g);
    g.version = 1;
    g.short_name = strdup("meandmyshadow");
    g.name = strdup("Me & My Shadow");
    g.image = strdup(main_image.c_str());
	for(int i=0;achievementList[i].id!=NULL;i++){
		std::string id = achievementList[i].id;
		std::transform(id.begin(), id.end(), id.begin(),	[](unsigned char c){ return std::tolower(c); });
		GamerzillaGameAddTrophy(&g, achievementList[i].name, achievementList[i].description, 0, (path + id + ".png").c_str(), false_image.c_str());
    }
    game_id = GamerzillaSetGame(&g);
    GamerzillaClearGame(&g);
#endif
}

void OnlineAchievementManager::setAchievement(const std::string &name){
#ifdef GAMERZILLA
	if (game_id != -1)
		GamerzillaSetTrophy(game_id, name.c_str());
#endif
}
