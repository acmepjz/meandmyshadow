/****************************************************************************
** Copyright (C) 2011 Luka Horvat <redreaper132 at gmail.com>
** Copyright (C) 2011 Edward Lii <edward_iii at myway.com>
** Copyright (C) 2011 O. Bahri Gordebak <gordebak at gmail.com>
**
**
** This file may be used under the terms of the GNU General Public
** License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
**  You should have received a copy of the GNU General Public License
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/
#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "Settings.h"

#include <SDL/SDL.h>
#include <string>
#include <vector>


//Loads an image.
//file: The image file to load.
//Returns: The SDL_surface containing the image.
SDL_Surface* loadImage(std::string file);

//Method for drawing an SDL_Surface onto another.
//x: The x location to draw the source on the desination.
//y: The y location to draw the source on the desination.
//source: The SDL_Surface to draw.
//dest: The SDL_Surface to draw on.
//clip: Rectangle which part of the source should be drawn.
void applySurface(int x,int y,SDL_Surface* source,SDL_Surface* dest,SDL_Rect* clip);

//Method used to draw an rectangle.
//x: The top left x location of the rectangle.
//y: The top left y location of the rectangle.
//w: The width of the rectangle,
//h: The height of the rectangle.
//dest: The SDL_Surface to draw on.
//color: The color of the rectangle border to draw.
void drawRect(int x,int y,int w,int h,SDL_Surface* dest,Uint32 color=0);

//Method used to draw a line.
//x1: The x location of the start point.
//y1: The y location of the start point.
//x2: The x location of the end point.
//y2: The y location of the end point.
//dest: The SDL_Surface to draw on.
//color: The color of the line to draw.
void drawLine(int x1,int y1,int x2,int y2,SDL_Surface* dest,Uint32 color=0);

//Initialises the game. This is done almost at the beginning of the program.
//It initialises: SDL, SDL_Mixer, SDL_ttf, the screen and the block types.
//Returns: True if everything goes well.
bool init();
//Loads some important files, like the background music and the default theme.
//Returns: True if everything goes well.
bool loadFiles();

//This method will load the settings from the settings file.
//Returns: False if there's an error while loading.
bool loadSettings();
//This method will save the settings to the settings file.
//Returns: False if there's an error while saving.
bool saveSettings();
//Method used to get a pointer to the settings object.
//Returns: A pointer to the settings object.
Settings* getSettings();

//Method used to clean up before quiting meandmyshadow.
void clean();

//Sets what the nextState will be.
//newstate: Integer containing the id of the newstate.
void setNextState(int newstate);
//Method that will perform the state change.
//It will fade out and in.
void changeState();

//Checks collision between two SDL_Rects.
//a: The first rectangle.
//b: The second rectangle.
//Returns: True if the two rectangles collide.
bool checkCollision(const SDL_Rect& a,const SDL_Rect& b);

//This method will check if the mouse is near a screen edge.
//If so it will move the camera.
//Note: This function only works with the leveleditor.
void setCamera();

//Parse the commandline arguments.
//argc: Integer containing the number of aruguments there are.
//argv: The arguments.
//Returns: False if something goes wrong while parsing.
bool parseArguments(int argc, char** argv);

enum msgBoxButtons{
	MsgBoxOKOnly=0,
	MsgBoxOKCancel=1,
	MsgBoxAbortRetryIgnore=2,
	MsgBoxYesNoCancel=3,
	MsgBoxYesNo=4,
	MsgBoxRetryCancel=5,
};

enum msgBoxResult{
	MsgBoxOK=1,
	MsgBoxCancel=2,
	MsgBoxAbort=3,
	MsgBoxRetry=4,
	MsgBoxIgnore=5,
	MsgBoxYes=6,
	MsgBoxNo=7,
};

msgBoxResult msgBox(std::string prompt,msgBoxButtons buttons,const std::string& title);

bool fileDialog(std::string& fileName,const char* title=NULL,const char* extension=NULL,const char* path=NULL,bool isSave=false,bool verifyFile=false,bool files=true);

#endif