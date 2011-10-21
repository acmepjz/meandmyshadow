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
#include "FileManager.h"
#include "Objects.h"
#include "Player.h"
#include "GameObjects.h"
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

ImageManager imageManager;

Settings* settings=0;

SDL_Surface* loadImage(string file){
	return imageManager.loadImage(file);
}

void applySurface(int x, int y, SDL_Surface* source, SDL_Surface* dest, SDL_Rect* clip ){
	SDL_Rect offset;
	offset.x=x;
	offset.y=y;

	SDL_BlitSurface(source,clip,dest,&offset);
}

bool init(){
	//Initialze SDL.
	if(SDL_Init(SDL_INIT_EVERYTHING)==-1) {
		fprintf(stderr,"FATAL ERROR: SDL_Init failed\n");
		return false;
	}

	//Initialze SDL_mixer (audio).
	if(Mix_OpenAudio(22050,MIX_DEFAULT_FORMAT,2,512)==-1){
		fprintf(stderr,"FATAL ERROR: Mix_OpenAudio failed\n");
		return false;
	}

	//Initialze SDL_ttf (fonts).
	if(TTF_Init()==-1){
		fprintf(stderr,"FATAL ERROR: TTF_Init failed\n");
		return false;
	}

	//Initialise the screen.
	screen=SDL_SetVideoMode(SCREEN_WIDTH,SCREEN_HEIGHT,SCREEN_BPP,SDL_HWSURFACE | SDL_DOUBLEBUF /*|SDL_FULLSCREEN*/ );
	if(screen==NULL){
		fprintf(stderr,"FATAL ERROR: SDL_SetVideoMode failed\n");
		return false;
	}

	//Set the the window caption.
	SDL_WM_SetCaption(("Me and my shadow "+version).c_str(),NULL);
	SDL_EnableUNICODE(1);

	//Create the types of blocks.
	for(int i=0;i<TYPE_MAX;i++){
		Game::g_BlockNameMap[Game::g_sBlockName[i]]=i;
	}

	//Nothing went wrong so we return true.
	return true;
}

bool loadFiles(){
	//Load the music.
	music = Mix_LoadMUS((getDataPath()+"sfx/music.mid").c_str());
	if(music==NULL){
		printf("WARNING: Unable to load background music! \n");
	}
	
	//Load the fonts.
	font=TTF_OpenFont((getDataPath()+"font/ComicBook.ttf").c_str(),28);
	fontSmall=TTF_OpenFont((getDataPath()+"font/ComicBook.ttf").c_str(),20);
	if(font==NULL || fontSmall==NULL){
		printf("ERROR: Unable to load fonts! \n");
		return false;
	}

	//Load the default theme.
	if(objThemes.appendThemeFromFile(getDataPath()+"themes/default/theme.mnmstheme")==NULL){
		printf("ERROR: Can't load default theme file\n");
		return false;
	}
	
	//Nothing failed so return true.
	return true;
}

bool loadSettings(){
	settings=new Settings(getUserPath()+"meandmyshadow.cfg");
	settings->parseFile();
  
	//Always return true?
	return true;
}

bool saveSettings(){
	settings->save();

	//Always return true?
	return true;
}

Settings* getSettings(){
	return settings;
}

void clean(){
	delete settings;
	settings=NULL;

	if(currentState) delete currentState;

	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}
	imageManager.destroy();
	TTF_CloseFont(font);
	TTF_CloseFont(fontSmall);
	TTF_Quit();
	SDL_Quit();
	Mix_CloseAudio();
}

void setNextState(int newstate){
	if(nextState!=STATE_EXIT){
		nextState = newstate;
	}
}

void changeState(){
	if(nextState!=STATE_NULL){
		delete currentState;
		currentState=NULL;

		stateID = nextState;
		nextState = STATE_NULL;

		switch(stateID){
		case STATE_GAME:
			currentState=new Game();
			break;
		case STATE_MENU:
			levels.clear();
			currentState=new Menu();
			break;
		case STATE_HELP:
			currentState=new Help();
			break;
		case STATE_LEVEL_SELECT:
			levels.loadLevels(getDataPath()+"/levelpacks/default/levels.lst",getUserPath()+"progress/default.progress");
			currentState = new LevelSelect();
			break;
		case STATE_LEVEL_EDITOR:
			levels.clear();
			currentState=new LevelEditor();
			break;
		case STATE_OPTIONS:
			currentState=new Options();
			break;
		case STATE_ADDONS:
			currentState=new Addons();
			break;  
		}

		//Fade out
		SDL_BlitSurface(screen,NULL,tempSurface,NULL);
		for(int i=255;i>=0;i-=17){
			SDL_FillRect(screen,NULL,0);
			SDL_SetAlpha(tempSurface, SDL_SRCALPHA, i);
			SDL_BlitSurface(tempSurface,NULL,screen,NULL);
			SDL_Flip(screen);
			SDL_Delay(25);
		}
	}
}

