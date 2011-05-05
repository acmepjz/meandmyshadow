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

#include <stdio.h>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_mixer.h>
#include <string>
#include "Globals.h"
#include "Functions.h"
#include "Objects.h"
#include "Player.h"
#include "GameObjects.h"
#include "Timer.h"
#include "Levels.h"
#include "Title_Menu.h"
#include "LevelEditor.h"
#include "Game.h"
#include "LevelSelect.h"
#include "ImageManager.h"
using namespace std;

#ifdef WIN32
#include <windows.h>
#include <shlobj.h>
#else
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#endif

string m_sUserPath,m_sDataPath,m_sAppPath,m_sEXEName;

ImageManager m_objImageManager;

SDL_Surface * load_image ( string file )
{
	return m_objImageManager.load_image(file);
}

void apply_surface ( int x, int y, SDL_Surface * src, SDL_Surface * dst, SDL_Rect * clip )
{
	SDL_Rect offset;
	offset.x = x; offset.y = y;

	SDL_BlitSurface ( src, clip, dst, &offset );
}

bool init()
{
	if ( SDL_Init(SDL_INIT_EVERYTHING) == -1 )
	{
		fprintf(stderr,"FATAL ERROR: SDL_Init failed\n");
		return false;
	}

	if ( Mix_OpenAudio( 22050, MIX_DEFAULT_FORMAT, 2 , 512 ) == -1 )
	{
		fprintf(stderr,"FATAL ERROR: Mix_OpenAudio failed\n");
		return false;
	}

	if ( TTF_Init() == -1 )
	{
		fprintf(stderr,"FATAL ERROR: TTF_Init failed\n");
		return false;
	}

	screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BPP, SDL_HWSURFACE | SDL_DOUBLEBUF /*|SDL_FULLSCREEN*/ );

	if ( screen == NULL )
	{
		fprintf(stderr,"FATAL ERROR: SDL_SetVideoMode failed\n");
		return false;
	}


	SDL_WM_SetCaption("Me and my shadow", NULL );
	SDL_EnableUNICODE(1);

	return true;
}

bool load_files()
{
	//get the app path
	{
		char s[4096];
		int i,m;
		#ifdef WIN32
		m=GetModuleFileNameA(NULL,s,sizeof(s));
		#else
		m=readlink("/proc/self/exe",s,sizeof(s));
		#endif
		s[m]=0;
		for(i=m-1;i>=0;i--){
			if(s[i]=='/'||s[i]=='\\'){
				s[i]=0;
				break;
			}
		}
		m_sAppPath=s;
		m_sEXEName=s+i+1;
	}
	//get the user path
	if(m_sUserPath.empty()){
#ifdef WIN32
		char s[1024];
		SHGetSpecialFolderPathA(NULL,s,CSIDL_PERSONAL,1);
		m_sUserPath=s;
		m_sUserPath+="\\My Games\\meandmyshadow\\";
		SHCreateDirectoryExA(NULL,m_sUserPath.c_str(),NULL);
#else
		m_sUserPath=getenv("HOME");
		m_sUserPath+="/.meandmyshadow/";
		mkdir(m_sUserPath.c_str(),0777);
#endif
	}
	//get the data path
	{
		FILE *f;
		string s;
		for(;;){
			//try existing one
			if(!m_sDataPath.empty()){
				s=m_sDataPath+"data/font/ComicBook.ttf";
				if((f=fopen(s.c_str(),"rb"))!=NULL){
					fclose(f);
					break;
				}
			}
			//try "./"
			m_sDataPath="./";
			s=m_sDataPath+"data/font/ComicBook.ttf";
			if((f=fopen(s.c_str(),"rb"))!=NULL){
				fclose(f);
				break;
			}
			//try "../"
			m_sDataPath="../";
			s=m_sDataPath+"data/font/ComicBook.ttf";
			if((f=fopen(s.c_str(),"rb"))!=NULL){
				fclose(f);
				break;
			}
			//try App.Path
			m_sDataPath=GetAppPath()+"/";
			s=m_sDataPath+"data/font/ComicBook.ttf";
			if((f=fopen(s.c_str(),"rb"))!=NULL){
				fclose(f);
				break;
			}
			//try App.Path+"/../"
			m_sDataPath=GetAppPath()+"/../";
			s=m_sDataPath+"data/font/ComicBook.ttf";
			if((f=fopen(s.c_str(),"rb"))!=NULL){
				fclose(f);
				break;
			}
			//try DATA_PATH
#ifdef DATA_PATH
			m_sDataPath=DATA_PATH;
			s=m_sDataPath+"data/font/ComicBook.ttf";
			if((f=fopen(s.c_str(),"rb"))!=NULL){
				fclose(f);
				break;
			}
#endif
			//error: can't find file
			return false;
		}
		font = TTF_OpenFont(s.c_str(), 28);
		font_small = TTF_OpenFont(s.c_str(), 20);
	}

	s_dark_block = load_image(GetDataPath()+"data/gfx/dark.png");
	s_black = load_image(GetDataPath()+"data/gfx/black.png");
	music = Mix_LoadMUS((GetDataPath()+"data/sfx/music.mid").c_str());
	bool b=o_mylevels.load_levels("%DATA%/data/level/levellist.txt","levelprogress.txt");
	b=s_dark_block!=NULL && s_black!=NULL
		&& font!=NULL && font_small != NULL && b;

	if(music==NULL)
		printf("Warning: Unable to load background music! \n");

	if(b){
		printf("Data files will be fetched from: '%s'\n",m_sDataPath.c_str());
		printf("User preferences will be fetched from: '%s'\n",m_sUserPath.c_str());
	}
	return b;
}

