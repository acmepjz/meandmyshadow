This is the fork of the puzzle game Me and My Shadow originally at [SourceForge](http://meandmyshadow.sourceforge.net/).
The new website is <https://acmepjz.github.io/meandmyshadow/>.

[![Build Status](https://travis-ci.org/acmepjz/meandmyshadow.svg?branch=master)](https://travis-ci.org/acmepjz/meandmyshadow)

[![Build status](https://ci.appveyor.com/api/projects/status/t0cfcb54fppa501c/branch/master?svg=true)](https://ci.appveyor.com/project/acmepjz/meandmyshadow/branch/master) <--
Click here for Windows nightly build (you also need `data` forder from latest git repository, just download from GitHub)

Me and My Shadow
====================
Me and My Shadow is a free libre puzzle/platform game in which you try to reach
the exit by solving puzzles. Spikes, moving blocks, fragile blocks and much
more stand between you and the exit. Record your moves and let your shadow 
mimic them to reach blocks you couldn't reach alone.

 - Tutorial for beginners
 - 2 level packs containing over 40 levels
 - 18 different block types
 - Built-in level editor
 - Easily installable addons
 - Original music by Juho-Petteri Yliuntinen
 - Cross platform

Compiling
=========

Compiling on Linux
------------------

You will need the following packages (and their -dev(el) files) to be installed:

  * libSDL2
  * libSDL2_image
  * libSDL2_ttf
  * libSDL2_mixer
  * libcurl
  * libarchive
  * liblua ( >=5.2 )
  * cmake
  * C++ compiler (found in packages like g++, gcc-c++, gcc)

The process is simple. Enter a terminal and move to directory containing
MeAndMyShadow. Then just type

~~~
  mkdir build && cd build
  cmake ..
~~~

to generate the Makefile. If everything configured properly you don't see any
errors and then you can start compiling by typing

~~~
  make
~~~

Finally you can run MeAndMyShadow with

~~~
  ./meandmyshadow
~~~

To install MeAndMyShadow on your system, run following as root

~~~
  make install
~~~

See `.travis.yml` for example.

Compiling on Windows
--------------------

If you are using VS2013 or VS2015 you can download the pre-built SDL2 dependencies
[here](https://github.com/acmepjz/meandmyshadow/releases/tag/v0.5-devel002 ),
and unzip the dependencies.
Otherwise you need to compile some dependencies from source. See `appveyor.yml-build-dependencies` for example.

Open a command prompt, move to directory containing
MeAndMyShadow. Then type

~~~
  set PATH=path\to\dependencies;%PATH%
  mkdir build && cd build
  path\to\cmake-gui.exe ..
~~~

Follow the instruction to generate Visual Studio solution files.
Open the solution file to compile.

Compiling on Mac
----------------
(Under construction)

It is almost the same as in Linux. Use brew to install dependencies.
See `.travis.yml` for example.

Translating
===========

We use the web-based translation system [Transifex](https://www.transifex.com/acmepjz/meandmyshadow/ )
to host MeAndMyShadow translation.

Meanwhile, you can also translate the game directly. See <http://meandmyshadow.sourceforge.net/wiki/index.php/Translating>
for more information.
