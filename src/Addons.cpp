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

#include "Addons.h"
#include "GameState.h"
#include "Functions.h"
#include "FileManager.h"
#include "Globals.h"
#include "Objects.h"
#include "GUIObject.h"
#include "GUIScrollBar.h"
#include "GUIListBox.h"
#include "POASerializer.h"
#include "InputManager.h"
#include <string>
#include <sstream>
#include <iostream>
#include <SDL/SDL.h>
#ifdef __APPLE__
#include <SDL_ttf/SDL_ttf.h>
#else
#include <SDL/SDL_ttf.h>
#endif


using namespace std;

Addons::Addons(){
	//Render the title.
	title=TTF_RenderUTF8_Blended(fontTitle,_("Addons"),themeTextColor);

	//Load placeholder addon icons.
	addonIcon[0]=loadImage(getDataPath()+"/gfx/addon1.png");
	SDL_SetAlpha(addonIcon[0],0,0);
	
	addonIcon[1]=loadImage(getDataPath()+"/gfx/addon2.png");
	SDL_SetAlpha(addonIcon[1],0,0);
	
	addonIcon[2]=loadImage(getDataPath()+"/gfx/addon3.png");
	SDL_SetAlpha(addonIcon[2],0,0);
	
	FILE* addon=fopen((getUserPath(USER_CACHE)+"addons").c_str(),"wb");
	action=NONE;

	addons=NULL;
	selected=NULL;
	
	//Clear the GUI if any.
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}
	
	//Try to get(download) the addonsList.
	if(getAddonsList(addon)==false) {
		//It failed so we show the error message.
		GUIObjectRoot=new GUIObject(0,0,SCREEN_WIDTH,SCREEN_HEIGHT);

		GUIObject* obj=new GUIObject(90,96,200,32,GUIObjectLabel,_("Unable to initialize addon menu:"));
		obj->name="lbl";
		GUIObjectRoot->addChild(obj);
		
		obj=new GUIObject(120,130,200,32,GUIObjectLabel,error.c_str());
		obj->name="lbl";
		GUIObjectRoot->addChild(obj);
		
		obj=new GUIObject(90,550,200,32,GUIObjectButton,_("Back"));
		obj->name="cmdBack";
		obj->eventCallback=this;
		GUIObjectRoot->addChild(obj);
		return;
	}
	
	//Now create the GUI.
	createGUI();
}

Addons::~Addons(){
	delete addons;
	
	//Free the title surface.
	SDL_FreeSurface(title);
	
	//If the GUIObjectRoot exist delete it.
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}
}

void Addons::createGUI(){	
	//Downloaded the addons file now we can create the GUI.
	GUIObjectRoot=new GUIObject(0,0,SCREEN_WIDTH,SCREEN_HEIGHT);
	
	//Create list of categories
	GUISingleLineListBox *listTabs=new GUISingleLineListBox((SCREEN_WIDTH-360)/2,100,360,36);
	listTabs->name="lstTabs";
	listTabs->item.push_back(_("Levels"));
	listTabs->item.push_back(_("Level Packs"));
	listTabs->item.push_back(_("Themes"));
	listTabs->value=0;
	listTabs->eventCallback=this;
	GUIObjectRoot->addChild(listTabs);

	//Create the list for the addons.
	//By default levels will be selected.
	list=new GUIListBox(SCREEN_WIDTH*0.1,160,SCREEN_WIDTH*0.8,SCREEN_HEIGHT-220);
	addonsToList("levels");
	list->name="lstAddons";
	list->eventCallback=this;
	list->value=-1;
	GUIObjectRoot->addChild(list);
	type="levels";
	
	//And the buttons at the bottom of the screen.
	GUIObject* obj=new GUIObject(SCREEN_WIDTH*0.3,SCREEN_HEIGHT-50,-1,32,GUIObjectButton,_("Back"),0,true,true,GUIGravityCenter);
	obj->name="cmdBack";
	obj->eventCallback=this;
	GUIObjectRoot->addChild(obj);
	actionButton=new GUIObject(SCREEN_WIDTH*0.7,SCREEN_HEIGHT-50,-1,32,GUIObjectButton,_("Install"),0,false,true,GUIGravityCenter);
	actionButton->name="cmdInstall";
	actionButton->eventCallback=this;
	GUIObjectRoot->addChild(actionButton);
	updateButton=new GUIObject(SCREEN_WIDTH*0.5,SCREEN_HEIGHT-50,-1,32,GUIObjectButton,_("Update"),0,false,false,GUIGravityCenter);
	updateButton->name="cmdUpdate";
	updateButton->eventCallback=this;
	GUIObjectRoot->addChild(updateButton);
}