bool checkCollision(const SDL_Rect& a,const SDL_Rect& b){
	//Check if the left side of box a isn't past the right side of b.
	if(a.x>=b.x+b.w){
		return false;
	}
	//Check if the right side of box a isn't left of the left side of b.
	if(a.x+a.w<=b.x){
		return false;
	}
	//Check if the top side of box a isn't under the bottom side of b.
	if(a.y>=b.y+b.h){
		return false;
	}
	//Check if the bottom side of box a isn't above the top side of b.
	if(a.y+a.h<=b.y){
		return false;
	}

	//We have collision.
	return true;
}

void setCamera(){
	//SetCamera only works in the Level editor.
	if(stateID==STATE_LEVEL_EDITOR){
		//Get the mouse coordinates.
		int x, y;
		SDL_GetMouseState(&x,&y);

		//Check if the mouse is near the left edge of the screen.
		//Else check if the mouse is near the right edge.
		if(x<50){
			//We're near the left edge so move the camera.
			camera.x-=10;
			//Make sure that the camera doesn't get negative.
			if(camera.x<0) camera.x=0;
		}else if(x>=SCREEN_WIDTH-50){
			//We're near the right edge so move the camera.
			camera.x+=10;
			//Make sure the camera doesn't move past the level width.
			if(camera.x>LEVEL_WIDTH-SCREEN_WIDTH) camera.x=LEVEL_WIDTH-SCREEN_WIDTH;
		}

		//Check if the mouse is near the top edge of the screen.
		//Else check if the mouse is near the bottom edge.
		if(y<50){
			//We're near the top edge so move the camera.
			camera.y-=10;
			//Make sure that the camera doesn't get negative.
			if(camera.y<0) camera.y=0;
		}else if(y>=SCREEN_HEIGHT-50){
			//We're near the bottom edge so move the camera.
			camera.y+=10;
			//Make sure the camera doesn't move past the level height.
			if(camera.y>LEVEL_HEIGHT-SCREEN_HEIGHT) camera.y=LEVEL_HEIGHT-SCREEN_HEIGHT;
		}
	}
}

