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
	{"newbie",__("Newbie"),"themes/Cloudscape/player.png",{0,0,23,40},__("Congratulations, you completed one level!"),ACHIEVEMENT_TITLE},
	{"experienced",__("Experienced player"),"themes/Cloudscape/player.png",{0,0,23,40},__("Completed 50 levels."),ACHIEVEMENT_PROGRESS},
	{"goodjob",__("Good job!"),"gfx/medals.png",{60,0,30,30},__("Get your first gold medal."),ACHIEVEMENT_ALL},
	{"expert",__("Expert"),"gfx/medals.png",{60,0,30,30},__("Earned 50 gold medal."),ACHIEVEMENT_PROGRESS},

	{"tutorial",__("Graduate"),"gfx/medals.png",{60,0,30,30},__("Complete the tutorial level pack."),ACHIEVEMENT_PROGRESS},
	{"tutorialGold",__("Outstanding graduate"),"gfx/medals.png",{60,0,30,30},__("Complete the tutorial level pack with all levels gold medal."),ACHIEVEMENT_PROGRESS},

	{"addicted",__("Addicted"),"themes/Cloudscape/player.png",{0,0,23,40},__("Played Me and My Shadow for more than 2 hours.")},
	{"loyalFan",__("Me and My Shadow loyal fan"),"themes/Cloudscape/player.png",{0,0,23,40},__("Played Me and My Shadow for more than 24 hours.")},

	{"constructor",__("Constructor"),"gfx/gui.png",{112,16,16,16},__("Use the level editor for more than 2 hours.")},
	{"constructor2",__("The creator"),"gfx/gui.png",{112,16,16,16},__("Use the level editor for more than 24 hours.")},

	{"create1",__("Look, cute level!"),"gfx/gui.png",{112,16,16,16},__("Created your first level."),ACHIEVEMENT_ALL},
	{"create50",__("The level museum"),"gfx/gui.png",{112,16,16,16},__("Created 50 levels."),ACHIEVEMENT_PROGRESS},

	{"frog",__("Frog"),"themes/Cloudscape/player.png",{0,0,23,40},__("Jump for 1000 times."),ACHIEVEMENT_PROGRESS},

	{"travel100",__("Wanderer"),"themes/Cloudscape/player.png",{0,0,23,40},__("Traveled for 100 meter."),ACHIEVEMENT_PROGRESS},
	{"travel1k",__("Runner"),"themes/Cloudscape/player.png",{0,0,23,40},__("Traveled for 1 kilometer."),ACHIEVEMENT_PROGRESS},
	{"travel10k",__("Long runner"),"themes/Cloudscape/player.png",{0,0,23,40},__("Traveled for 10 kilometer."),ACHIEVEMENT_PROGRESS},
	{"travel42k",__("Marathon runner"),"themes/Cloudscape/player.png",{0,0,23,40},__("Traveled for 42,195 meter."),ACHIEVEMENT_PROGRESS},

	{"die1",__("Be careful!"),"themes/Cloudscape/deathright.png",{0,14,23,40},__("The first death."),ACHIEVEMENT_ALL},
	{"die50",__("It doesn't matter..."),"themes/Cloudscape/deathright.png",{0,14,23,40},__("Died for 50 times.")},
	{"die1000",__("Expert of trial and error"),"themes/Cloudscape/deathright.png",{0,14,23,40},__("Died for 1000 times.")},

	{"squash1",__("Keep an eye for moving walls!"),"themes/Cloudscape/deathright.png",{0,14,23,40},__("First time being squashed.")},
	{"squash50",__("Potato masher"),"themes/Cloudscape/deathright.png",{0,14,23,40},__("Squashed for 50 times.")},

	{"doubleKill",__("Double kill"),"themes/Cloudscape/deathright.png",{0,14,23,40},__("Make both player and shadow die.")},

	{"die5in5",__("Panic death"),"themes/Cloudscape/deathright.png",{0,14,23,40},__("Died 5 times in 5 seconds.")},
	{"die10in5",__("This level is too dangerous"),"themes/Cloudscape/deathright.png",{0,14,23,40},__("Died 10 times in 5 seconds.")},

	{"forget",__("You forget your friend"),"themes/Cloudscape/player.png",{0,0,23,40},__("Finish the level with player or shadow died.")},
	{"jit",__("Just in time"),"themes/Cloudscape/player.png",{0,0,23,40},__("Player and shadow come to exit simultaneously.")},

	{"record100",__("Recorder"),"themes/Cloudscape/player.png",{0,0,23,40},__("Record for 100 times."),ACHIEVEMENT_PROGRESS},
	{"record1k",__("Shadowmaster"),"themes/Cloudscape/shadow.png",{0,0,23,40},__("Record for 1000 times."),ACHIEVEMENT_PROGRESS},

	{"switch100",__("Switch puller"),"themes/Cloudscape/player.png",{0,0,23,40},__("Pulled the switch 100 times."),ACHIEVEMENT_PROGRESS},
	{"switch1k",__("The switch is broken!"),"themes/Cloudscape/player.png",{0,0,23,40},__("Pulled the switch 1000 times.")},

	{"swap100",__("Swapper"),"themes/Cloudscape/player.png",{0,0,23,40},__("Swapped 100 times."),ACHIEVEMENT_PROGRESS},
	{"swap1k",__("Player to shadow to player to shadow..."),"themes/Cloudscape/player.png",{0,0,23,40},__("Swapped 1000 times.")},

	{"save1k",__("Save scumm"),"themes/Cloudscape/player.png",{0,0,23,40},__("Saved the game 1000 times.")},
	{"load1k",__("This game is too hard"),"themes/Cloudscape/player.png",{0,0,23,40},__("Loaded the game 1000 times.")},

	{"panicSave",__("Panic save"),"themes/Cloudscape/player.png",{0,0,23,40},__("Saved the game twice in 1 second.")},
	{"panicLoad",__("Panic load"),"themes/Cloudscape/player.png",{0,0,23,40},__("Loaded the game twice in 1 second.")},

	{"loadAndDie",__("Bad saving position"),"themes/Cloudscape/deathright.png",{0,14,23,40},__("Loaded the game and died in 1 second.")},
	{"loadAndDie100",__("This level is too hard"),"themes/Cloudscape/deathright.png",{0,14,23,40},__("Loaded the same saving and died 100 times.")},

	{"quickswap",__("Quick swap"),"themes/Cloudscape/player.png",{0,0,23,40},__("Swap player and shadow twice in 1 second.")},

	//ripped from Achievements Unlocked
	{"horizontal",__("Horizontal confusion"),"gfx/emotions.png",{0,0,23,40},__("Press left and right button simultaneously.")},

	{"programmer",__("Programmer"),"gfx/gui.png",{112,16,16,16},__("Played the development version of Me and My Shadow."),ACHIEVEMENT_TITLE},

	//end of achievements
	{}
};
