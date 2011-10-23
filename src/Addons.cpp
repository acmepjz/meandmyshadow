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
#include <string>
#include <sstream>
#include <iostream>
#include <SDL/SDL_ttf.h>
#include <SDL/SDL.h>
using namespace std;

Addons::Addons(){
	//Load the backgroundimage and the addons file.
	background=loadImage(getDataPath()+"gfx/menu/addons.png");
	FILE* addon=fopen((getUserPath()+"addons").c_str(),"wb");	
	action=NONE;

	//Create the gui.
	GUIObject* obj;
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}

	//Try to get(download) the addonsList.
	if(getAddonsList(addon)==false) {
		//It failed so we show the error message.
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
	
	//Downloaded the addons file now we can create the GUI.
	GUIObjectRoot=new GUIObject(0,0,800,600);
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

	//Create the list for the addons.
	//By default levels will be selected.
	list=new GUIListBox(90,140,620,400);
	list->Item=addonsToList("levels");
	list->Name="lstAddons";
	list->EventCallback=this;
	GUIObjectRoot->ChildControls.push_back(list);
	type="levels";
	
	//And the buttons at the bottom of the screen.
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
	//If the GUIObjectRoot exist delete it.
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}
}

bool Addons::getAddonsList(FILE* file){
	//First we download the file.
	if(downloadFile("http://meandmyshadow.sourceforge.net/game/addons",file)==false){
		error="ERROR: unable to download addons file!";
		cerr<<error<<endl;
		return false;
	}
	fclose(file);
	
	//Load the downloaded file.
	ifstream addonFile;
	addonFile.open((getUserPath()+"addons").c_str());
	
	if(addonFile==false) {
		error="ERROR: unable to load addon_list file!";
		cerr<<error<<endl;
		return false;
	}
	
	//Parse the addonsfile.
	TreeStorageNode obj;
	{
		POASerializer objSerializer;
		if(!objSerializer.ReadNode(addonFile,&obj,true)){
			error="ERROR: Invalid file format of addons file!";
			cerr<<error<<endl;
			return false;
		}
	}
	
	//Also load the installed_addons file.
	ifstream iaddonFile;
	iaddonFile.open((getUserPath()+"installed_addons").c_str());
	
	if(!iaddonFile) {
		//The installed_addons file doesn't exist, so we create it.
		ofstream iaddons;
		iaddons.open((getUserPath()+"installed_addons").c_str());
		iaddons<<" "<<endl;
		iaddons.close();
		
		//Also load the installed_addons file.
		iaddonFile.open((getUserPath()+"installed_addons").c_str());
		if(!iaddonFile) {
			error="ERROR: Unable to create the installed_addons file.";
			cerr<<error<<endl;
			return false;
		}
	}
	
	//And parse the installed_addons file.
	TreeStorageNode obj1;
	{
		POASerializer objSerializer;
		if(!objSerializer.ReadNode(iaddonFile,&obj1,true)){
			error="ERROR: Invalid file format of the installed_addons!";
			cerr<<error<<endl;
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

std::vector<std::string> Addons::addonsToList(const std::string &type){
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

bool Addons::saveInstalledAddons(){
	//Open the file.
	ofstream iaddons;
	iaddons.open((getUserPath()+"installed_addons").c_str());
	if(!iaddons) return false;
	
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
	
	return true;
}

void Addons::handleEvents(){
	//Check if we should quit.
	if(event.type==SDL_QUIT){
		//Save the installed addons before exiting.
		saveInstalledAddons();
		setNextState(STATE_EXIT);
	}

	//Check if escape is pressed, if so return to the levelselect screen.
	if(event.type==SDL_KEYUP && event.key.keysym.sym==SDLK_ESCAPE){
		setNextState(STATE_LEVEL_SELECT);
	}
}

void Addons::logic(){}

void Addons::render(){
	//We only need to draw the background.
	applySurface(0,0,background,screen,NULL);
}

void Addons::GUIEventCallback_OnEvent(std::string name,GUIObject* obj,int eventType){
	if(name=="cmdLvlpacks"){
		list->Item=addonsToList("levelpacks");
		list->Value=0;
		type="levelpacks";
		GUIEventCallback_OnEvent("lstAddons",list,GUIEventChange);
	}else if(name=="cmdLvls"){
		list->Item=addonsToList("levels");
		list->Value=0;
		type="levels";
		GUIEventCallback_OnEvent("lstAddons",list,GUIEventChange);
	}else if(name=="cmdThemes"){
		list->Item=addonsToList("themes");
		list->Value=0;
		type="themes";
		GUIEventCallback_OnEvent("lstAddons",list,GUIEventChange);
	}else if(name=="lstAddons"){
		//Get the addon struct that belongs to it.
		Addon *addon=NULL;
		if(list->Item.size()>0) {
			string entry = list->Item[list->Value];
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
		}
		
		selected=addon;
		updateActionButton();
	}else if(name=="cmdBack"){
		saveInstalledAddons();
		setNextState(STATE_LEVEL_SELECT);
	}else if(name=="cmdInstall"){
		switch(action) {
		  case NONE:
		    break;
		  case INSTALL:
			//Download the addon.
			if(type.compare("levels")==0) {
				if(downloadFile(selected->file,processFileName("%USER%/levels/"))!=false){
					selected->upToDate=true;
					selected->installed=true;
					selected->installedVersion=selected->version;
					list->Item=addonsToList("levels");
					updateActionButton();
				}else{
					cerr<<"ERROR: Unable to download addon!"<<endl;
					msgBox("ERROR: Unable to download addon!",MsgBoxOKOnly,"ERROR:");
					return;
				}
			}else if(type.compare("levelpacks")==0) {
				if(downloadFile(selected->file,processFileName("%USER%/tmp/"))!=false){
					extractFile(processFileName("%USER%/tmp/"+fileNameFromPath(selected->file)),processFileName("%USER%/levelpacks/"+selected->folder+"/"));
					selected->upToDate=true;
					selected->installed=true;
					selected->installedVersion=selected->version;
					list->Item=addonsToList("levelpacks");
					updateActionButton();
				}else{
					cerr<<"ERROR: Unable to download addon!"<<endl;
					msgBox("ERROR: Unable to download addon!",MsgBoxOKOnly,"ERROR:");
					return;
				}
			}else if(type.compare("themes")==0) {
				if(downloadFile(selected->file,processFileName("%USER%/tmp/"))!=false){
					extractFile(processFileName("%USER%/tmp/"+fileNameFromPath(selected->file)),processFileName("%USER%/themes/"+selected->folder+"/"));
					selected->upToDate=true;
					selected->installed=true;
					selected->installedVersion=selected->version;
					list->Item=addonsToList("themes");
					updateActionButton();
				}else{
					cerr<<"ERROR: Unable to download addon!"<<endl;
					msgBox("ERROR: Unable to download addon!",MsgBoxOKOnly,"ERROR:");
					return;
				}
			}
		    break;
		  case UNINSTALL:
			//Uninstall the addon.
			if(type.compare("levels")==0) {
				if(remove((getUserPath() + "levels/" + fileNameFromPath(selected->file)).c_str())){
					cerr<<"ERROR: Unable to remove the file "<<(getUserPath() + "levels/" + fileNameFromPath(selected->file))<<"."<<endl;
					return;
				}
				
				selected->upToDate=false;
				selected->installed=false;
				list->Item=addonsToList("levels");
				updateActionButton();
			}else if(type.compare("levelpacks")==0) {
				if(removeDirectory((getUserPath() + "levelpacks/" + selected->folder+"/").c_str())){
					cerr<<"ERROR: Unable to remove the directory "<<processFileName(getUserPath() + "levelpacks/" + selected->folder+"/")<<"."<<endl;
					return;
				}
				  
				selected->upToDate=false;
				selected->installed=false;
				list->Item=addonsToList("levelpacks");
				updateActionButton();
			}else if(type.compare("themes")==0) {
				if(removeDirectory((getUserPath() + "themes/" + selected->folder+"/").c_str())){
					cerr<<"ERROR: Unable to remove the directory "<<processFileName(getUserPath() + "themes/" + selected->folder+"/")<<"."<<endl;
					return;
				}
				  
				selected->upToDate=false;
				selected->installed=false;
				list->Item=addonsToList("themes");
				updateActionButton();
			}
		    break;
		  case UPDATE:
			//First remove the addon and then install it again.
			if(type.compare("levels")==0) {	
				if(downloadFile(selected->file,(getUserPath()+"/levels/"))!=false){
					selected->upToDate=true;
					selected->installedVersion=selected->version;
					list->Item=addonsToList("levels");
					updateActionButton();
				}else{
					cerr<<"ERROR: Unable to download addon!"<<endl;
					msgBox("ERROR: Unable to download addon!",MsgBoxOKOnly,"ERROR:");
					return;
				}
			}else if(type.compare("levelpacks")==0) {
				if(removeDirectory((getUserPath() + "levelpacks/" + selected->folder+"/").c_str())){
					cerr<<"ERROR: Unable to remove the directory "<<(getUserPath() + "levelpacks/" + selected->folder+"/")<<"."<<endl;
					return;
				}
				
				if(downloadFile(selected->file,(getUserPath()+"%USER%/tmp/"))!=false){
					extractFile((getUserPath()+"/tmp/"+fileNameFromPath(selected->file,true)),(getUserPath()+"/levelpacks/"+selected->folder+"/"));
					selected->upToDate=true;
					selected->installedVersion=selected->version;
					list->Item=addonsToList("levelpacks");
					updateActionButton();
				}else{
					cerr<<"ERROR: Unable to download addon!"<<endl;
					msgBox("ERROR: Unable to download addon!",MsgBoxOKOnly,"ERROR:");
					return;
				}
			}else if(type.compare("themes")==0) {
				if(removeDirectory((getUserPath() + "themes/" + selected->folder+"/").c_str())){
					cerr<<"ERROR: Unable to remove the directory "<<(getUserPath() + "themes/" + selected->folder+"/")<<"."<<endl;
					return;
				}
				
				if(downloadFile(selected->file,(getUserPath()+"/tmp/"))!=false){
					extractFile((getUserPath()+"/tmp/"+fileNameFromPath(selected->file,true)),(getUserPath()+"/themes/"+selected->folder+"/"));
					selected->upToDate=true;
					selected->installedVersion=selected->version;
					list->Item=addonsToList("themes");
					updateActionButton();
				}else{
					cerr<<"ERROR: Unable to download addon!"<<endl;
					msgBox("ERROR: Unable to download addon!",MsgBoxOKOnly,"ERROR:");
					return;
				}
			}
		    break;
		}
	}
}

void Addons::updateActionButton(){
	//some sanity check
	if(selected==NULL){
		actionButton->Enabled=false;
		action = NONE;
		return;
	}

	//Check if the selected addon is installed.
	if(selected->installed){
		//It is installed, but is it uptodate?
		if(selected->upToDate){
			//The addon is installed and uptodate so we can only uninstall it.
			actionButton->Enabled=true;
			actionButton->Caption="Uninstall";
			action = UNINSTALL;
		}else{
			//TODO: With this configuration a not uptodate addons can't be uninstalled without updating.
			//The addon is installed but not uptodate so we can only update it.
			actionButton->Enabled=true;
			actionButton->Caption="Update";
			action = UPDATE;
		}
	}else{
		//The addon isn't installed so we can only install it.
		actionButton->Enabled=true;
		actionButton->Caption="Install";
		action = INSTALL;
	}
}