bool Addons::getAddonsList(FILE* file){
	//First we download the file.
	if(downloadFile("http://sourceforge.net/p/meandmyshadow/addons/ci/HEAD/tree/addons04?format=raw",file)==false){
		//NOTE: We keep the console output English so we put the string literal here twice.
		cerr<<"ERROR: unable to download addons file!"<<endl;
		error=_("ERROR: unable to download addons file!");
		return false;
	}
	fclose(file);
	
	//Load the downloaded file.
	ifstream addonFile;
	addonFile.open((getUserPath(USER_CACHE)+"addons").c_str());
	
	if(!addonFile.good()) {
		//NOTE: We keep the console output English so we put the string literal here twice.
		cerr<<"ERROR: unable to load addon_list file!"<<endl;
		error=_("ERROR: unable to load addon_list file!");
		return false;
	}
	
	//Parse the addonsfile.
	TreeStorageNode obj;
	{
		POASerializer objSerializer;
		if(!objSerializer.readNode(addonFile,&obj,true)){
			//NOTE: We keep the console output English so we put the string literal here twice.
			cerr<<"ERROR: Invalid file format of addons file!"<<endl;
			error=_("ERROR: Invalid file format of addons file!");
			return false;
		}
	}
	
	//Also load the installed_addons file.
	ifstream iaddonFile;
	iaddonFile.open((getUserPath(USER_CONFIG)+"installed_addons").c_str());
	
	if(!iaddonFile) {
		//The installed_addons file doesn't exist, so we create it.
		ofstream iaddons;
		iaddons.open((getUserPath(USER_CONFIG)+"installed_addons").c_str());
		iaddons<<" "<<endl;
		iaddons.close();
		
		//Also load the installed_addons file.
		iaddonFile.open((getUserPath(USER_CONFIG)+"installed_addons").c_str());
		if(!iaddonFile) {
			//NOTE: We keep the console output English so we put the string literal here twice.
			cerr<<"ERROR: Unable to create the installed_addons file."<<endl;
			error=_("ERROR: Unable to create the installed_addons file.");
			return false;
		}
	}
	
	//And parse the installed_addons file.
	TreeStorageNode obj1;
	{
		POASerializer objSerializer;
		if(!objSerializer.readNode(iaddonFile,&obj1,true)){
			//NOTE: We keep the console output English so we put the string literal here twice.
			cerr<<"ERROR: Invalid file format of the installed_addons!"<<endl;
			error=_("ERROR: Invalid file format of the installed_addons!");
			return false;
		}
	}
	
	
	//Fill the vector.
	addons = new std::vector<Addon>;
	fillAddonList(*addons,obj,obj1);
		
	//Close the files.
	iaddonFile.close();
	addonFile.close();
	return true;
}

