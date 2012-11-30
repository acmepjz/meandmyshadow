/*
 * Copyright (C) 2011-2012 Me and My Shadow
 *
 * This file is part of Me and My Shadow.
 *
 * Me and My Shadow is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Me and My Shadow is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Me and My Shadow.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Functions.h"
#include "GameState.h"
#include "Globals.h"
#include "TitleMenu.h"
#include "GUIListBox.h"
#include "InputManager.h"
#include "StatisticsManager.h"
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
	//SDL_Color black={0,0,0};
	entries[0]=TTF_RenderUTF8_Blended(fontTitle,_("Play"),themeTextColor);
	entries[1]=TTF_RenderUTF8_Blended(fontTitle,_("Options"),themeTextColor);
	entries[2]=TTF_RenderUTF8_Blended(fontTitle,_("Map Editor"),themeTextColor);
	entries[3]=TTF_RenderUTF8_Blended(fontTitle,_("Addons"),themeTextColor);
	entries[4]=TTF_RenderUTF8_Blended(fontTitle,_("Quit"),themeTextColor);
	entries[5]=TTF_RenderUTF8_Blended(fontTitle,">",themeTextColor);
	entries[6]=TTF_RenderUTF8_Blended(fontTitle,"<",themeTextColor);

	//Load the credits icon.
	statisticsIcon=loadImage(getDataPath()+"gfx/menu/statistics.png");
	creditsIcon=loadImage(getDataPath()+"gfx/menu/credits.png");
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
		highlight=0;

		if(x>=200&&x<SCREEN_WIDTH-200&&y>=(SCREEN_HEIGHT-250)/2&&y<(SCREEN_HEIGHT-200)/2+320){
			highlight=(y-((SCREEN_HEIGHT-200)/2-64))/64;
			if(highlight>0) currentCursor=CURSOR_POINT;
			if(highlight>5) highlight=0;
		}

		//Also check the icons.
		if(y>=SCREEN_HEIGHT-56&&y<SCREEN_HEIGHT-8){
			if(x>=SCREEN_WIDTH-8){
				//do nothing
			}else if(x>=SCREEN_WIDTH-56){
				highlight=7;
				currentCursor=CURSOR_POINT;
			}else if(x>=SCREEN_WIDTH-104){
				highlight=6;
				currentCursor=CURSOR_POINT;
			}
		}
	}
	
	//Down/Up -arrows move highlight
	if(inputMgr.isKeyDownEvent(INPUTMGR_DOWN)){
		highlight++;
		if(highlight>7)
			highlight=0;
	}
	if(inputMgr.isKeyDownEvent(INPUTMGR_UP)){
		highlight--;
		if(highlight<1)
			highlight=7;
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
			
			//Enter the addons state.
			setNextState(STATE_ADDONS);
			break;
		case 5:
			//We quit, so we enter the exit state.
			setNextState(STATE_EXIT);
			break;
		case 6:
			//Show the statistics screen.
			setNextState(STATE_STATISTICS);
			break;
		case 7:
			//Show the credits screen.
			setNextState(STATE_CREDITS);
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
	//Draw background.
	objThemes.getBackground(true)->draw(screen);
	objThemes.getBackground(true)->updateAnimation();
	
	//Draw the title.
	applySurface((SCREEN_WIDTH-title->w)/2,40,title,screen,NULL);
	
	//Draw the menu entries.
	for(unsigned int i=0;i<5;i++){
		applySurface((SCREEN_WIDTH-entries[i]->w)/2,(SCREEN_HEIGHT-200)/2+64*i+(64-entries[i]->h)/2,entries[i],screen,NULL);
	}
	
	//Check if an option is selected/highlighted.
	if(highlight>0 && highlight<=5){
		//Draw the '>' sign, which is entry 5.
		int x=(SCREEN_WIDTH-entries[highlight-1]->w)/2-(25-abs(animation)/2)-entries[5]->w;
		int y=(SCREEN_HEIGHT-200)/2-64+64*highlight+(64-entries[5]->h)/2;
		applySurface(x,y,entries[5],screen,NULL);
		
		//Draw the '<' sign, which is entry 6.
		x=(SCREEN_WIDTH-entries[highlight-1]->w)/2+entries[highlight-1]->w+(25-abs(animation)/2);
		y=(SCREEN_HEIGHT-200)/2-64+64*highlight+(64-entries[6]->h)/2;
		applySurface(x,y,entries[6],screen,NULL);
	}

	//Check if an icon is selected/highlighted and draw tooltip
	if(highlight==6){
		SDL_Color fg={0,0,0};
		SDL_Surface *surface=TTF_RenderUTF8_Blended(fontText,_("Achievements and Statistics"),fg);
		drawGUIBox(SCREEN_WIDTH-64-surface->w-2,SCREEN_HEIGHT-56-surface->h-2,surface->w+4,surface->h+4,screen,0xFFFFFF00|230);
		applySurface(SCREEN_WIDTH-64-surface->w,SCREEN_HEIGHT-56-surface->h,surface,screen,NULL);
		SDL_FreeSurface(surface);
	}
	if(highlight==7){
		SDL_Color fg={0,0,0};
		SDL_Surface *surface=TTF_RenderUTF8_Blended(fontText,_("Credits"),fg);
		drawGUIBox(SCREEN_WIDTH-16-surface->w-2,SCREEN_HEIGHT-56-surface->h-2,surface->w+4,surface->h+4,screen,0xFFFFFF00|230);
		applySurface(SCREEN_WIDTH-16-surface->w,SCREEN_HEIGHT-56-surface->h,surface,screen,NULL);
		SDL_FreeSurface(surface);
	}

	//Draw the credits icon.
	applySurface(SCREEN_WIDTH-96,SCREEN_HEIGHT-48,statisticsIcon,screen,NULL);
	applySurface(SCREEN_WIDTH-48,SCREEN_HEIGHT-48,creditsIcon,screen,NULL);
}

void Menu::resize(){}


/////////////////////////OPTIONS_MENU//////////////////////////////////

//Some varables for the options.
static bool fullscreen,leveltheme,internet;
static string themeName,languageName;
static int lastLang,lastRes;

static bool useProxy;
static string internetProxy;

static bool restartFlag;

static _res currentRes;
static vector<_res> resolutionList;

Options::Options(){
	//Render the title.
	title=TTF_RenderUTF8_Blended(fontTitle,_("Settings"),themeTextColor);
	
	lastJumpSound=0;
	
	//Set some default settings.
	fullscreen=getSettings()->getBoolValue("fullscreen");
	languageName=getSettings()->getValue("lang");
	themeName=processFileName(getSettings()->getValue("theme"));
	leveltheme=getSettings()->getBoolValue("leveltheme");
	internet=getSettings()->getBoolValue("internet");
	internetProxy=getSettings()->getValue("internet-proxy");
	useProxy=!internetProxy.empty();
	
	//Set the restartFlag false.
	restartFlag=false;
	
	//Now create the gui.
	createGUI();
}

Options::~Options(){
	//Delete the GUI.
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}
	
	//Free the title image.
	SDL_FreeSurface(title);
}

void Options::createGUI(){
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
	GUIObject* obj=new GUIObject(x,150-liftY,240,36,GUIObjectLabel,_("Music"));
	GUIObjectRoot->addChild(obj);
	
	musicSlider=new GUISlider(x+220,150-liftY,256,36,atoi(getSettings()->getValue("music").c_str()),0,128,15);
	musicSlider->name="sldMusic";
	musicSlider->eventCallback=this;
	GUIObjectRoot->addChild(musicSlider);
	
	obj=new GUIObject(x,190-liftY,240,36,GUIObjectLabel,_("Sound"));
	GUIObjectRoot->addChild(obj);
	
	soundSlider=new GUISlider(x+220,190-liftY,256,36,atoi(getSettings()->getValue("sound").c_str()),0,128,15);
	soundSlider->name="sldSound";
	soundSlider->eventCallback=this;
	GUIObjectRoot->addChild(soundSlider);
		
	obj=new GUIObject(x,230-liftY,240,36,GUIObjectCheckBox,_("Fullscreen"),fullscreen?1:0);
	obj->name="chkFullscreen";
	obj->eventCallback=this;
	GUIObjectRoot->addChild(obj);
	
	obj=new GUIObject(x,270-liftY,240,36,GUIObjectLabel,_("Resolution"));
	obj->name="lstResolution";
	GUIObjectRoot->addChild(obj);
	
	//Create list with many different resolutions
	resolutions = new GUISingleLineListBox(x+220,270-liftY,300,36);
	resolutions->value=-1;
	
	//Enumerate avaliable resolutions using SDL_ListModes()
	//Note: we enumerate fullscreen resolutions because
	// windowed resolutions always can be arbitrary
	if(resolutionList.empty()){
		SDL_Rect **modes=SDL_ListModes(NULL,SDL_FULLSCREEN|SDL_HWSURFACE);

		if(modes==NULL || ((intptr_t)modes) == -1){
			cout<<"Error: Can't enumerate avaliable screen resolutions."
				" Use predefined screen resolutions list instead."<<endl;

			static const _res predefinedResolutionList[] = {
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

			for(unsigned int i=0;i<sizeof(predefinedResolutionList)/sizeof(_res);i++){
				resolutionList.push_back(predefinedResolutionList[i]);
			}
		}else{
			for(unsigned int i=0;modes[i]!=NULL;i++){
				//Check if the resolution is big enough
				if(modes[i]->w>=800 && modes[i]->h>=600){
					_res res={modes[i]->w, modes[i]->h};
					resolutionList.push_back(res);
				}
			}
			reverse(resolutionList.begin(),resolutionList.end());
		}
	}
	
	//Get current resolution from config file. Thus it can be user defined
	currentRes.w=atoi(getSettings()->getValue("width").c_str());
	currentRes.h=atoi(getSettings()->getValue("height").c_str());
	
	for (int i=0; i<(int)resolutionList.size();i++){
		//Create a string from width and height and then add it to list
		ostringstream out;
		out << resolutionList[i].w << "x" << resolutionList[i].h;
		resolutions->item.push_back(out.str());
		
		//Check if current resolution matches, select it
		if (resolutionList[i].w==currentRes.w && resolutionList[i].h==currentRes.h){
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
	
	GUIObjectRoot->addChild(resolutions);
	
	obj=new GUIObject(x,310-liftY,240,36,GUIObjectLabel,_("Language"));
	obj->name="lstResolution";
	GUIObjectRoot->addChild(obj);
	
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
		if(getSettings()->getValue("lang")==s0->str()){
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
	GUIObjectRoot->addChild(langs);
	
	obj=new GUIObject(x,350-liftY,240,36,GUIObjectLabel,_("Theme"));
	obj->name="theme";
	GUIObjectRoot->addChild(obj);
	
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
	GUIObjectRoot->addChild(theme);

	obj=new GUIObject(x,390-liftY,240,36,GUIObjectCheckBox,_("Level themes"),leveltheme?1:0);
	obj->name="chkLeveltheme";
	obj->eventCallback=this;
	GUIObjectRoot->addChild(obj);
	
	obj=new GUIObject(x,430-liftY,240,36,GUIObjectCheckBox,_("Internet"),internet?1:0);
	obj->name="chkInternet";
	obj->eventCallback=this;
	GUIObjectRoot->addChild(obj);

	//new: proxy settings
	obj=new GUIObject(x,470-liftY,240,36,GUIObjectLabel,_("Internet proxy"));
	obj->name="chkProxy";
	obj->eventCallback=this;
	GUIObjectRoot->addChild(obj);
	obj=new GUIObject(x+220,470-liftY,300,36,GUIObjectTextBox,internetProxy.c_str());
	obj->name="txtProxy";
	obj->eventCallback=this;
	GUIObjectRoot->addChild(obj);

	//new: key settings
	GUIObject* b1=new GUIObject(SCREEN_WIDTH*0.3,SCREEN_HEIGHT-120,-1,36,GUIObjectButton,_("Config Keys"),0,true,true,GUIGravityCenter);
	b1->name="cmdKeys";
	b1->eventCallback=this;
	GUIObjectRoot->addChild(b1);
	
	//Reset progress settings.
	/// TRANSLATORS: Used for button which clear any level progress like unlocked levels and highscores.
	GUIObject* b2=new GUIObject(SCREEN_WIDTH*0.7,SCREEN_HEIGHT-120,-1,36,GUIObjectButton,_("Clear Progress"),0,true,true,GUIGravityCenter);
	b2->name="cmdReset";
	b2->eventCallback=this;
	GUIObjectRoot->addChild(b2);
	
	b1->render(0,0,false);
	b2->render(0,0,false);
	if(b2->left-b2->gravityX < b1->left+b1->width-b1->gravityX){
		b1->smallFont=true;
		b1->width=-1;
		b2->smallFont=true;
		b2->width=-1;
	}

	b1=new GUIObject(SCREEN_WIDTH*0.3,SCREEN_HEIGHT-60,-1,36,GUIObjectButton,_("Cancel"),0,true,true,GUIGravityCenter);
	b1->name="cmdBack";
	b1->eventCallback=this;
	GUIObjectRoot->addChild(b1);
		
	b2=new GUIObject(SCREEN_WIDTH*0.7,SCREEN_HEIGHT-60,-1,36,GUIObjectButton,_("Save Changes"),0,true,true,GUIGravityCenter);
	b2->name="cmdSave";
	b2->eventCallback=this;
	GUIObjectRoot->addChild(b2);
	
	b1->render(0,0,false);
	b2->render(0,0,false);
	if(b2->left-b2->gravityX < b1->left+b1->width-b1->gravityX){
		b1->smallFont=true;
		b1->width=-1;
		b2->smallFont=true;
		b2->width=-1;
	}
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
			
			//Reset the music volume.
			getMusicManager()->setVolume(atoi(getSettings()->getValue("music").c_str()));
			Mix_Volume(-1,atoi(getSettings()->getValue("sound").c_str()));
			
			//And goto the main menu.
			setNextState(STATE_MENU);
		}else if(name=="cmdSave"){
			//Save is pressed thus save 
			char s[64];
			sprintf(s,"%d",soundSlider->value);
			getSettings()->setValue("sound",s);
			sprintf(s,"%d",musicSlider->value);
			getSettings()->setValue("music",s);
			getMusicManager()->setEnabled(musicSlider->value>0);
			Mix_Volume(-1,soundSlider->value);
			getSettings()->setValue("fullscreen",fullscreen?"1":"0");
			getSettings()->setValue("leveltheme",leveltheme?"1":"0");
			getSettings()->setValue("internet",internet?"1":"0");
			getSettings()->setValue("theme",themeName);
			loadTheme(themeName);
			if(!useProxy)
				internetProxy.clear();
			getSettings()->setValue("internet-proxy",internetProxy);
			
			getSettings()->setValue("lang",langValues.at(langs->value));
			
			//Is resolution from the list or is it user defined in config file
			if(resolutions->value<(int)resolutionList.size()){
				getSettings()->setValue("width",convertInt(resolutionList[resolutions->value].w));
				getSettings()->setValue("height",convertInt(resolutionList[resolutions->value].h));
			}else{
				getSettings()->setValue("width",convertInt(currentRes.w));
				getSettings()->setValue("height",convertInt(currentRes.h));
			}
			
			//Save the key configuration.
			inputMgr.saveConfig();
			
			//Save the settings.
			saveSettings();
			
			//Before we return check if some .
			if(restartFlag || resolutions->value!=lastRes){
				//The resolution changed so we need to recreate the screen.
				if(!createScreen()){
					//Screen creation failed so set to safe settings.
					getSettings()->setValue("fullscreen","0");
					getSettings()->setValue("width",convertInt(resolutionList[lastRes].w));
					getSettings()->setValue("height",convertInt(resolutionList[lastRes].h));
					
					if(!createScreen()){
						//Everything fails so quit.
						setNextState(STATE_EXIT);
						return;
					}
				}
				
				//The screen is created, now load the (menu) theme.
				if(!loadTheme("")){
					//Loading the theme failed so quit.
					setNextState(STATE_EXIT);
					return;
				}
			}
			if(langs->value!=lastLang){
				//We set the language.
				language=langValues.at(langs->value);
				dictionaryManager->set_language(tinygettext::Language::from_name(langValues.at(langs->value)));
				getLevelPackManager()->updateLanguage();
				
				//And reload the font.
				if(!loadFonts()){
					//Loading failed so quit.
					setNextState(STATE_EXIT);
					return;
				}
			}
			
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
				//Resets statistics.
				statsMgr.reloadCompletedLevelsAndAchievements();
			}
			return;
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
	}else if(name=="sldMusic"){
		getMusicManager()->setEnabled(musicSlider->value>0);
		getMusicManager()->setVolume(musicSlider->value);
	}else if(name=="sldSound"){
		Mix_Volume(-1,soundSlider->value);
		if(lastJumpSound==0){
			Mix_PlayChannel(-1,jumpSound,0);
			lastJumpSound=15;
		}
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

void Options::logic(){
	//Increase the lastJumpSound variable if needed.
	if(lastJumpSound!=0){
		lastJumpSound--;
	}
}

void Options::render(){
	//Draw background.
	objThemes.getBackground(true)->draw(screen);
	objThemes.getBackground(true)->updateAnimation();
	//Now render the title.
	applySurface((SCREEN_WIDTH-title->w)/2,40-TITLE_FONT_RAISE,title,screen,NULL);
	
	//NOTE: The rendering of the GUI is done in Main.
}

void Options::resize(){
	//Recreate the gui to fit the new resolution.
	createGUI();
}

/////////////////////////CREDITS_MENU//////////////////////////////////

Credits::Credits(){
	//Render the title.
	title=TTF_RenderUTF8_Blended(fontTitle,_("Credits"),themeTextColor);

	//Vector that will hold every line of the credits.
	vector<string> credits;

	//Open the AUTHORS file and read every line.
	{
		ifstream fin((getDataPath()+"/../AUTHORS").c_str());
		if(!fin.is_open()) {
			cerr<<"ERROR: Unable to open the AUTHORS file."<<endl;
			credits.push_back("ERROR: Unable to open the AUTHORS file.");
			credits.push_back("");
		}

		//Loop the lines of the file.
		string line;
		while(getline(fin,line)){
			credits.push_back(line);
		}
	}

	//Enter a new line between the two files.
	credits.push_back("");
	
	//Open the Credits.text file and read every line.
	{
		ifstream fin((getDataPath()+"/Credits.txt").c_str());
		if(!fin.is_open()) {
			cerr<<"ERROR: Unable to open the Credits.txt file."<<endl;
			credits.push_back("ERROR: Unable to open the Credits.txt file.");
			credits.push_back("");
		}

		//Loop the lines of the file.
		string line;
		while(getline(fin,line)){
			credits.push_back(line);
			
			//NOTE: Some sections point to other credits files.
			if(line=="music/") {
				vector<string> musicCredits=getMusicManager()->createCredits();
				credits.insert(credits.end(),musicCredits.begin(),musicCredits.end());
			}
		}
	}

	//Now determine the number of lines and calculate the height of the resulting credits surface.
	int lines=credits.size();
	int fontHeight=TTF_FontLineSkip(fontText);
	int maxW=0;
	
	//Find out the width of the longest line
	for(int i=0;i<lines;i++){
		if(credits[i][0]!='\0'){
			int w;
			TTF_SizeUTF8(fontText,credits[i].c_str(),&w,NULL);
			
			if(w>maxW)
				maxW=w;
		}
	}
	
	//Finally create the surface and draw every line of text there
	creditsText=SDL_CreateRGBSurface(SDL_SWSURFACE,maxW,lines*fontHeight,32,RMASK,GMASK,BMASK,AMASK);
	
	for(int i=0;i<lines;i++){
		if(credits[i][0]!='\0'){
			SDL_Surface* lineSurf=TTF_RenderUTF8_Blended(fontText,credits[i].c_str(),themeTextColor);
		
			SDL_SetAlpha(lineSurf,0,0xFF);
			SDL_SetAlpha(creditsText,SDL_SRCALPHA,SDL_ALPHA_TRANSPARENT);
		
			applySurface(0,fontHeight*i,lineSurf,creditsText,NULL);
		
			SDL_FreeSurface(lineSurf);
		}
	}
	
	//Create GUI
	createGUI();
}

Credits::~Credits(){
	//Delete the GUI.
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}
	
	//Free images
	SDL_FreeSurface(title);
	SDL_FreeSurface(creditsText);
}

void Credits::createGUI(){
	//Create the root element of the GUI.
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}
	GUIObjectRoot=new GUIObject(0,0,SCREEN_WIDTH,SCREEN_HEIGHT,GUIObjectNone);
	
	//Create back button.
	GUIObject* obj=new GUIObject(SCREEN_WIDTH*0.5,SCREEN_HEIGHT-60,-1,36,GUIObjectButton,_("Back"),0,true,true,GUIGravityCenter);
	obj->name="cmdBack";
	obj->eventCallback=this;
	GUIObjectRoot->addChild(obj);
	
	//Create vertical scrollbar.
	scrollbarV=new GUIScrollBar(SCREEN_WIDTH-64-16,128,16,SCREEN_HEIGHT-128-92,1,0,0,creditsText->h/8-(SCREEN_HEIGHT-128-92)/8);
	GUIObjectRoot->addChild(scrollbarV);
	
	//If text is too long, create horizontal scrollbar.
	if(creditsText->w>SCREEN_WIDTH-128){
		scrollbarH=new GUIScrollBar(64,SCREEN_HEIGHT-92,SCREEN_WIDTH-128-16,16,0,0,0,creditsText->w/8-(SCREEN_WIDTH-64-92)/8);
		GUIObjectRoot->addChild(scrollbarH);
	}else{
		scrollbarH=NULL;
	}
}

void Credits::GUIEventCallback_OnEvent(std::string name,GUIObject* obj,int eventType){
	//Check what type of event it was.
	if(eventType==GUIEventClick){
		if(name=="cmdBack"){
			//Goto the main menu.
			setNextState(STATE_MENU);
		}
	}
}

void  Credits::handleEvents(){
	//Check if we need to quit, if so enter the exit state.
	if(event.type==SDL_QUIT){
		setNextState(STATE_EXIT);
	}

	//Check if the escape button is pressed, if so go back to the main menu.
	if(inputMgr.isKeyUpEvent(INPUTMGR_ESCAPE)){
		setNextState(STATE_MENU);
	}
	
	//Check for scrolling down and up with mouse scroll wheel.
	if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button==SDL_BUTTON_WHEELDOWN && scrollbarV){
		if(scrollbarV->value<scrollbarV->maxValue)
			scrollbarV->value+=scrollbarV->smallChange;
		if(scrollbarV->value>scrollbarV->maxValue)
			scrollbarV->value=scrollbarV->maxValue;
		return;
	}else if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button==SDL_BUTTON_WHEELUP && scrollbarV){
		if(scrollbarV->value>0)
			scrollbarV->value-=scrollbarV->smallChange;
		if(scrollbarV->value<0)
			scrollbarV->value=0;
		return;
	}
}

void Credits::logic(){

}

void Credits::render(){
	//Draw background.
	objThemes.getBackground(true)->draw(screen);
	objThemes.getBackground(true)->updateAnimation();
	//Now render the title.
	applySurface((SCREEN_WIDTH-title->w)/2,40-TITLE_FONT_RAISE,title,screen,NULL);
	
	//Clip and draw text accoring to scrollbars' values.
	SDL_Rect r;
	if(scrollbarH)
		r.x = scrollbarH->value*8;
	else
		r.x = 0;
	r.y = scrollbarV->value*8;
	r.w = SCREEN_WIDTH-128-16;
	r.h = SCREEN_HEIGHT-128-92;
	applySurface(64,128,creditsText,screen,&r);
	
	//NOTE: The rendering of the GUI is done in Main.
}

void Credits::resize(){
	//Recreate the gui to fit the new resolution.
	createGUI();
}
