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

#include <stdio.h>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_mixer.h>
#include <string>
#include "Globals.h"
#include "Functions.h"
#include "Objects.h"
#include "Player.h"
#include "GameObjects.h"
#include "Levels.h"
#include "TitleMenu.h"
#include "LevelEditor.h"
#include "Game.h"
#include "LevelSelect.h"
#include "Addons.h"
#include "ImageManager.h"
#include "ThemeManager.h"
#include "GUIListBox.h"
#include <archive.h>
#include <archive_entry.h>
using namespace std;

#ifdef WIN32
#include <windows.h>
#include <shlobj.h>
#else
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#endif


string userPath,dataPath,appPath,exeName,pathPrefix;

bool configurePaths() {
	//get the app path
	{
		char s[4096];
		int i,m;
		#ifdef WIN32
		m=GetModuleFileNameA(NULL,s,sizeof(s));
		#else
		m=readlink("/proc/self/exe",s,sizeof(s));
		#endif
		s[m]=0;
		for(i=m-1;i>=0;i--){
			if(s[i]=='/'||s[i]=='\\'){
				s[i]=0;
				break;
			}
		}
		appPath=s;
		exeName=s+i+1;
	}
	//get the user path
	if(getUserPath().empty()){
#ifdef WIN32
		char s[1024];
		SHGetSpecialFolderPathA(NULL,s,CSIDL_PERSONAL,1);
		userPath=s;
		userPath+="\\My Games\\meandmyshadow\\";
		SHCreateDirectoryExA(NULL,userPath.c_str(),NULL);
		SHCreateDirectoryExA(NULL,(userPath+"levels").c_str(),NULL);
		SHCreateDirectoryExA(NULL,(userPath+"levelpacks").c_str(),NULL);
		SHCreateDirectoryExA(NULL,(userPath+"themes").c_str(),NULL);
		SHCreateDirectoryExA(NULL,(userPath+"progress").c_str(),NULL);
		SHCreateDirectoryExA(NULL,(userPath+"tmp").c_str(),NULL);
#else
		userPath=getenv("HOME");
		userPath+="/.meandmyshadow/";
		mkdir(userPath.c_str(),0777);
		//Also create other folders in the userpath.
		mkdir((userPath+"/levels").c_str(),0777);
		mkdir((userPath+"/levelpacks").c_str(),0777);
		mkdir((userPath+"/themes").c_str(),0777);
		mkdir((userPath+"/progress").c_str(),0777);
		mkdir((userPath+"/tmp").c_str(),0777);
#endif
	}
	//get the data path
	{
		FILE *f;
		string s;
		for(;;){
			//try existing one
			if(!dataPath.empty()){
				s=dataPath+"font/ComicBook.ttf";
				if((f=fopen(s.c_str(),"rb"))!=NULL){
					fclose(f);
					break;
				}
			}
			//try "./"
			dataPath="./data/";
			s=dataPath+"font/ComicBook.ttf";
			if((f=fopen(s.c_str(),"rb"))!=NULL){
				fclose(f);
				break;
			}
			//try "../"
			dataPath="../data/";
			s=dataPath+"font/ComicBook.ttf";
			if((f=fopen(s.c_str(),"rb"))!=NULL){
				fclose(f);
				break;
			}
			//try App.Path
			dataPath=getAppPath()+"/data/";
			s=dataPath+"font/ComicBook.ttf";
			if((f=fopen(s.c_str(),"rb"))!=NULL){
				fclose(f);
				break;
			}
			//try App.Path+"/../"
			dataPath=getAppPath()+"/../data/";
			s=dataPath+"font/ComicBook.ttf";
			if((f=fopen(s.c_str(),"rb"))!=NULL){
				fclose(f);
				break;
			}
			//try DATA_PATH
#ifdef DATA_PATH
			dataPath=DATA_PATH;
			s=dataPath+"font/ComicBook.ttf";
			if((f=fopen(s.c_str(),"rb"))!=NULL){
				fclose(f);
				break;
			}
#endif
			//error: can't find file
			return false;
		}
		font=TTF_OpenFont(s.c_str(),28);
		fontSmall=TTF_OpenFont(s.c_str(),20);
	}
	return true;
}

std::vector<std::string> EnumAllFiles(std::string sPath,const char* sExtension){
	vector<string> v;
#ifdef WIN32
	string s1;
	WIN32_FIND_DATAA f;
	if(!sPath.empty()){
		char c=sPath[sPath.size()-1];
		if(c!='/'&&c!='\\') sPath+="\\";
	}
	s1=sPath;
	if(sExtension!=NULL && *sExtension){
		s1+="*.";
		s1+=sExtension;
	}else{
		s1+="*";
	}
	HANDLE h=FindFirstFileA(s1.c_str(),&f);
	if(h==NULL||h==INVALID_HANDLE_VALUE) return v;
	do{
		if(!(f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)){
			v.push_back(/*sPath+*/f.cFileName);
		}
	}while(FindNextFileA(h,&f));
	FindClose(h);
	return v;
#else
	int len=0;
	if(sExtension!=NULL && *sExtension) len=strlen(sExtension);
	if(!sPath.empty()){
		char c=sPath[sPath.size()-1];
		if(c!='/'&&c!='\\') sPath+="/";
	}
	DIR *pDir;
	struct dirent *pDirent;
	pDir=opendir(sPath.c_str());
	if(pDir==NULL) return v;
	while((pDirent=readdir(pDir))!=NULL){
		if(pDirent->d_name[0]=='.'){
			if(pDirent->d_name[1]==0||
				(pDirent->d_name[1]=='.'&&pDirent->d_name[2]==0)) continue;
		}
		string s1=sPath+pDirent->d_name;
		struct stat S_stat;
		lstat(s1.c_str(),&S_stat);
		if(!S_ISDIR(S_stat.st_mode)){
			if(len>0){
				if((int)s1.size()<len+1) continue;
				if(s1[s1.size()-len-1]!='.') continue;
				if(strcasecmp(&s1[s1.size()-len],sExtension)) continue;
			}
			v.push_back(/*s1*/string(pDirent->d_name));
		}
	}
	closedir(pDir);
	return v;
#endif
}

