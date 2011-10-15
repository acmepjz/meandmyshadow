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

#include "ImageManager.h"
#include <stdio.h>

SDL_Surface* ImageManager::loadImage(std::string file){
	SDL_Surface* load=NULL;
	SDL_Surface* opt=NULL;

	opt=imageCollection[file];
	if(opt!=NULL) return opt;

	load=IMG_Load(file.c_str());

	if(load!=NULL){
		if(load->format->Amask){
			opt = SDL_DisplayFormatAlpha(load);
			SDL_FreeSurface(load);
		}else{
			SDL_SetColorKey ( load, SDL_SRCCOLORKEY, SDL_MapRGB(load->format, 0, 0xFF, 0xFF) );
			opt = SDL_DisplayFormat(load);
			SDL_FreeSurface(load);
		}
	}else{
		fprintf(stderr,"ERROR: Can't open image file %s\n",file.c_str());
		return NULL;
	}

	imageCollection[file]=opt;
	return opt;
}

ImageManager::~ImageManager(){
	//We call destroy().
	destroy();
}

void ImageManager::destroy(){
	//Loop through the imageCollection and free them.
	std::map<std::string,SDL_Surface*>::iterator i;
	for(i=imageCollection.begin();i!=imageCollection.end();i++){
		SDL_FreeSurface(i->second);
	}
	imageCollection.clear();
}
