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

#include "AchievementList.h"

// We redefined it here to prevent including more header files.
#ifndef __
#define __(X) (X)
#endif

// Version must be incremented when more achievements are added.
int achievementVersion = 1;

// Format: {<id>, <name>, <file>, <pos>, <description>, <type>, nullptr}
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
// NOTE: WARNING: All arguments should now be specified, as surface being nullptr is being relied upon.

AchievementInfo achievementList[]={
	{"newbie",__("Newbie"),"gfx/medals.png",{0,0,30,30},__("Complete a level."),ACHIEVEMENT_ALL,nullptr},
	{"tutorial",__("Graduate"),"gfx/achievements/tutorial.png",{0,0,51,27},__("Complete the tutorial level pack."),ACHIEVEMENT_PROGRESS,nullptr},
	{"experienced",__("Experienced player"),"gfx/achievements/experienced.png",{0,0,51,51},__("Complete 50 levels."),ACHIEVEMENT_PROGRESS,nullptr},

	{ "complete_levelpack", __("You did it!"), "gfx/achievements/complete_levelpack.png", { 0, 0, 51, 27 }, __("Complete any level pack besides tutorial."), ACHIEVEMENT_ALL, nullptr },

	{"goodjob",__("Good job!"),"gfx/medals.png",{60,0,30,30},__("Receive a gold medal."),ACHIEVEMENT_ALL,nullptr},
	{"tutorialGold",__("Outstanding graduate"),"gfx/achievements/tutorialgold.png",{0,0,51,27},__("Complete the tutorial level pack with gold for all levels."),ACHIEVEMENT_PROGRESS,nullptr},
	{"expert",__("Expert"),"gfx/achievements/expert.png",{0,0,51,51},__("Earn 50 gold medals."),ACHIEVEMENT_PROGRESS,nullptr},

	{ "mainBronze", __("Victory"), "gfx/emotions.png", { 0, 80, 23, 40 }, __("Complete all official levels."), ACHIEVEMENT_PROGRESS, nullptr },
	{ "mainSilver", __("Great victory"), "gfx/emotions.png", { 23, 80, 23, 40 }, __("Complete all official levels with silver or gold."), ACHIEVEMENT_PROGRESS, nullptr },
	{ "mainGold", __("Epic victory"), "gfx/emotions.png", { 46, 80, 23, 40 }, __("Complete all official levels with gold."), ACHIEVEMENT_PROGRESS, nullptr },

	{ "survivalist", __("Survivalist"), "gfx/emotions.png", { 0, 40, 23, 40 }, __("Complete 10 levels in row without dying."), ACHIEVEMENT_PROGRESS, nullptr },
	{ "survivalist2", __("Expert survivalist"), "gfx/emotions.png", { 46, 40, 23, 40 }, __("Complete the default level pack from start to finish without dying or resetting once."), ACHIEVEMENT_ALL, nullptr },

	{"addicted",__("Hooked"),"gfx/achievements/addicted.png",{0,0,50,50},__("Play Me and My Shadow for more than 2 hours."),ACHIEVEMENT_TITLE,nullptr},
	{"loyalFan",__("Loyal fan of Me and My Shadow"),"gfx/achievements/loyalfan.png",{0,0,50,50},__("Play Me and My Shadow for more than 24 hours."),ACHIEVEMENT_HIDDEN,nullptr},

	{"constructor",__("Constructor"),"gfx/achievements/constructor.png",{0,0,50,50},__("Use the level editor for more than 2 hours."),ACHIEVEMENT_HIDDEN,nullptr},
	{"constructor2",__("The creator"),"gfx/achievements/constructor2.png",{0,0,50,50},__("Use the level editor for more than 8 hours."),ACHIEVEMENT_HIDDEN,nullptr},

	{"create1",__("Look, cute level!"),"gfx/achievements/create1.png",{0,0,50,50},__("Create a level for the first time."),ACHIEVEMENT_ALL,nullptr},
	{"create10",__("The level museum"),"gfx/achievements/create10.png",{0,0,50,50},__("Create 10 levels."),ACHIEVEMENT_PROGRESS,nullptr},

	{ "helloworld", __("Hello, World!"), "gfx/achievements/helloworld.png", { 0, 0, 50, 50 }, __("Write a script for the first time."), ACHIEVEMENT_TITLE, nullptr },

	{ "jump100", __("Frog"), "gfx/achievements/jump100.png", { 0, 0, 50, 50 }, __("Jump 100 times."), ACHIEVEMENT_PROGRESS, nullptr },
	{ "jump1k", __("Kangaroo"), "gfx/achievements/jump1k.png", { 0, 0, 50, 50 }, __("Jump 1000 times."), ACHIEVEMENT_PROGRESS, nullptr },

	{ "carry100", __("Carrier"), "gfx/achievements/carrier.png", { 0, 0, 50, 50 }, __("Player or shadow hold other 100 times."), ACHIEVEMENT_PROGRESS, nullptr },
	{ "carry1k", __("Acrobat"), "gfx/achievements/acrobat.png", { 0, 0, 50, 50 }, __("Player or shadow hold other 1000 times."), ACHIEVEMENT_PROGRESS, nullptr },

	{"travel100",__("Wanderer"),"gfx/achievements/travel100.png",{0,0,50,50},__("Travel 100 meters."),ACHIEVEMENT_PROGRESS,nullptr},
	{"travel1k",__("Runner"),"gfx/achievements/travel1k.png",{0,0,50,50},__("Travel 1 kilometer."),ACHIEVEMENT_PROGRESS,nullptr},
	{"travel10k",__("Long distance runner"),"gfx/achievements/travel10k.png",{0,0,50,50},__("Travel 10 kilometers."),ACHIEVEMENT_PROGRESS,nullptr},
	{"travel42k",__("Marathon runner"),"gfx/achievements/travel42k.png",{0,0,50,50},__("Travel 42,195 meters."),ACHIEVEMENT_PROGRESS,nullptr},

	{ "push100", __("Sokoban"), "themes/Cloudscape/tiles/tiles.png", { 0, 150, 50, 50 }, __("Push blocks for 100 meters."), ACHIEVEMENT_PROGRESS, nullptr },
	{ "push1k", __("Box delivering company"), "gfx/achievements/box_delivery_company.png", { 0, 0, 50, 50 }, __("Push blocks for 1 kilometer."), ACHIEVEMENT_HIDDEN, nullptr },

	{ "boxcrush1", __("I'm not intentional"), "gfx/achievements/broken_box.png", { 0, 0, 50, 50 }, __("Break a pushable block."), ACHIEVEMENT_HIDDEN, nullptr },
	{ "boxcrush100", __("Only pieces of wood left"), "gfx/achievements/broken_box2.png", { 0, 0, 50, 50 }, __("Break 100 pushable blocks."), ACHIEVEMENT_HIDDEN, nullptr },

	{"die1",__("Be careful!"),"themes/Cloudscape/characters/deathright.png",{0,14,23,40},__("Die for the first time."),ACHIEVEMENT_ALL,nullptr},
	{"die50",__("It doesn't matter..."),"gfx/achievements/die50.png",{0,0,50,50},__("Die 50 times."),ACHIEVEMENT_HIDDEN,nullptr},
	{"die1000",__("Expert of trial and error"),"gfx/achievements/die1000.png",{0,0,50,50},__("Die 1000 times."),ACHIEVEMENT_HIDDEN,nullptr},

	{"squash1",__("Keep an eye for moving blocks!"),"gfx/achievements/squash1.png",{0,0,50,50},__("Get squashed for the first time."),ACHIEVEMENT_HIDDEN,nullptr},
	{"squash50",__("Potato masher"),"gfx/achievements/squash50.png",{0,0,50,50},__("Get squashed 50 times."),ACHIEVEMENT_HIDDEN,nullptr},

	{"doubleKill",__("Double kill"),"gfx/achievements/doublekill.png",{0,0,50,50},__("Get both the player and the shadow dead."),ACHIEVEMENT_HIDDEN,nullptr},

	{"die5in5",__("Bad luck"),"gfx/achievements/die5in5.png",{0,0,50,50},__("Die 5 times in under 5 seconds."),ACHIEVEMENT_TITLE,nullptr},
	{"die10in5",__("This level is too dangerous"),"gfx/achievements/die10in5.png",{0,0,50,50},__("Die 10 times in under 5 seconds."),ACHIEVEMENT_HIDDEN,nullptr},

	{"forget",__("You forgot your friend"),"gfx/achievements/forget.png",{0,0,45,53},__("Finish the level with the player or the shadow dead."),ACHIEVEMENT_HIDDEN,nullptr},
	{"jit",__("Just in time"),"gfx/achievements/jit.png",{0,0,50,50},__("Reach the exit with the player and the shadow simultaneously."),ACHIEVEMENT_TITLE,nullptr},
	{ "duel", __("Duel"), "gfx/achievements/duel.png", { 0, 0, 50, 50 }, __("Player and shadow push the same pushable block in opposite directions."), ACHIEVEMENT_HIDDEN, nullptr },

	{"record100",__("Recorder"),"gfx/achievements/record100.png",{0,0,50,50},__("Record 100 times."),ACHIEVEMENT_PROGRESS,nullptr},
	{"record1k",__("Shadowmaster"),"themes/Cloudscape/characters/shadow.png",{23,0,23,40},__("Record 1000 times."),ACHIEVEMENT_PROGRESS,nullptr},

	{ "underpar", __("Under par"), "gfx/achievements/under_par.png", { 0, 0, 50, 50 }, __("Complete a level with recordings less than the target recordings."), ACHIEVEMENT_HIDDEN, nullptr },

	{ "fragile1", __("It just breaks"), "themes/Cloudscape/tiles/tiles.png", { 350, 0, 50, 50 }, __("Break a fragile block."), ACHIEVEMENT_ALL, nullptr },
	{ "fragile100", __("Block breaker"), "gfx/achievements/broken_fragile.png", { 0, 0, 50, 50 }, __("Break 100 fragile blocks."), ACHIEVEMENT_PROGRESS, nullptr },

	{"switch100",__("Switch puller"),"themes/Cloudscape/tiles/tiles.png",{100,100,50,50},__("Pull the switch 100 times."),ACHIEVEMENT_PROGRESS,nullptr},
	{"switch1k",__("The switch is broken!"),"gfx/achievements/switch1k.png",{0,0,50,50},__("Pull the switch 1000 times."),ACHIEVEMENT_HIDDEN,nullptr},

	{"swap100",__("Swapper"),"themes/Cloudscape/tiles/swap.png",{0,0,50,50},__("Swap 100 times."),ACHIEVEMENT_PROGRESS,nullptr},

	{"save100",__("Play it save"),"gfx/achievements/save1k.png",{0,0,50,50},__("Save 100 times."),ACHIEVEMENT_HIDDEN,nullptr},
	{"load100",__("This game is too hard"),"gfx/achievements/load1k.png",{0,0,50,50},__("Load the game 100 times."),ACHIEVEMENT_HIDDEN,nullptr},

	{ "collect100", __("Enriched"), "themes/Cloudscape/tiles/tiles.png", { 50, 150, 50, 50 }, __("Collect 100 collectibles."), ACHIEVEMENT_PROGRESS, nullptr },
	{ "collect1k", __("Treasure hunter"), "gfx/achievements/collect1k.png", { 0, 0, 50, 50 }, __("Collect 1000 collectibles."), ACHIEVEMENT_HIDDEN, nullptr },

	{ "withoutsave", __("No, thanks"), "gfx/achievements/withoutsave.png", { 0, 0, 50, 50 }, __("Complete a level with checkpoint, but without saving."), ACHIEVEMENT_TITLE, nullptr },

	{"panicSave",__("Panic save"),"gfx/achievements/panicsave.png",{0,0,50,50},__("Save twice in 1 second."),ACHIEVEMENT_HIDDEN,nullptr},
	{"panicLoad",__("Panic load"),"gfx/achievements/panicload.png",{0,0,50,50},__("Load twice in 1 second."),ACHIEVEMENT_HIDDEN,nullptr},

	{"loadAndDie",__("Bad saving position"),"gfx/achievements/loadanddie.png",{0,0,50,50},__("Load the game and die within 1 second."),ACHIEVEMENT_TITLE,nullptr},
	{"loadAndDie100",__("This level is too hard"),"gfx/achievements/loadanddie100.png",{0,0,50,50},__("Load the same save and die 100 times."),ACHIEVEMENT_HIDDEN,nullptr},

	{"quickswap",__("Quick swap"),"gfx/achievements/quickswap.png",{0,0,50,50},__("Swap twice in under a second."),ACHIEVEMENT_HIDDEN,nullptr},

	//ripped from Achievements Unlocked
	{"horizontal",__("Horizontal confusion"),"gfx/emotions.png",{0,0,23,40},__("Press left and right simultaneously."),ACHIEVEMENT_HIDDEN,nullptr},

	{"programmer",__("Programmer"),"gfx/achievements/programmer.png",{0,0,50,50},__("Play the development version of Me and My Shadow."),ACHIEVEMENT_TITLE,nullptr},

	//end of achievements
	{}
};

