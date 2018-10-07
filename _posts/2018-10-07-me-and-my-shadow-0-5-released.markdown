---
layout: post
title:  "Me and My Shadow 0.5 released"
date:   2018-10-07 15:30:00
author: acmepjz
---

After two months of polishing,
Me and My Shadow 0.5 has just been released.
You can find it on [Sourceforge](https://sourceforge.net/projects/meandmyshadow/files/0.5/).
TRANSLATORS: Please help us translate the game into other languages on
[Hosted Weblate](https://hosted.weblate.org/projects/me-and-my-shadow/)!
This also helps after 0.5 is released.

New features
------------

It's recommended to read the
[reports on some of new features of 0.5]({{ site.baseurl }}{% post_url 2018-08-06-status-of-0-5-development-version %})
first.
Let's briefly list them here:

[1]: {{ "/media/mnms0.5dev-new-achievement.jpg" | prepend: site.baseurl }}
[2]: {{ "/media/mnms0.5dev-achievement-screen.jpg" | prepend: site.baseurl }}
[3]: {{ "/media/mnms0.5dev-statistics-screen.jpg" | prepend: site.baseurl }}
[5]: {{ "/media/mnms0.5dev-scenery-mockup.jpg" | prepend: site.baseurl }}

* Switch to SDL2.
* Menu theme.
* Achievement and statistics system.<br>
  [![An achievement][1]{:width="400px"}][1]<br>
  [![Achievement screen][2]{:width="400px"}][2]<br>
  [![Statistics screen][3]{:width="400px"}][3]
* Scripting with lua.
* Pushable block.<br>
  <video width="360" height="280" src="{{ "/media/pushable-demo.webm" | prepend: site.baseurl }}" loop="true" muted="true" autoplay="true" preload="auto" controls="true">
  Sorry, your browser does not support the video tag.
  </video>
* Improved addon dialog.
* Improved level editor user interface.
* Sizable blocks.
* Scenery blocks.<br>
  [![Scenery block][5]{:width="400px"}][5]
* Undo/redo in level editor originally written by squarecross.
* Added `visible` property to blocks.

A brief list of changes since
[0.5 RC2]({{ site.baseurl }}{% post_url 2018-09-30-me-and-my-shadow-0-5-rc2-released %}):

* The pushable blocks also collide with spikes and moving spikes.
  This changes was already in V0.5 RC2 but I missed it from the V0.5 RC2 release note.
* Update Russian translation and added Ukrainian translation.
* Add an exit to classic level 24.
* Fix some glitches after renaming or moving levels.
* Add some achievement images by Wuzzy and reordered first few achievements.
* Revert an old code changes to fix a fragile block hit test bug when it is breaking.
* Show time and recordings during replay.
* Make addon popup dialog bigger.
* Make the display of number on screen locale dependent.
* Fix the addon installation status lost bug.

Translations since
[0.4.1]({{ site.baseurl }}{% post_url 2012-07-25-me-and-my-shadow-0-4-released %}):

* Updated Simplified Chinese (by acme_pjz),
  German (by Wuzzy), Russian (by mesnevi and eugeneloza),
  French (by Poussinou) and Scottish Gaelic (by GunChleoc) translations.
* Added Hungarian translation by SanskritFritz.
* Added Norwegian translation by Petter Reinholdtsen and Allan Nordhøy.
* Added Ukrainian translation by eugeneloza.

Bugs and limitations
--------------------

Known bugs and limitations:

* No proper IME support for text box, etc.
* OSX support is broken.
* The button specification in theme file is changed, old theme may render button incorrectly in new version.
  (The built-in themes and all addon themes are already fixed.)
* Some achievements are not realistic, and there is a typo in achievements.
* Invisible collectible is counted in total number of collectibles.
* The scenery layer naming convention is confusing.
* The target time for tutorial level 10 seems unbeatable. Needs further investigation.
* There seems to be a random crash bug when exiting the game.
  I haven't found a way to reproduce it; it only occurred twice when I change the language multiple times,
  viewing achievements and statistics, and viewing level play select screen.
  I haven't found what's wrong with the code yet, it looks like something like double delete of ThemeBlock, but I'm not sure.
  If you find a way to reproduce this bug or you have located the part of code which is wrong,
  please tell me by either post on the forum, or by submit an issue or pull request on Github.