void clean()
{
	delete currentState;

	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}
	m_objImageManager.Destroy();
	TTF_CloseFont(font);
	TTF_CloseFont(font_small);
	TTF_Quit();
	SDL_Quit();
	Mix_CloseAudio();
}

void next_state( int newstate )
{
	if ( nextState != STATE_EXIT )
	{
		nextState = newstate;
	}
}

void change_state()
{
	if ( nextState != STATE_NULL )
	{
		if ( nextState != STATE_EXIT )
		{
			delete currentState;
		}

		stateID = nextState;
		nextState = STATE_NULL;

		switch ( stateID )
		{
		case STATE_GAME:
			{
				currentState = new Game();
				break;
			}

		case STATE_MENU:
			{
				currentState = new Menu();
				break;
			}

		case STATE_HELP:
			{
				currentState = new Help();
				break;
			}
		case STATE_LEVEL_SELECT:
			{
				currentState = new LevelSelect();
				break;
			}
		case STATE_LEVEL_EDITOR:
			{
				currentState = new LevelEditor(m_sLevelName.c_str());
				break;
			}
		}

		//fade out
		SDL_BlitSurface(screen,NULL,s_temp,NULL);
		int i;
		for(i=255;i>=0;i-=17)
		{
			SDL_FillRect(screen,NULL,0);
			SDL_SetAlpha(s_temp, SDL_SRCALPHA, i);
			SDL_BlitSurface(s_temp,NULL,screen,NULL);
			SDL_Flip(screen);
			SDL_Delay(25);
		}


	}
}

bool check_collision( const SDL_Rect& A, const SDL_Rect& B )
{
	if ( A.x >= B.x + B.w )
	{
		return false;
	}

	if ( A.x + A.w <= B.x )
	{
		return false;
	}

	if ( A.y >= B.y + B.h )
	{
		return false;
	}

	if ( A.y + A.h <= B.y )
	{
		return false;
	}

	return true;
}

void set_camera()
{
	if ( stateID == STATE_LEVEL_EDITOR )
	{
		int x, y;

		SDL_GetMouseState(&x,&y);

		if(x<50){
			camera.x-=10;
			if(camera.x<0) camera.x=0;
		}else if(x>=SCREEN_WIDTH-50){
			camera.x+=10;
			if(camera.x>LEVEL_WIDTH-SCREEN_WIDTH) camera.x=LEVEL_WIDTH-SCREEN_WIDTH;
		}

		if(y<50){
			camera.y-=10;
			if(camera.y<0) camera.y=0;
		}else if(y>=SCREEN_HEIGHT-50){
			camera.y+=10;
			if(camera.y>LEVEL_HEIGHT-SCREEN_HEIGHT) camera.y=LEVEL_HEIGHT-SCREEN_HEIGHT;
		}
	}
}

