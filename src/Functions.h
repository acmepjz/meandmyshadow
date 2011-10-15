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

void apply_surface(int x,int y,SDL_Surface* src,SDL_Surface* dst,SDL_Rect *clip);

//Initialises the game. This is done almost at the beginning of the program.
//It initialises: SDL, SDL_Mixer, SDL_ttf, the screen and the block types.
//Returns: True if everything goes well.
bool init();
//Loads some important files, like the background music and the default theme.
//Returns: True if everything goes well.
bool loadFiles();

bool loadSettings();
bool saveSettings();

Settings* getSettings();

void clean();

//Sets what the nextState will be.
//newstate: Integer containing the id of the newstate.
void setNextState(int newstate);
//Method that will perform the state change.
//It will fade out and in.
void changeState();

bool check_collision( const SDL_Rect& A, const SDL_Rect& B );

void set_camera();

bool ParseCommandLines(int argc, char ** argv);

enum eMsgBoxButtons{
	MsgBoxOKOnly=0,
	MsgBoxOKCancel = 1,
	MsgBoxAbortRetryIgnore = 2,MsgBoxYesNoCancel = 3,
	MsgBoxYesNo = 4,
	MsgBoxRetryCancel = 5,
};

enum eMsgBoxResult{
	MsgBoxOK=1,
	MsgBoxCancel=2,
	MsgBoxAbort=3,
	MsgBoxRetry=4,
	MsgBoxIgnore=5,
	MsgBoxYes=6,
	MsgBoxNo=7,
};

eMsgBoxResult MsgBox(std::string Prompt,eMsgBoxButtons Buttons,const std::string& Title);

bool FileDialog(std::string& FileName,const char* sTitle=NULL,const char* sExtension=NULL,const char* sPath=NULL,bool is_save=false,bool verify_file=false,bool files=true);

#endif