void Addons::fillAddonList(std::vector<Addons::Addon> &list, TreeStorageNode &addons, TreeStorageNode &installed_addons){
	//Loop through the blocks of the addons file.
	//These should contain the types levels, levelpacks, themes.
	for(unsigned int i=0;i<addons.subNodes.size();i++){
		TreeStorageNode* block=addons.subNodes[i];
		if(block==NULL) continue;
		
		string type;
		type=block->name;
		//Now loop the entries(subNodes) of the block.
		for(unsigned int i=0;i<block->subNodes.size();i++){
			TreeStorageNode* entry=block->subNodes[i];
			if(entry==NULL) continue;
			if(entry->name=="entry" && entry->value.size()==1){
				//The entry is valid so create a new Addon.
				Addon addon = *(new Addon);
				addon.type=type;
				addon.name=entry->value[0];
				addon.file=entry->attributes["file"][0];
				if(!entry->attributes["folder"].empty()){
					addon.folder=entry->attributes["folder"][0];
				}
				addon.author=entry->attributes["author"][0];
				addon.version=atoi(entry->attributes["version"][0].c_str());
				addon.upToDate=false;
				addon.installed=false;
				
				//Check if the addon is already installed.
				for(unsigned int i=0;i<installed_addons.subNodes.size();i++){
					TreeStorageNode* installed=installed_addons.subNodes[i];
					if(installed==NULL) continue;
					if(installed->name=="entry" && installed->value.size()==3){
						if(addon.type.compare(installed->value[0])==0 && addon.name.compare(installed->value[1])==0) {
							addon.installed=true;
							addon.installedVersion=atoi(installed->value[2].c_str());
							if(addon.installedVersion>=addon.version) {
								addon.upToDate=true;
							}

						}
					}
				}
				
				//Finally put him in the list.
				list.push_back(addon);
			}
		}
	}
}

void Addons::addonsToList(const std::string &type){
	list->clearItems();
	for(unsigned int i=0;i<addons->size();i++) {
		//Check if the addon is from the right type.
		if((*addons)[i].type==type) {
			string entry = (*addons)[i].name + " by " + (*addons)[i].author;
			if((*addons)[i].installed) {
				if((*addons)[i].upToDate) {
					entry += " *";
				} else {
					entry += " +";
				}
			}
			
			SDL_Surface* surf=SDL_CreateRGBSurface(SDL_SWSURFACE,list->width,74,32,RMASK,GMASK,BMASK,AMASK);

			if(type=="levels")
				applySurface(5,5,addonIcon[0],surf,NULL);
			else if(type=="levelpacks")
				applySurface(5,5,addonIcon[1],surf,NULL);
			else
				applySurface(5,5,addonIcon[2],surf,NULL);
			
			SDL_Color black={0,0,0,0};
			SDL_Surface* nameSurf=TTF_RenderUTF8_Blended(fontGUI,(*addons)[i].name.c_str(),black);
			SDL_SetAlpha(nameSurf,0,0xFF);
			applySurface(74,-1,nameSurf,surf,NULL);
			
			string authorLine = "by " + (*addons)[i].author;
			SDL_Surface* authorSurf=TTF_RenderUTF8_Blended(fontText,authorLine.c_str(),black);
			SDL_SetAlpha(authorSurf,0,0xFF);
			applySurface(74,43,authorSurf,surf,NULL);
			
			if((*addons)[i].installed){
				if((*addons)[i].upToDate){
					SDL_Surface* infoSurf=TTF_RenderUTF8_Blended(fontText,_("Installed"),black);
					SDL_SetAlpha(infoSurf,0,0xFF);
					applySurface(surf->w-infoSurf->w-16,(surf->h-infoSurf->h)/2,infoSurf,surf,NULL);
				}else{
					SDL_Surface* infoSurf=TTF_RenderUTF8_Blended(fontText,_("Update"),black);
					SDL_SetAlpha(infoSurf,0,0xFF);
					applySurface(surf->w-infoSurf->w-16,(surf->h-infoSurf->h)/2,infoSurf,surf,NULL);
				}
			}else{
				SDL_Color grey={127,127,127};
				SDL_Surface* infoSurf=TTF_RenderUTF8_Blended(fontText,_("Not installed"),grey);
				SDL_SetAlpha(infoSurf,0,0xFF);
				applySurface(surf->w-infoSurf->w-16,(surf->h-infoSurf->h)/2,infoSurf,surf,NULL);
			}
			
			list->addItem(entry,surf);
		}
	}
}

