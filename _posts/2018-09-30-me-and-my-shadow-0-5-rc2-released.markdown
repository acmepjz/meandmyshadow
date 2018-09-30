---
layout: post
title:  "Me and My Shadow 0.5 RC2 released"
date:   2018-09-30 17:00:00
author: acmepjz
---

Me and My Shadow 0.5 RC2 has been released.
You can find it on [Sourceforge](https://sourceforge.net/projects/meandmyshadow/files/0.5rc2/).
This fixed a BIG BAD BUG of pushable blocks which affects V0.5 Beta and V0.5 RC.
If everything goes well, The 0.5 final version will be released next weekend.
TRANSLATORS: Please help us translate the game into other languages on
[Hosted Weblate](https://hosted.weblate.org/projects/me-and-my-shadow/)!

A brief list of changes since
[0.5 RC]({{ site.baseurl }}{% post_url 2018-09-12-me-and-my-shadow-0-5-rc-released %}):

* Some tweaks to achievements.
* Make the F3 key work in level completed screen.
* Some fixes to the theme for resizable blocks.
* Some GUI fixes and improvements.
* Change tutorial level 15 target time.
* Fix the target time and target records editing in level editor.
* Revert the starting position image to something similar to that of older version.
* Some fixes to the pushable blocks, this includes a big bad bug which affects V0.5 Beta and V0.5 RC.
* Add `block:getBaseLocation()` and `block:getBaseSize()` to script API.
* Fix the copy and paste of blocks in level editor with scripts.

Translations:

* Updated Simplified Chinese, German, Russian, French and Scottish Gaelic translations.
* Added Hungarian translation by SanskritFritz.
* Added Norwegian translation by Petter Reinholdtsen and Allan Nordhøy.

Known bugs and limitations:

* No proper IME support for text box, etc.
* OSX support is broken.
* The button specification in theme file is changed, old theme may render button incorrectly in new version.
  The built-in themes and all addon themes are already fixed.
* Some achievements are not realistic, and there is a typo in achievements.
* Invisible collectible still counted in total number of collectibles.
* The scenery layer naming convention is confusing.

The missing characters in Hungarian translation are also fixed, by introducing another font.