// The "Cheater" achievement is defined separately.
AchievementInfo achievementCheat = { "cheat", __("Cheater"), "gfx/achievements/cheat.png", { 0, 0, 50, 50 }, __("Cheat in game."), ACHIEVEMENT_ALL, nullptr };

AchievementUnlockInfo achievementUnlockList[] = {
	{ "addicted", "loyalFan", ACHIEVEMENT_TITLE },

	{ "create1", "constructor", ACHIEVEMENT_TITLE },
	{ "create10", "constructor", ACHIEVEMENT_ALL },
	{ "constructor", "constructor2", ACHIEVEMENT_TITLE },

	{ "create10", "helloworld", ACHIEVEMENT_ALL },
	{ "constructor", "helloworld", ACHIEVEMENT_ALL },
	{ "programmer", "helloworld", ACHIEVEMENT_ALL },

	{ "push100", "push1k", ACHIEVEMENT_TITLE },

	{ "die1", "die50", ACHIEVEMENT_TITLE },
	{ "die50", "die1000", ACHIEVEMENT_TITLE },

	{ "die50", "squash1", ACHIEVEMENT_TITLE },
	{ "die1000", "squash1", ACHIEVEMENT_ALL },
	{ "die1000", "forget", ACHIEVEMENT_ALL },
	{ "squash1", "squash50", ACHIEVEMENT_TITLE },
	{ "squash50", "loadAndDie100", ACHIEVEMENT_TITLE },

	{ "switch100", "switch1k", ACHIEVEMENT_TITLE },

	{ "swap100", "quickswap", ACHIEVEMENT_TITLE },

	{ "save100", "withoutsave", ACHIEVEMENT_ALL },
	{ "save100", "panicSave", ACHIEVEMENT_TITLE },

	{ "load100", "panicLoad", ACHIEVEMENT_TITLE },

	{ "collect100", "collect1k", ACHIEVEMENT_TITLE },

	{ "panicSave", "panicLoad", ACHIEVEMENT_TITLE },
	{ "panicLoad", "panicSave", ACHIEVEMENT_TITLE },

	{ "expert", "jit", ACHIEVEMENT_ALL },

	{ "forget", "doubleKill", ACHIEVEMENT_TITLE },
	{ "forget", "duel", ACHIEVEMENT_TITLE },

	{ "jit", "duel", ACHIEVEMENT_TITLE },

	{ "travel10k", "horizontal", ACHIEVEMENT_TITLE },
	{ "travel42k", "horizontal", ACHIEVEMENT_ALL },

	{ "newbie", "underpar", ACHIEVEMENT_TITLE },
	{ "goodjob", "underpar", ACHIEVEMENT_ALL },

	//end of list
	{}
};
