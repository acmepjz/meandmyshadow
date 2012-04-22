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
#include "InputManager.h"
#include <iostream>
#include <algorithm>
#include <sstream>

#include "libs/tinygettext/tinygettext.hpp"

using namespace std;

/////////////////////////MAIN_MENU//////////////////////////////////

//Integer containing the highlighted/selected menu option.
static int highlight=0;

Menu::Menu(){
	highlight=0;
	animation=0;
	
	//Load the title image.
	title=loadImage(getDataPath()+"gfx/menu/title.png");
	
	//Now render the five entries.
	SDL_Color black={0,0,0};
	entries[0]=TTF_RenderUTF8_Blended(fontTitle,_("Play"),black);
	entries[1]=TTF_RenderUTF8_Blended(fontTitle,_("Options"),black);
	entries[2]=TTF_RenderUTF8_Blended(fontTitle,_("Map Editor"),black);
	entries[3]=TTF_RenderUTF8_Blended(fontTitle,_("Addons"),black);
	entries[4]=TTF_RenderUTF8_Blended(fontTitle,_("Exit"),black);
	entries[5]=TTF_RenderUTF8_Blended(fontTitle,">",black);
	entries[6]=TTF_RenderUTF8_Blended(fontTitle,"<",black);
}

Menu::~Menu(){
	//We need to free the five text surfaceses.
	for(unsigned int i=0;i<7;i++)
		SDL_FreeSurface(entries[i]);
}


void Menu::handleEvents(){
	//Get the x and y location of the mouse.
	int x,y;
	SDL_GetMouseState(&x,&y);

	//Calculate which option is highlighted using the location of the mouse.
	//Only if mouse is 'doing something'
	if(event.type==SDL_MOUSEMOTION || event.type==SDL_MOUSEBUTTONDOWN){
		if(x>=200&&x<SCREEN_WIDTH-200&&y>=(SCREEN_HEIGHT-200)/2&&y<(SCREEN_HEIGHT-200)/2+320){
			highlight=(y-((SCREEN_HEIGHT-200)/2-64))/64;
		}
	}
	
	//Down/Up -arrows move highlight
	if(inputMgr.isKeyDownEvent(INPUTMGR_DOWN)){
		highlight++;
		if(highlight>=6)
			highlight=5;
	}
	if(inputMgr.isKeyDownEvent(INPUTMGR_UP)){
		highlight--;
		if(highlight<1)
			highlight=1;
	}
	
	//Check if there's a press event.
	if((event.type==SDL_MOUSEBUTTONUP && event.button.button==SDL_BUTTON_LEFT) ||
		(inputMgr.isKeyUpEvent(INPUTMGR_SELECT))){
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
			setNextState(STATE_LEVEL_EDIT_SELECT);
			break;
		case 4:
			//Check if internet is enabled.
			if(!getSettings()->getBoolValue("internet")){
				msgBox(_("Enable internet in order to install addons."),MsgBoxOKOnly,_("Internet disabled"));
				break;
			}
			
			//Enter the help state.
			setNextState(STATE_ADDONS);
			break;
		case 5:
			//We quit, so we enter the exit state.
			setNextState(STATE_EXIT);
			break;
		}
	}
	
	//We also need to quit the menu when escape is pressed.
	if(inputMgr.isKeyUpEvent(INPUTMGR_ESCAPE)){
		setNextState(STATE_EXIT);
	}
	
	//Check if we need to quit, if so we enter the exit state.
	if(event.type==SDL_QUIT){
		setNextState(STATE_EXIT);
	}
}

//Nothing to do here
void Menu::logic(){
	animation++;
	if(animation>10)
		animation=-10;
}


