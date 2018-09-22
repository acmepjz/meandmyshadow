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

#include "Addons.h"
#include "GameState.h"
#include "Functions.h"
#include "FileManager.h"
#include "GUIObject.h"
#include "GUIOverlay.h"
#include "GUIScrollBar.h"
#include "GUITextArea.h"
#include "GUIListBox.h"
#include "POASerializer.h"
#include "LevelPackManager.h"
#include "InputManager.h"
#include "ThemeManager.h"
#include <string>
#include <sstream>
#include <iostream>
#include "libs/tinyformat/tinyformat.h"
#include <SDL.h>
#include <SDL_ttf.h>


using namespace std;

static const char* predefinedCategories[] = {
	"levels", __("Levels"), __("Single level which usually contain demanding puzzles"),
	"levelpacks", __("Levelpacks"), __("Collection of levels with the same author or style"),
	"themes", __("Themes"), __("Give every block and background a new look and feel"),
	NULL,
};

static std::map<std::string, std::string> categoryNameMap;
static std::map<std::string, std::string> categoryDescriptionMap;

Addons::Addons(SDL_Renderer &renderer, ImageManager &imageManager):selected(NULL){
	//Render the title.
	title = titleTextureFromText(renderer, _("Addons"), objThemes.getTextColor(false), SCREEN_WIDTH);

	//Load placeholder addon icons and screenshot.
	addonIcon["levels"] = imageManager.loadImage(getDataPath() + "/gfx/addon1.png");
	addonIcon["levelpacks"] = imageManager.loadImage(getDataPath() + "/gfx/addon2.png");
	addonIcon["themes"] = imageManager.loadImage(getDataPath() + "/gfx/addon3.png");
	addonIcon[std::string()] = imageManager.loadImage(getDataPath() + "/gfx/addon0.png");

	//Load predefined categories.
	if (categoryNameMap.empty()) {
		for (int i = 0; predefinedCategories[i]; i += 3) {
			categoryNameMap[predefinedCategories[i]] = predefinedCategories[i + 1];
			categoryDescriptionMap[predefinedCategories[i]] = predefinedCategories[i + 2];
		}
	}

	screenshot=imageManager.loadTexture(getDataPath()+"/gfx/screenshot.png", renderer);

	//Open the addons file in the user cache path for writing (downloading) to.
	FILE* addon=fopen((getUserPath(USER_CACHE)+"addons").c_str(),"wb");
	
	//Clear the GUI if any.
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}

	//Show a loading screen
	{
		int w = 0, h = 0;
		SDL_GetRendererOutputSize(&renderer, &w, &h);
		SDL_Color fg = { 255, 255, 255, 0 };
		TexturePtr loadingTexture = titleTextureFromText(renderer, _("Loading..."), fg, w);
		SDL_Rect loadingRect = rectFromTexture(*loadingTexture);
		loadingRect.x = (w - loadingRect.w) / 2;
		loadingRect.y = (h - loadingRect.h) / 2;
		SDL_RenderCopy(&renderer, loadingTexture.get(), NULL, &loadingRect);
		SDL_RenderPresent(&renderer);
		SDL_RenderClear(&renderer);
	}

	//Try to get(download) the addonsList.
    if(getAddonsList(addon, renderer, imageManager)==false){
		//It failed so we show the error message.
        GUIObjectRoot=new GUIObject(imageManager,renderer,0,0,SCREEN_WIDTH,SCREEN_HEIGHT);

        GUIObject* obj=new GUILabel(imageManager,renderer,90,96,200,32,_("Unable to initialize addon menu:"));
		obj->name="lbl";
		GUIObjectRoot->addChild(obj);
		
        obj=new GUILabel(imageManager,renderer,120,130,200,32,error.c_str());
		obj->name="lbl";
		GUIObjectRoot->addChild(obj);
		
        obj=new GUIButton(imageManager,renderer,90,550,200,32,_("Back"));
		obj->name="cmdBack";
		obj->eventCallback=this;
		GUIObjectRoot->addChild(obj);
		return;
	}
	
	//Now create the GUI.
    createGUI(renderer, imageManager);
}

Addons::~Addons(){
	//If the GUIObjectRoot exist delete it.
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}
}