bool Addons::saveInstalledAddons(){
	if(!addons) return false;

	//Open the file.
	ofstream iaddons;
	iaddons.open((getUserPath(USER_CONFIG)+"installed_addons").c_str());
	if(!iaddons) return false;
	
	//Loop all the levels.
	TreeStorageNode installed;
	for(unsigned int i=0;i<addons->size();i++) {
		//Check if the level is installed or not.
		if((*addons)[i].installed) {
			TreeStorageNode *entry=new TreeStorageNode;
			entry->name="entry";
			entry->value.push_back((*addons)[i].type);
			entry->value.push_back((*addons)[i].name);
			char version[64];
			sprintf(version,"%d",(*addons)[i].installedVersion);
			entry->value.push_back(version);
			
			installed.subNodes.push_back(entry);
		}
	}
	
	
	//And write away the file.
	POASerializer objSerializer;
	objSerializer.writeNode(&installed,iaddons,true,true);
	
	return true;
}

void Addons::handleEvents(){
	//Check if we should quit.
	if(event.type==SDL_QUIT){
		//Save the installed addons before exiting.
		saveInstalledAddons();
		setNextState(STATE_EXIT);
	}

	//Check if escape is pressed, if so return to the main menu.
	if(inputMgr.isKeyUpEvent(INPUTMGR_ESCAPE)){
		setNextState(STATE_MENU);
	}
}

void Addons::logic(){}

void Addons::render(){
	//Draw background.
	objThemes.getBackground(true)->draw(screen);
	
	//Draw the title.
	applySurface((SCREEN_WIDTH-title->w)/2,40-TITLE_FONT_RAISE,title,screen,NULL);
}

void Addons::resize(){
	//Delete the gui (if any).
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}
	
	//Now create a new one.
	createGUI();
}