void Menu::render(){
	applySurface(0,0,menuBackground,screen,NULL);
	
	//Draw the title.
	applySurface((SCREEN_WIDTH-title->w)/2,40,title,screen,NULL);
	
	//Draw the menu entries.
	for(unsigned int i=0;i<5;i++){
		applySurface((SCREEN_WIDTH-entries[i]->w)/2,(SCREEN_HEIGHT-200)/2+64*i+(64-entries[i]->h)/2,entries[i],screen,NULL);
	}
	
	//Check if an option is selected/highlighted.
	if(highlight>0){
		//Draw the '>' sign, which is entry 5.
		int x=(SCREEN_WIDTH-entries[highlight-1]->w)/2-(25-abs(animation)/2)-entries[5]->w;
		int y=(SCREEN_HEIGHT-200)/2-64+64*highlight+(64-entries[5]->h)/2;
		applySurface(x,y,entries[5],screen,NULL);
		
		//Draw the '<' sign, which is entry 6.
		x=(SCREEN_WIDTH-entries[highlight-1]->w)/2+entries[highlight-1]->w+(25-abs(animation)/2);
		y=(SCREEN_HEIGHT-200)/2-64+64*highlight+(64-entries[6]->h)/2;
		applySurface(x,y,entries[6],screen,NULL);
	}
}


/////////////////////////OPTIONS_MENU//////////////////////////////////

//Some varables for the options.
static bool sound,music,fullscreen,leveltheme,internet;
static string themeName,languageName;
static int lastLang,lastRes;

static bool useProxy;
static string internetProxy;

static bool restartFlag;

static _res currentRes;
#define RES_COUNT 19
static _res resolution_list[RES_COUNT] = {
	{800,600},
	{1024,600},
	{1024,768},
	{1152,864},
	{1280,720},
	{1280,768},
	{1280,800},
	{1280,960},
	{1280,1024},
	{1360,768},
	{1366,768},
	{1440,900},
	{1600,900},
	{1600,1200},
	{1680,1080},
	{1920,1080},
	{1920,1200},
	{2560,1440},
	{3840,2160}
};