void Addons::createGUI(SDL_Renderer& renderer, ImageManager& imageManager){
	//Downloaded the addons file now we can create the GUI.
    GUIObjectRoot=new GUIObject(imageManager,renderer,0,0,SCREEN_WIDTH,SCREEN_HEIGHT);
	
	//Create list of categories
	categoryList = new GUISingleLineListBox(imageManager, renderer, (SCREEN_WIDTH - 500) / 2, 100, 500, 32);
	categoryList->name="lstTabs";
	//Loop through the categories and add them to the list.
	
	//FIXME: Hack for easy detecting which categories there are.
	{
		set<string> categories;
		for (const auto& a : addons) {
			categories.insert(a.type);
		}
		for (const auto& c : categories) {
			auto it = categoryNameMap.find(c);
			categoryList->addItem(c, it == categoryNameMap.end() ? c.c_str() : _(it->second));
		}
	}
	categoryList->value=0;
	categoryList->eventCallback=this;
	GUIObjectRoot->addChild(categoryList);

	//category description
	categoryDescription = new GUILabel(imageManager, renderer, 0, 136, SCREEN_WIDTH, 32, "", 0, true, true, GUIGravityCenter);
	if (categoryList->value >= 0 && categoryList->value < (int)categoryList->item.size()) {
		auto it = categoryDescriptionMap.find(categoryList->item[categoryList->value].first);
		if (it != categoryDescriptionMap.end()) categoryDescription->caption = _(it->second);
	}
	GUIObjectRoot->addChild(categoryDescription);

	//Create the list for the addons.
	//By default levels will be selected.
    list=new GUIListBox(imageManager,renderer,SCREEN_WIDTH*0.1,176,SCREEN_WIDTH*0.8,SCREEN_HEIGHT-228);
    addonsToList(categoryList->getName(), renderer, imageManager);
	list->name="lstAddons";
	list->clickEvents=true;
	list->eventCallback=this;
	list->value=-1;
	GUIObjectRoot->addChild(list);
	type="levels";
	
	//The back button.
    GUIObject* obj=new GUIButton(imageManager,renderer,20,20,-1,32,_("Back"));
	obj->name="cmdBack";
	obj->eventCallback=this;
	GUIObjectRoot->addChild(obj);
}

bool Addons::getAddonsList(FILE* file, SDL_Renderer& renderer, ImageManager& imageManager){
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
    fillAddonList(obj,obj1, renderer, imageManager);
	
	//Close the files.
	iaddonFile.close();
	addonFile.close();
	return true;
}