void Addons::GUIEventCallback_OnEvent(std::string name,GUIObject* obj,int eventType){
	if(name=="lstTabs"){
		if(obj->value==0){
			addonsToList("levels");
			type="levels";
		}else if(obj->value==1){
			addonsToList("levelpacks");
			type="levelpacks";
		}else{
			addonsToList("themes");
			type="themes";
		}
		list->value=0;
		GUIEventCallback_OnEvent("lstAddons",list,GUIEventChange);
	}else if(name=="lstAddons"){
		//Get the addon struct that belongs to it.
		Addon *addon=NULL;
		if(!list->item.empty()) {
			string entry = list->getItem(list->value);
			for(unsigned int i=0;i<addons->size();i++) {
				std::string prefix=(*addons)[i].name;
				if(!entry.compare(0, prefix.size(), prefix)) {
					addon=&(*addons)[i];
				}
			}
		}
		
		selected=addon;
		updateActionButton();
		updateUpdateButton();
	}else if(name=="cmdBack"){
		saveInstalledAddons();
		setNextState(STATE_MENU);
	}else if(name=="cmdUpdate"){

		//First remove the addon and then install it again.
		if(type.compare("levels")==0) {	
			if(downloadFile(selected->file,(getUserPath(USER_DATA)+"/levels/"))!=false){
				selected->upToDate=true;
				selected->installedVersion=selected->version;
				addonsToList("levels");
				updateActionButton();
				updateUpdateButton();
			}else{
				cerr<<"ERROR: Unable to download addon!"<<endl;
				msgBox(_("ERROR: Unable to download addon!"),MsgBoxOKOnly,_("ERROR:"));
				return;
			}
		}else if(type.compare("levelpacks")==0) {
			if(!removeDirectory((getUserPath(USER_DATA)+"levelpacks/"+selected->folder+"/").c_str())){
				cerr<<"ERROR: Unable to remove the directory "<<(getUserPath(USER_DATA)+"levelpacks/"+selected->folder+"/")<<"."<<endl;
				return;
			}	
			if(downloadFile(selected->file,(getUserPath(USER_CACHE)+"/tmp/"))!=false){
				extractFile(getUserPath(USER_CACHE)+"/tmp/"+fileNameFromPath(selected->file,true),getUserPath(USER_DATA)+"/levelpacks/"+selected->folder+"/");
				selected->upToDate=true;
				selected->installedVersion=selected->version;
				addonsToList("levelpacks");
				updateActionButton();
				updateUpdateButton();
			}else{
				cerr<<"ERROR: Unable to download addon!"<<endl;
				msgBox(_("ERROR: Unable to download addon!"),MsgBoxOKOnly,_("ERROR:"));
				return;
			}
		}else if(type.compare("themes")==0) {
			if(!removeDirectory((getUserPath(USER_DATA)+"themes/"+selected->folder+"/").c_str())){
				cerr<<"ERROR: Unable to remove the directory "<<(getUserPath(USER_DATA)+"themes/"+selected->folder+"/")<<"."<<endl;
				return;
			}		
			if(downloadFile(selected->file,(getUserPath(USER_CACHE)+"/tmp/"))!=false){
				extractFile((getUserPath(USER_CACHE)+"/tmp/"+fileNameFromPath(selected->file,true)),(getUserPath(USER_DATA)+"/themes/"+selected->folder+"/"));
				selected->upToDate=true;
				selected->installedVersion=selected->version;
				addonsToList("themes");
				updateActionButton();
				updateUpdateButton();
			}else{
				cerr<<"ERROR: Unable to download addon!"<<endl;
				msgBox(_("ERROR: Unable to download addon!"),MsgBoxOKOnly,_("ERROR:"));
				return;
			}
		}
		    
	
	}else if(name=="cmdInstall"){
		switch(action) {
		  case NONE:
		    break;
		  case INSTALL:
			//Download the addon.
			if(type.compare("levels")==0) {
				if(downloadFile(selected->file,getUserPath(USER_DATA)+"/levels/")!=false){
					selected->upToDate=true;
					selected->installed=true;
					selected->installedVersion=selected->version;
					addonsToList("levels");
					updateActionButton();
					;
					
					//And add the level to the levels levelpack.
					LevelPack* levelsPack=getLevelPackManager()->getLevelPack("Levels");
					levelsPack->addLevel(getUserPath(USER_DATA)+"/levels/"+fileNameFromPath(selected->file));
					levelsPack->setLocked(levelsPack->getLevelCount()-1);
				}else{
					cerr<<"ERROR: Unable to download addon!"<<endl;
					msgBox(_("ERROR: Unable to download addon!"),MsgBoxOKOnly,_("ERROR:"));
					return;
				}
			}else if(type.compare("levelpacks")==0) {
				if(downloadFile(selected->file,getUserPath(USER_CACHE)+"/tmp/")!=false){
					extractFile(getUserPath(USER_CACHE)+"/tmp/"+fileNameFromPath(selected->file,true),getUserPath(USER_DATA)+"/levelpacks/"+selected->folder+"/");
					selected->upToDate=true;
					selected->installed=true;
					selected->installedVersion=selected->version;
					addonsToList("levelpacks");
					updateActionButton();
					updateUpdateButton();
					
					//And add the levelpack to the levelpackManager.
					getLevelPackManager()->loadLevelPack(getUserPath(USER_DATA)+"/levelpacks/"+selected->folder);
				}else{
					cerr<<"ERROR: Unable to download addon!"<<endl;
					msgBox(_("ERROR: Unable to download addon!"),MsgBoxOKOnly,_("ERROR:"));
					return;
				}
			}else if(type.compare("themes")==0) {
				if(downloadFile(selected->file,getUserPath(USER_CACHE)+"/tmp/")!=false){
					extractFile(getUserPath(USER_CACHE)+"/tmp/"+fileNameFromPath(selected->file,true),getUserPath(USER_DATA)+"/themes/"+selected->folder+"/");
					selected->upToDate=true;
					selected->installed=true;
					selected->installedVersion=selected->version;
					addonsToList("themes");
					updateActionButton();
					updateUpdateButton();
				}else{
					cerr<<"ERROR: Unable to download addon!"<<endl;
					msgBox(_("ERROR: Unable to download addon!"),MsgBoxOKOnly,_("ERROR:"));
					return;
				}
			}
		    break;
		  case UNINSTALL:
			//Uninstall the addon.
			if(type.compare("levels")==0) {
				if(remove((getUserPath(USER_DATA)+"levels/"+fileNameFromPath(selected->file)).c_str())){
					cerr<<"ERROR: Unable to remove the file "<<(getUserPath(USER_DATA) + "levels/" + fileNameFromPath(selected->file))<<"."<<endl;
					return;
				}
				
				selected->upToDate=false;
				selected->installed=false;
				addonsToList("levels");
				updateActionButton();
				updateUpdateButton();
				
				//And remove the level from the levels levelpack.
				LevelPack* levelsPack=getLevelPackManager()->getLevelPack("Levels");
				for(int i=0;i<levelsPack->getLevelCount();i++){
					if(levelsPack->getLevelFile(i)==(getUserPath(USER_DATA)+"levels/"+fileNameFromPath(selected->file))){
						//Remove the level and break out of the loop.
						levelsPack->removeLevel(i);
						break;
					}
				}
			}else if(type.compare("levelpacks")==0) {
				if(!removeDirectory((getUserPath(USER_DATA)+"levelpacks/"+selected->folder+"/").c_str())){
					cerr<<"ERROR: Unable to remove the directory "<<(getUserPath(USER_DATA)+"levelpacks/"+selected->folder+"/")<<"."<<endl;
					return;
				}
				  
				selected->upToDate=false;
				selected->installed=false;
				addonsToList("levelpacks");
				updateActionButton();
				updateUpdateButton();
				
				//And remove the levelpack from the levelpack manager.
				getLevelPackManager()->removeLevelPack(selected->folder);
			}else if(type.compare("themes")==0) {
				if(!removeDirectory((getUserPath(USER_DATA)+"themes/"+selected->folder+"/").c_str())){
					cerr<<"ERROR: Unable to remove the directory "<<(getUserPath(USER_DATA)+"themes/"+selected->folder+"/")<<"."<<endl;
					return;
				}
				  
				selected->upToDate=false;
				selected->installed=false;
				addonsToList("themes");
				updateActionButton();
				updateUpdateButton();
			}
		    break;
		}
	}
}

void Addons::updateUpdateButton(){
	//some sanity check
	if(selected==NULL){
		updateButton->enabled=false;
		return;
	}

	//Check if the selected addon is installed.
	if(selected->installed){
		//It is installed, but is it uptodate?
		if(selected->upToDate){
			//The addon is installed and there is no need to show the button.
			updateButton->enabled=false;
			updateButton->visible=false;
		}else{
			//Otherwise show the button
			updateButton->enabled=true;
			updateButton->visible=true;
		}
	}else{
		//The addon isn't installed so we can only install it.
		updateButton->enabled=false;
	}
}



void Addons::updateActionButton(){
	//some sanity check
	if(selected==NULL){
		actionButton->enabled=false;
		action = NONE;
		return;
	}

	//Check if the selected addon is installed.
	if(selected->installed){
		//It is installed, but is it uptodate?
		actionButton->enabled=true;
		actionButton->caption=_("Uninstall");
		action = UNINSTALL;
	}else{
		//The addon isn't installed so we can only install it.
		actionButton->enabled=true;
		actionButton->caption=_("Install");
		action = INSTALL;
	}
}
