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
#include "Functions.h"
#include "GameState.h"
#include "Globals.h"
#include "TitleMenu.h"
#include "GUIListBox.h"
#include <iostream>
using namespace std;

/////////////////////////MAIN_MENU//////////////////////////////////

//Integer containing the highlighted/selected menu option.
static int highlight=0;

Menu::Menu(){
	background=loadImage(getDataPath()+"gfx/menu/menu.png");
	highlight=0;
}

Menu::~Menu(){}


void Menu::handleEvents(){
	//Get the x and y location of the mouse.
	int x,y;
	SDL_GetMouseState(&x,&y);

	//Calculate which option is highlighted using the location of the mouse.
	highlight=0;
	if(x>=200&&x<600&&y>=150&&y<550){
		highlight=(y-70)/80;
	}

	//Check if there's a press event.
	if(event.type==SDL_MOUSEBUTTONUP && event.button.button==SDL_BUTTON_LEFT){
		//We have one so check which selected/highlighted option needs to be done.
		switch(highlight){
		case 1:
			//Enter the levelSelect state.
			setNextState(STATE_LEVEL_SELECT);
			break;
		case 2:
			//Enter the options state.
			setNextState(STATE_OPTIONS);
			break;
		case 3:
			//Enter the levelEditor, but first set the level to a default leveledit map.
			levelName="";
			setNextState(STATE_LEVEL_EDITOR);
			break;
		case 4:
			//Enter the help state.
			setNextState(STATE_HELP);
			break;
		case 5:
			//We quit, so we enter the exit state.
			setNextState(STATE_EXIT);
			break;
		}
	}

	//Check if we need to quit, if so we enter the exit state.
	if(event.type == SDL_QUIT){
		setNextState(STATE_EXIT);
	}
}

//Nothing to do here
void Menu::logic(){}


void Menu::render(){
	applySurface(0,0,background,screen,NULL);
	
	//Check if an option is selected/highlighted.
	if(highlight>0){
		SDL_Rect r,r1;
		r.x=200;
		r.y=70+80*highlight;
		r.w=400;
		r.h=1;
		SDL_FillRect(screen,&r,0);
		r1.x=200;
		r1.y=r.y;
		r1.w=1;
		r1.h=80;
		SDL_FillRect(screen,&r1,0);
		r1.x=600;
		SDL_FillRect(screen,&r1,0);
		r.y+=80;
		SDL_FillRect(screen,&r,0);
	}
}


/////////////////////////HELP_MENU//////////////////////////////////
Help::Help(){
	background=loadImage(getDataPath()+"gfx/menu/help.png");
}

Help::~Help(){}

void Help::handleEvents(){
	//Check if a button is pressed, if so we go back to the main menu.
	if(event.type==SDL_KEYUP || event.type==SDL_MOUSEBUTTONUP){
		setNextState(STATE_MENU);
	}

	//Check if we need to quit, if so we enter the exit state.
	if(event.type==SDL_QUIT){
		setNextState(STATE_EXIT);
	}
}

//Nothing to do here.
void Help::logic(){}

void Help::render(){
	applySurface(0,0,background,screen,NULL);
}


/////////////////////////OPTIONS_MENU//////////////////////////////////

//Some varables for the options.
static bool sound,fullscreen,leveltheme,internet;
static string themeName;

//new
static bool useProxy;
static string internetProxy;