Options::Options(){
	//Render the title.
	SDL_Color black={0,0,0};
	title=TTF_RenderUTF8_Blended(fontTitle,_("Settings"),black);
	
	//Set some default settings.
	music=getSettings()->getBoolValue("music");
	sound=getSettings()->getBoolValue("sound");
	fullscreen=getSettings()->getBoolValue("fullscreen");
	languageName=getSettings()->getValue("lang");
	themeName=processFileName(getSettings()->getValue("theme"));
	leveltheme=getSettings()->getBoolValue("leveltheme");
	internet=getSettings()->getBoolValue("internet");
	internetProxy=getSettings()->getValue("internet-proxy");
	useProxy=!internetProxy.empty();
	
	//Set the restartFlag false.
	restartFlag=false;
	
	//Variables for positioning
	int x = (SCREEN_WIDTH-540)/2;
	int liftY=40; //TODO: This is variable for laziness of maths...
	
	//Create the root element of the GUI.
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}
	GUIObjectRoot=new GUIObject(0,0,SCREEN_WIDTH,SCREEN_HEIGHT,GUIObjectNone);

	//Now we create GUIObjects for every option.
	GUIObject *obj=new GUIObject(x,150-liftY,240,36,GUIObjectCheckBox,_("Music"),music?1:0);
	obj->name="chkMusic";
	obj->eventCallback=this;
	GUIObjectRoot->childControls.push_back(obj);
	
	obj=new GUIObject(x,190-liftY,240,36,GUIObjectCheckBox,_("Sound"),sound?1:0);
	obj->name="chkSound";
	obj->eventCallback=this;
	GUIObjectRoot->childControls.push_back(obj);
		
	obj=new GUIObject(x,230-liftY,240,36,GUIObjectCheckBox,_("Fullscreen"),fullscreen?1:0);
	obj->name="chkFullscreen";
	obj->eventCallback=this;
	GUIObjectRoot->childControls.push_back(obj);
	
	obj=new GUIObject(x,270-liftY,240,36,GUIObjectLabel,_("Resolution:"));
	obj->name="lstResolution";
	GUIObjectRoot->childControls.push_back(obj);
	
	//Create list with many different resolutions
	resolutions = new GUISingleLineListBox(x+220,270-liftY,300,36);
	resolutions->value=-1;
	
	//Get current resolution from config file. Thus it can be user defined
	currentRes.w=atoi(getSettings()->getValue("width").c_str());
	currentRes.h=atoi(getSettings()->getValue("height").c_str());
	
	for (int i=0; i<RES_COUNT;i++){
		//Create a string from width and height and then add it to list
		ostringstream out;
		out << resolution_list[i].w << "x" << resolution_list[i].h;
		resolutions->item.push_back(out.str());
		
		//Check if current resolution matches, select it
		if (resolution_list[i].w==currentRes.w && resolution_list[i].h==currentRes.h){
			resolutions->value=i;
		}
	}
	
	//Add current resolution if it isn't already in the list
	if(resolutions->value==-1){
		ostringstream out;
		out << currentRes.w << "x" << currentRes.h;
		resolutions->item.push_back(out.str());
		resolutions->value=resolutions->item.size()-1;
	}
	lastRes=resolutions->value;
	
	GUIObjectRoot->childControls.push_back(resolutions);
	
	obj=new GUIObject(x,310-liftY,240,36,GUIObjectLabel,_("Language:"));
	obj->name="lstResolution";
	GUIObjectRoot->childControls.push_back(obj);
	
	//Create GUI list with available languages
	langs = new GUISingleLineListBox(x+220,310-liftY,300,36);
	langs->name="lstLanguages";
	
	/// TRANSLATORS: as detect user's language automatically
	langs->item.push_back(_("Auto-Detect"));
	langValues.push_back("");
	
	langs->item.push_back("English");
	langValues.push_back("en");
	
	//Get a list of every available language
	set<tinygettext::Language> languages = dictionaryManager->get_languages();
	for (set<tinygettext::Language>::iterator s0 = languages.begin(); s0 != languages.end(); ++s0){
		//If language in loop is the same in config file, then select it
		if (getSettings()->getValue("lang")==s0->str()){
			lastLang=distance(languages.begin(),s0)+2;
		}
		//Add language in loop to list and listbox
		langs->item.push_back(s0->get_name());
		langValues.push_back(s0->str());
	}
	
	//If Auto or English are selected
	if(getSettings()->getValue("lang")==""){
		lastLang=0;
	}else if(getSettings()->getValue("lang")=="en"){
		lastLang=1;
	}
	
	langs->value=lastLang;
	GUIObjectRoot->childControls.push_back(langs);
	
	obj=new GUIObject(x,350-liftY,240,36,GUIObjectLabel,_("Theme:"));
	obj->name="theme";
	GUIObjectRoot->childControls.push_back(obj);
	
	//Create the theme option gui element.
	theme=new GUISingleLineListBox(x+220,350-liftY,300,36);
	theme->name="lstTheme";
	vector<string> v=enumAllDirs(getUserPath(USER_DATA)+"themes/");
	for(vector<string>::iterator i = v.begin(); i != v.end(); ++i){
		themeLocations[*i]=getUserPath(USER_DATA)+"themes/"+*i;
	}
	vector<string> v2=enumAllDirs(getDataPath()+"themes/");
	for(vector<string>::iterator i = v2.begin(); i != v2.end(); ++i){
		themeLocations[*i]=getDataPath()+"themes/"+*i;
	}
	v.insert(v.end(), v2.begin(), v2.end());

	//Try to find the configured theme so we can display it.
	int value=-1;
	for(vector<string>::iterator i = v.begin(); i != v.end(); ++i){
		if(themeLocations[*i]==themeName) {
			value=i-v.begin();
		}
	}
	theme->item=v;
	if(value==-1)
		value=theme->item.size()-1;
	theme->value=value;
	//NOTE: We call the event handling method to correctly set the themename.
	GUIEventCallback_OnEvent("lstTheme",theme,GUIEventChange);
	theme->eventCallback=this;
	GUIObjectRoot->childControls.push_back(theme);

	obj=new GUIObject(x,390-liftY,240,36,GUIObjectCheckBox,_("Level themes"),leveltheme?1:0);
	obj->name="chkLeveltheme";
	obj->eventCallback=this;
	GUIObjectRoot->childControls.push_back(obj);
	
	obj=new GUIObject(x,430-liftY,240,36,GUIObjectCheckBox,_("Internet"),internet?1:0);
	obj->name="chkInternet";
	obj->eventCallback=this;
	GUIObjectRoot->childControls.push_back(obj);

	//new: proxy settings
	obj=new GUIObject(x,470-liftY,240,36,GUIObjectLabel,_("Internet proxy"));
	obj->name="chkProxy";
	obj->eventCallback=this;
	GUIObjectRoot->childControls.push_back(obj);
	obj=new GUIObject(x+220,470-liftY,300,36,GUIObjectTextBox,internetProxy.c_str());
	obj->name="txtProxy";
	obj->eventCallback=this;
	GUIObjectRoot->childControls.push_back(obj);

	//new: key settings
	obj=new GUIObject((SCREEN_WIDTH-240)/2-150,SCREEN_HEIGHT-120,240,36,GUIObjectButton,_("Config Keys"));
	obj->name="cmdKeys";
	obj->eventCallback=this;
	GUIObjectRoot->childControls.push_back(obj);
	
	//Reset progress settings.
	/// TRANSLATORS: Used for button which clear any level progress like unlocked levels and highscores.
	obj=new GUIObject((SCREEN_WIDTH-260)/2+150,SCREEN_HEIGHT-120,260,36,GUIObjectButton,_("Clear Progress"));
	obj->name="cmdReset";
	obj->eventCallback=this;
	GUIObjectRoot->childControls.push_back(obj);

	obj=new GUIObject((SCREEN_WIDTH-284)/2-150,SCREEN_HEIGHT-60,284,36,GUIObjectButton,_("Cancel"));
	obj->name="cmdBack";
	obj->eventCallback=this;
	GUIObjectRoot->childControls.push_back(obj);
		
	obj=new GUIObject((SCREEN_WIDTH-284)/2+150,SCREEN_HEIGHT-60,284,36,GUIObjectButton,_("Save Changes"));
	obj->name="cmdSave";
	obj->eventCallback=this;
	GUIObjectRoot->childControls.push_back(obj);
}

