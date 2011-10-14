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
#include <archive.h>
#include <archive_entry.h>
#include <dirent.h>
using namespace std;

/////////////////////ADDONS/////////////////////
static GUIScrollBar *m_oLvScrollBar=NULL;

Addons::Addons(){
	s_background = load_image(get_data_path()+"gfx/menu/addons.png");
	FILE *addon=fopen((get_user_path() + "addons").c_str(), "wb");	
	curl = curl_easy_init();
	action = NONE;

	//create GUI (test only)
	GUIObject* obj;
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}

	if(!get_addons_list(addon)) {
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
	list->Item=addons_to_list("levels");
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

bool Addons::get_addons_list(FILE *file)
{
	//Download the addons file.
	curl_easy_setopt(curl,CURLOPT_URL,"192.168.2.250/game/addons");
	curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,write_data);
	curl_easy_setopt(curl,CURLOPT_WRITEDATA,file);
	curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	fclose(file);
	
	//Load the file.
	ifstream addon_file;
	addon_file.open((get_user_path()+"addons").c_str());
	
	if(!addon_file) {
		error="Error: unable to load addon_list file!";
		cerr<<error<<endl;
		return false;
	}
	
	TreeStorageNode obj;
	{
		POASerializer objSerializer;
		if(!objSerializer.ReadNode(addon_file,&obj,true)){
			error="Error: Invalid file format of addon_list!";
			cerr<<error<<endl;
			return false;
		}
	}
	
	//Also load the installed_addons file.
	ifstream iaddon_file;
	iaddon_file.open((get_user_path()+"installed_addons").c_str());
	
	if(!iaddon_file) {
		//The installed_addons file doesn't exist, so we create it.
		ofstream iaddons;
		iaddons.open((get_user_path()+"installed_addons").c_str());
		iaddons.close();
		
		//Also load the installed_addons file.
		iaddon_file.open((get_user_path()+"installed_addons").c_str());
		if(!iaddon_file) {
			error="Unable to create the installed_addons file.";
			cerr<<error<<endl;
			return false;
		}
	}
	
	TreeStorageNode obj1;
	{
		POASerializer objSerializer;
		if(!objSerializer.ReadNode(iaddon_file,&obj1,true)){
			error="Error: Invalid file format of the installed_addons!";
			cerr<<error<<endl;
			return false;
		}
	}
	
	
	//Fill the vectors.
	addons = new std::vector<Addon>;
	fill_addon_list(*addons,obj,obj1);
		
	//Close the files.
	iaddon_file.close();
	addon_file.close();
	return true;
}

