/*
 * Copyright (C) 2011-2013 Me and My Shadow
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
#include "TitleMenu.h"
#include "ThemeManager.h"
#include "GUIListBox.h"
#include "GUITextArea.h"
#include "InputManager.h"
#include "LevelPackManager.h"
#include "StatisticsManager.h"
#include "MusicManager.h"
#include "SoundManager.h"
#include <iostream>
#include <algorithm>
#include <sstream>

#include "libs/tinygettext/tinygettext.hpp"

using namespace std;

/////////////////////////MAIN_MENU//////////////////////////////////

Menu::Menu(ImageManager &imageManager, SDL_Renderer& renderer){
	animation=highlight=0;
	
	//Load the title image.
    titleTexture=imageManager.loadTexture(getDataPath()+"gfx/menu/title.png",renderer);


    auto tft = [&](const char* text){
        return textureFromText(renderer, *fontTitle, text, objThemes.getTextColor(false));
    };

    //Now render the five entries.
    entries[0]=tft(_("Play"));
    entries[1]=tft(_("Options"));
    entries[2]=tft(_("Map Editor"));
    entries[3]=tft(_("Addons"));
    entries[4]=tft(_("Quit"));
    entries[5]=tft(">");
    entries[6]=tft("<");

    //Load the textures for the credits and statistics buttons and their tooltips.
    creditsIcon=imageManager.loadTexture(getDataPath()+"gfx/menu/credits.png", renderer);
    creditsTooltip=textureFromText(renderer, *fontText, _("Credits"), objThemes.getTextColor(true));
    statisticsIcon=imageManager.loadTexture(getDataPath()+"gfx/menu/statistics.png", renderer);
    statisticsTooltip=textureFromText(renderer, *fontText, _("Achievements and Statistics"), objThemes.getTextColor(true));

    // Check if textures were loaded.
    //TODO: Handle this better.
    if(!titleTexture || !creditsIcon || !statisticsIcon) {
        std::cerr << "Failed to load one or more images for the main menu, exiting. : " << SDL_GetError() << std::endl;
        std::terminate();
    }
}

Menu::~Menu(){
}


void Menu::handleEvents(ImageManager& imageManager, SDL_Renderer& renderer){
	//Get the x and y location of the mouse.
	int x,y;
	SDL_GetMouseState(&x,&y);

	//Calculate which option is highlighted using the location of the mouse.
	//Only if mouse is 'doing something'
	if(event.type==SDL_MOUSEMOTION || event.type==SDL_MOUSEBUTTONDOWN){
		highlight=0;

		if(x>=200&&x<SCREEN_WIDTH-200&&y>=(SCREEN_HEIGHT-250)/2&&y<(SCREEN_HEIGHT-200)/2+320){
			highlight=(y-((SCREEN_HEIGHT-200)/2-64))/64;
			if(highlight>5) highlight=0;
		}

		//Also check the icons.
		if(y>=SCREEN_HEIGHT-56&&y<SCREEN_HEIGHT-8){
			if(x>=SCREEN_WIDTH-8){
				//do nothing
			}else if(x>=SCREEN_WIDTH-56){
				highlight=7;
			}else if(x>=SCREEN_WIDTH-104){
				highlight=6;
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
                msgBox(imageManager,renderer,_("Enable internet in order to install addons."),MsgBoxOKOnly,_("Internet disabled"));
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
			//Show the credits screen.
			setNextState(STATE_CREDITS);
			break;
		case 7:
			//Show the statistics screen.
			setNextState(STATE_STATISTICS);
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

void Menu::logic(ImageManager&, SDL_Renderer&){
	animation++;
	if(animation>10)
		animation=-10;
}

void Menu::render(ImageManager&, SDL_Renderer& renderer){
	//Draw background.
    objThemes.getBackground(true)->draw(renderer);
	objThemes.getBackground(true)->updateAnimation();
	
	//Draw the title.
    {
        int titleWidth, titleHeight = 0;
        SDL_QueryTexture(titleTexture.get(), NULL, NULL, &titleWidth, &titleHeight);
        const SDL_Rect rect = SDL_Rect{(SCREEN_WIDTH-titleWidth)/2, 40, titleWidth, titleHeight};
        SDL_RenderCopy(&renderer, titleTexture.get(), NULL, &rect);
    }

    // Position where we start drawing the menu entries from.
    const int menuStartY = std::max(SCREEN_HEIGHT - 200, 0) / 2;

	//Draw the menu entries.
	for(unsigned int i=0;i<5;i++){
        SDL_Rect dstRect = rectFromTexture(0, 0, *entries[i]);
        dstRect.x = std::max(SCREEN_WIDTH - dstRect.w, 0) / 2;
        dstRect.y = menuStartY + 64*i+(64-dstRect.h) / 2;
        SDL_RenderCopy(&renderer, entries[i].get(), NULL, &dstRect);
	}
	
	//Check if an option is selected/highlighted.
	if(highlight>0 && highlight<=5){
        // Width of highlighted entry.
        const int highlightWidth = rectFromTexture(0, 0, *entries[highlight - 1]).w;
        const int leftOfHighlight = (SCREEN_WIDTH-highlightWidth)/2;
        const SDL_Rect leftSize = rectFromTexture(0, 0, *entries[5]);
        const int rightHeight = rectFromTexture(0, 0, *entries[6]).h;
        // How much to offset the arrows to create the animation.
        const int animationX = (25-abs(animation)/2);
        // The common value of both arrow's y positions.
        const int yCommon = menuStartY-64+64*highlight;

		//Draw the '>' sign, which is entry 5.
        int x=leftOfHighlight-animationX-leftSize.w;
        int y=yCommon+(64-leftSize.h)/2;
        applyTexture(x, y, *entries[5], renderer);

		//Draw the '<' sign, which is entry 6.
        x=leftOfHighlight+highlightWidth+animationX;
        y=yCommon+(64-rightHeight)/2;
        applyTexture(x, y, *entries[6], renderer);
	}

	//Check if an icon is selected/highlighted and draw tooltip
	if(highlight==6 || highlight==7){
        SDL_Texture* texture;
        if(highlight==6) {
            texture = creditsTooltip.get();
        } else {
            texture = statisticsTooltip.get();
        }
        const SDL_Rect textureSize = rectFromTexture(*texture);
        drawGUIBox(-2,SCREEN_HEIGHT-textureSize.h-2,textureSize.w+4,textureSize.h+4,renderer,0xFFFFFFFF);
        applyTexture(0, SCREEN_HEIGHT - textureSize.h, *texture, renderer);
	}

	//Draw icons.
    applyTexture(SCREEN_WIDTH-96,SCREEN_HEIGHT-48,*creditsIcon,renderer);
    applyTexture(SCREEN_WIDTH-48,SCREEN_HEIGHT-48,*statisticsIcon,renderer);
}

void Menu::resize(ImageManager &, SDL_Renderer&){}


/////////////////////////OPTIONS_MENU//////////////////////////////////

//Some variables for the options.
static bool fullscreen,internet,fade,quickrec;
static string themeName,languageName;
static int lastLang,lastRes;

static bool useProxy;
static string internetProxy;

static bool restartFlag;

static _res currentRes;
static vector<_res> resolutionList;

Options::Options(ImageManager& imageManager,SDL_Renderer& renderer){
	//Render the title.
    title=textureFromText(renderer, *fontTitle, _("Settings"), objThemes.getTextColor(false));
	
	//Initialize variables.
	lastJumpSound=0;
	clearIconHower=false;
	
    //Load icon image and tooltip text.
    clearIcon=imageManager.loadTexture(getDataPath()+"gfx/menu/clear-progress.png",renderer);
    /// TRANSLATORS: Used for button which clear any level progress like unlocked levels and highscores.
    clearTooltip=textureFromText(renderer, *fontText, _("Clear Progress"), objThemes.getTextColor(true));

	//Set some default settings.
	fullscreen=getSettings()->getBoolValue("fullscreen");
	languageName=getSettings()->getValue("lang");
	themeName=processFileName(getSettings()->getValue("theme"));
	internet=getSettings()->getBoolValue("internet");
	internetProxy=getSettings()->getValue("internet-proxy");
	useProxy=!internetProxy.empty();
	fade=getSettings()->getBoolValue("fading");
	quickrec=getSettings()->getBoolValue("quickrecord");
	
	//Set the restartFlag false.
	restartFlag=false;
	
	//Now create the gui.
    createGUI(imageManager,renderer);
}

Options::~Options(){
	//Delete the GUI.
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}
}

void Options::createGUI(ImageManager& imageManager,SDL_Renderer& renderer){
	//Variables for positioning	
	const int columnW=SCREEN_WIDTH*0.3;
	const int column1X=SCREEN_WIDTH*0.15;
	const int column2X=SCREEN_WIDTH*0.55;
	const int lineHeight=40;
	
	//Create the root element of the GUI.
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}
    GUIObjectRoot=new GUIObject(imageManager,renderer,0,0,SCREEN_WIDTH,SCREEN_HEIGHT);
	
	//Single line list for different tabs.
    GUISingleLineListBox* listBox=new GUISingleLineListBox(imageManager,renderer,(SCREEN_WIDTH-500)/2,104,500,32);
	listBox->addItem(_("General"));
	listBox->addItem(_("Controls"));
	listBox->value=0;
	listBox->name="lstTabs";
	listBox->eventCallback=this;
	GUIObjectRoot->addChild(listBox);
	
	//Create general tab.
    tabGeneral=new GUIObject(imageManager,renderer,0,150,SCREEN_WIDTH,SCREEN_HEIGHT);
	GUIObjectRoot->addChild(tabGeneral);
	
	//Now we create GUIObjects for every option.
    GUIObject* obj=new GUILabel(imageManager,renderer,column1X,0,columnW,36,_("Music"));
	tabGeneral->addChild(obj);
	
    musicSlider=new GUISlider(imageManager,renderer,column2X,0,columnW,36,atoi(getSettings()->getValue("music").c_str()),0,128,15);
	musicSlider->name="sldMusic";
	musicSlider->eventCallback=this;
	tabGeneral->addChild(musicSlider);
	
    obj=new GUILabel(imageManager,renderer,column1X,lineHeight,columnW,36,_("Sound"));
	tabGeneral->addChild(obj);
	
    soundSlider=new GUISlider(imageManager,renderer,column2X,lineHeight,columnW,36,atoi(getSettings()->getValue("sound").c_str()),0,128,15);
	soundSlider->name="sldSound";
	soundSlider->eventCallback=this;
	tabGeneral->addChild(soundSlider);
	
    obj=new GUILabel(imageManager,renderer,column1X,2*lineHeight,columnW,36,_("Resolution"));
	obj->name="lstResolution";
	tabGeneral->addChild(obj);
	
	//Create list with many different resolutions.
    resolutions = new GUISingleLineListBox(imageManager,renderer,column2X,2*lineHeight,columnW,36);
	resolutions->value=-1;
	
	//Only get the resolution list if it hasn't been done before.
	if(resolutionList.empty()){
		resolutionList=getResolutionList();
	}
	
	//Get current resolution from config file. Thus it can be user defined.
	currentRes.w=atoi(getSettings()->getValue("width").c_str());
	currentRes.h=atoi(getSettings()->getValue("height").c_str());
	
	for(int i=0; i<(int)resolutionList.size();i++){
		//Create a string from width and height and then add it to list.
		ostringstream out;
		out << resolutionList[i].w << "x" << resolutionList[i].h;
		resolutions->addItem(out.str());
		
		//Check if current resolution matches, select it.
		if(resolutionList[i].w==currentRes.w && resolutionList[i].h==currentRes.h){
			resolutions->value=i;
		}
	}
	
	//Add current resolution if it isn't already in the list.
	if(resolutions->value==-1){
		ostringstream out;
		out << currentRes.w << "x" << currentRes.h;
		resolutions->addItem(out.str());
		resolutions->value=resolutions->item.size()-1;
	}
	lastRes=resolutions->value;
	
	tabGeneral->addChild(resolutions);
	
    obj=new GUILabel(imageManager,renderer,column1X,3*lineHeight,columnW,36,_("Language"));
	tabGeneral->addChild(obj);
	
	//Create GUI list with available languages.
    langs = new GUISingleLineListBox(imageManager,renderer,column2X,3*lineHeight,columnW,36);
	langs->name="lstLanguages";
	
	/// TRANSLATORS: as detect user's language automatically
	langs->addItem("",_("Auto-Detect"));
	langs->addItem("en","English");
	
	//Get a list of every available language.
	set<tinygettext::Language> languages = dictionaryManager->get_languages();
	for (set<tinygettext::Language>::iterator s0 = languages.begin(); s0 != languages.end(); ++s0){
		//If language in loop is the same in config file, then select it
		if(getSettings()->getValue("lang")==s0->str()){
			lastLang=distance(languages.begin(),s0)+2;
		}
		//Add language in loop to list and listbox.
		langs->addItem(s0->str(),s0->get_name());
	}
	
	//If Auto or English are selected.
	if(getSettings()->getValue("lang")==""){
		lastLang=0;
	}else if(getSettings()->getValue("lang")=="en"){
		lastLang=1;
	}
	
	langs->value=lastLang;
	tabGeneral->addChild(langs);
	
    obj=new GUILabel(imageManager,renderer,column1X,4*lineHeight,columnW,36,_("Theme"));
	obj->name="theme";
	tabGeneral->addChild(obj);
	
	//Create the theme option gui element.
    theme=new GUISingleLineListBox(imageManager,renderer,column2X,4*lineHeight,columnW,36);
	theme->name="lstTheme";
	
	//Vector containing the theme locations and names.
	vector<pair<string,string> > themes;
	vector<string> v=enumAllDirs(getUserPath(USER_DATA)+"themes/");
	for(vector<string>::iterator i=v.begin(); i!=v.end(); ++i){
		string location=getUserPath(USER_DATA)+"themes/"+*i;
		themes.push_back(pair<string,string>(location,*i));
	}
	vector<string> v2=enumAllDirs(getDataPath()+"themes/");
	for(vector<string>::iterator i=v2.begin(); i!=v2.end(); ++i){
		string location=getDataPath()+"themes/"+*i;
		themes.push_back(pair<string,string>(location,*i));
	}
	
	//Try to find the configured theme so we can display it.
	int value=-1;
	for(vector<pair<string,string> >::iterator i=themes.begin(); i!=themes.end(); ++i){
		if(i->first==themeName) {
			value=i-themes.begin();
		}
	}
	theme->addItems(themes);
	if(value==-1)
		value=theme->item.size()-1;
	theme->value=value;
	//NOTE: We call the event handling method to correctly set the themename.
    GUIEventCallback_OnEvent(imageManager,renderer,"lstTheme",theme,GUIEventChange);
	theme->eventCallback=this;
	tabGeneral->addChild(theme);

	//Proxy settings.
    obj=new GUILabel(imageManager,renderer,column1X,5*lineHeight,columnW,36,_("Internet proxy"));
	obj->name="chkProxy";
	obj->eventCallback=this;
	tabGeneral->addChild(obj);
    obj=new GUITextBox(imageManager,renderer,column2X,5*lineHeight,columnW,36,internetProxy.c_str());
	obj->name="txtProxy";
	obj->eventCallback=this;
	tabGeneral->addChild(obj);
	
    obj=new GUICheckBox(imageManager,renderer,column1X,6*lineHeight,columnW,36,_("Fullscreen"),fullscreen?1:0);
	obj->name="chkFullscreen";
	obj->eventCallback=this;
	tabGeneral->addChild(obj);

    obj=new GUICheckBox(imageManager,renderer,column2X,6*lineHeight,columnW,36,_("Internet"),internet?1:0);
	obj->name="chkInternet";
	obj->eventCallback=this;
	tabGeneral->addChild(obj);
	
    obj=new GUICheckBox(imageManager,renderer,column2X,7*lineHeight,columnW,36,_("Fade transition"),fade?1:0);
	obj->name="chkFade";
	obj->eventCallback=this;
	tabGeneral->addChild(obj);
	
    obj=new GUICheckBox(imageManager,renderer,column1X,7*lineHeight,columnW,36,_("Quick record"),quickrec?1:0);
	obj->name="chkQuickRec";
	obj->eventCallback=this;
	tabGeneral->addChild(obj);
	
	//Create the controls tab.
    tabControls=inputMgr.showConfig(imageManager,renderer,SCREEN_HEIGHT-210);
	tabControls->top=140;
	tabControls->visible=false;
	GUIObjectRoot->addChild(tabControls);
	
	//Save original keys.
	for(int i=0;i<INPUTMGR_MAX;i++){
		tmpKeys[i]=inputMgr.getKeyCode((InputManagerKeys)i,false);
		tmpAlternativeKeys[i]=inputMgr.getKeyCode((InputManagerKeys)i,true);
	}
	
	//Create buttons.
    GUIObject*b1=new GUIButton(imageManager,renderer,SCREEN_WIDTH*0.3,SCREEN_HEIGHT-60,-1,36,_("Cancel"),0,true,true,GUIGravityCenter);
	b1->name="cmdBack";
	b1->eventCallback=this;
	GUIObjectRoot->addChild(b1);
		
    GUIObject* b2=new GUIButton(imageManager,renderer,SCREEN_WIDTH*0.7,SCREEN_HEIGHT-60,-1,36,_("Save Changes"),0,true,true,GUIGravityCenter);
	b2->name="cmdSave";
	b2->eventCallback=this;
	GUIObjectRoot->addChild(b2);
}

static string convertInt(int i){
	stringstream ss;
	ss << i;
	return ss.str();
}

void Options::GUIEventCallback_OnEvent(ImageManager& imageManager, SDL_Renderer& renderer, std::string name,GUIObject* obj,int eventType){
	//Check what type of event it was.
	if(eventType==GUIEventClick){
		if(name=="cmdBack"){
			//Reset the key changes.
			for(int i=0;i<INPUTMGR_MAX;i++){
				inputMgr.setKeyCode((InputManagerKeys)i,tmpKeys[i],false);
				inputMgr.setKeyCode((InputManagerKeys)i,tmpAlternativeKeys[i],true);
			}
			
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
			getSettings()->setValue("internet",internet?"1":"0");
			getSettings()->setValue("theme",themeName);
			getSettings()->setValue("fading",fade?"1":"0");
			getSettings()->setValue("quickrecord",quickrec?"1":"0");
			//Before loading the theme remove the previous one from the stack.
			objThemes.removeTheme();
            loadTheme(imageManager,renderer,themeName);
			if(!useProxy)
				internetProxy.clear();
			getSettings()->setValue("internet-proxy",internetProxy);
			getSettings()->setValue("lang",langs->getName());
			
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
                if(!loadTheme(imageManager,renderer,"")){
					//Loading the theme failed so quit.
					setNextState(STATE_EXIT);
					return;
                }
            }

			if(langs->value!=lastLang){
				//We set the language.
				language=langs->getName();
				dictionaryManager->set_language(tinygettext::Language::from_name(language));
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

		}else if(name=="chkInternet"){
			internet=obj->value?true:false;
		}else if(name=="chkProxy"){
			useProxy=obj->value?true:false;
		}else if(name=="chkFade"){
			fade=obj->value?true:false;
		}else if(name=="chkQuickRec"){
			quickrec=obj->value?true:false;
		}
	}
	if(name=="lstTheme"){
		if(theme!=NULL && theme->value>=0 && theme->value<(int)theme->item.size()){
			//Convert the themeName to contain %DATA%, etc...
			themeName=compressFileName(theme->item[theme->value].first);
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
			getSoundManager()->playSound("jump");
			lastJumpSound=15;
		}
	}
	if(name=="lstTabs"){
		if(obj->value==0){
			tabGeneral->visible=true;
			tabControls->visible=false;
		}else{
			tabGeneral->visible=false;
			tabControls->visible=true;
		}
	}
}

void Options::handleEvents(ImageManager& imageManager, SDL_Renderer& renderer){
	//Get the x and y location of the mouse.
	int x,y;
	SDL_GetMouseState(&x,&y);
	
	//Check icon.
	if(event.type==SDL_MOUSEMOTION || event.type==SDL_MOUSEBUTTONDOWN){
		if(y>=SCREEN_HEIGHT-56&&y<SCREEN_HEIGHT-8&&x>=SCREEN_WIDTH-56)
			clearIconHower=true;
		else
			clearIconHower=false;
	}
	
	if(event.type==SDL_MOUSEBUTTONUP && event.button.button==SDL_BUTTON_LEFT && clearIconHower){
        if(msgBox(imageManager,renderer,_("Do you really want to reset level progress?"),MsgBoxYesNo,_("Warning"))==MsgBoxYes){
			//We delete the progress folder.
#ifdef WIN32
			removeDirectory((getUserPath()+"progress").c_str());
			createDirectory((getUserPath()+"progress").c_str());
#else
			removeDirectory((getUserPath(USER_DATA)+"/progress").c_str());
			createDirectory((getUserPath(USER_DATA)+"/progress").c_str());
#endif
			//Reset statistics.
			statsMgr.reloadCompletedLevelsAndAchievements();
		}
	}
	
	//Check if we need to quit, if so enter the exit state.
	if(event.type==SDL_QUIT){
		setNextState(STATE_EXIT);
	}

	//Check if the escape button is pressed, if so go back to the main menu.
	if(inputMgr.isKeyUpEvent(INPUTMGR_ESCAPE)){
		setNextState(STATE_MENU);
	}
}

void Options::logic(ImageManager&, SDL_Renderer&){
	//Increase the lastJumpSound variable if needed.
	if(lastJumpSound!=0){
		lastJumpSound--;
	}
}

void Options::render(ImageManager&, SDL_Renderer& renderer){
	//Draw background.
    objThemes.getBackground(true)->draw(renderer);
	objThemes.getBackground(true)->updateAnimation();
	
	//Now render the title.
    drawTitleTexture(SCREEN_WIDTH, *title, renderer);

	//Check if an icon is selected/highlighted and draw tooltip
	if(clearIconHower){
        const SDL_Rect texSize = rectFromTexture(*clearTooltip);
        drawGUIBox(-2,SCREEN_HEIGHT-texSize.h-2,texSize.w+4,texSize.h+4,renderer,0xFFFFFFFF);
        applyTexture(0,SCREEN_HEIGHT-texSize.h,clearTooltip,renderer);
	}

	//Draw icon.
    applyTexture(SCREEN_WIDTH-48,SCREEN_HEIGHT-48,*clearIcon,renderer);

	//NOTE: The rendering of the GUI is done in Main.
}

void Options::resize(ImageManager& imageManager, SDL_Renderer& renderer){
	//Recreate the gui to fit the new resolution.
    createGUI(imageManager,renderer);
}

/////////////////////////CREDITS_MENU//////////////////////////////////

Credits::Credits(ImageManager& imageManager,SDL_Renderer& renderer){
	//Render the title.
    title=textureFromText(renderer, *fontTitle,_("Credits"),objThemes.getTextColor(false));

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
	
	//Create the root element of the GUI.
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}
    GUIObjectRoot=new GUIObject(imageManager,renderer,0,0,SCREEN_WIDTH,SCREEN_HEIGHT);
	
	//Create back button.
    backButton=new GUIButton(imageManager,renderer,SCREEN_WIDTH*0.5,SCREEN_HEIGHT-60,-1,36,_("Back"),0,true,true,GUIGravityCenter);
	backButton->name="cmdBack";
	backButton->eventCallback=this;
	GUIObjectRoot->addChild(backButton);
	
	//Create a text area for credits.
    textArea=new GUITextArea(imageManager,renderer,SCREEN_WIDTH*0.05,114,SCREEN_WIDTH*0.9,SCREEN_HEIGHT-200);
	textArea->setFont(fontMono);
    textArea->setStringArray(renderer, std::move(credits));
	textArea->editable=false;
	textArea->extractHyperlinks();
	GUIObjectRoot->addChild(textArea);
}

Credits::~Credits(){
	//Delete the GUI.
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}
}

void Credits::GUIEventCallback_OnEvent(ImageManager& imageManager, SDL_Renderer& renderer, std::string name,GUIObject* obj,int eventType){
	//Check what type of event it was.
	if(eventType==GUIEventClick){
		if(name=="cmdBack"){
			//Goto the main menu.
			setNextState(STATE_MENU);
		}
	}
}

void Credits::handleEvents(ImageManager&, SDL_Renderer&){
	//Check if we need to quit, if so enter the exit state.
	if(event.type==SDL_QUIT){
		setNextState(STATE_EXIT);
	}

	//Check if the escape button is pressed, if so go back to the main menu.
	if(inputMgr.isKeyUpEvent(INPUTMGR_ESCAPE)){
		setNextState(STATE_MENU);
	}
}

void Credits::logic(ImageManager&, SDL_Renderer&){}

void Credits::render(ImageManager&,SDL_Renderer &renderer){
	//Draw background.
    objThemes.getBackground(true)->draw(renderer);
	objThemes.getBackground(true)->updateAnimation();
	
	//Now render the title.
    drawTitleTexture(SCREEN_WIDTH, *title, renderer);

	//NOTE: The rendering of the GUI is done in Main.
}

void Credits::resize(ImageManager&, SDL_Renderer&){
	//Resize and position widgets.
	GUIObjectRoot->width=SCREEN_WIDTH;
	GUIObjectRoot->height=SCREEN_HEIGHT;
	
	backButton->left=SCREEN_WIDTH/2;
	backButton->top=SCREEN_HEIGHT-60;
	
	textArea->left=SCREEN_WIDTH*0.05;
	textArea->width=SCREEN_WIDTH*0.9;
	textArea->height=SCREEN_HEIGHT-200;
	textArea->resize();
}