bool parseArguments(int argc, char** argv){
	//Loop through all arguments.
	//We start at one since 0 is the command itself.
	for(int i=1;i<argc;i++){
		string argument=argv[i];
		
		//Check if the argument is the data-dir.
		if(argument=="--data-dir"){
			//We need a second argument so we increase i.
			i++;
			if(i>=argc){
				printf("ERROR: Missing argument for command '%s'\n\n",argument.c_str());
				return false;
			}
			
			//Configure the dataPath with the given path.
			dataPath=argv[i];
			if(!getDataPath().empty()){
				char c=dataPath[dataPath.size()-1];
				if(c!='/'&&c!='\\') dataPath+="/";
			}
		}else if(argument=="--user-dir"){
			//We need a second argument so we increase i.
			i++;
			if(i>=argc){
				printf("ERROR: Missing argument for command '%s'\n\n",argument.c_str());
				return false;
			}
			
			//Configure the userPath with the given path.
			userPath=argv[i];
			if(!userPath.empty()){
				char c=userPath[userPath.size()-1];
				if(c!='/'&&c!='\\') userPath+="/";
			}
		}else if(argument=="-v" || argument=="-version" || argument=="--version"){
			//Print the version.
			printf("Version: '%s'\n\n",version.c_str());
		}else if(argument=="-h" || argument=="-help" || argument=="--help"){
			//If the help is requested we'll return false without printing an error.
			//This way the usage/help text will be printed.
			return false;
		}else{
			//Any other argument is unknow so we return false.
			printf("ERROR: Unknown argument %s\n\n",argument.c_str());
			return false;
		}
	}
	
	//If everything went well we can return true.
	return true;
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

msgBoxResult msgBox(string Prompt,msgBoxButtons Buttons,const string& Title){
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
	SDL_SetAlpha(tempSurface, SDL_SRCALPHA, 100);
	SDL_BlitSurface(tempSurface,NULL,screen,NULL);
	while(GUIObjectRoot){
		while(SDL_PollEvent(&event)) GUIObjectHandleEvents();
		if(GUIObjectRoot) GUIObjectRoot->render();
		SDL_Flip(screen);
		SDL_Delay(30);
	}
	GUIObjectRoot=tmp;
	return (msgBoxResult)objHandler.ret;
}

struct cFileDialogHandler:public GUIEventCallback{
public:
	bool ret,isSave,verifyFile,files;
	GUIObject* txtName;
	GUIListBox* lstFile;
	const char* extension;
	string sFileName,path;
	vector<string> sSearchPath;
public:
	cFileDialogHandler(bool isSave=false,bool verifyFile=false, bool files=true):ret(false),isSave(isSave),verifyFile(verifyFile),files(files),txtName(NULL){}
	void GUIEventCallback_OnEvent(std::string Name,GUIObject* obj,int nEventType){
		if(Name=="cmdOK"){
			std::string s=txtName->Caption;

			if(s.find_first_of("/")==string::npos) s=path+s;

			if(s.empty() || s.find_first_of("*?")!=string::npos) return;
			//verify?
			if(isSave){
				FILE *f;
				f=fopen(processFileName(s).c_str(),"rb");
				if(f){
					fclose(f);
					if(msgBox(s+" already exists.\nDo you want to overwrite it?",MsgBoxYesNo,"Overwrite Prompt")!=MsgBoxYes){
						return;
					}
				}
				if(verifyFile && files){
					f=fopen(processFileName(s).c_str(),"wb");
					if(f){
						fclose(f);
					}else{
						msgBox("Can't open file "+s+".",MsgBoxOKOnly,"Error");
						return;
					}
				}
			}else if(verifyFile && files){
				FILE *f;
				f=fopen(processFileName(s).c_str(),"rb");
				if(f){
					fclose(f);
				}else{
					msgBox("Can't open file "+s+".",MsgBoxOKOnly,"Error");
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
				txtName->Caption=path+obj1->Item[obj1->Value];
			}
		}else if(Name=="lstSearchIn"){
			GUISingleLineListBox *obj1=dynamic_cast<GUISingleLineListBox*>(obj);
			if(obj1!=NULL && lstFile!=NULL && obj1->Value>=0 && obj1->Value<(int)sSearchPath.size()){
				string s;
				path=sSearchPath[obj1->Value];
				if(!path.empty()){
					s=processFileName(path);
				}else{
					s=getUserPath();
				}
				if(files) {
					lstFile->Item=enumAllFiles(s,extension);
				}else
					lstFile->Item=enumAllDirs(s);
				lstFile->Value=-1;
			}
		}
	}
};

bool fileDialog(string& fileName,const char* title,const char* extension,const char* path,bool isSave,bool verifyFile,bool files){
	GUIObject *obj,*tmp=GUIObjectRoot;
	cFileDialogHandler objHandler(isSave,verifyFile,files);
	vector<string> pathNames;
	//===
	objHandler.extension=extension;
	if(path && path[0]){
		char *lp=(char*)path;
		char *lps=strchr(lp,'\n');
		char *lpe;
		if(lps){
			for(;;){
				objHandler.sSearchPath.push_back(string(lp,lps-lp));
				lpe=strchr(lps+1,'\n');
				if(lpe){
					pathNames.push_back(string(lps+1,lpe-lps-1));
					lp=lpe+1;
	}else{
					pathNames.push_back(string(lps+1));
					break;
				}
				lps=strchr(lp,'\n');
				if(!lps) break;
			}
		}else{
			objHandler.sSearchPath.push_back(path);
		}
	}else{
		objHandler.sSearchPath.push_back(string());
	}
	//===
	int base_y=pathNames.size()>0?40:0;
	GUIObjectRoot=new GUIObject(100,100-base_y/2,600,400+base_y,GUIObjectFrame,title?title:(isSave?"Save File":"Load File"));
	if(pathNames.size()>0){
		GUIObjectRoot->ChildControls.push_back(new GUIObject(8,20,184,36,GUIObjectLabel,"Search In"));
		GUISingleLineListBox *obj1=new GUISingleLineListBox(160,20,432,36);
		obj1->Item=pathNames;
		obj1->Value=0;
		obj1->Name="lstSearchIn";
		obj1->EventCallback=&objHandler;
		GUIObjectRoot->ChildControls.push_back(obj1);
	}
	//===
	GUIObjectRoot->ChildControls.push_back(new GUIObject(8,20+base_y,184,36,GUIObjectLabel,"File Name"));
	{
		string s=fileName;
		if(s.empty() && extension && extension[0]) s=string("*.")+string(extension);
		objHandler.txtName=new GUIObject(160,20+base_y,432,36,GUIObjectTextBox,s.c_str());
		GUIObjectRoot->ChildControls.push_back(objHandler.txtName);
	}
	{
		GUIListBox *obj1=new GUIListBox(8,60+base_y,584,292);
		string s=objHandler.sSearchPath[0];
		if(!s.empty()){
			objHandler.path=s;
			s=processFileName(s);
		}else{
			s=getUserPath();
		}
		if(files) {
			obj1->Item=enumAllFiles(s,extension);
		} else 
			obj1->Item=enumAllDirs(s);
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
	SDL_SetAlpha(tempSurface, SDL_SRCALPHA, 100);
	SDL_BlitSurface(tempSurface,NULL,screen,NULL);
	while(GUIObjectRoot){
		while(SDL_PollEvent(&event)) GUIObjectHandleEvents();
		if(GUIObjectRoot) GUIObjectRoot->render();
		SDL_Flip(screen);
		SDL_Delay(30);
	}
	GUIObjectRoot=tmp;
	//===
	if(objHandler.ret) fileName=objHandler.sFileName;
	return objHandler.ret;
}
