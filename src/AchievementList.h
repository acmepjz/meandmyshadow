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

// NOTE: This is an internal file for all avaliable achievements.
// Don't include it in other files!

// Format: {id, name, file, pos, description, [type]}
// id: Identifier of achievement. Should not change once the new achievement is added,
//   otherwise the old statistics will be lost.
// name: Name of achievement. Translatable.
// file: The icon file. NULL for no icon.
// pos: Specifies which part of the icon will be displayed (left, top, width, height).
// description: Description of achievement. Translatable. Can be multi-line text.
// type [optional]: Specifies the display type of achievement.
//   ACHIEVEMENT_HIDDEN [default]: Show "Unknown achievement" when unfinished.
//   ACHIEVEMENT_TITLE: Only show icon and title when unfinished.
//   ACHIEVEMENT_ALL: Always show icon, title and description.
//   ACHIEVEMENT_PROGRESS: Show icon, title and description and a progress bar.
//     StatisticsManager::getAchievementProgress() function should return the progress (between 0 and 1).

AchievementInfo achievementList[]={
	{"newbie",__("Newbie"),"themes/Cloudscape/characters/player.png",{0,0,23,40},__("Complete a level."),ACHIEVEMENT_ALL},
	{"experienced",__("Experienced player"),"themes/Cloudscape/characters/player.png",{0,0,23,40},__("Complete 50 levels."),ACHIEVEMENT_PROGRESS},
	{"goodjob",__("Good job!"),"gfx/medals.png",{60,0,30,30},__("Receive a gold medal."),ACHIEVEMENT_ALL},
	{"expert",__("Expert"),"gfx/medals.png",{60,0,30,30},__("Earn 50 gold medal."),ACHIEVEMENT_PROGRESS},

	{"tutorial",__("Graduate"),"gfx/medals.png",{60,0,30,30},__("Complete the tutorial level pack."),ACHIEVEMENT_PROGRESS},
	{"tutorialGold",__("Outstanding graduate"),"gfx/medals.png",{60,0,30,30},__("Complete the tutorial level pack with gold for all levels."),ACHIEVEMENT_PROGRESS},

	{"addicted",__("Hooked"),"themes/Cloudscape/characters/player.png",{0,0,23,40},__("Play Me and My Shadow for more than 2 hours.")},
	{"loyalFan",__("Loyal fan of Me and My Shadow"),"themes/Cloudscape/characters/player.png",{0,0,23,40},__("Play Me and My Shadow for more than 24 hours.")},

	{"constructor",__("Constructor"),"gfx/gui.png",{112,16,16,16},__("Use the level editor for more than 2 hours.")},
	{"constructor2",__("The creator"),"gfx/gui.png",{112,16,16,16},__("Use the level editor for more than 24 hours.")},

	{"create1",__("Look, cute level!"),"gfx/gui.png",{112,16,16,16},__("Create a level for the first time."),ACHIEVEMENT_ALL},
	{"create50",__("The level museum"),"gfx/gui.png",{112,16,16,16},__("Create 50 levels."),ACHIEVEMENT_PROGRESS},

	{"frog",__("Frog"),"themes/Cloudscape/characters/player.png",{0,0,23,40},__("Jump 1000 times."),ACHIEVEMENT_PROGRESS},

	{"travel100",__("Wanderer"),"themes/Cloudscape/characters/player.png",{0,0,23,40},__("Travel 100 meters."),ACHIEVEMENT_PROGRESS},
	{"travel1k",__("Runner"),"themes/Cloudscape/characters/player.png",{0,0,23,40},__("Travel 1 kilometer."),ACHIEVEMENT_PROGRESS},
	{"travel10k",__("Long distance runner"),"themes/Cloudscape/characters/player.png",{0,0,23,40},__("Travel 10 kilometers."),ACHIEVEMENT_PROGRESS},
	{"travel42k",__("Marathon runner"),"themes/Cloudscape/characters/player.png",{0,0,23,40},__("Travel 42,195 meters."),ACHIEVEMENT_PROGRESS},

	{"die1",__("Be careful!"),"themes/Cloudscape/characters/deathright.png",{0,14,23,40},__("Die for the first time."),ACHIEVEMENT_ALL},
	{"die50",__("It doesn't matter..."),"themes/Cloudscape/characters/deathright.png",{0,14,23,40},__("Die 50 times.")},
	{"die1000",__("Expert of trial and error"),"themes/Cloudscape/characters/deathright.png",{0,14,23,40},__("Die 1000 times.")},

	{"squash1",__("Keep an eye for moving blocks!"),"themes/Cloudscape/characters/deathright.png",{0,14,23,40},__("Get squashed for the first time.")},
	{"squash50",__("Potato masher"),"themes/Cloudscape/characters/deathright.png",{0,14,23,40},__("Get squashed 50 times.")},

	{"doubleKill",__("Double kill"),"themes/Cloudscape/characters/deathright.png",{0,14,23,40},__("Get both the player and the shadow dead.")},

	{"die5in5",__("Bad luck"),"themes/Cloudscape/characters/deathright.png",{0,14,23,40},__("Die 5 times in under 5 seconds.")},
	{"die10in5",__("This level is too dangerous"),"themes/Cloudscape/characters/deathright.png",{0,14,23,40},__("Die 10 times in under 5 seconds.")},

	{"forget",__("You forgot your friend"),"themes/Cloudscape/characters/player.png",{0,0,23,40},__("Finish the level with the player or the shadow dead.")},
	{"jit",__("Just in time"),"themes/Cloudscape/characters/player.png",{0,0,23,40},__("Reach the exit with the player and the shadow simultaneously.")},

	{"record100",__("Recorder"),"themes/Cloudscape/characters/player.png",{0,0,23,40},__("Record 100 times."),ACHIEVEMENT_PROGRESS},
	{"record1k",__("Shadowmaster"),"themes/Cloudscape/characters/shadow.png",{0,0,23,40},__("Record 1000 times."),ACHIEVEMENT_PROGRESS},

	{"switch100",__("Switch puller"),"themes/Cloudscape/characters/player.png",{0,0,23,40},__("Pull the switch 100 times."),ACHIEVEMENT_PROGRESS},
	{"switch1k",__("The switch is broken!"),"themes/Cloudscape/characters/player.png",{0,0,23,40},__("Pull the switch 1000 times.")},

	{"swap100",__("Swapper"),"themes/Cloudscape/characters/player.png",{0,0,23,40},__("Swap 100 times."),ACHIEVEMENT_PROGRESS},
	{"swap1k",__("Player to shadow to player to shadow..."),"themes/Cloudscape/characters/player.png",{0,0,23,40},__("Swap 1000 times.")},

	{"save1k",__("Play it save"),"themes/Cloudscape/characters/player.png",{0,0,23,40},__("Save 1000 times.")},
	{"load1k",__("This game is too hard"),"themes/Cloudscape/characters/player.png",{0,0,23,40},__("Load the game 1000 times.")},

	{"panicSave",__("Panic save"),"themes/Cloudscape/characters/player.png",{0,0,23,40},__("Save twice in 1 second.")},
	{"panicLoad",__("Panic load"),"themes/Cloudscape/characters/player.png",{0,0,23,40},__("Load twice in 1 second.")},

	{"loadAndDie",__("Bad saving position"),"themes/Cloudscape/characters/deathright.png",{0,14,23,40},__("Load the game and die within 1 second.")},
	{"loadAndDie100",__("This level is too hard"),"themes/Cloudscape/characters/deathright.png",{0,14,23,40},__("Load the same save and die 100 times.")},

	{"quickswap",__("Quick swap"),"themes/Cloudscape/characters/player.png",{0,0,23,40},__("Swap twice in under a second.")},

	//ripped from Achievements Unlocked
	{"horizontal",__("Horizontal confusion"),"gfx/emotions.png",{0,0,23,40},__("Press left and right simultaneously.")},

	{"programmer",__("Programmer"),"gfx/gui.png",{112,16,16,16},__("Play the development version of Me and My Shadow."),ACHIEVEMENT_TITLE},

	//end of achievements
	{}
};
