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

#ifndef ACHIEVEMENTLIST_H
#define ACHIEVEMENTLIST_H

#include <SDL_rect.h>

struct SDL_Surface;

enum AchievementDisplayStyle{
	ACHIEVEMENT_HIDDEN,
	ACHIEVEMENT_TITLE,
	ACHIEVEMENT_ALL,
	ACHIEVEMENT_PROGRESS,
};

//internal struct for achievement info
struct AchievementInfo{
	//achievement id for save to statistics file
	const char* id;
	//achievement name for display
	const char* name;
	//achievement image. NULL for no image. will be loaded at getDataPath()+imageFile
	const char* imageFile;
	//image offset and size.
	SDL_Rect r;
	//achievement description. supports multi-line text
	const char* description;
	//display style
	AchievementDisplayStyle displayStyle;

	//SDL_Surface of achievement image. Should always be NULL before loading.
	SDL_Surface* imageSurface;
};

extern int achievementVersion;
extern AchievementInfo achievementList[];

// The "Cheater" achievement is defined separately.
extern AchievementInfo achievementCheat;

//internal struct for achievement unlock info
struct AchievementUnlockInfo {
	//achievement id
	const char* id;
	//id of the achievement to be unlocked
	const char* unlockId;
	//new display style of achievement to be unlocked
	AchievementDisplayStyle displayStyle;
};

extern AchievementUnlockInfo achievementUnlockList[];

#endif
