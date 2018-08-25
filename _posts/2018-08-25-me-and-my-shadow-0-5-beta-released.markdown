---
layout: post
title:  "Me and My Shadow 0.5 Beta released"
date:   2018-08-25 18:00:00
author: acmepjz
---

Version 0.5 Beta has been released.
You can find it on [Sourceforge](https://sourceforge.net/projects/meandmyshadow/files/0.5beta/).

A brief list of changes since last
[progress report]({{ site.baseurl }}{% post_url 2018-08-06-status-of-0-5-development-version %}):

* Allow to specify a scenery name for blocks,
  which can change the appearance of blocks to other blocks or scenery (don't abuse this feature 😂).
* Now the scenery layers has moving speed and following-camera speed, which can be used for parallax scrolling.
* Applied [charlie's suggestion](https://forum.freegamedev.net/viewtopic.php?f=48&t=8047&p=77664#p77664)
  that a teleporter covered with a pushable block should not work.
* The V0.5 version uses a new config file name which will not interference with the old version.
  You still need to reconfigure the keys if you previously use V0.4/V0.4.1.
* The popup dialogs in level editor can be resized properly.

Edward_Lii and I also fixed some more collision bugs, like this:

[![bug](https://user-images.githubusercontent.com/3397779/44252516-fce71c80-a22e-11e8-850a-7f81a465d3b4.gif)](https://github.com/acmepjz/meandmyshadow/issues/16)

and this:

[![bug](https://user-images.githubusercontent.com/3397779/44318533-268b8800-a469-11e8-9d77-365143925512.gif)](https://github.com/acmepjz/meandmyshadow/issues/18)

However, there are some more collision bugs which are not so easy to fix, like [this](https://github.com/acmepjz/meandmyshadow/issues/19).
We plan to leave it in V0.5 and fix it in next release 😅.

Also the known bugs and limitations are still here:

* No proper IME support for text box, etc.
* OSX support is broken.

Helps are welcome!
