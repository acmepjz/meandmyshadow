---
layout: post
title:  "Status of 0.5 Development Version"
date:   2018-08-06 07:00:00
author: acmepjz
---

Here I'd like to introduce some new features of Me and My Shadow 0.5 development version.
In fact, some of them are already completed for many years.
I think the most of them are stable enough, so maybe the 0.5 beta version will be available soon.

Switch to SDL2
--------------

The graphics library is switched from SDL1.2 to SDL2.
This work is mostly done by [oyvindln](https://github.com/oyvindln) during 2016-2018.
The SDL2 has builtin hardware accelerated 2D graphics rendering,
which allows us to render 2D image with additional alpha modulation and/or color modulation,
stretching and rotation.

Achievement and statistics system
---------------------------------

The achievements and statistics system was indeed implemented by me in 2012.
You can see:

* The idea thread <https://forum.freegamedev.net/viewtopic.php?f=48&t=3322>
* The call for review thread <https://forum.freegamedev.net/viewtopic.php?f=48&t=3883>

This is an example of achievement:

[![An achievement][1]{:width="400px"}][1]

This is the achievement screen:

[![Achievement screen][2]{:width="400px"}][2]

This is the statistics screen:

[![Statistics screen][3]{:width="400px"}][3]

[1]: {{ "/media/mnms0.5dev-new-achievement.jpg" | prepend: site.baseurl }}
[2]: {{ "/media/mnms0.5dev-achievement-screen.jpg" | prepend: site.baseurl }}
[3]: {{ "/media/mnms0.5dev-statistics-screen.jpg" | prepend: site.baseurl }}

Scripting
---------

The scripting support was also developed in 2012-2013.
At first it was discussed in [V0.5 brainstorm thread](https://forum.freegamedev.net/viewtopic.php?f=48&t=3334).
One of the main developer, Edward Lii, was a little bit worried about it:

> I've thought about scripting support before and although I see some uses for it I'm afraid it will be misused.<br>
> Take for example [XMoto](http://xmoto.tuxfamily.org/), they have scripting support in their levels and there are some fun/impressive levels made thanks to that.<br>
> But there are also a lot of levels that are unclear, they change certain things without giving any graphical feedback

Interestingly the initial implementation of scripting support is also by Edward Lii:

> I've made some progress regarding scripting.<br>
> I decided to try and implement Lua and it is easier than I expected. 😄

Currently the scripting language is Lua, version 5.2. Later I may bump the version to 5.3.
You can find the scripting API document [here](https://github.com/acmepjz/meandmyshadow/blob/master/docs/ScriptAPI.md).
Also you can fine the (possibly outdated) scripting API discussions [here](https://forum.freegamedev.net/viewtopic.php?f=48&t=4128).

### Known limitations and tasks

The following may be implemented after V0.5 is released due to the amounts of work.

* Non-POD types in save/load of delayed execution object doesn't work (e.g. closure)
* Make scenery blocks scriptable
* Add more scripting API
  * Add/remove blocks dynamically
  * Add/remove path points of moving blocks
  * Show custom notification/scoreboard
  * And so on...

Block improvements
------------------

### Pushable block

There is a new type of block added in game: the pushable block.
This is also implemented by Edward Lii in 2012, see [here](https://forum.freegamedev.net/viewtopic.php?f=48&t=3334&start=25#p36529).

This is an early preview of pushable block which uses
the image of moving block as a temporary placeholder, and showed the old behavior of moving blocks.

<video width="120" height="120" src="{{ "/media/pushable-early.webm" | prepend: site.baseurl }}" loop="true" muted="true" autoplay="true" preload="auto" controls="true">
Sorry, your browser does not support the video tag.
</video>

This shows an early bug of collision with pushable blocks.
The current behavior is you will get squashed once you teleport into any blocks.

<video width="800" height="600" src="{{ "/media/pushable-bug.webm" | prepend: site.baseurl }}" muted="true" preload="auto" controls="true">
Sorry, your browser does not support the video tag.
</video>

This is the current version of pushable blocks as well as new behavior.

<video width="360" height="280" src="{{ "/media/pushable-demo.webm" | prepend: site.baseurl }}" loop="true" muted="true" autoplay="true" preload="auto" controls="true">
Sorry, your browser does not support the video tag.
</video>

### Sizable block

As you can see in the above screenshot, now the blocks can be resized in game.
This is again implemented by Edward Lii, in 2013, see [here](https://forum.freegamedev.net/viewtopic.php?f=48&t=4673).

This shows a development experiment which confirms the sizable block codes are actually working.
Ugly programmer art, isn't it?

[![Sizable block][4]{:width="400px"}][4]

[4]: {{ "/media/LargeBlocks.jpg" | prepend: site.baseurl }}

NOTE: The current themes are still not adapted to the sizable blocks. Art contributions are welcome!

### Scenery block

The suggestion to add scenery block support is given by Tedium in 2013, see [here](https://forum.freegamedev.net/viewtopic.php?f=48&t=4207).

This is a mockup by Tedium for the applications of scenery blocks (NOTE: highly compressed).

[![Scenery block][5]{:width="400px"}][5]

[5]: {{ "/media/mnms0.5dev-scenery-mockup.jpg" | prepend: site.baseurl }}

### Block visibility

The block visibility property is added. This is usually used with scripting.

Level editor improvements
-------------------------

### Context menu

The context menu for block and level is added, which supersedes the ENTER key to configuration method in V0.4.

### Undo/redo

The level editor undo/redo patch is provided by squarecross in 2014, see [here](https://forum.freegamedev.net/viewtopic.php?f=48&t=5432).
However, the patch is not reviewed until recently, due to the game development halted.
Now the patch is merged into the master branch with substantial modifications.
The corresponding GUI is also added.

Addon improvements
------------------

The addon GUI is nicer than the one in V0.4.
We adopted the mockup provided by odamite, see [here](https://forum.freegamedev.net/viewtopic.php?f=48&t=2959&p=61832#p29138).

Also, the addon format is changed (which is still undocumented yet 😅) and the addon repository
is moved to GitHub.
This means that anyone who wants to upload an addon can fork the addon repository then submit a pull request.

Of course, you can make a clone and host the addon anywhere (even use the addon offline by using `file://` protocol)
by passing command line arguments to the game.

Known limitations
-----------------

* No proper IME support for text box, etc.
* OSX support is broken.

Helps are welcome!