void Addons::fill_addon_list(std::vector<Addons::Addon> &list, TreeStorageNode &addons, TreeStorageNode &installed_addons)
{
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
				addon.uptodate=false;
				addon.installed=false;
				
				//Check if the addon is already installed.
				for(unsigned int i=0;i<installed_addons.subNodes.size();i++){
					TreeStorageNode* installed=installed_addons.subNodes[i];
					if(installed==NULL) continue;
					if(installed->name=="entry" && installed->value.size()==3){
						if(addon.type.compare(installed->value[0])==0 && addon.name.compare(installed->value[1])==0) {
							addon.installed=true;
							addon.installed_version=atoi(installed->value[2].c_str());
							if(addon.installed_version>=addon.version) {
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

size_t Addons::write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
  return fwrite(ptr, size, nmemb, (FILE *)stream);
}

std::vector<std::string> Addons::addons_to_list(const std::string &type) {
	std::vector<std::string> result;
	
	for(int i=0;i<addons->size();i++) {
		//Check if the addon is from the right type.
		if((*addons)[i].type==type) {
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
	}
	return result;
}

void Addons::download_file(const string &path, const string &destination) {
	string filename = FileNameFromPath(path);
	
	FILE *file = fopen(ProcessFileName(destination+filename).c_str(), "wb");
	//delete curl;
	curl = curl_easy_init();
	curl_easy_setopt(curl,CURLOPT_URL,path.c_str());
	curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,write_data);
	curl_easy_setopt(curl,CURLOPT_WRITEDATA,file);
	curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	fclose(file);
}

void Addons::extract_file(const string &path, const string &destination) {
	//Create the archive we're going to extract.
	archive *file;
	//Create the destination we're going to extract to.
	archive *dest;
	
	file = archive_read_new();
	dest = archive_write_disk_new();
	archive_write_disk_set_options(dest, ARCHIVE_EXTRACT_TIME);
	
	archive_read_support_format_zip(file);
	
	//Now read the archive.
	if(archive_read_open_file(file, ProcessFileName(path).c_str(), 10240)) {
		cerr<<"Error while reading archive "+path<<endl;
	}
	
	//Now write every entry to disk.
	int status;
	archive_entry *entry;
	while(true) {
		status=archive_read_next_header(file,&entry);
		if(status==ARCHIVE_EOF){
			break;
		}
		if(status!=ARCHIVE_OK){
			cerr<<"Error while reading archive "+path<<endl;
		}
		archive_entry_set_pathname(entry,(ProcessFileName(destination) + archive_entry_pathname(entry)).c_str());
		
		status=archive_write_header(dest,entry);
		if(status!=ARCHIVE_OK){
			cerr<<"Error while extracting archive "+path<<endl;
		}else{
			copyData(file, dest);
			status=archive_write_finish_entry(dest);
			if(status!=ARCHIVE_OK){
				cerr<<"Error while extracting archive "+path<<endl;
			}

		}
	}
	archive_read_close(file);
	archive_read_finish(file);
}

int Addons::removeDirectory(const char *path)
{
	DIR *d = opendir(path);
	size_t path_len = strlen(path);
	int r = -1;

	if(d) {
		struct dirent *p;
		r = 0;

		while(!r && (p=readdir(d))) {
			int r2 = -1;
			char *buf;
			size_t len;

			/* Skip the names "." and ".." as we don't want to recurse on them. */
			if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, "..")) {
				continue;
			}

			len = path_len + strlen(p->d_name) + 2; 
			buf = (char*) malloc(len);

			if(buf) {
				struct stat statbuf;
				snprintf(buf, len, "%s/%s", path, p->d_name);

				if(!stat(buf, &statbuf)){
					if (S_ISDIR(statbuf.st_mode)){
						r2 = removeDirectory(buf);
					}else{
						r2 = unlink(buf);
					}
				}
				free(buf);
			}
			r = r2;
		}
		closedir(d);
	}
	
	if (!r){
		r = rmdir(path);
	}
	
	return r;
}


void Addons::copyData(archive *file, archive *dest) {
	int status;
	const void *buff;
	size_t size;
	off_t offset;

	while(true) {
		status=archive_read_data_block(file, &buff, &size, &offset);
		if(status==ARCHIVE_EOF){
			return;
		}
		if(status!=ARCHIVE_OK){
			cerr<<"Error while writing data to disk."<<endl;
			return;
		}
		status=archive_write_data_block(dest, buff, size, offset);
		if(status!=ARCHIVE_OK) {
			cerr<<"Error while writing data to disk."<<endl;
			return;
		}
	}
}

void Addons::saveInstalledAddons() {
	//Open the file.
	ofstream iaddons;
	iaddons.open((get_user_path()+"installed_addons").c_str());
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
			sprintf(version,"%d",(*addons)[i].installed_version);
			entry->value.push_back(version);
			
			installed.subNodes.push_back(entry);
		}
	}
	
	
	//And write away the file.
	POASerializer objSerializer;
	objSerializer.WriteNode(&installed,iaddons,true,true);
}