void Addons::fillAddonList(TreeStorageNode &objAddons, TreeStorageNode &objInstalledAddons,
                           SDL_Renderer& renderer, ImageManager& imageManager){
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
                    addon.icon=nullptr;
                    addon.screenshot=nullptr;
					addon.type=type;
					addon.name=entry->value[0];
					addon.version = 0;
					addon.installedVersion = 0;

					if(!entry->attributes["file"].empty())
						addon.file=entry->attributes["file"][0];
					if(!entry->attributes["author"].empty())
						addon.author=entry->attributes["author"][0];
					if(!entry->attributes["description"].empty())
						addon.description=entry->attributes["description"][0];
					if(!entry->attributes["license"].empty())
						addon.license=entry->attributes["license"][0];
					if(!entry->attributes["website"].empty())
						addon.website=entry->attributes["website"][0];
					if(entry->attributes["icon"].size()>1){
						//There are (at least) two values, the url to the icon and its md5sum used for caching.
                        addon.icon=loadCachedImage(
                                entry->attributes["icon"][0].c_str(),
                                entry->attributes["icon"][1].c_str(),
                                imageManager
                        );
					}
					if(entry->attributes["screenshot"].size()>1){
						//There are (at least) two values, the url to the screenshot and its md5sum used for caching.
                        addon.screenshot=loadCachedTexture(
                                entry->attributes["screenshot"][0].c_str(),
                                entry->attributes["screenshot"][1].c_str(),
                                renderer,
                                imageManager
                        );
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

								//Read the dependencies and content from the file.
								for(unsigned int j=0;j<installed->subNodes.size();j++){
									if(installed->subNodes[j]->name=="content"){
										TreeStorageNode* obj=installed->subNodes[j];
										for(unsigned int k=0;k<obj->subNodes.size();k++){
											if(obj->subNodes[k]->value.size()==1)
												addon.content.push_back(pair<string,string>(obj->subNodes[k]->name,obj->subNodes[k]->value[0]));
										}
									}else if(installed->subNodes[j]->name=="dependencies"){
										TreeStorageNode* obj=installed->subNodes[j];
										for(unsigned int k=0;k<obj->subNodes.size();k++){
											if(obj->subNodes[k]->value.size()==1)
												addon.dependencies.push_back(pair<string,string>(obj->subNodes[k]->name,obj->subNodes[k]->value[0]));
										}
									}
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

void Addons::addonsToList(const std::string &type, SDL_Renderer& renderer, ImageManager&){
	//Clear the list.
	list->clearItems();
	//Loop through the addons.
	for(unsigned int i=0;i<addons.size();i++) {
		//Make sure the addon is of the requested type.
		if(addons[i].type!=type)
			continue;
		
        const Addon& addon=addons[i];
		
		string entry=addon.name+" by "+addon.author;
		if(addon.installed){
			if(addon.upToDate){
				entry+=" *";
			}else{
				entry+=" +";
			}
		}
        SurfacePtr surf = createSurface(list->width,74);

		//Check if there's an icon for the addon.
		if(addon.icon){
			applySurface(5, 5, addon.icon, surf.get(), NULL);
		}else{
			auto it = addonIcon.find(type);
			if (it == addonIcon.end()) it = addonIcon.find(std::string());
			assert(it != addonIcon.end());

			applySurface(5, 5, it->second, surf.get(), NULL);
		}

		SDL_Surface* nameSurf=TTF_RenderUTF8_Blended(fontGUI,addon.name.c_str(),objThemes.getTextColor(true));
		SDL_SetSurfaceAlphaMod(nameSurf,0xFF);
        applySurface(74,-1,nameSurf,surf.get(),NULL);
		SDL_FreeSurface(nameSurf);
		
		/// TRANSLATORS: indicates the author of an addon.
		string authorLine = tfm::format(_("by %s"),addon.author);
		SDL_Surface* authorSurf=TTF_RenderUTF8_Blended(fontText,authorLine.c_str(),objThemes.getTextColor(true));
		SDL_SetSurfaceAlphaMod(authorSurf,0xFF);
        applySurface(74,43,authorSurf,surf.get(),NULL);
		SDL_FreeSurface(authorSurf);
		
		if(addon.installed){
			if(addon.upToDate){
				SDL_Surface* infoSurf=TTF_RenderUTF8_Blended(fontText,_("Installed"),objThemes.getTextColor(true));
				SDL_SetSurfaceAlphaMod(infoSurf,0xFF);
                applySurface(surf->w-infoSurf->w-32,(surf->h-infoSurf->h)/2,infoSurf,surf.get(),NULL);
				SDL_FreeSurface(infoSurf);
			}else{
				SDL_Surface* infoSurf=TTF_RenderUTF8_Blended(fontText,_("Updatable"),objThemes.getTextColor(true));
				SDL_SetSurfaceAlphaMod(infoSurf,0xFF);
                applySurface(surf->w-infoSurf->w-32,(surf->h-infoSurf->h)/2,infoSurf,surf.get(),NULL);
				SDL_FreeSurface(infoSurf);
			}
		}else{
			SDL_Color c = objThemes.getTextColor(true);
			c.r = c.r / 2 + 128;
			c.g = c.g / 2 + 128;
			c.b = c.b / 2 + 128;
			SDL_Surface* infoSurf = TTF_RenderUTF8_Blended(fontText, _("Not installed"), c);
			SDL_SetSurfaceAlphaMod(infoSurf,0xFF);
            applySurface(surf->w-infoSurf->w-32,(surf->h-infoSurf->h)/2,infoSurf,surf.get(),NULL);
			SDL_FreeSurface(infoSurf);
		}
		
        list->addItem(renderer,entry,textureFromSurface(renderer,std::move(surf)));
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
			TreeStorageNode* content=new TreeStorageNode;
			content->name="content";
			for(unsigned int i=0;i<it->content.size();i++){
				TreeStorageNode* contentEntry=new TreeStorageNode;
				contentEntry->name=it->content[i].first;
				contentEntry->value.push_back(it->content[i].second);

				//Add the content node to the entry node.
				content->subNodes.push_back(contentEntry);
			}
			entry->subNodes.push_back(content);

			//Now add a sub node for the dependencies.
			TreeStorageNode* deps=new TreeStorageNode;
			deps->name="dependencies";
			for(unsigned int i=0;i<it->dependencies.size();i++){
				TreeStorageNode* depsEntry=new TreeStorageNode;
				depsEntry->name=it->dependencies[i].first;
				depsEntry->value.push_back(it->dependencies[i].second);

				//Add the content node to the entry node.
				deps->subNodes.push_back(depsEntry);
			}
			entry->subNodes.push_back(deps);

			//And add the entry to the top node.
			installed.subNodes.push_back(entry);
		}
	}
	
	//And write away the file.
	POASerializer objSerializer;
	objSerializer.writeNode(&installed,iaddons,true,true);
	
	return true;
}

SharedTexture Addons::loadCachedTexture(const char* url,const char* md5sum,
                                     SDL_Renderer& renderer, ImageManager& imageManager){
	//Check if the image is cached.
	string imageFile=getUserPath(USER_CACHE)+"images/"+md5sum;
	if(fileExists(imageFile.c_str())){
		//It is, so load the image.
        return imageManager.loadTexture(imageFile, renderer);
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
        return imageManager.loadTexture(imageFile, renderer);
	}
}

SDL_Surface* Addons::loadCachedImage(const char* url, const char* md5sum,
	ImageManager& imageManager){
	//Check if the image is cached.
	string imageFile = getUserPath(USER_CACHE) + "images/" + md5sum;
	if (fileExists(imageFile.c_str())){
		//It is, so load the image.
		return imageManager.loadImage(imageFile);
	} else{
		//Download the image.
		FILE* file = fopen(imageFile.c_str(), "wb");

		//Downloading failed.
		if (!downloadFile(url, file)){
			cerr << "ERROR: Unable to download image from " << url << endl;
			fclose(file);
			return NULL;
		}
		fclose(file);

		//Load the image.
		return imageManager.loadImage(imageFile);
	}
}

void Addons::handleEvents(ImageManager& imageManager, SDL_Renderer& renderer){
	//Check if we should quit.
	if(event.type==SDL_QUIT){
		//Save the installed addons before exiting.
		saveInstalledAddons();
		setNextState(STATE_EXIT);
	}

	//Check if escape is pressed, if so return to the main menu.
	if(inputMgr.isKeyDownEvent(INPUTMGR_ESCAPE)){
		setNextState(STATE_MENU);
	}

	//Check horizontal movement
	int value = categoryList->value;
	if (inputMgr.isKeyDownEvent(INPUTMGR_RIGHT)){
		isKeyboardOnly = true;
		value++;
		if (value >= (int)categoryList->item.size()) value = 0;
	} else if (inputMgr.isKeyDownEvent(INPUTMGR_LEFT)){
		isKeyboardOnly = true;
		value--;
		if (value < 0) value = categoryList->item.size() - 1;
	}

	if (value >= 0 && value < (int)categoryList->item.size()) {
		if (categoryList->value != value) {
			categoryList->value = value;
			GUIEventCallback_OnEvent(imageManager, renderer, categoryList->name, categoryList, GUIEventChange);
			return;
		}

		//Check vertical movement
		if (inputMgr.isKeyDownEvent(INPUTMGR_UP)){
			isKeyboardOnly = true;
			list->value--;
			if (list->value < 0) list->value = 0;

			//FIXME: ad-hoc stupid code
			list->scrollScrollbar(0xC0000000);
			list->scrollScrollbar(list->value);
		} else if (inputMgr.isKeyDownEvent(INPUTMGR_DOWN)){
			isKeyboardOnly = true;
			list->value++;
			if (list->value >= (int)list->item.size()) list->value = list->item.size() - 1;

			//FIXME: ad-hoc stupid code
			list->scrollScrollbar(0xC0000000);
			list->scrollScrollbar(list->value);
		}

		if (isKeyboardOnly && inputMgr.isKeyDownEvent(INPUTMGR_SELECT) && list->value >= 0 && list->value<(int)list->item.size()) {
			GUIEventCallback_OnEvent(imageManager, renderer, list->name, list, GUIEventChange); // ???
			GUIEventCallback_OnEvent(imageManager, renderer, list->name, list, GUIEventClick);
			return;
		}
	}
}

void Addons::logic(ImageManager&, SDL_Renderer&){}

void Addons::render(ImageManager&, SDL_Renderer& renderer){
	//Draw background.
    objThemes.getBackground(true)->draw(renderer);
	
	//Draw the title.
    drawTitleTexture(SCREEN_WIDTH, *title, renderer);
}

void Addons::resize(ImageManager& imageManager, SDL_Renderer& renderer){
	//Delete the gui (if any).
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}
	
	//Now create a new one.
    createGUI(renderer, imageManager);
}

void Addons::showAddon(ImageManager& imageManager, SDL_Renderer& renderer){
	//Make sure an addon is selected.
	if(!selected)
		return;

	//Skip next mouse up event since we're clicking a list box and showing a new window.
	GUISkipNextMouseUpEvent = true;

	//Create a root object.
    GUIObject* root=new GUIFrame(imageManager,renderer,(SCREEN_WIDTH-600)/2,(SCREEN_HEIGHT-400)/2,600,400,selected->name.c_str());

	//Create the 'by creator' label.
    GUIObject* obj=new GUILabel(imageManager,renderer,0,50,600,50,tfm::format(_("by %s"),selected->author).c_str(),0,true,true,GUIGravityCenter);
	root->addChild(obj);

	//Create the description text.
	std::string s = tfm::format(_("Version: %d\n"), selected->version);
	if (selected->installed) {
		s += tfm::format(_("Installed version: %d\n"), selected->installedVersion);
	}
	if (!selected->license.empty()) {
		s += tfm::format(_("License: %s\n"), appendURLToLicense(selected->license));
	}
	if (!selected->website.empty()) {
		s += tfm::format(_("Website: %s\n"), selected->website);
	}
	s += '\n';
	if (selected->description.empty()) {
		s += _("(No descriptions provided)");
	} else {
		s += selected->description;
	}

    GUITextArea* description=new GUITextArea(imageManager,renderer,10,100,370,200);
    description->setString(renderer, s, true);
	description->editable=false;
	description->onResize();
	description->extractHyperlinks();
	root->addChild(description);

    //Create the screenshot image. (If a screenshot is missing, we use the default screenshot.)
    GUIImage* img=new GUIImage(imageManager,renderer,390,100,200,150,selected->screenshot?selected->screenshot:screenshot);
	root->addChild(img);

	GUIButton *cancelButton;

	//Add buttons depending on the installed/update status.
	if(selected->installed && !selected->upToDate){
        GUIObject* bRemove=new GUIButton(imageManager,renderer,root->width*0.97,350,-1,32,_("Remove"),0,true,true,GUIGravityRight);
		bRemove->name="cmdRemove";
		bRemove->eventCallback=this;
		root->addChild(bRemove);
		//Create a back button.
		cancelButton = new GUIButton(imageManager, renderer, root->width*0.03, 350, -1, 32, _("Back"), 0, true, true, GUIGravityLeft);
		cancelButton->name = "cmdCloseOverlay";
		cancelButton->eventCallback = this;
		root->addChild(cancelButton);
		
		//Update widget sizes.
        root->render(renderer, 0,0,false);
		
		//Create a nicely centered button.
		obj = new GUIButton(imageManager, renderer,
			(int)floor((cancelButton->left + cancelButton->width + bRemove->left - bRemove->width)*0.5), 350,
			-1, 32, _("Update"), 0, true, true, GUIGravityCenter);
		obj->name="cmdUpdate";
		obj->eventCallback=this;
		root->addChild(obj);
	}else{
		if(!selected->installed){
            obj=new GUIButton(imageManager,renderer,root->width*0.9,350,-1,32,_("Install"),0,true,true,GUIGravityRight);
			obj->name="cmdInstall";
			obj->eventCallback=this;
			root->addChild(obj);
		}else if(selected->upToDate){
            obj=new GUIButton(imageManager,renderer,root->width*0.9,350,-1,32,_("Remove"),0,true,true,GUIGravityRight);
			obj->name="cmdRemove";
			obj->eventCallback=this;
			root->addChild(obj);
		}
		//Create a back button.
		cancelButton = new GUIButton(imageManager, renderer, root->width*0.1, 350, -1, 32, _("Back"), 0, true, true, GUIGravityLeft);
		cancelButton->name = "cmdCloseOverlay";
		cancelButton->eventCallback = this;
		root->addChild(cancelButton);
	}
	
	new AddonOverlay(renderer, root, cancelButton, description, TabFocus | ReturnControls);
}

void Addons::GUIEventCallback_OnEvent(ImageManager& imageManager, SDL_Renderer& renderer, std::string name,GUIObject* obj,int eventType){
    if(name=="lstTabs"){
		//Get the category type.
		type=categoryList->getName();
		//Get the description of current category.
		auto it = categoryDescriptionMap.find(type);
		if (it != categoryDescriptionMap.end()) categoryDescription->caption = _(it->second);
		else categoryDescription->caption.clear();
		//Get the list corresponding with the category and select the first entry.
        addonsToList(type, renderer, imageManager);
		list->value=-1;
		//Call an event as if an entry in the addons listbox was clicked.
        GUIEventCallback_OnEvent(imageManager, renderer, "lstAddons",list,GUIEventChange);
	}else if(name=="lstAddons"){
		//Check which type of event.
		if(eventType==GUIEventChange){
			//Get the addon struct that belongs to it.
			Addon* addon=NULL;

			//Make sure the addon list on screen isn't empty.
			if (list->value >= 0 && list->value < (int)list->item.size()){
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
			if (!isKeyboardOnly) list->value = -1;
		}else if(eventType==GUIEventClick){
			//Make sure an addon is selected.
			if(selected){
                showAddon(imageManager,renderer);
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
            removeAddon(imageManager,renderer,selected);
            installAddon(imageManager,renderer,selected);
		}
        addonsToList(categoryList->getName(), renderer, imageManager);
	}else if(name=="cmdInstall"){
		if(selected)
            installAddon(imageManager,renderer,selected);
        addonsToList(categoryList->getName(), renderer, imageManager);
	}else if(name=="cmdRemove"){
		//TODO: Check for dependencies.
		//Loop through the addons to check if this addon is a dependency of another addon.
		vector<Addon>::iterator it;
		for(it=addons.begin();it!=addons.end();++it){
			//Check if the addon has dependencies.
			if(!it->dependencies.empty()){
				vector<pair<string,string> >::iterator depIt;
				for(depIt=it->dependencies.begin();depIt!=it->dependencies.end();++depIt){
					if(depIt->first=="addon" && depIt->second==selected->name){
                        msgBox(imageManager,renderer,tfm::format(_("This addon can't be removed because it's needed by %s."),it->name),MsgBoxOKOnly,_("Dependency"));
						return;
					}
				}
			}
		}
		
		if(selected)
            removeAddon(imageManager,renderer,selected);
        addonsToList(categoryList->getName(), renderer, imageManager);
	}

	//NOTE: In case of install/remove/update we can delete the GUIObjectRoot, since it's managed by the GUIOverlay.
	if(name=="cmdUpdate" || name=="cmdInstall" || name=="cmdRemove"){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}
}

void Addons::removeAddon(ImageManager& imageManager,SDL_Renderer& renderer, Addon* addon){
	//To remove an addon we loop over the content vector in the structure.
	//NOTE: This should contain all INSTALLED content, if something failed during installation it isn't added.
	for(unsigned int i=0;i<addon->content.size();i++){
		//Check the type of content.
		if(addon->content[i].first=="file"){
			string file=getUserPath(USER_DATA)+addon->content[i].second;
			//Check if the file exists.
			if(!fileExists(file.c_str())){
				cerr<<"WARNING: File '"<<file<<"' appears to have been removed already."<<endl;
                msgBox(imageManager,renderer,tfm::format(_("WARNING: File '%s' appears to have been removed already."),file),MsgBoxOKOnly,_("Addon error"));
				continue;
			}
			
			//Remove the file.
			if(!removeFile(file.c_str())){
				cerr<<"ERROR: Unable to remove file '"<<file<<"'!"<<endl;
                msgBox(imageManager,renderer,tfm::format(_("ERROR: Unable to remove file '%s'!"),file),MsgBoxOKOnly,_("Addon error"));
				continue;
			}
		}else if(addon->content[i].first=="folder"){
			string dir=getUserPath(USER_DATA)+addon->content[i].second;
			//Check if the directory exists.
			if(!dirExists(dir.c_str())){
				cerr<<"WARNING: Directory '"<<dir<<"' appears to have been removed already."<<endl;
                msgBox(imageManager,renderer,tfm::format(_("WARNING: Directory '%s' appears to have been removed already."),dir),MsgBoxOKOnly,_("Addon error"));
				continue;
			}
			
			//Remove the directory.
			if(!removeDirectory(dir.c_str())){
				cerr<<"ERROR: Unable to remove directory '"<<dir<<"'!"<<endl;
                msgBox(imageManager,renderer,tfm::format(_("ERROR: Unable to remove directory '%s'!"),dir),MsgBoxOKOnly,_("Addon error"));
				continue;
			}
		}else if(addon->content[i].first=="level"){
			string file=getUserPath(USER_DATA)+"levels/"+addon->content[i].second;

			//Check if the level file exists.
			if(!fileExists(file.c_str())){
				cerr<<"WARNING: Level '"<<file<<"' appears to have been removed already."<<endl;
                msgBox(imageManager,renderer,tfm::format(_("WARNING: Level '%s' appears to have been removed already."),file),MsgBoxOKOnly,_("Addon error"));
				continue;
			}
			//Remove the level file.
			if(!removeFile(file.c_str())){
				cerr<<"ERROR: Unable to remove level '"<<file<<"'!"<<endl;
                msgBox(imageManager,renderer,tfm::format(_("ERROR: Unable to remove level '%s'!"),file),MsgBoxOKOnly,_("Addon error"));
				continue;
			}
			//Also remove the level from the Levels levelpack.
			LevelPack* levelsPack=getLevelPackManager()->getLevelPack(LEVELS_PATH);

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
                msgBox(imageManager,renderer,tfm::format(_("WARNING: Levelpack directory '%s' appears to have been removed already."),dir),MsgBoxOKOnly,_("Addon error"));
				continue;
			}

			//Remove the directory.
			if(!removeDirectory(dir.c_str())){
				cerr<<"ERROR: Unable to remove levelpack directory '"<<dir<<"'!"<<endl;
                msgBox(imageManager,renderer,tfm::format(_("ERROR: Unable to remove levelpack directory '%s'!"),dir),MsgBoxOKOnly,_("Addon error"));
				continue;
			}
			
			//Also remove the levelpack from the levelpackManager.
			getLevelPackManager()->removeLevelPack(dir, true);
		}
	}

	//Now that the content has been removed clear the content list itself.
	addon->content.clear();
	//And finally set the addon to not installed.
	addon->installed=false;
	addon->installedVersion=0;

	//Also clear the 'offline' information.
	addon->content.clear();
	addon->dependencies.clear();
}

void Addons::installAddon(ImageManager& imageManager,SDL_Renderer& renderer, Addon* addon){
	string tmpDir=getUserPath(USER_CACHE)+"tmp/";
	string fileName=fileNameFromPath(addon->file,true);

	//Download the selected addon to the tmp folder.
	if(!downloadFile(addon->file,tmpDir)){
		cerr<<"ERROR: Unable to download addon file "<<addon->file<<endl;
        msgBox(imageManager,renderer,tfm::format(_("ERROR: Unable to download addon file %s."),addon->file),MsgBoxOKOnly,_("Addon error"));
		return;
	}

	//Now extract the addon.
	if(!extractFile(tmpDir+fileName,tmpDir+"/addon/")){
		cerr<<"ERROR: Unable to extract addon file "<<addon->file<<endl;
        msgBox(imageManager,renderer,tfm::format(_("ERROR: Unable to extract addon file %s."),addon->file),MsgBoxOKOnly,_("Addon error"));
		return;
	}

	ifstream metadata((tmpDir+"/addon/metadata").c_str());
	if(!metadata){
		cerr<<"ERROR: Addon is missing metadata!"<<endl;
        msgBox(imageManager,renderer,_("ERROR: Addon is missing metadata!"),MsgBoxOKOnly,_("Addon error"));
		return;
	}

	//Read the metadata from the addon.
	TreeStorageNode obj;
	{
		POASerializer objSerializer;
		if(!objSerializer.readNode(metadata,&obj,true)){
			//NOTE: We keep the console output English so we put the string literal here twice.
			cerr<<"ERROR: Invalid file format for metadata file!"<<endl;
            msgBox(imageManager,renderer,_("ERROR: Invalid file format for metadata file!"),MsgBoxOKOnly,_("Addon error"));
			return;
		}
	}

	//Loop through the subNodes.
	for(unsigned int i=0;i<obj.subNodes.size();i++){
		//Check for the content subNode (there should only be one).
		if(obj.subNodes[i]->name=="content"){
			TreeStorageNode* obj1=obj.subNodes[i];

			//Loop through the subNodes of that.
			for(unsigned int j=0;j<obj1->subNodes.size();j++){
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
                        msgBox(imageManager,renderer,tfm::format(_("WARNING: File '%s' already exists, addon may be broken or not working!"),dest),MsgBoxOKOnly,_("Addon error"));
						continue;
					}
					if(!copyFile(source.c_str(),dest.c_str())){
						cerr<<"WARNING: Unable to copy file '"<<source<<"' to '"<<dest<<"', addon may be broken or not working!"<<endl;
                        msgBox(imageManager,renderer,tfm::format(_("WARNING: Unable to copy file '%s' to '%s', addon may be broken or not working!"),source,dest),MsgBoxOKOnly,_("Addon error"));
						continue;
					}

					//Add it to the content vector.
					addon->content.push_back(pair<string,string>("file",obj2->value[1]));
				}else if(obj2->name=="folder" && obj2->value.size()==2){
					//The dest must NOT exist, otherwise it will fail.
					if(dirExists(dest.c_str())){
						cerr<<"WARNING: Destination directory '"<<dest<<"' already exists, addon may be broken or not working!"<<endl;
                        msgBox(imageManager,renderer,tfm::format(_("WARNING: Destination directory '%s' already exists, addon may be broken or not working!"),dest),MsgBoxOKOnly,_("Addon error"));
						continue;
					}
					//FIXME: Copy the directory instead of renaming it, in case the same folder/parts of the folder are needed in different places.
					if(!renameDirectory(source.c_str(),dest.c_str())){
						cerr<<"WARNING: Unable to move directory '"<<source<<"' to '"<<dest<<"', addon may be broken or not working!"<<endl;
                        msgBox(imageManager,renderer,tfm::format(_("WARNING: Unable to move directory '%s' to '%s', addon may be broken or not working!"),source,dest),MsgBoxOKOnly,_("Addon error"));
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
                        msgBox(imageManager,renderer,tfm::format(_("WARNING: Level '%s' already exists, addon may be broken or not working!"),dest),MsgBoxOKOnly,_("Addon error"));
						continue;
					}
					if(!copyFile(source.c_str(),dest.c_str())){
						cerr<<"WARNING: Unable to copy level '"<<source<<"' to '"<<dest<<"', addon may be broken or not working!"<<endl;
                        msgBox(imageManager,renderer,tfm::format(_("WARNING: Unable to copy level '%s' to '%s', addon may be broken or not working!"),source,dest),MsgBoxOKOnly,_("Addon error"));
						continue;
					}

					//It's a level so add it to the Levels levelpack.
					LevelPack* levelsPack=getLevelPackManager()->getLevelPack(LEVELS_PATH);
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
                        msgBox(imageManager,renderer,tfm::format(_("WARNING: Levelpack directory '%s' already exists, addon may be broken or not working!"),dest),MsgBoxOKOnly,_("Addon error"));
						continue;
					}
					//FIXME: Copy the directory instead of renaming it, in case the same folder/parts of the folder are needed in different places.
					if(!renameDirectory(source.c_str(),dest.c_str())){
						cerr<<"WARNING: Unable to move directory '"<<source<<"' to '"<<dest<<"', addon may be broken or not working!"<<endl;
                        msgBox(imageManager,renderer,tfm::format(_("WARNING: Unable to move directory '%s' to '%s', addon may be broken or not working!"),source,dest),MsgBoxOKOnly,_("Addon error"));
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
			for(unsigned int j=0;j<obj1->subNodes.size();j++){
				TreeStorageNode* obj2=obj1->subNodes[j];

				if(obj2->name=="addon" && obj2->value.size()>0){
					Addon* dep=NULL;
					
					//Check if the requested addon can be found.
					vector<Addon>::iterator it;
					for(it=addons.begin();it!=addons.end();++it){
						if(it->name==obj2->value[0]){
							dep=&(*it);
							break;
						}
					}

					if(!dep){
						cerr<<"ERROR: Addon requires another addon ("<<obj2->value[0]<<") which can't be found!"<<endl;
						msgBox(imageManager, renderer, tfm::format(_("ERROR: Addon requires another addon (%s) which can't be found!"), obj2->value[0]), MsgBoxOKOnly, _("Addon error"));
						continue;
					}

					//The addon has been found, try to install it.
					//FIXME: Somehow prevent recursion, maybe max depth (??)
					if(!dep->installed){
						msgBox(imageManager, renderer, tfm::format(_("The addon %s is needed and will be installed now."), dep->name), MsgBoxOKOnly, _("Dependency"));
                        installAddon(imageManager,renderer, dep);
					}
					
					//Add the dependency to the addon.
					addon->dependencies.push_back(pair<string,string>("addon",dep->name));
				}
			}
		}
	}

	//The addon is installed and up to date, but not necessarily flawless.
	addon->installed=true;
	addon->upToDate=true;
	addon->installedVersion=addon->version;
}