Options::Options(){
	//Load the background image.
	background=loadImage(getDataPath()+"gfx/menu/options.png");
	
	//Set some default settings.
	sound=getSettings()->getBoolValue("sound");
	fullscreen=getSettings()->getBoolValue("fullscreen");
	themeName=processFileName(getSettings()->getValue("theme"));
	leveltheme=getSettings()->getBoolValue("leveltheme");
	internet=getSettings()->getBoolValue("internet");
	internetProxy=getSettings()->getValue("internet-proxy");
	useProxy=!internetProxy.empty();
	
	//Create the root element of the GUI.
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}
	GUIObjectRoot=new GUIObject(100,(SCREEN_HEIGHT-400)/2 + 50,600,350,GUIObjectFrame,"");

	//Now we create GUIObjects for every option.
	GUIObject *obj=new GUIObject(50,20,240,36,GUIObjectCheckBox,"Sound",sound?1:0);
	obj->Name="chkSound";
	obj->EventCallback=this;
	GUIObjectRoot->ChildControls.push_back(obj);
		
	obj=new GUIObject(50,60,240,36,GUIObjectCheckBox,"Fullscreen",fullscreen?1:0);
	obj->Name="chkFullscreen";
	obj->EventCallback=this;
	GUIObjectRoot->ChildControls.push_back(obj);
	
	obj=new GUIObject(50,100,240,36,GUIObjectLabel,"Theme:");
	obj->Name="theme";
	GUIObjectRoot->ChildControls.push_back(obj);
	
	//Create the theme option gui element.
	theme=new GUISingleLineListBox(250,100,300,36);
	theme->Name="lstTheme";
	vector<string> v=enumAllDirs(getUserPath()+"themes/");
	for(vector<string>::iterator i = v.begin(); i != v.end(); ++i){
		themeLocations[*i]=getUserPath()+"themes/"+*i;
	}
	vector<string> v2=enumAllDirs(getDataPath()+"themes/");
	for(vector<string>::iterator i = v2.begin(); i != v2.end(); ++i){
		themeLocations[*i]=getDataPath()+"themes/"+*i;
	}
	v.insert(v.end(), v2.begin(), v2.end());

	//Try to find the configured theme so we can display it.
	int value = -1;
	for(vector<string>::iterator i = v.begin(); i != v.end(); ++i){
		if(themeLocations[*i]==themeName) {
			value=i - v.begin();
		}
	}
	theme->Item=v;
	if(value == -1)
		value=theme->Item.size() - 1;
	theme->Value=value;
	theme->EventCallback=this;
	GUIObjectRoot->ChildControls.push_back(theme);

	obj=new GUIObject(50,140,240,36,GUIObjectCheckBox,"Level themes",leveltheme?1:0);
	obj->Name="chkLeveltheme";
	obj->EventCallback=this;
	GUIObjectRoot->ChildControls.push_back(obj);
	
	obj=new GUIObject(50,180,240,36,GUIObjectCheckBox,"Internet",internet?1:0);
	obj->Name="chkInternet";
	obj->EventCallback=this;
	GUIObjectRoot->ChildControls.push_back(obj);

	//new: proxy settings
	obj=new GUIObject(50,220,240,36,GUIObjectCheckBox,"Internet proxy",useProxy?1:0);
	obj->Name="chkProxy";
	obj->EventCallback=this;
	GUIObjectRoot->ChildControls.push_back(obj);
	obj=new GUIObject(250,220,300,36,GUIObjectTextBox,internetProxy.c_str());
	obj->Name="txtProxy";
	obj->EventCallback=this;
	GUIObjectRoot->ChildControls.push_back(obj);

	obj=new GUIObject(10,300,284,36,GUIObjectButton,"Back");
	obj->Name="cmdBack";
	obj->EventCallback=this;
	GUIObjectRoot->ChildControls.push_back(obj);
		
	obj=new GUIObject(306,300,284,36,GUIObjectButton,"Save");
	obj->Name="cmdSave";
	obj->EventCallback=this;
	GUIObjectRoot->ChildControls.push_back(obj);
	
	restartLabel=new GUIObject(10,250,284,36,GUIObjectLabel,"You need to restart before the changes have effect.");
	restartLabel->Name="restart";
	restartLabel->Visible=false;
	GUIObjectRoot->ChildControls.push_back(restartLabel);
}

Options::~Options(){
	//Also delete the GUI.
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}
}

void Options::GUIEventCallback_OnEvent(std::string name,GUIObject* obj,int eventType){
	//Check what type of event it was.
	if(eventType==GUIEventClick){
		if(name=="cmdBack"){
			setNextState(STATE_MENU);
		}
		else if(name=="cmdSave"){
			if(!useProxy) internetProxy.clear();
			getSettings()->setValue("internet-proxy",internetProxy);
			saveSettings();
		}
		else if(name=="chkSound"){
			sound=obj->Value?true:false;
			getSettings()->setValue("sound",sound?"1":"0");
			if(!sound){
				Mix_HaltMusic();
			}else{
				Mix_PlayMusic(music,-1);
			}
		}
		else if(name=="chkFullscreen"){
			fullscreen=obj->Value?true:false;
			getSettings()->setValue("fullscreen",fullscreen?"1":"0");
			
			//Set the restart text visible.
			restartLabel->Visible = true;
		}
		else if(name=="chkLeveltheme"){
			leveltheme=obj->Value?true:false;
			getSettings()->setValue("leveltheme",leveltheme?"1":"0");
		}
		else if(name=="chkInternet"){
			internet=obj->Value?true:false;
			getSettings()->setValue("internet",internet?"1":"0");
		}
		else if(name=="chkProxy"){
			useProxy=obj->Value?true:false;
		}
	}
	if(name=="lstTheme"){
		if(theme!=NULL && theme->Value>=0 && theme->Value<(int)theme->Item.size()){
			//Check if the theme is installed in the data path.
			if(themeLocations[theme->Item[theme->Value]].find(getDataPath())!=string::npos){
				getSettings()->setValue("theme","%DATA%/themes/"+fileNameFromPath(themeLocations[theme->Item[theme->Value]]));
			}else if(themeLocations[theme->Item[theme->Value]].find(getUserPath())!=string::npos){
				getSettings()->setValue("theme","%USER%/themes/"+fileNameFromPath(themeLocations[theme->Item[theme->Value]]));
			}else{
				getSettings()->setValue("theme",themeLocations[theme->Item[theme->Value]]);
			}
		}
	}
	else if(name=="txtProxy"){
		internetProxy=obj->Caption;
	}
}

void Options::handleEvents(){
	//Check if we need to quit, if so enter the exit state.
	if(event.type==SDL_QUIT){
		setNextState(STATE_EXIT);
	}

	//Check if the escape button is pressed, if so go back to the main menu.
	if(event.type==SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE){
		setNextState(STATE_MENU);
	}
}

//Nothing to do here.
void Options::logic(){}

void Options::render(){
	applySurface(0,0,background,screen,NULL);
	//The rendering of the GUI is done in Main.
}