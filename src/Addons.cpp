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
#include "GameState.h"
#include "Functions.h"
#include "FileManager.h"
#include "Globals.h"
#include "Objects.h"
#include "Addons.h"
#include "GUIObject.h"
#include "GUIScrollBar.h"
#include "GUIListBox.h"
#include "POASerializer.h"
#include <SDL/SDL_ttf.h>
#include <SDL/SDL.h>
#include <string>
#include <sstream>
#include <iostream>
using namespace std;

/////////////////////ADDONS/////////////////////
static GUIScrollBar *m_oLvScrollBar=NULL;

Addons::Addons(){
	background=loadImage(getDataPath()+"gfx/menu/addons.png");
	FILE* addon=fopen((getUserPath() + "addons").c_str(), "wb");	
	action=NONE;

	//create GUI (test only)
	GUIObject* obj;
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}

	if(!getAddonsList(addon)) {
		GUIObjectRoot=new GUIObject(0,0,800,600);

		obj=new GUIObject(90,96,200,32,GUIObjectLabel,"Unable to initialze addon menu:");
		obj->Name="lbl";
		GUIObjectRoot->ChildControls.push_back(obj);
		
		obj=new GUIObject(120,130,200,32,GUIObjectLabel,error.c_str());
		obj->Name="lbl";
		GUIObjectRoot->ChildControls.push_back(obj);

		
		obj=new GUIObject(90,550,200,32,GUIObjectButton,"Back");
		obj->Name="cmdBack";
		obj->EventCallback=this;
		GUIObjectRoot->ChildControls.push_back(obj);
		return;
	}
	
	GUIObjectRoot=new GUIObject(0,0,800,600);
	m_oLvScrollBar=new GUIScrollBar(768,140,16,370,ScrollBarVertical,0,0,0,1,5,true,false);
	GUIObjectRoot->ChildControls.push_back(m_oLvScrollBar);

	obj=new GUIObject(90,96,200,32,GUIObjectButton,"Levels");
	obj->Name="cmdLvls";
	obj->EventCallback=this;
	GUIObjectRoot->ChildControls.push_back(obj);
	obj=new GUIObject(300,96,200,32,GUIObjectButton,"Level Packs");
	obj->Name="cmdLvlpacks";
	obj->EventCallback=this;
	GUIObjectRoot->ChildControls.push_back(obj);
	obj=new GUIObject(510,96,200,32,GUIObjectButton,"Themes");
	obj->Name="cmdThemes";
	obj->EventCallback=this;
	GUIObjectRoot->ChildControls.push_back(obj);

	list=new GUIListBox(90,140,620,400);
	list->Item=addonsToList("levels");
	list->Name="lstAddons";
	list->EventCallback=this;
	GUIObjectRoot->ChildControls.push_back(list);
	type="levels";
	
	obj=new GUIObject(90,550,200,32,GUIObjectButton,"Back");
	obj->Name="cmdBack";
	obj->EventCallback=this;
	GUIObjectRoot->ChildControls.push_back(obj);
	actionButton=new GUIObject(510,550,200,32,GUIObjectButton,"Install");
	actionButton->Name="cmdInstall";
	actionButton->EventCallback=this;
	GUIObjectRoot->ChildControls.push_back(actionButton);

}

Addons::~Addons(){
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}
	m_oLvScrollBar=NULL;
}

bool Addons::getAddonsList(FILE* file){
	downloadFile("192.168.2.250/game/addons",file);
	fclose(file);
	
	//Load the file.
	ifstream addon_file;
	addon_file.open((getUserPath()+"addons").c_str());
	
	if(!addon_file) {
		error="ERROR: unable to load addon_list file!";
		cerr<<error<<endl;
		return false;
	}
	
	TreeStorageNode obj;
	{
		POASerializer objSerializer;
		if(!objSerializer.ReadNode(addon_file,&obj,true)){
			error="ERROR: Invalid file format of addon_list!";
			cerr<<error<<endl;
			return false;
		}
	}
	
	//Also load the installed_addons file.
	ifstream iaddon_file;
	iaddon_file.open((getUserPath()+"installed_addons").c_str());
	
	if(!iaddon_file) {
		//The installed_addons file doesn't exist, so we create it.
		ofstream iaddons;
		iaddons.open((getUserPath()+"installed_addons").c_str());
		iaddons.close();
		
		//Also load the installed_addons file.
		iaddon_file.open((getUserPath()+"installed_addons").c_str());
		if(!iaddon_file) {
			error="ERROR: Unable to create the installed_addons file.";
			cerr<<error<<endl;
			return false;
		}
	}
	
	TreeStorageNode obj1;
	{
		POASerializer objSerializer;
		if(!objSerializer.ReadNode(iaddon_file,&obj1,true)){
			error="ERROR: Invalid file format of the installed_addons!";
			cerr<<error<<endl;
			return false;
		}
	}
	
	
	//Fill the vectors.
	addons = new std::vector<Addon>;
	fillAddonList(*addons,obj,obj1);
		
	//Close the files.
	iaddon_file.close();
	addon_file.close();
	return true;
}

