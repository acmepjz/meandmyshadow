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
#include "TitleMenu.h"
#include "LevelEditor.h"
#include "Game.h"
#include "LevelSelect.h"
#include "Addons.h"
#include "ImageManager.h"
#include "ThemeManager.h"
#include "GUIListBox.h"
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

Settings* m_settings=0;

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

	for(int i=0;i<TYPE_MAX;i++){
		Game::g_BlockNameMap[Game::g_sBlockName[i]]=i;
	}

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
		SHCreateDirectoryExA(NULL,(m_sUserPath+"levels").c_str(),NULL);
		SHCreateDirectoryExA(NULL,)m_sUserPath+"levelpacks").c_str(),NULL);
		SHCreateDirectoryExA(NULL,)m_sUserPath+"themes").c_str(),NULL);
		SHCreateDirectoryExA(NULL,)m_sUserPath+"progress").c_str(),NULL);
#else
		m_sUserPath=getenv("HOME");
		m_sUserPath+="/.meandmyshadow/";
		mkdir(m_sUserPath.c_str(),0777);
		//Also create other folders in the userpath.
		mkdir((m_sUserPath+"/levels").c_str(),0777);
		mkdir((m_sUserPath+"/levelpacks").c_str(),0777);
		mkdir((m_sUserPath+"/themes").c_str(),0777);
		mkdir((m_sUserPath+"/progress").c_str(),0777);