void Addons::handle_events()
{
	if ( event.type == SDL_QUIT )
	{
		saveInstalledAddons();
		next_state(STATE_EXIT);
	}

	if ( event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT )
	{
		check_mouse();
	}

	if ( event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_ESCAPE )
	{
		next_state(STATE_LEVEL_SELECT);
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
		list->Item=addons_to_list("levelpacks");
		list->Value=0;
		type="levelpacks";
		GUIEventCallback_OnEvent("lstAddons",list,GUIEventChange);
	}else if(Name=="cmdLvls"){
		list->Item=addons_to_list("levels");
		list->Value=0;
		type="levels";
		GUIEventCallback_OnEvent("lstAddons",list,GUIEventChange);
	}else if(Name=="cmdThemes"){
		list->Item=addons_to_list("themes");
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
		update_actionButton();
	}else if(Name=="cmdBack"){
		saveInstalledAddons();
		next_state(STATE_LEVEL_SELECT);
	}else if(Name=="cmdInstall"){
		switch(action) {
		  case NONE:
		    break;
		  case INSTALL:
			//Download the addon.
			if(type.compare("levels")==0) {
				download_file(selected->file,"%USER%/levels/");
				selected->uptodate=true;
				selected->installed=true;
				selected->installed_version=selected->version;
				list->Item=addons_to_list("levels");
				update_actionButton();
			}else if(type.compare("levelpacks")==0) {
				download_file(selected->file,"%USER%/tmp/");
				extract_file("%USER%/tmp/"+FileNameFromPath(selected->file),"%USER%/levelpacks/"+selected->name+"/");
				selected->uptodate=true;
				selected->installed=true;
				selected->installed_version=selected->version;
				list->Item=addons_to_list("levelpacks");
				update_actionButton();
			}else if(type.compare("themes")==0) {
				download_file(selected->file,"%USER%/tmp/");
				extract_file("%USER%/tmp/"+FileNameFromPath(selected->file),"%USER%/themes/"+selected->name+"/");
				selected->uptodate=true;
				selected->installed=true;
				selected->installed_version=selected->version;
				list->Item=addons_to_list("themes");
				update_actionButton();
			}
		    break;
		  case UNINSTALL:
			//Uninstall the addon.
			if(type.compare("levels")==0) {
				if(remove((get_user_path() + "levels/" + FileNameFromPath(selected->file)).c_str())) {
					//TODO error handling.
				}
				  
				selected->uptodate=false;
				selected->installed=false;
				list->Item=addons_to_list("levels");
				update_actionButton();
			}else if(type.compare("levelpacks")==0) {
				if(removeDirectory(ProcessFileName(get_user_path() + "levelpacks/" + selected->name+"/").c_str())) {
					//TODO error handling.
				}
				  
				selected->uptodate=false;
				selected->installed=false;
				list->Item=addons_to_list("levelpacks");
				update_actionButton();
			}else if(type.compare("themes")==0) {
				if(removeDirectory(ProcessFileName(get_user_path() + "themes/" + selected->name+"/").c_str())) {
					//TODO error handling.
				}
				  
				selected->uptodate=false;
				selected->installed=false;
				list->Item=addons_to_list("themes");
				update_actionButton();
			}
		    break;
		  case UPDATE:
			//Download the addon.
			if(type.compare("levels")==0) {	
				download_file(selected->file,"%USER%/levels/");
				selected->uptodate=true;
				selected->installed_version=selected->version;
				list->Item=addons_to_list("levels");
				update_actionButton();
			}else if(type.compare("levelpacks")==0) {
				if(removeDirectory(ProcessFileName(get_user_path() + "levelpacks/" + selected->name).c_str())) {
					//TODO error handling.
				}
				download_file(selected->file,"%USER%/tmp/");
				extract_file("%USER%/tmp/"+FileNameFromPath(selected->file),"%USER%/levelpacks/"+selected->name+"/");
				selected->uptodate=true;
				selected->installed_version=selected->version;
				list->Item=addons_to_list("levelpacks");
				update_actionButton();
			}else if(type.compare("themes")==0) {
				if(removeDirectory(ProcessFileName(get_user_path() + "themes/" + selected->name).c_str())) {
					//TODO error handling.
				}
				download_file(selected->file,"%USER%/tmp/");
				extract_file("%USER%/tmp/"+FileNameFromPath(selected->file),"%USER%/themes/"+selected->name+"/");
				selected->uptodate=true;
				selected->installed_version=selected->version;
				list->Item=addons_to_list("themes");
				update_actionButton();
			}
		    break;
		}
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
