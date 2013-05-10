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
#include "GUIOverlay.h"
#include "GUIScrollBar.h"
#include "GUITextArea.h"
#include "GUIListBox.h"
#include "POASerializer.h"
#include "InputManager.h"
#include <string>
#include <sstream>
#include <iostream>
#include "libs/tinyformat/tinyformat.h"
#include <SDL/SDL.h>
#ifdef __APPLE__
#include <SDL_ttf/SDL_ttf.h>
#else
#include <SDL/SDL_ttf.h>
#endif


using namespace std;

Addons::Addons():selected(NULL){
	//Render the title.
	title=TTF_RenderUTF8_Blended(fontTitle,_("Addons"),themeTextColor);

	//Load placeholder addon icons and screenshot.
	addonIcon[0]=loadImage(getDataPath()+"/gfx/addon1.png");
	SDL_SetAlpha(addonIcon[0],0,0);
	addonIcon[1]=loadImage(getDataPath()+"/gfx/addon2.png");
	SDL_SetAlpha(addonIcon[1],0,0);
	addonIcon[2]=loadImage(getDataPath()+"/gfx/addon3.png");
	SDL_SetAlpha(addonIcon[2],0,0);
	screenshot=loadImage(getDataPath()+"/gfx/screenshot.png");

	//Open the addons file in the user cache path for writing (downloading) to.
	FILE* addon=fopen((getUserPath(USER_CACHE)+"addons").c_str(),"wb");
	
	//Clear the GUI if any.
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}
	
	//Try to get(download) the addonsList.
	if(getAddonsList(addon)==false){
		//It failed so we show the error message.
		GUIObjectRoot=new GUIObject(0,0,SCREEN_WIDTH,SCREEN_HEIGHT);

		GUIObject* obj=new GUILabel(90,96,200,32,_("Unable to initialize addon menu:"));
		obj->name="lbl";
		GUIObjectRoot->addChild(obj);
		
		obj=new GUILabel(120,130,200,32,error.c_str());
		obj->name="lbl";
		GUIObjectRoot->addChild(obj);
		
		obj=new GUIButton(90,550,200,32,_("Back"));
		obj->name="cmdBack";
		obj->eventCallback=this;
		GUIObjectRoot->addChild(obj);
		return;
	}
	
	//Now create the GUI.
	createGUI();
}

Addons::~Addons(){
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
	categoryList=new GUISingleLineListBox((SCREEN_WIDTH-360)/2,100,360,36);
	categoryList->name="lstTabs";
	//Loop through the categories and add them to the list.
	
	//FIXME: Hack for easy detecting which categories there are.
	{
		map<string,bool> categories;
		map<string,bool>::iterator mapIt;
		vector<Addon>::iterator it;
		for(it=addons.begin();it!=addons.end();++it)
			categories[it->type]=true;
		for(mapIt=categories.begin();mapIt!=categories.end();++mapIt)
			categoryList->addItem(mapIt->first,_(mapIt->first));
	}
	categoryList->value=0;
	categoryList->eventCallback=this;
	GUIObjectRoot->addChild(categoryList);

	//Create the list for the addons.
	//By default levels will be selected.
	list=new GUIListBox(SCREEN_WIDTH*0.1,160,SCREEN_WIDTH*0.8,SCREEN_HEIGHT-210);
	addonsToList(categoryList->getName());
	list->name="lstAddons";
	list->clickEvents=true;
	list->eventCallback=this;
	list->value=-1;
	GUIObjectRoot->addChild(list);
	type="levels";
	
	//The back button.
	GUIObject* obj=new GUIButton(20,20,-1,32,_("Back"));
	obj->name="cmdBack";
	obj->eventCallback=this;
	GUIObjectRoot->addChild(obj);
}