Options::~Options(){
	//Delete the GUI.
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}
	
	//And free the title image.
	SDL_FreeSurface(title);
}

static string convertInt(int i){
	stringstream ss;
	ss << i;
	return ss.str();
}

void Options::GUIEventCallback_OnEvent(std::string name,GUIObject* obj,int eventType){
	//Check what type of event it was.
	if(eventType==GUIEventClick){
		if(name=="cmdBack"){
			//TODO: Reset the key changes.
			
			//And goto the main menu.
			setNextState(STATE_MENU);
		}else if(name=="cmdSave"){
			//Save is pressed thus save 
			getSettings()->setValue("sound",sound?"1":"0");
			getSettings()->setValue("music",music?"1":"0");
			getMusicManager()->setEnabled(music);
			getSettings()->setValue("fullscreen",fullscreen?"1":"0");
			getSettings()->setValue("leveltheme",leveltheme?"1":"0");
			getSettings()->setValue("internet",internet?"1":"0");
			getSettings()->setValue("theme",themeName);
			if(!useProxy)
				internetProxy.clear();
			getSettings()->setValue("internet-proxy",internetProxy);
			
			getSettings()->setValue("lang",langValues.at(langs->value));
			//language=langValues.at(langs->value);
			//dictionaryManager->set_language(tinygettext::Language::from_name(langValues.at(langs->value)));
			
			//Is resolution from the list or is it user defined in config file
			if(resolutions->value<RES_COUNT){
				getSettings()->setValue("width",convertInt(resolution_list[resolutions->value].w));
				getSettings()->setValue("height",convertInt(resolution_list[resolutions->value].h));
			}else{
				getSettings()->setValue("width",convertInt(currentRes.w));
				getSettings()->setValue("height",convertInt(currentRes.h));
			}
			
			//Save the key configuration.
			inputMgr.saveConfig();
			
			//Save the settings.
			saveSettings();
			
			//Before we return show a restart message, if needed.
			if(restartFlag || langs->value!=lastLang || resolutions->value!=lastRes)
				/// TRANSLATORS: Some settings require to restart the game
				msgBox(_("Restart needed before the changes have effect."),MsgBoxOKOnly,_("Restart needed"));
			
			//Now return to the main menu.
			setNextState(STATE_MENU);
		}else if(name=="cmdKeys"){
			inputMgr.showConfig();
		}else if(name=="cmdReset"){
			if(msgBox(_("Do you really want to reset level progress?"),MsgBoxYesNo,_("Warning"))==MsgBoxYes){
				//We delete the progress folder.
#ifdef WIN32
				removeDirectory((getUserPath()+"progress").c_str());
				createDirectory((getUserPath()+"progress").c_str());
#else
				removeDirectory((getUserPath(USER_DATA)+"/progress").c_str());
				createDirectory((getUserPath(USER_DATA)+"/progress").c_str());
#endif
			}
			return;
		}else if(name=="chkMusic"){
			music=obj->value?true:false;
		}else if(name=="chkSound"){
			sound=obj->value?true:false;
		}else if(name=="chkFullscreen"){
			fullscreen=obj->value?true:false;
			
			//Check if fullscreen changed.
			if(fullscreen==getSettings()->getBoolValue("fullscreen")){
				//We disable the restart message flag.
				restartFlag=false;
			}else{
				//We set the restart message flag.
				restartFlag=true;
			}
			  
		}else if(name=="chkLeveltheme"){
			leveltheme=obj->value?true:false;
		}else if(name=="chkInternet"){
			internet=obj->value?true:false;
		}else if(name=="chkProxy"){
			useProxy=obj->value?true:false;
		}
	}
	if(name=="lstTheme"){
		if(theme!=NULL && theme->value>=0 && theme->value<(int)theme->item.size()){
			//Check if the theme is installed in the data path.
			if(themeLocations[theme->item[theme->value]].find(getDataPath())!=string::npos){
				themeName="%DATA%/themes/"+fileNameFromPath(themeLocations[theme->item[theme->value]]);
			}else if(themeLocations[theme->item[theme->value]].find(getUserPath(USER_DATA))!=string::npos){
				themeName="%USER%/themes/"+fileNameFromPath(themeLocations[theme->item[theme->value]]);
			}else{
				themeName=themeLocations[theme->item[theme->value]];
			}
		}
	}else if(name=="txtProxy"){
		internetProxy=obj->caption;
		
		//Check if the internetProxy field is empty.
		useProxy=!internetProxy.empty();
	}
}

void Options::handleEvents(){
	//Check if we need to quit, if so enter the exit state.
	if(event.type==SDL_QUIT){
		setNextState(STATE_EXIT);
	}

	//Check if the escape button is pressed, if so go back to the main menu.
	if(inputMgr.isKeyUpEvent(INPUTMGR_ESCAPE)){
		setNextState(STATE_MENU);
	}
}

//Nothing to do here.
void Options::logic(){}

void Options::render(){
	//Render the menu background image.
	applySurface(0,0,menuBackground,screen,NULL);
	//Now render the title.
	applySurface((SCREEN_WIDTH-title->w)/2,40,title,screen,NULL);
	
	//NOTE: The rendering of the GUI is done in Main.
}
