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
#include "Classes.h"
#include "Functions.h"
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
#include <curl/curl.h>
using namespace std;

/////////////////////ADDONS/////////////////////
static GUIScrollBar *m_oLvScrollBar=NULL;

Addons::Addons()
{
	s_background = load_image(GetDataPath()+"data/gfx/menu/addons.png");
	FILE *addon=fopen((GetUserPath() + "addons").c_str(), "wb");	
	curl = curl_easy_init();
	get_addons_list(addon);
	
	action = NONE;
	
	//create GUI (test only)
	GUIObject* obj;
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
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
	list->Item=addons_to_list(levels);
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

Addons::~Addons()
{
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}
	m_oLvScrollBar=NULL;
}

void Addons::get_addons_list(FILE *file)
{
	//Download the addons file.
	curl_easy_setopt(curl,CURLOPT_URL,"127.0.0.1/game/addons");
	curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,write_data);
	curl_easy_setopt(curl,CURLOPT_WRITEDATA,file);
	curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	fclose(file);
	
	//Load the file.
	ifstream addon_file;
	addon_file.open((GetUserPath()+"addons").c_str());
	
	if(!addon_file) {
		cerr<<"Error: unable to load addon_list file!"<<endl;
		return;
	}
	
	TreeStorageNode obj;
	{
		POASerializer objSerializer;
		if(!objSerializer.ReadNode(addon_file,&obj,true)){
			cerr<<"Error: Invalid file format of addon_list!"<<endl;
			return;
		}
	}
	
	//Also load the installed_addons file.
	ifstream iaddon_file;
	iaddon_file.open((GetUserPath()+"installed_addons").c_str());
	
	if(!iaddon_file) {
		cerr<<"Error: unable to load the installed_addons file!"<<endl;
		return;
	}
	
	TreeStorageNode obj1;
	{
		POASerializer objSerializer;
		if(!objSerializer.ReadNode(iaddon_file,&obj1,true)){
			cerr<<"Error: Invalid file format of the installed_addons!"<<endl;
			return;
		}
	}
	
	
	//Fill the vectors.
	levels = new std::vector<Addon>;
	fill_addon_list(*levels,"levels",obj,obj1);
	themes = new std::vector<Addon>;
	fill_addon_list(*themes,"themes",obj,obj1);
	levelpacks = new std::vector<Addon>;
	fill_addon_list(*levelpacks,"levelpacks",obj,obj1);
}

void Addons::fill_addon_list(std::vector<Addons::Addon> &list, string type, TreeStorageNode &addons, TreeStorageNode &installed_addons)
{
	for(unsigned int i=0;i<addons.SubNodes.size();i++){
		TreeStorageNode* block=addons.SubNodes[i];
		if(block==NULL) continue;
		if(block->Name==type){
			for(unsigned int i=0;i<block->SubNodes.size();i++){
				TreeStorageNode* entry=block->SubNodes[i];
				if(entry==NULL) continue;
				if(entry->Name=="entry" && entry->Value.size()==1){
					Addon addon = *(new Addon);
					addon.type=type;
					addon.name=entry->Value[0];
					addon.file=entry->Attributes["file"][0];
					addon.author=entry->Attributes["author"][0];
					addon.version=atoi(entry->Attributes["version"][0].c_str());
					addon.uptodate=false;
					addon.installed=false;
					
					//Check if the addon is already installed.
					for(unsigned int i=0;i<installed_addons.SubNodes.size();i++){
						TreeStorageNode* installed=installed_addons.SubNodes[i];
						if(installed==NULL) continue;
						if(installed->Name=="entry" && installed->Value.size()==2){
							if(addon.name.compare(installed->Value[0])==0) {
								addon.installed=true;
								if(atoi(installed->Value[1].c_str())>=addon.version) {
									addon.uptodate=true;
								}
							}
						}
					}
					
					list.push_back(addon);
				}
			}
		}
	}
}

size_t Addons::write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
  return fwrite(ptr, size, nmemb, (FILE *)stream);
}

std::vector<std::string> Addons::addons_to_list(std::vector<Addon> *addons) {
	std::vector<std::string> result;
	
	for(int i=0;i<addons->size();i++) {
		string entry = (*addons)[i].name + " by " + (*addons)[i].author;
		if((*addons)[i].installed) {
			if((*addons)[i].uptodate) {
				entry += " *";
			} else {
				entry += " +";
			}
		}
		result.push_back(entry);
	}
	return result;
}

void Addons::download_file(const string &path) {
	string filename = FileNameFromPath(path);
	
	FILE *file = fopen((GetUserPath() + filename).c_str(), "wb");
	//delete curl;
	curl = curl_easy_init();
	curl_easy_setopt(curl,CURLOPT_URL,path.c_str());
	curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,write_data);
	curl_easy_setopt(curl,CURLOPT_WRITEDATA,file);
	curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	fclose(file);
}