bool Addons::getAddonsList(FILE* file){
	//First we download the file.
	if(downloadFile(getSettings()->getValue("addon_url"),file)==false){
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
		/// TRANSLATORS: addon_list is the name of a file and should not be translated.
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

	//Check the addon version in the addons list.
	int version=0;
	if(!obj.attributes["version"].empty())
		version=atoi(obj.attributes["version"][0].c_str());
	if(version<MIN_VERSION || version>MAX_VERSION){
		//NOTE: We keep the console output English so we put the string literal here twice.
		cerr<<"ERROR: Addon list version is unsupported! (received: "<<version<<" supported:"<<MIN_VERSION<<"-"<<MAX_VERSION<<")"<<endl;
		error=_("ERROR: Addon list version is unsupported!");
		return false;
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
			/// TRANSLATORS: installed_addons is the name of a file and should not be translated.
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
	fillAddonList(obj,obj1);
	
	//Close the files.
	iaddonFile.close();
	addonFile.close();
	return true;
}

void Addons::fillAddonList(TreeStorageNode &objAddons, TreeStorageNode &objInstalledAddons){
	//Loop through the blocks of the addons file.
	//These should contain the types levels, levelpacks, themes.
	for(unsigned int i=0;i<objAddons.subNodes.size();i++){
		TreeStorageNode* block=objAddons.subNodes[i];
		if(block==NULL) continue;

		//Check what kind of block it is, only category at the moment.
		if(block->name=="category" && block->value.size()>0){
			string type=block->value[0];
			
			//Now loop the entries(subNodes) of the block.
			for(unsigned int i=0;i<block->subNodes.size();i++){
				TreeStorageNode* entry=block->subNodes[i];
				if(entry==NULL) continue;
				if(entry->name=="entry" && entry->value.size()==1){
					//The entry is valid so create a new Addon.
					Addon addon;
					addon.icon=addon.screenshot=NULL;
					addon.type=type;
					addon.name=entry->value[0];

					if(!entry->attributes["file"].empty())
						addon.file=entry->attributes["file"][0];
					if(!entry->attributes["author"].empty())
						addon.author=entry->attributes["author"][0];
					if(!entry->attributes["description"].empty())
						addon.description=entry->attributes["description"][0];
					if(entry->attributes["icon"].size()>1){
						//There are (at least) two values, the url to the icon and its md5sum used for caching.
						addon.icon=loadCachedImage(entry->attributes["icon"][0].c_str(),entry->attributes["icon"][1].c_str());
						if(addon.icon)
							SDL_SetAlpha(addon.icon,0,0);
					}
					if(entry->attributes["screenshot"].size()>1){
						//There are (at least) two values, the url to the screenshot and its md5sum used for caching.
						addon.screenshot=loadCachedImage(entry->attributes["screenshot"][0].c_str(),entry->attributes["screenshot"][1].c_str());
						if(addon.screenshot)
							SDL_SetAlpha(addon.screenshot,0,0);
					}
					if(!entry->attributes["version"].empty())
						addon.version=atoi(entry->attributes["version"][0].c_str());
					addon.upToDate=false;
					addon.installed=false;

					//Check if the addon is already installed.
					for(unsigned int i=0;i<objInstalledAddons.subNodes.size();i++){
						TreeStorageNode* installed=objInstalledAddons.subNodes[i];
						if(installed==NULL) continue;
						if(installed->name=="entry" && installed->value.size()==3){
							if(addon.type.compare(installed->value[0])==0 && addon.name.compare(installed->value[1])==0) {
								addon.installed=true;
								addon.installedVersion=atoi(installed->value[2].c_str());
								if(addon.installedVersion>=addon.version) {
									addon.upToDate=true;
								}

								//Also read the content vector.
								for(unsigned int j=0;j<installed->subNodes.size();j++){
									if(installed->subNodes[j]->value.size()==1)
										addon.content.push_back(pair<string,string>(installed->subNodes[j]->name,installed->subNodes[j]->value[0]));
								}
							}
						}
					}

					//Finally put him in the list.
					addons.push_back(addon);
				}
			}
		}
	}
}

void Addons::addonsToList(const std::string &type){
	//Clear the list.
	list->clearItems();
	//Loop through the addons.
	for(unsigned int i=0;i<addons.size();i++) {
		//Make sure the addon is of the requested type.
		if(addons[i].type!=type)
			continue;
		
		Addon addon=addons[i];
		
		string entry=addon.name+" by "+addon.author;
		if(addon.installed){
			if(addon.upToDate){
				entry+=" *";
			}else{
				entry+=" +";
			}
		}
		
		SDL_Surface* surf=SDL_CreateRGBSurface(SDL_SWSURFACE,list->width,74,32,RMASK,GMASK,BMASK,AMASK);

		//Check if there's an icon for the addon.
		if(addon.icon){
			applySurface(5,5,addon.icon,surf,NULL);
		}else{
			if(type=="levels")
				applySurface(5,5,addonIcon[0],surf,NULL);
			else if(type=="levelpacks")
				applySurface(5,5,addonIcon[1],surf,NULL);
			else
				applySurface(5,5,addonIcon[2],surf,NULL);
		}
			
		SDL_Color black={0,0,0,0};
		SDL_Surface* nameSurf=TTF_RenderUTF8_Blended(fontGUI,addon.name.c_str(),black);
		SDL_SetAlpha(nameSurf,0,0xFF);
		applySurface(74,-1,nameSurf,surf,NULL);
		SDL_FreeSurface(nameSurf);
		
		/// TRANSLATORS: indicates the author of an addon.
		string authorLine = tfm::format(_("by %s"),addon.author);
		SDL_Surface* authorSurf=TTF_RenderUTF8_Blended(fontText,authorLine.c_str(),black);
		SDL_SetAlpha(authorSurf,0,0xFF);
		applySurface(74,43,authorSurf,surf,NULL);
		SDL_FreeSurface(authorSurf);
		
		if(addon.installed){
			if(addon.upToDate){
				SDL_Surface* infoSurf=TTF_RenderUTF8_Blended(fontText,_("Installed"),black);
				SDL_SetAlpha(infoSurf,0,0xFF);
				applySurface(surf->w-infoSurf->w-32,(surf->h-infoSurf->h)/2,infoSurf,surf,NULL);
				SDL_FreeSurface(infoSurf);
			}else{
				SDL_Surface* infoSurf=TTF_RenderUTF8_Blended(fontText,_("Updatable"),black);
				SDL_SetAlpha(infoSurf,0,0xFF);
				applySurface(surf->w-infoSurf->w-32,(surf->h-infoSurf->h)/2,infoSurf,surf,NULL);
				SDL_FreeSurface(infoSurf);
			}
		}else{
			SDL_Color grey={127,127,127};
			SDL_Surface* infoSurf=TTF_RenderUTF8_Blended(fontText,_("Not installed"),grey);
			SDL_SetAlpha(infoSurf,0,0xFF);
			applySurface(surf->w-infoSurf->w-32,(surf->h-infoSurf->h)/2,infoSurf,surf,NULL);
			SDL_FreeSurface(infoSurf);
		}
		
		list->addItem(entry,surf);
	}
}

bool Addons::saveInstalledAddons(){
	//Open the file.
	ofstream iaddons;
	iaddons.open((getUserPath(USER_CONFIG)+"installed_addons").c_str());
	if(!iaddons) return false;

	TreeStorageNode installed;
	
	//Loop through all the addons.
	vector<Addon>::iterator it;
	for(it=addons.begin();it!=addons.end();++it){
		//Check if the level is installed or not.
		if(it->installed) {
			TreeStorageNode *entry=new TreeStorageNode;
			entry->name="entry";
			entry->value.push_back(it->type);
			entry->value.push_back(it->name);
			char version[64];
			sprintf(version,"%d",it->installedVersion);
			entry->value.push_back(version);

			//Now add a subNode for each content.
			for(int i=0;i<it->content.size();i++){
				TreeStorageNode* content=new TreeStorageNode;
				content->name=it->content[i].first;
				content->value.push_back(it->content[i].second);

				//Add the content node to the entry node.
				entry->subNodes.push_back(content);
			}
		
			installed.subNodes.push_back(entry);
		}
	}
	
	//And write away the file.
	POASerializer objSerializer;
	objSerializer.writeNode(&installed,iaddons,true,true);
	
	return true;
}

SDL_Surface* Addons::loadCachedImage(const char* url,const char* md5sum){
	//Check if the image is cached.
	string imageFile=getUserPath(USER_CACHE)+"images/"+md5sum;
	if(fileExists(imageFile.c_str())){
		//It is, so load the image.
		return loadImage(imageFile);
	}else{
		//Download the image.
		FILE* file=fopen(imageFile.c_str(),"wb");

		//Downloading failed.
		if(!downloadFile(url,file)){
			cerr<<"ERROR: Unable to download image from "<<url<<endl;
			fclose(file);
			return NULL;
		}
		fclose(file);

		//Load the image.
		return loadImage(imageFile);
	}
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

void Addons::showAddon(){
	//Make sure an addon is selected.
	if(!selected)
		return;

	//Create a root object.
	GUIObject* root=new GUIFrame((SCREEN_WIDTH-600)/2,(SCREEN_HEIGHT-400)/2,600,400,selected->name.c_str());

	//Create the 'by creator' label.
	GUIObject* obj=new GUILabel(0,50,600,50,tfm::format(_("by %s"),selected->author).c_str(),0,true,true,GUIGravityCenter);
	root->addChild(obj);

	//Create the description text.
	GUITextArea* description=new GUITextArea(10,100,370,200);
	description->setString(selected->description.c_str());
	description->editable=false;
	description->resize();
	root->addChild(description);

	//Create the screenshot image.
	GUIImage* img=new GUIImage(390,100,200,150);
	img->setImage(selected->screenshot?selected->screenshot:screenshot);
	root->addChild(img);

	//Add buttons depending on the installed/update status.
	if(selected->installed && !selected->upToDate){
		GUIObject* bRemove=new GUIButton(root->width*0.97,350,-1,32,_("Remove"),0,true,true,GUIGravityRight);
		bRemove->name="cmdRemove";
		bRemove->eventCallback=this;
		root->addChild(bRemove);
		//Create a back button.
		GUIObject* bBack=new GUIButton(root->width*0.03,350,-1,32,_("Back"),0,true,true,GUIGravityLeft);
		bBack->name="cmdCloseOverlay";
		bBack->eventCallback=this;
		root->addChild(bBack);
		
		//Update widget sizes.
		root->render(0,0,false);
		
		//Create a nicely centered button.
		obj=new GUIButton((int)floor((bBack->left+bBack->width+bRemove->left-bRemove->width)*0.5),350,-1,32,_("Update"),0,true,true,GUIGravityCenter);
		obj->name="cmdUpdate";
		obj->eventCallback=this;
		root->addChild(obj);
	}else{
		if(!selected->installed){
			obj=new GUIButton(root->width*0.9,350,-1,32,_("Install"),0,true,true,GUIGravityRight);
			obj->name="cmdInstall";
			obj->eventCallback=this;
			root->addChild(obj);
		}else if(selected->upToDate){
			obj=new GUIButton(root->width*0.9,350,-1,32,_("Remove"),0,true,true,GUIGravityRight);
			obj->name="cmdRemove";
			obj->eventCallback=this;
			root->addChild(obj);
		}
		//Create a back button.
		obj=new GUIButton(root->width*0.1,350,-1,32,_("Back"),0,true,true,GUIGravityLeft);
		obj->name="cmdCloseOverlay";
		obj->eventCallback=this;
		root->addChild(obj);
	}
	
	new GUIOverlay(root);
}

void Addons::GUIEventCallback_OnEvent(std::string name,GUIObject* obj,int eventType){
	if(name=="lstTabs"){
		//Get the category type.
		type=categoryList->getName();
		//Get the list corresponding with the category and select the first entry.
		addonsToList(type);
		list->value=0;
		//Call an event as if an entry in the addons listbox was clicked.
		GUIEventCallback_OnEvent("lstAddons",list,GUIEventChange);
	}else if(name=="lstAddons"){
		//Check which type of event.
		if(eventType==GUIEventChange){
			//Get the addon struct that belongs to it.
			Addon* addon=NULL;

			//Make sure the addon list on screen isn't empty.
			if(!list->item.empty()){
				//Get the name of the (newly) selected entry.
				string entry=list->getItem(list->value);

				//Loop through the addons of the selected category.
				for(unsigned int i=0;i<addons.size();i++){
					//Make sure the addons are of the same type.
					if(addons[i].type!=categoryList->getName())
						continue;
					
					string prefix=addons[i].name;
					if(!entry.compare(0,prefix.size(),prefix)){
						addon=&addons[i];
					}
				}
			}

			//Set the new addon as selected and unselect the list.
			selected=addon;
			list->value=-1;
		}else if(eventType==GUIEventClick){
			//Make sure an addon is selected.
			if(selected){
				showAddon();
			}
		}
	}else if(name=="cmdBack"){
		saveInstalledAddons();
		setNextState(STATE_MENU);
	}else if(name=="cmdCloseOverlay"){
		//We can safely delete the GUIObjectRoot, since it's handled by the GUIOverlay.
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}else if(name=="cmdUpdate"){
		//NOTE: This simply removes the addon and reinstalls it.
		//The complete addon is downloaded either way so no need for checking what has been changed/added/removed/etc...
		if(selected){
			removeAddon(selected);
			installAddon(selected);
		}
		addonsToList(categoryList->getName());
	}else if(name=="cmdInstall"){
		if(selected)
			installAddon(selected);
		addonsToList(categoryList->getName());
	}else if(name=="cmdRemove"){
		//TODO: Check for dependencies.
		if(selected)
			removeAddon(selected);
		addonsToList(categoryList->getName());
	}

	//NOTE: In case of install/remove/update we can delete the GUIObjectRoot, since it's managed by the GUIOverlay.
	if(name=="cmdUpdate" || name=="cmdInstall" || name=="cmdRemove"){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}
}

void Addons::removeAddon(Addon* addon){
	//To remove an addon we loop over the content vector in the structure.
	//NOTE: This should contain all INSTALLED content, if something failed during installation it isn't added.
	for(int i=0;i<addon->content.size();i++){
		//Check the type of content.
		if(addon->content[i].first=="file"){
			string file=getUserPath(USER_DATA)+addon->content[i].second;
			//Check if the file exists.
			if(!fileExists(file.c_str())){
				cerr<<"WARNING: File '"<<file<<"' appears to have been removed already."<<endl;
				msgBox("WARNING: File '"+file+"' appears to have been removed already.",MsgBoxOKOnly,"Addon error");
				continue;
			}
			
			//Remove the file.
			if(!removeFile(file.c_str())){
				cerr<<"ERROR: Unable to remove file '"<<file<<"'!"<<endl;
				msgBox("ERROR: Unable to remove file '"+file+"'!",MsgBoxOKOnly,"Addon error");
				continue;
			}
		}else if(addon->content[i].first=="folder"){
			string dir=getUserPath(USER_DATA)+addon->content[i].second;
			//Check if the directory exists.
			if(!dirExists(dir.c_str())){
				cerr<<"WARNING: Directory '"<<dir<<"' appears to have been removed already."<<endl;
				msgBox("WARNING: Directory '"+dir+"' appears to have been removed already.",MsgBoxOKOnly,"Addon error");
				continue;
			}
			
			//Remove the directory.
			if(!removeDirectory(dir.c_str())){
				cerr<<"ERROR: Unable to remove directory '"<<dir<<"'!"<<endl;
				msgBox("ERROR: Unable to remove directory '"+dir+"'!",MsgBoxOKOnly,"Addon error");
				continue;
			}
		}else if(addon->content[i].first=="level"){
			string file=getUserPath(USER_DATA)+"levels/"+addon->content[i].second;

			//Check if the level file exists.
			if(!fileExists(file.c_str())){
				cerr<<"WARNING: Level '"<<file<<"' appears to have been removed already."<<endl;
				msgBox("WARNING: Level '"+file+"' appears to have been removed already.",MsgBoxOKOnly,"Addon error");
				continue;
			}
			//Remove the level file.
			if(!removeFile(file.c_str())){
				cerr<<"ERROR: Unable to remove level '"<<file<<"'!"<<endl;
				msgBox("ERROR: Unable to remove level '"+file+"'!",MsgBoxOKOnly,"Addon error");
				continue;
			}
			//Also remove the level from the Levels levelpack.
			LevelPack* levelsPack=getLevelPackManager()->getLevelPack("Levels/");

			for(int i=0;i<levelsPack->getLevelCount();i++){
				if(levelsPack->getLevelFile(i)==file){
					//Remove the level and break out of the loop.
					levelsPack->removeLevel(i);
					break;
				}
			}
		}else if(addon->content[i].first=="levelpack"){
			//FIXME: We assume no trailing slash since there mustn't be one for installing, bad :(
			string dir=getUserPath(USER_DATA)+"levelpacks/"+addon->content[i].second+"/";
			//Check if the directory exists.
			if(!dirExists(dir.c_str())){
				cerr<<"WARNING: Levelpack directory '"<<dir<<"' appears to have been removed already."<<endl;
				msgBox("WARNING: Levelpack directory '"+dir+"' appears to have been removed already.",MsgBoxOKOnly,"Addon error");
				continue;
			}

			//Remove the directory.
			if(!removeDirectory(dir.c_str())){
				cerr<<"ERROR: Unable to remove levelpack directory '"<<dir<<"'!"<<endl;
				msgBox("ERROR: Unable to remove levelpack directory '"+dir+"'!",MsgBoxOKOnly,"Addon error");
				continue;
			}
			
			//Also remove the levelpack from the levelpackManager.
			getLevelPackManager()->removeLevelPack(dir);
		}
	}

	//Now that the content has been removed clear the content list itself.
	addon->content.clear();
	//And finally set the addon to not installed.
	addon->installed=false;
	addon->installedVersion=0;
}

void Addons::installAddon(Addon* addon){
	string tmpDir=getUserPath(USER_CACHE)+"tmp/";
	string fileName=fileNameFromPath(addon->file,true);

	//Download the selected addon to the tmp folder.
	if(!downloadFile(addon->file,tmpDir)){
		cerr<<"ERROR: Unable to download addon file "<<addon->file<<endl;
		msgBox("ERROR: Unable to download addon file "+addon->file,MsgBoxOKOnly,"Addon error");
		return;
	}

	//Now extract the addon.
	if(!extractFile(tmpDir+fileName,tmpDir+"/addon/")){
		cerr<<"ERROR: Unable to extract addon file "<<addon->file<<endl;
		msgBox("ERROR: Unable to extract addon file "+addon->file,MsgBoxOKOnly,"Addon error");
		return;
	}

	ifstream metadata((tmpDir+"/addon/metadata").c_str());
	if(!metadata){
		cerr<<"ERROR: Addon is missing metadata!"<<endl;
		msgBox("ERROR: Addon is missing metadata!",MsgBoxOKOnly,"Addon error");
		return;
	}

	//Read the metadata from the addon.
	TreeStorageNode obj;
	{
		POASerializer objSerializer;
		if(!objSerializer.readNode(metadata,&obj,true)){
			//NOTE: We keep the console output English so we put the string literal here twice.
			cerr<<"ERROR: Invalid file format for metadata file!"<<endl;
			msgBox("ERROR: Invalid file format for metadata file!",MsgBoxOKOnly,"Addon error");
			return;
		}
	}

	//Loop through the subNodes.
	for(int i=0;i<obj.subNodes.size();i++){
		//Check for the content subNode (there should only be one).
		if(obj.subNodes[i]->name=="content"){
			TreeStorageNode* obj1=obj.subNodes[i];

			//Loop through the subNodes of that.
			for(int j=0;j<obj1->subNodes.size();j++){
				TreeStorageNode* obj2=obj1->subNodes[j];

				//This code happens for all types of content.
				string source=tmpDir+"addon/content/";
				if(obj2->value.size()>0)
					source+=obj2->value[0];
				//The destination MUST be in the user data path.
				string dest=getUserPath(USER_DATA);
				if(obj2->value.size()>1)
					dest+=obj2->value[1];

				//Check what the content type is.
				if(obj2->name=="file" && obj2->value.size()==2){
					//Now copy the file.
					if(fileExists(dest.c_str())){
						cerr<<"WARNING: File '"<<dest<<"' already exists, addon may be broken or not working!"<<endl;
						msgBox("WARNING: File '"+dest+"' already exists, addon may be broken or not working!",MsgBoxOKOnly,"Addon error");
						continue;
					}
					if(!copyFile(source.c_str(),dest.c_str())){
						cerr<<"WARNING: Unable to copy file '"<<source<<"' to '"<<dest<<"', addon may be broken or not working!"<<endl;
						msgBox("WARNING: Unable to copy file '"+source+"' to '"+dest+"', addon may be broken or not working!",MsgBoxOKOnly,"Addon error");
						continue;
					}

					//Add it to the content vector.
					addon->content.push_back(pair<string,string>("file",obj2->value[1]));
				}else if(obj2->name=="folder" && obj2->value.size()==2){
					//The dest must NOT exist, otherwise it will fail.
					if(dirExists(dest.c_str())){
						cerr<<"WARNING: Destination directory '"<<dest<<"' already exists, addon may be broken or not working!"<<endl;
						msgBox("WARNING: Destination directory '"+dest+"' already exists, addon may be broken or not working!",MsgBoxOKOnly,"Addon error");
						continue;
					}
					//FIXME: Copy the directory instead of renaming it, in case the same folder/parts of the folder are needed in different places.
					if(!renameDirectory(source.c_str(),dest.c_str())){
						cerr<<"WARNING: Unable to move directory '"<<source<<"' to '"<<dest<<"', addon may be broken or not working!"<<endl;
						msgBox("WARNING: Unable to move directory '"+source+"' to '"+dest+"', addon may be broken or not working!",MsgBoxOKOnly,"Addon error");
						continue;
					}

					//Add it to the content vector.
					addon->content.push_back(pair<string,string>("folder",obj2->value[1]));
				}else if(obj2->name=="level" && obj2->value.size()==1){
					//The destination MUST be in the levels folder in the user data path.
					dest+="levels/"+fileNameFromPath(source);

					//Now copy the file.
					if(fileExists(dest.c_str())){
						cerr<<"WARNING: Level '"<<dest<<"' already exists, addon may be broken or not working!"<<endl;
						msgBox("WARNING: Level '"+dest+"' already exists, addon may be broken or not working!",MsgBoxOKOnly,"Addon error");
						continue;
					}
					if(!copyFile(source.c_str(),dest.c_str())){
						cerr<<"WARNING: Unable to copy level '"<<source<<"' to '"<<dest<<"', addon may be broken or not working!"<<endl;
						msgBox("WARNING: Unable to copy level '"+source+"' to '"+dest+"', addon may be broken or not working!",MsgBoxOKOnly,"Addon error");
						continue;
					}

					//It's a level so add it to the Levels levelpack.
					LevelPack* levelsPack=getLevelPackManager()->getLevelPack("Levels/");
					if(levelsPack){
						levelsPack->addLevel(dest);
						levelsPack->setLocked(levelsPack->getLevelCount()-1);
					}else{
						cerr<<"ERROR: Unable to add level to Levels levelpack"<<endl;
					}
					addon->content.push_back(pair<string,string>("level",fileNameFromPath(source)));
				}else if(obj2->name=="levelpack" && obj2->value.size()==1){
					//TODO: Check if the source contains a trailing slash.

					//The destination MUST be in the user data path.
					dest+="levelpacks/"+fileNameFromPath(source);

					//The dest must NOT exist, otherwise it will fail.
					if(dirExists(dest.c_str())){
						cerr<<"WARNING: Levelpack directory '"<<dest<<"' already exists, addon may be broken or not working!"<<endl;
						msgBox("WARNING: Levelpack directory '"+dest+"' already exists, addon may be broken or not working!",MsgBoxOKOnly,"Addon error");
						continue;
					}
					//FIXME: Copy the directory instead of renaming it, in case the same folder/parts of the folder are needed in different places.
					if(!renameDirectory(source.c_str(),dest.c_str())){
						cerr<<"WARNING: Unable to move directory '"<<source<<"' to '"<<dest<<"', addon may be broken or not working!"<<endl;
						msgBox("WARNING: Unable to move directory '"+source+"' to '"+dest+"', addon may be broken or not working!",MsgBoxOKOnly,"Addon error");
						continue;
					}

					//It's a levelpack so add it to the levelpack manager.
					getLevelPackManager()->loadLevelPack(dest);
					addon->content.push_back(pair<string,string>("levelpack",fileNameFromPath(source)));
				}
			}
		}else if(obj.subNodes[i]->name=="dependencies"){
			TreeStorageNode* obj1=obj.subNodes[i];

			//Loop through the subNodes of that.
			for(int j=0;j<obj1->subNodes.size();j++){
				TreeStorageNode* obj2=obj1->subNodes[j];

				if(obj2->name=="addon"){
					//TODO: Dependencies
				}
			}
		}
	}

	//The addon is installed and up to date, but not necessarily flawless.
	addon->installed=true;
	addon->upToDate=true;
	addon->installedVersion=addon->version;
}