std::vector<std::string> EnumAllFiles(std::string sPath,const char* sExtension){
	vector<string> v;
#ifdef WIN32
	string s1;
	WIN32_FIND_DATAA f;
	if(!sPath.empty()){
		char c=sPath[sPath.size()-1];
		if(c!='/'&&c!='\\') sPath+="\\";
	}
	s1=sPath;
	if(sExtension!=NULL && *sExtension){
		s1+="*.";
		s1+=sExtension;
	}else{
		s1+="*";
	}
	HANDLE h=FindFirstFileA(s1.c_str(),&f);
	if(h==NULL||h==INVALID_HANDLE_VALUE) return v;
	do{
		if(!(f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)){
			v.push_back(/*sPath+*/f.cFileName);
		}
	}while(FindNextFileA(h,&f));
	FindClose(h);
	return v;
#else
	int len=0;
	if(sExtension!=NULL && *sExtension) len=strlen(sExtension);
	if(!sPath.empty()){
		char c=sPath[sPath.size()-1];
		if(c!='/'&&c!='\\') sPath+="/";
	}
	DIR *pDir;
	struct dirent *pDirent;
	pDir=opendir(sPath.c_str());
	if(pDir==NULL) return v;
	while((pDirent=readdir(pDir))!=NULL){
		if(pDirent->d_name[0]=='.'){
			if(pDirent->d_name[1]==0||
				(pDirent->d_name[1]=='.'&&pDirent->d_name[2]==0)) continue;
		}
		string s1=sPath+pDirent->d_name;
		struct stat S_stat;
		lstat(s1.c_str(),&S_stat);
		if(!S_ISDIR(S_stat.st_mode)){
			if(len>0){
				if((int)s1.size()<len+1) continue;
				if(s1[s1.size()-len-1]!='.') continue;
				if(strcasecmp(&s1[s1.size()-len],sExtension)) continue;
			}
			v.push_back(/*s1*/string(pDirent->d_name));
		}
	}
	closedir(pDir);
	return v;
#endif
}

bool ParseCommandLines(int argc, char ** argv){
	for(int i=1;i<argc;i++){
		string s=argv[i];
		if(s=="--data-dir"){
			i++;
			if(i>=argc){
				printf("Command line error: Missing parameter for command '%s'\n\n",s.c_str());
				return false;
			}
			m_sDataPath=argv[i];
			if(!m_sDataPath.empty()){
				char c=m_sDataPath[m_sDataPath.size()-1];
				if(c!='/'&&c!='\\') m_sDataPath+="/";
			}
		}else if(s=="--user-dir"){
			i++;
			if(i>=argc){
				printf("Command line error: Missing parameter for command '%s'\n\n",s.c_str());
				return false;
			}
			m_sUserPath=argv[i];
			if(!m_sUserPath.empty()){
				char c=m_sUserPath[m_sUserPath.size()-1];
				if(c!='/'&&c!='\\') m_sUserPath+="/";
			}
		}else if(s=="-h" || s=="-help" || s=="--help"){
			return false;
		}else{
			printf("Command line error: Unknown command '%s'\n\n",s.c_str());
			return false;
		}
	}
	return true;
}

std::string ProcessFileName(const std::string& s){
	if(s.compare(0,6,"%DATA%")==0){
		if(s.size()>6 && (s[6]=='/' || s[6]=='\\')){
			return m_sDataPath+s.substr(7);
		}else{
			return m_sDataPath+s.substr(6);
		}
	}else if(s.size()>0 && (s[0]=='/' || s[0]=='\\')){
		return s;
	}else{
		return m_sUserPath+s;
	}
}