void Addons::fillAddonList(std::vector<Addons::Addon> &list, TreeStorageNode &addons, TreeStorageNode &installed_addons){
	for(unsigned int i=0;i<addons.subNodes.size();i++){
		TreeStorageNode* block=addons.subNodes[i];
		if(block==NULL) continue;
		
		string type;
		type=block->name;
		for(unsigned int i=0;i<block->subNodes.size();i++){
			TreeStorageNode* entry=block->subNodes[i];
			if(entry==NULL) continue;
			if(entry->name=="entry" && entry->value.size()==1){
				Addon addon = *(new Addon);
				addon.type=type;
				addon.name=entry->value[0];
				addon.file=entry->attributes["file"][0];
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
				
				list.push_back(addon);
			}
		}
	}
}

std::vector<std::string> Addons::addonsToList(const std::string &type) {
	std::vector<std::string> result;
	
	for(int i=0;i<addons->size();i++) {
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
			result.push_back(entry);
		}
	}
	return result;
}

void Addons::saveInstalledAddons() {
	//Open the file.
	ofstream iaddons;
	iaddons.open((getUserPath()+"installed_addons").c_str());
	if(!iaddons) return;
	
	//Loop all the levels.
	TreeStorageNode installed;
	for(int i=0;i<addons->size();i++) {
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
	objSerializer.WriteNode(&installed,iaddons,true,true);
}

void Addons::handleEvents()
{
	if (event.type==SDL_QUIT){
		saveInstalledAddons();
		setNextState(STATE_EXIT);
	}

	if(event.type==SDL_KEYUP && event.key.keysym.sym==SDLK_ESCAPE){
		setNextState(STATE_LEVEL_SELECT);
	}

	if(event.type==SDL_KEYUP && event.key.keysym.sym==SDLK_s){
		if(Mix_PlayingMusic()==1){
			Mix_HaltMusic();
		}else{
			Mix_PlayMusic(music,-1);
		}				
	}else if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button==SDL_BUTTON_WHEELDOWN && m_oLvScrollBar){
		if(m_oLvScrollBar->Value<m_oLvScrollBar->Max)	m_oLvScrollBar->Value++;
		return;
	}else if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button==SDL_BUTTON_WHEELUP && m_oLvScrollBar){
		if(m_oLvScrollBar->Value>0) m_oLvScrollBar->Value--;
		return;
	}
}

void Addons::logic() {}

void Addons::render(){
	applySurface(0,0,background,screen,NULL);
}

