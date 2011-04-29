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

#include <SDL/SDL.h>
#include <string>
#include <vector>

SDL_Surface * load_image( std::string file );

void apply_surface ( int x, int y, SDL_Surface * src, SDL_Surface * dst, SDL_Rect *clip );

bool init();

bool load_files();

void clean();

void next_state ( int newstate );

bool check_collision( const SDL_Rect& A, const SDL_Rect& B );

void change_state();

void set_camera();

std::string GetUserPath();
std::string GetDataPath();
std::string GetAppPath();
std::vector<std::string> EnumAllFiles(std::string sPath,const char* sExtension=NULL);

bool ParseCommandLines(int argc, char ** argv);

#endif