void Addons::handle_events()
{
	if ( event.type == SDL_QUIT )
	{
		next_state(STATE_EXIT);
	}

	if ( event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT )
	{
		check_mouse();
	}

	if ( event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_ESCAPE )
	{
		next_state(STATE_MENU);
	}

	if ( event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_s )
	{
		if ( Mix_PlayingMusic() == 1 )
		{
			Mix_HaltMusic();
		}

		else 
		{
			Mix_PlayMusic(music,-1);
		}				
	}
	else if ( event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_WHEELDOWN && m_oLvScrollBar)
	{
		if(m_oLvScrollBar->Value<m_oLvScrollBar->Max) m_oLvScrollBar->Value++;
		return;
	}

	else if ( event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_WHEELUP && m_oLvScrollBar)
	{
		if(m_oLvScrollBar->Value>0) m_oLvScrollBar->Value--;
		return;
	}
}

void Addons::check_mouse()
{
	int x,y,dy=0,m=o_mylevels.get_level_count();

	SDL_GetMouseState(&x,&y);

	if(m_oLvScrollBar) dy=m_oLvScrollBar->Value;
	if(m>dy*10+50) m=dy*10+50;
	y+=dy*80;

}


void Addons::logic() {}

void Addons::render()
{
	int x,y,dy=0,m=o_mylevels.get_level_count();

	SDL_GetMouseState(&x,&y);

	if(m_oLvScrollBar) dy=m_oLvScrollBar->Value;
	if(m>dy*10+50) m=dy*10+50;
	y+=dy*80;

	apply_surface( 0 , 0, s_background, screen, NULL );
}

void Addons::GUIEventCallback_OnEvent(std::string Name,GUIObject* obj,int nEventType){
	string s;
	if(Name=="cmdLvlpacks"){
		list->Item=addons_to_list(levelpacks);
		list->Value=0;
		type="levelpacks";
		GUIEventCallback_OnEvent("lstAddons",list,GUIEventChange);
	}else if(Name=="cmdLvls"){
		list->Item=addons_to_list(levels);
		list->Value=0;
		type="levels";
		GUIEventCallback_OnEvent("lstAddons",list,GUIEventChange);
	}else if(Name=="cmdThemes"){
		list->Item=addons_to_list(themes);
		list->Value=0;
		type="themes";
		GUIEventCallback_OnEvent("lstAddons",list,GUIEventChange);
	}else if(Name=="lstAddons"){
		string entry = list->Item[list->Value];
		//Get the addon struct that belongs to it.
		Addon *addon;
		if(type.compare("levels")==0) {
			for(int i=0;i<levels->size();i++) {
				std::string prefix=(*levels)[i].name;
				if(!entry.compare(0, prefix.size(), prefix)) {
					addon=&(*levels)[i];
				}
			}
		} else if(type.compare("levelpacks")==0) {
			for(int i=0;i<levelpacks->size();i++) {
				std::string prefix=(*levelpacks)[i].name;
				if(!entry.compare(0, prefix.size(), prefix)) {
					addon=&(*levelpacks)[i];
				}
			} 
		} else if(type.compare("themes")==0) {
			for(int i=0;i<themes->size();i++) {
				std::string prefix=(*themes)[i].name;
				if(!entry.compare(0, prefix.size(), prefix)) {
					addon=&(*themes)[i];
				}
			}
		}
		
		selected=addon;
		update_actionButton();
	}else if(Name=="cmdBack"){
		next_state(STATE_MENU);
	}else if(Name=="cmdInstall"){
		switch(action) {
		  case NONE:
		    break;
		  case INSTALL:
			//Download the addon.
			if(type.compare("levels")==0) {
				download_file(selected->file);
				selected->uptodate=true;
				selected->installed=true;
				list->Item=addons_to_list(levels);
				update_actionButton();
			}
		    break;
		  case UNINSTALL:
			//Uninstall the addon.
			if(type.compare("levels")==0) {
				if(remove((GetUserPath() + FileNameFromPath(selected->file)).c_str())) {
					
				}
				  
				selected->uptodate=false;
				selected->installed=false;
				list->Item=addons_to_list(levels);
				update_actionButton();
			}

		    break;
		  case UPDATE:
			//Download the addon.
			if(type.compare("levels")==0) {
				download_file(selected->file);
				selected->uptodate=true;
				list->Item=addons_to_list(levels);
				update_actionButton();
			}
		    break;
		}
	} else {
		return;
	}
}

void Addons::update_actionButton() {
	if(selected->installed) {
		if(selected->uptodate) {
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