std::vector<std::string> EnumAllDirs(std::string sPath){
	vector<string> v;
#ifdef WIN32
	string s1;
	WIN32_FIND_DATAA f;
	if(!sPath.empty()){
		char c=sPath[sPath.size()-1];
		if(c!='/'&&c!='\\') sPath+="\\";
	}
	s1=sPath;
	HANDLE h=FindFirstFileA(s1.c_str(),&f);
	if(h==NULL||h==INVALID_HANDLE_VALUE) return v;
	do{
		if(!(f.dwDirAttributes & FILE_ATTRIBUTE_DIRECTORY)){
			v.push_back(/*sPath+*/f.cFileName);
		}
	}while(FindNextFileA(h,&f));
	FindClose(h);
	return v;
#else
	if(!sPath.empty()){
		char c=sPath[sPath.size()-1];
		if(c!='/'&&c!='\\') sPath+="/";
	}
	DIR *pDir;
	struct dirent *pDirent;
	pDir=opendir(sPath.c_str());
	if(pDir==NULL) return v;
	while((pDirent=readdir(pDir))!=NULL){
		if(pDirent->d_name[0]=='.'){
			if(pDirent->d_name[1]==0||
				(pDirent->d_name[1]=='.'&&pDirent->d_name[2]==0)) continue;
		}
		string s1=sPath+pDirent->d_name;
		struct stat S_stat;
		lstat(s1.c_str(),&S_stat);
		if(S_ISDIR(S_stat.st_mode)){
			//Skip hidden folders.
			s1=string(pDirent->d_name);
			if(s1.find('.')==0) continue;
			
			//Add result to vector.
			v.push_back(s1);
		}
	}
	closedir(pDir);
	return v;
#endif
}

void setPathPrefix(std::string prefix){
      pathPrefix=prefix;
}

std::string processFileName(const std::string& s){
	string prefix=pathPrefix;
	if(prefix.empty()) prefix=dataPath;
  
	if(s.compare(0,6,"%DATA%")==0){
		if(s.size()>6 && (s[6]=='/' || s[6]=='\\')){
			return dataPath+s.substr(7);
		}else{
			return dataPath+s.substr(6);
		}
	}else if(s.compare(0,6,"%USER%")==0){
		if(s.size()>6 && (s[6]=='/' || s[6]=='\\')){
			return userPath+s.substr(7);
		}else{
			return userPath+s.substr(6);
		}
	}else if(s.compare(0,9,"%LVLPACK%")==0){
		if(s.size()>9 && (s[9]=='/' || s[9]=='\\')){
			return prefix+"levelpacks/"+s.substr(10);
		}else{
			return prefix+"levelpacks/"+s.substr(9);
		}
	}else if(s.compare(0,5,"%LVL%")==0){
		if(s.size()>5 && (s[5]=='/' || s[5]=='\\')){
			return prefix+"levels/"+s.substr(6);
		}else{
			return prefix+"levels/"+s.substr(5);
		}
	}else if(s.compare(0,8,"%THEMES%")==0){
		if(s.size()>8 && (s[8]=='/' || s[8]=='\\')){
			return prefix+"themes/"+s.substr(9);
		}else{
			return prefix+"themes/"+s.substr(8);
		}
	}else if(s.size()>0 && (s[0]=='/' || s[0]=='\\')){
		return s;
	}else{
		return prefix+s;
	}
}

std::string fileNameFromPath(const std::string &path){
	std::string filename;
#ifdef WIN32
	size_t pos = path.find_last_of("\\");
#else
	size_t pos = path.find_last_of("\/");
#endif
	if(pos != std::string::npos)
		filename.assign(path.begin() + pos + 1, path.end());
	else
		filename = path;
	
	return filename;
}

bool extractFile(const string &fileName, const string &destination) {
	//Create the archive we're going to extract.
	archive *file;
	//Create the destination we're going to extract to.
	archive *dest;
	
	file = archive_read_new();
	dest = archive_write_disk_new();
	archive_write_disk_set_options(dest, ARCHIVE_EXTRACT_TIME);
	
	archive_read_support_format_zip(file);
	
	//Now read the archive.
	if(archive_read_open_file(file,fileName.c_str(),10240)) {
		cerr<<"Error while reading archive "+fileName<<endl;
		return false;
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
			cerr<<"Error while reading archive "+fileName<<endl;
			return false;
		}
		archive_entry_set_pathname(entry,(destination+archive_entry_pathname(entry)).c_str());
		
		status=archive_write_header(dest,entry);
		if(status!=ARCHIVE_OK){
			cerr<<"Error while extracting archive "+fileName<<endl;
			return false;
		}else{
			copyData(file, dest);
			status=archive_write_finish_entry(dest);
			if(status!=ARCHIVE_OK){
				cerr<<"Error while extracting archive "+fileName<<endl;
				return false;
			}

		}
	}
	
	//Finally close the archive.
	archive_read_close(file);
	archive_read_finish(file);
	return true;
}

bool removeDirectory(const char *path){
	//Fi
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


void copyData(archive *file, archive *dest) {
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