#endif
	}
	//get the data path
	{
		FILE *f;
		string s;
		for(;;){
			//try existing one
			if(!m_sDataPath.empty()){
				s=m_sDataPath+"font/ComicBook.ttf";
				if((f=fopen(s.c_str(),"rb"))!=NULL){
					fclose(f);
					break;
				}
			}
			//try "./"
			m_sDataPath="./data/";
			s=m_sDataPath+"font/ComicBook.ttf";
			if((f=fopen(s.c_str(),"rb"))!=NULL){
				fclose(f);
				break;
			}
			//try "../"
			m_sDataPath="../data/";
			s=m_sDataPath+"font/ComicBook.ttf";
			if((f=fopen(s.c_str(),"rb"))!=NULL){
				fclose(f);
				break;
			}
			//try App.Path
			m_sDataPath=get_app_path()+"/data/";
			s=m_sDataPath+"font/ComicBook.ttf";
			if((f=fopen(s.c_str(),"rb"))!=NULL){
				fclose(f);
				break;
			}
			//try App.Path+"/../"
			m_sDataPath=get_app_path()+"/../data/";
			s=m_sDataPath+"font/ComicBook.ttf";
			if((f=fopen(s.c_str(),"rb"))!=NULL){
				fclose(f);
				break;
			}
			//try DATA_PATH
#ifdef DATA_PATH
			m_sDataPath=DATA_PATH;
			s=m_sDataPath+"font/ComicBook.ttf";
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

	s_dark_block = load_image(get_data_path()+"gfx/dark.png");
	s_black = load_image(get_data_path()+"gfx/black.png");
	music = Mix_LoadMUS((get_data_path()+"sfx/music.mid").c_str());
	bool b=s_dark_block!=NULL && s_black!=NULL
		&& font!=NULL && font_small != NULL;

	if(music==NULL)
		printf("Warning: Unable to load background music! \n");

	if(m_objThemes.AppendThemeFromFile(get_data_path()+"themes/default/theme.mnmstheme")==NULL){
		b=false;
		printf("ERROR: Can't load default theme file\n");
	}

	if(b){
		printf("Data files will be fetched from: '%s'\n",m_sDataPath.c_str());
		printf("User preferences will be fetched from: '%s'\n",m_sUserPath.c_str());
	}
	return b;
}

bool load_settings()
{
	m_settings=new Settings(m_sUserPath+"meandmyshadow.cfg");
	m_settings->parseFile();
  
	//Always return true?
	return true;
}

void save_settings()
{
	m_settings->save();
}

Settings* get_settings()
{
	return m_settings;
}

void clean()
{
	delete m_settings;
	m_settings=NULL;

	if(currentState) delete currentState;

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
		//if ( nextState != STATE_EXIT )
		//{
			delete currentState;
			currentState=NULL;
		//}

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
				o_mylevels.clear();
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
				o_mylevels.load_levels("%DATA%/levelpacks/default/levels.lst","%USER%progress/default.progress");
				currentState = new LevelSelect();
				break;
			}
		case STATE_LEVEL_EDITOR:
			{
				o_mylevels.clear();
				currentState = new LevelEditor(m_sLevelName.c_str());
				break;
			}
		case STATE_OPTIONS:
			{
				currentState = new Options();
				break;
			}
		case STATE_ADDONS:
			{
				currentState = new Addons();
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

std::vector<std::string> EnumAllDirs(std::string sPath){
	vector<string> v;
#ifdef WIN32
	string s1;
	WIN32_FIND_DATAA f;
	if(!sPath.empty()){
		char c=sPath[sPath.size()-1];
		if(c!='/'&&c!='\\') sPath+="\\";
	}
	s1=sPath;
	HANDLE h=FindFirstFileA(s1.c_str(),&f);
	if(h==NULL||h==INVALID_HANDLE_VALUE) return v;
	do{
		if(!(f.dwDirAttributes & FILE_ATTRIBUTE_DIRECTORY)){
			v.push_back(/*sPath+*/f.cFileName);
		}
	}while(FindNextFileA(h,&f));
	FindClose(h);
	return v;
#else
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
		if(S_ISDIR(S_stat.st_mode)){
			//Skip hidden folders.
			s1=string(pDirent->d_name);
			if(s1.find('.')==0) continue;
			
			//Add result to vector.
			v.push_back(s1);
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

std::string ProcessFileName(const std::string& s, bool addon){
	string prefix;
	if(addon) {
		prefix=m_sUserPath;
	} else {
		prefix=m_sDataPath;
	}
  
	if(s.compare(0,6,"%DATA%")==0){
		if(s.size()>6 && (s[6]=='/' || s[6]=='\\')){
			return m_sDataPath+s.substr(7);
		}else{
			return m_sDataPath+s.substr(6);
		}
	}else if(s.compare(0,6,"%USER%")==0){
		if(s.size()>6 && (s[6]=='/' || s[6]=='\\')){
			return m_sUserPath+s.substr(7);
		}else{
			return m_sUserPath+s.substr(6);
		}
	}else if(s.compare(0,9,"%LVLPACK%")==0){
		if(s.size()>9 && (s[9]=='/' || s[9]=='\\')){
			return prefix+"levelpacks/"+s.substr(10);
		}else{
			return prefix+"levelpacks/"+s.substr(9);
		}
	}else if(s.compare(0,5,"%LVL%")==0){
		if(s.size()>5 && (s[5]=='/' || s[5]=='\\')){
			return prefix+"levels/"+s.substr(6);
		}else{
			return prefix+"levels/"+s.substr(5);
		}
	}else if(s.compare(0,8,"%THEMES%")==0){
		if(s.size()>8 && (s[8]=='/' || s[8]=='\\')){
			return prefix+"themes/"+s.substr(9);
		}else{
			return prefix+"themes/"+s.substr(8);
		}
	}else if(s.size()>0 && (s[0]=='/' || s[0]=='\\')){
		return s;
	}else{
		return prefix+s;
	}
}

std::string FileNameFromPath(const std::string &path){
	std::string filename;
#ifdef WIN32
	size_t pos = path.find_last_of("\\");
#else
	size_t pos = path.find_last_of("\/");
#endif
	if(pos != std::string::npos)
		filename.assign(path.begin() + pos + 1, path.end());
	else
		filename = path;
	
	return filename;
}

struct cMsgBoxHandler:public GUIEventCallback{
public:
	int ret;
public:
	cMsgBoxHandler():ret(0){}
	void GUIEventCallback_OnEvent(std::string Name,GUIObject* obj,int nEventType){
		if(nEventType==GUIEventClick){
			ret=obj->Value;
			if(GUIObjectRoot){
				delete GUIObjectRoot;
				GUIObjectRoot=NULL;
			}
		}
	}
};

eMsgBoxResult MsgBox(string Prompt,eMsgBoxButtons Buttons,const string& Title){
	cMsgBoxHandler objHandler;
	GUIObject *obj,*tmp=GUIObjectRoot;
	GUIObjectRoot=new GUIObject(100,200,600,200,GUIObjectFrame,Title.c_str());
	//process prompt
	{
		char *lps=(char*)Prompt.c_str(),*lp;
		int y=20;
		for(;;){
			for(lp=lps;*lp!='\n'&&*lp!='\r'&&*lp!=0;lp++);
			char c=*lp;
			*lp=0;
			GUIObjectRoot->ChildControls.push_back(new GUIObject(8,y,584,25,GUIObjectLabel,lps));
			y+=25;
			if(c==0) break;
			lps=lp+1;
		}
	}
	//===
	int nCount=0,nValue[3]={0};
	char *sButton[3]={0};
	switch(Buttons){
	case MsgBoxOKCancel:
		nCount=2;
		sButton[0]="OK";nValue[0]=MsgBoxOK;
		sButton[1]="Cancel";nValue[1]=MsgBoxCancel;
		break;
	case MsgBoxAbortRetryIgnore:
		nCount=3;
		sButton[0]="Abort";nValue[0]=MsgBoxAbort;
		sButton[1]="Retry";nValue[1]=MsgBoxRetry;
		sButton[2]="Ignore";nValue[2]=MsgBoxIgnore;
		break;
	case MsgBoxYesNoCancel:
		nCount=3;
		sButton[0]="Yes";nValue[0]=MsgBoxYes;
		sButton[1]="No";nValue[1]=MsgBoxNo;
		sButton[2]="Cancel";nValue[2]=MsgBoxCancel;
		break;
	case MsgBoxYesNo:
		nCount=2;
		sButton[0]="Yes";nValue[0]=MsgBoxYes;
		sButton[1]="No";nValue[1]=MsgBoxNo;
		break;
	case MsgBoxRetryCancel:
		nCount=2;
		sButton[0]="Retry";nValue[0]=MsgBoxRetry;
		sButton[1]="Cancel";nValue[1]=MsgBoxCancel;
		break;
	default:
		nCount=1;
		sButton[0]="OK";nValue[0]=MsgBoxOK;
		break;
	}
	//===
	{
		int x=302-nCount*50;
		for(int i=0;i<nCount;i++,x+=100){
			obj=new GUIObject(x,160,96,36,GUIObjectButton,sButton[i],nValue[i]);
			obj->EventCallback=&objHandler;
			GUIObjectRoot->ChildControls.push_back(obj);
		}
	}
	//===
	SDL_FillRect(screen,NULL,0);
	SDL_SetAlpha(s_temp, SDL_SRCALPHA, 100);
	SDL_BlitSurface(s_temp,NULL,screen,NULL);
	while(GUIObjectRoot){
		while(SDL_PollEvent(&event)) GUIObjectHandleEvents();
		if(GUIObjectRoot) GUIObjectRoot->render();
		SDL_Flip(screen);
		SDL_Delay(30);
	}
	GUIObjectRoot=tmp;
	return (eMsgBoxResult)objHandler.ret;
}

struct cFileDialogHandler:public GUIEventCallback{
public:
	bool ret,is_save,verify_file,files;
	GUIObject* txtName;
	GUIListBox* lstFile;
	const char* sExtension;
	string sFileName,sPath;
	vector<string> sSearchPath;
public:
	cFileDialogHandler(bool is_save=false,bool verify_file=false, bool files=true):ret(false),is_save(is_save),verify_file(verify_file),files(files),txtName(NULL){}
	void GUIEventCallback_OnEvent(std::string Name,GUIObject* obj,int nEventType){
		if(Name=="cmdOK"){
			std::string s=txtName->Caption;
			if(s.empty() || s.find_first_of("*?")!=string::npos) return;
			//verify?
			if(is_save){
				FILE *f;
				f=fopen(ProcessFileName(s).c_str(),"rb");
				if(f){
					fclose(f);
					if(MsgBox(s+" already exists.\nDo you want to overwrite it?",MsgBoxYesNo,"Overwrite Prompt")!=MsgBoxYes){
						return;
					}
				}
				if(verify_file && files){
					f=fopen(ProcessFileName(s).c_str(),"wb");
					if(f){
						fclose(f);
					}else{
						MsgBox("Can't open file "+s+".",MsgBoxOKOnly,"Error");
						return;
					}
				}
			}else if(verify_file && files){
				FILE *f;
				f=fopen(ProcessFileName(s).c_str(),"rb");
				if(f){
					fclose(f);
				}else{
					MsgBox("Can't open file "+s+".",MsgBoxOKOnly,"Error");
					return;
				}
			}
			//over
			sFileName=s;
			if(GUIObjectRoot){
				delete GUIObjectRoot;
				GUIObjectRoot=NULL;
			}
			ret=true;
		}else if(Name=="cmdCancel"){
			if(GUIObjectRoot){
				delete GUIObjectRoot;
				GUIObjectRoot=NULL;
			}
		}else if(Name=="lstFile"){
			GUIListBox *obj1=lstFile; //dynamic_cast<GUIListBox*>(obj);
			if(obj1!=NULL && txtName!=NULL && obj1->Value>=0 && obj1->Value<(int)obj1->Item.size()){
				txtName->Caption=sPath+obj1->Item[obj1->Value];
			}
		}else if(Name=="lstSearchIn"){
			GUISingleLineListBox *obj1=dynamic_cast<GUISingleLineListBox*>(obj);
			if(obj1!=NULL && lstFile!=NULL && obj1->Value>=0 && obj1->Value<(int)sSearchPath.size()){
				string s;
				sPath=sSearchPath[obj1->Value];
				if(!sPath.empty()){
					s=ProcessFileName(sPath);
				}else{
					s=get_user_path();
				}
				if(files) {
					lstFile->Item=EnumAllFiles(s,sExtension);
				}else
					lstFile->Item=EnumAllDirs(s);
				lstFile->Value=-1;
			}
		}
	}
};

bool FileDialog(string& FileName,const char* sTitle,const char* sExtension,const char* sPath,bool is_save,bool verify_file,bool files){
	GUIObject *obj,*tmp=GUIObjectRoot;
	cFileDialogHandler objHandler(is_save,verify_file,files);
	vector<string> sPathNames;
	//===
	objHandler.sExtension=sExtension;
	if(sPath && sPath[0]){
		char *lp=(char*)sPath;
		char *lps=strchr(lp,'\n');
		char *lpe;
		if(lps){
			for(;;){
				objHandler.sSearchPath.push_back(string(lp,lps-lp));
				lpe=strchr(lps+1,'\n');
				if(lpe){
					sPathNames.push_back(string(lps+1,lpe-lps-1));
					lp=lpe+1;
	}else{
					sPathNames.push_back(string(lps+1));
					break;
				}
				lps=strchr(lp,'\n');
				if(!lps) break;
			}
		}else{
			objHandler.sSearchPath.push_back(sPath);
		}
	}else{
		objHandler.sSearchPath.push_back(string());
	}
	//===
	int base_y=sPathNames.size()>0?40:0;
	GUIObjectRoot=new GUIObject(100,100-base_y/2,600,400+base_y,GUIObjectFrame,sTitle?sTitle:(is_save?"Save File":"Load File"));
	if(sPathNames.size()>0){
		GUIObjectRoot->ChildControls.push_back(new GUIObject(8,20,184,36,GUIObjectLabel,"Search In"));
		GUISingleLineListBox *obj1=new GUISingleLineListBox(160,20,432,36);
		obj1->Item=sPathNames;
		obj1->Value=0;
		obj1->Name="lstSearchIn";
		obj1->EventCallback=&objHandler;
		GUIObjectRoot->ChildControls.push_back(obj1);
	}
	//===
	GUIObjectRoot->ChildControls.push_back(new GUIObject(8,20+base_y,184,36,GUIObjectLabel,"File Name"));
	{
		string s=FileName;
		if(s.empty() && sExtension && sExtension[0]) s=string("*.")+string(sExtension);
		objHandler.txtName=new GUIObject(160,20+base_y,432,36,GUIObjectTextBox,s.c_str());
		GUIObjectRoot->ChildControls.push_back(objHandler.txtName);
	}
	{
		GUIListBox *obj1=new GUIListBox(8,60+base_y,584,292);
		string s=objHandler.sSearchPath[0];
		if(!s.empty()){
			objHandler.sPath=s;
			s=ProcessFileName(s);
		}else{
			s=get_user_path();
		}
		if(files) {
			obj1->Item=EnumAllFiles(s,sExtension);
		} else 
			obj1->Item=EnumAllDirs(s);
		obj1->Name="lstFile";
		obj1->EventCallback=&objHandler;
		GUIObjectRoot->ChildControls.push_back(obj1);
		objHandler.lstFile=obj1;
	}
	obj=new GUIObject(200,360+base_y,192,36,GUIObjectButton,"OK");
	obj->Name="cmdOK";
	obj->EventCallback=&objHandler;
	GUIObjectRoot->ChildControls.push_back(obj);
	obj=new GUIObject(400,360+base_y,192,36,GUIObjectButton,"Cancel");
	obj->Name="cmdCancel";
	obj->EventCallback=&objHandler;
	GUIObjectRoot->ChildControls.push_back(obj);
	//===
	SDL_FillRect(screen,NULL,0);
	SDL_SetAlpha(s_temp, SDL_SRCALPHA, 100);
	SDL_BlitSurface(s_temp,NULL,screen,NULL);
	while(GUIObjectRoot){
		while(SDL_PollEvent(&event)) GUIObjectHandleEvents();
		if(GUIObjectRoot) GUIObjectRoot->render();
		SDL_Flip(screen);
		SDL_Delay(30);
	}
	GUIObjectRoot=tmp;
	//===
	if(objHandler.ret) FileName=objHandler.sFileName;
	return objHandler.ret;
}