void Addons::GUIEventCallback_OnEvent(std::string Name,GUIObject* obj,int nEventType){
	string s;
	if(Name=="cmdLvlpacks"){
		list->Item=addonsToList("levelpacks");
		list->Value=0;
		type="levelpacks";
		GUIEventCallback_OnEvent("lstAddons",list,GUIEventChange);
	}else if(Name=="cmdLvls"){
		list->Item=addonsToList("levels");
		list->Value=0;
		type="levels";
		GUIEventCallback_OnEvent("lstAddons",list,GUIEventChange);
	}else if(Name=="cmdThemes"){
		list->Item=addonsToList("themes");
		list->Value=0;
		type="themes";
		GUIEventCallback_OnEvent("lstAddons",list,GUIEventChange);
	}else if(Name=="lstAddons"){
		string entry = list->Item[list->Value];
		//Get the addon struct that belongs to it.
		Addon *addon;
		if(type.compare("levels")==0) {
			for(int i=0;i<addons->size();i++) {
				std::string prefix=(*addons)[i].name;
				if(!entry.compare(0, prefix.size(), prefix)) {
					addon=&(*addons)[i];
				}
			}
		} else if(type.compare("levelpacks")==0) {
			for(int i=0;i<addons->size();i++) {
				std::string prefix=(*addons)[i].name;
				if(!entry.compare(0, prefix.size(), prefix)) {
					addon=&(*addons)[i];
				}
			} 
		} else if(type.compare("themes")==0) {
			for(int i=0;i<addons->size();i++) {
				std::string prefix=(*addons)[i].name;
				if(!entry.compare(0, prefix.size(), prefix)) {
					addon=&(*addons)[i];
				}
			}
		}
		
		selected=addon;
		updateActionButton();
	}else if(Name=="cmdBack"){
		saveInstalledAddons();
		setNextState(STATE_LEVEL_SELECT);
	}else if(Name=="cmdInstall"){
		switch(action) {
		  case NONE:
		    break;
		  case INSTALL:
			//Download the addon.
			if(type.compare("levels")==0) {
				downloadFile(selected->file,"%USER%/levels/");
				selected->upToDate=true;
				selected->installed=true;
				selected->installedVersion=selected->version;
				list->Item=addonsToList("levels");
				updateActionButton();
			}else if(type.compare("levelpacks")==0) {
				downloadFile(selected->file,"%USER%/tmp/");
				extractFile("%USER%/tmp/"+fileNameFromPath(selected->file),"%USER%/levelpacks/"+selected->name+"/");
				selected->upToDate=true;
				selected->installed=true;
				selected->installedVersion=selected->version;
				list->Item=addonsToList("levelpacks");
				updateActionButton();
			}else if(type.compare("themes")==0) {
				downloadFile(selected->file,"%USER%/tmp/");
				extractFile("%USER%/tmp/"+fileNameFromPath(selected->file),"%USER%/themes/"+selected->name+"/");
				selected->upToDate=true;
				selected->installed=true;
				selected->installedVersion=selected->version;
				list->Item=addonsToList("themes");
				updateActionButton();
			}
		    break;
		  case UNINSTALL:
			//Uninstall the addon.
			if(type.compare("levels")==0) {
				if(remove((getUserPath() + "levels/" + fileNameFromPath(selected->file)).c_str())) {
					//TODO error handling.
				}
				  
				selected->upToDate=false;
				selected->installed=false;
				list->Item=addonsToList("levels");
				updateActionButton();
			}else if(type.compare("levelpacks")==0) {
				if(removeDirectory(processFileName(getUserPath() + "levelpacks/" + selected->name+"/").c_str())) {
					//TODO error handling.
				}
				  
				selected->upToDate=false;
				selected->installed=false;
				list->Item=addonsToList("levelpacks");
				updateActionButton();
			}else if(type.compare("themes")==0) {
				if(removeDirectory(processFileName(getUserPath() + "themes/" + selected->name+"/").c_str())) {
					//TODO error handling.
				}
				  
				selected->upToDate=false;
				selected->installed=false;
				list->Item=addonsToList("themes");
				updateActionButton();
			}
		    break;
		  case UPDATE:
			//Download the addon.
			if(type.compare("levels")==0) {	
				downloadFile(selected->file,"%USER%/levels/");
				selected->upToDate=true;
				selected->installedVersion=selected->version;
				list->Item=addonsToList("levels");
				updateActionButton();
			}else if(type.compare("levelpacks")==0) {
				if(removeDirectory(processFileName(getUserPath() + "levelpacks/" + selected->name).c_str())) {
					//TODO error handling.
				}
				downloadFile(selected->file,"%USER%/tmp/");
				extractFile("%USER%/tmp/"+fileNameFromPath(selected->file),"%USER%/levelpacks/"+selected->name+"/");
				selected->upToDate=true;
				selected->installedVersion=selected->version;
				list->Item=addonsToList("levelpacks");
				updateActionButton();
			}else if(type.compare("themes")==0) {
				if(removeDirectory(processFileName(getUserPath() + "themes/" + selected->name).c_str())) {
					//TODO error handling.
				}
				downloadFile(selected->file,"%USER%/tmp/");
				extractFile("%USER%/tmp/"+fileNameFromPath(selected->file),"%USER%/themes/"+selected->name+"/");
				selected->upToDate=true;
				selected->installedVersion=selected->version;
				list->Item=addonsToList("themes");
				updateActionButton();
			}
		    break;
		}
	}
}

void Addons::updateActionButton() {
	if(selected->installed) {
		if(selected->upToDate) {
			(*actionButton).Caption="Uninstall";
			action = UNINSTALL;
		} else {
			(*actionButton).Caption="Update";
			action = UPDATE;
		}
	} else {
		(*actionButton).Caption="Install";
		action = INSTALL;
	}
}
