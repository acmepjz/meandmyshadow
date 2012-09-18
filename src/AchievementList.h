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

// Note: This is an internal file for all avaliable achievements.
// Don't include it in other files!

AchievementInfo achievementList[]={
	{"newbie",__("Newbie"),"themes/Cloudscape/player.png",{0,0,23,40},__("Congratulations, you completed one level!"),ACHIEVEMT_TITLE},
	{"experienced",__("Experienced player"),"themes/Cloudscape/player.png",{0,0,23,40},__("Completed 50 levels."),ACHIEVEMT_PROGRESS},
	{"goodjob",__("Good job!"),"gfx/medals.png",{60,0,30,30},__("Get your first gold medal."),ACHIEVEMT_ALL},
	{"expert",__("Expert"),"gfx/medals.png",{60,0,30,30},__("Earned 50 gold medal."),ACHIEVEMT_PROGRESS},

	{"tutorial",__("Graduate"),"gfx/medals.png",{60,0,30,30},__("Complete the tutorial level pack."),ACHIEVEMT_PROGRESS},
	{"tutorialGold",__("Outstanding graduate"),"gfx/medals.png",{60,0,30,30},__("Complete the tutorial level pack with all levels gold medal."),ACHIEVEMT_PROGRESS},

	{"addicted",__("Addicted"),"themes/Cloudscape/player.png",{0,0,23,40},__("Played Me and My Shadow for more than 2 hours.")},
	{"loyalFan",__("Me and My Shadow loyal fan"),"themes/Cloudscape/player.png",{0,0,23,40},__("Played Me and My Shadow for more than 24 hours.")},

	{"constructor",__("Constructor"),"gfx/gui.png",{112,16,16,16},__("Use the level editor for more than 2 hours.")},
	{"constructor2",__("The creator"),"gfx/gui.png",{112,16,16,16},__("Use the level editor for more than 24 hours.")},

	{"create1",__("Look, cute level!"),"gfx/gui.png",{112,16,16,16},__("Created your first level."),ACHIEVEMT_ALL},
	{"create50",__("The level museum"),"gfx/gui.png",{112,16,16,16},__("Created 50 levels."),ACHIEVEMT_PROGRESS},

	{"frog",__("Frog"),"themes/Cloudscape/player.png",{0,0,23,40},__("Jump for 1000 times."),ACHIEVEMT_PROGRESS},

	{"travel100",__("Wanderer"),"themes/Cloudscape/player.png",{0,0,23,40},__("Traveled for 100 meter."),ACHIEVEMT_PROGRESS},
	{"travel1k",__("Runner"),"themes/Cloudscape/player.png",{0,0,23,40},__("Traveled for 1 kilometer."),ACHIEVEMT_PROGRESS},
	{"travel10k",__("Long runner"),"themes/Cloudscape/player.png",{0,0,23,40},__("Traveled for 10 kilometer."),ACHIEVEMT_PROGRESS},
	{"travel42k",__("Marathon runner"),"themes/Cloudscape/player.png",{0,0,23,40},__("Traveled for 42,195 meter."),ACHIEVEMT_PROGRESS},

	{"die1",__("Be careful!"),"themes/Cloudscape/deathright.png",{0,14,23,40},__("The first death."),ACHIEVEMT_ALL},
	{"die50",__("It doesn't matter..."),"themes/Cloudscape/deathright.png",{0,14,23,40},__("Died for 50 times.")},
	{"die1000",__("Expert of trial and error"),"themes/Cloudscape/deathright.png",{0,14,23,40},__("Died for 1000 times.")},

	{"squash1",__("Keep an eye for moving walls!"),"themes/Cloudscape/deathright.png",{0,14,23,40},__("First time being squashed.")},
	{"suqash50",__("Potato masher"),"themes/Cloudscape/deathright.png",{0,14,23,40},__("Squashed for 50 times.")},

	{"doubleKill",__("Double kill"),"themes/Cloudscape/deathright.png",{0,14,23,40},__("Make both player and shadow die.")},

	{"record100",__("Recorder"),"themes/Cloudscape/player.png",{0,0,23,40},__("Record for 100 times."),ACHIEVEMT_TITLE},
	{"record1k",__("Shadowmaster"),"themes/Cloudscape/shadow.png",{0,0,23,40},__("Record for 1000 times."),ACHIEVEMT_TITLE},

	//ripped from Achievements Unlocked
	{"horizontal",__("Horizontal confusion"),"gfx/emotions.png",{0,0,23,40},__("Press left and right button simultaneously.")},

	{"programmer",__("Programmer"),"gfx/gui.png",{112,16,16,16},__("Played the development version of Me and My Shadow."),ACHIEVEMT_TITLE},

	//end of achievements
	{}
};
