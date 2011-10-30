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
#include <iostream>
#include <string>
#include <vector>
#include "Globals.h"
#include "FileManager.h"
#include "Functions.h"
#include <archive.h>
#include <archive_entry.h>
using namespace std;

#ifdef WIN32
#include <windows.h>
#include <shlobj.h>
#include <direct.h>
#else
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#endif


string userPath,dataPath,appPath,exeName;

bool configurePaths() {
	//Get the appPath and the exeName.
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
	
	//TODO: Check if the userpath is empty before setting userPath???
	//Check if the userPath is empty.
	if(getUserPath().empty()){
#ifdef WIN32
		//Get the userPath.
		char s[1024];
		SHGetSpecialFolderPathA(NULL,s,CSIDL_PERSONAL,1);
		userPath=s;
		userPath+="\\My Games\\meandmyshadow\\";
		
		//Create the userPath folder and other subfolders.
		createDirectory(userPath.c_str());
		createDirectory((userPath+"levels").c_str());
		createDirectory((userPath+"levelpacks").c_str());
		createDirectory((userPath+"themes").c_str());
		createDirectory((userPath+"progress").c_str());
		createDirectory((userPath+"tmp").c_str());
#else
		//Get the userPath.
		userPath=getenv("HOME");
		userPath+="/.meandmyshadow/";
		
		//Create the userPath.
		createDirectory(userPath.c_str());
		//Also create other folders in the userpath.
		createDirectory((userPath+"/levels").c_str());
		createDirectory((userPath+"/levelpacks").c_str());
		createDirectory((userPath+"/themes").c_str());
		createDirectory((userPath+"/progress").c_str());
		createDirectory((userPath+"/tmp").c_str());
#endif
		
		//Print the userPath.
		cout<<"User preferences will be fetched from: "<<userPath<<endl;
	}
	
	//Get the dataPath by trying multiple relative locations.
	{
		FILE *f;
		string s;
		while(true){
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

		//Print the dataPath.
		cout<<"Data files will be fetched from: "<<dataPath<<endl;
	}
	return true;
}

std::vector<std::string> enumAllFiles(std::string path,const char* extension){
	vector<string> v;
#ifdef WIN32
	string s1;
	WIN32_FIND_DATAA f;
	if(!path.empty()){
		char c=path[path.size()-1];
		if(c!='/'&&c!='\\') path+="\\";
	}
	s1=path;
	if(extension!=NULL && *extension){
		s1+="*.";
		s1+=extension;
	}else{
		s1+="*";
	}
	HANDLE h=FindFirstFileA(s1.c_str(),&f);
	if(h==NULL||h==INVALID_HANDLE_VALUE) return v;
	do{
		if(!(f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)){
			v.push_back(/*path+*/f.cFileName);
		}
	}while(FindNextFileA(h,&f));
	FindClose(h);
	return v;
#else
	int len=0;
	if(extension!=NULL && *extension) len=strlen(extension);
	if(!path.empty()){
		char c=path[path.size()-1];
		if(c!='/'&&c!='\\') path+="/";
	}
	DIR *pDir;
	struct dirent *pDirent;
	pDir=opendir(path.c_str());
	if(pDir==NULL) return v;
	while((pDirent=readdir(pDir))!=NULL){
		if(pDirent->d_name[0]=='.'){
			if(pDirent->d_name[1]==0||
				(pDirent->d_name[1]=='.'&&pDirent->d_name[2]==0)) continue;
		}
		string s1=path+pDirent->d_name;
		struct stat S_stat;
		lstat(s1.c_str(),&S_stat);
		if(!S_ISDIR(S_stat.st_mode)){
			if(len>0){
				if((int)s1.size()<len+1) continue;
				if(s1[s1.size()-len-1]!='.') continue;
				if(strcasecmp(&s1[s1.size()-len],extension)) continue;
			}
			v.push_back(/*s1*/string(pDirent->d_name));
		}
	}
	closedir(pDir);
	return v;
#endif
}

std::vector<std::string> enumAllDirs(std::string path){
	vector<string> v;
#ifdef WIN32
	string s1;
	WIN32_FIND_DATAA f;
	if(!path.empty()){
		char c=path[path.size()-1];
		if(c!='/'&&c!='\\') path+="\\";
	}
	s1=path+"*";
	HANDLE h=FindFirstFileA(s1.c_str(),&f);
	if(h==NULL||h==INVALID_HANDLE_VALUE) return v;
	do{
		// skip '.' and '..' and hidden folders
		if(f.cFileName[0]=='.'){
			/*if(f.cFileName[1]==0||
				(f.cFileName[1]=='.'&&f.cFileName[2]==0))*/ continue;
		}
		if(f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
			v.push_back(/*path+*/f.cFileName);
		}
	}while(FindNextFileA(h,&f));
	FindClose(h);
	return v;
#else
	if(!path.empty()){
		char c=path[path.size()-1];
		if(c!='/'&&c!='\\') path+="/";
	}
	DIR *pDir;
	struct dirent *pDirent;
	pDir=opendir(path.c_str());
	if(pDir==NULL) return v;
	while((pDirent=readdir(pDir))!=NULL){
		if(pDirent->d_name[0]=='.'){
			if(pDirent->d_name[1]==0||
				(pDirent->d_name[1]=='.'&&pDirent->d_name[2]==0)) continue;
		}
		string s1=path+pDirent->d_name;
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

std::string processFileName(const std::string& s){
	string prefix=dataPath;
  
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

std::string fileNameFromPath(const std::string &path, const bool webURL){
	std::string filename;
	size_t pos;
#ifdef WIN32
	// FIXME: '/' in string should be '/' not '\/',
	// we don't need to escape it
	if(webURL){
		pos = path.find_last_of("/");
	}else{
		// NOTE: sometimes path separator in Windows can be '/',
		// so we must check botn '\' and '/'
		pos = path.find_last_of("\\/");
	}
#else
	// FIXME: '/' in string should be '/' not '\/',
	// we don't need to escape it
	pos = path.find_last_of("/");
#endif
	if(pos != std::string::npos)
		filename.assign(path.begin() + pos + 1, path.end());
	else
		filename=path;
	
	return filename;
}

std::string pathFromFileName(const std::string &filename){
	std::string path;
	// FIXME: '/' in string should be '/' not '\/',
	// we don't need to escape it
#ifdef WIN32
	// NOTE: sometimes path separator in Windows can be '/',
	// so we must check botn '\' and '/'
	size_t pos = filename.find_last_of("\\/");
#else
	size_t pos = filename.find_last_of("/");
#endif
	if(pos != std::string::npos)
		path.assign(filename.begin(), filename.begin() + pos +1);
	else
		path=filename;
	
	return path;
}

bool downloadFile(const string &path, const string &destination) {
	string filename=fileNameFromPath(path,true);
	
	FILE* file = fopen((destination+filename).c_str(), "wb");
	bool status=downloadFile(path,file);
	fclose(file);
	
	//And return the status.
	return status;
}

bool downloadFile(const string &path, FILE* destination) {
	CURL* curl=curl_easy_init();

	// proxy test (test only)
	string internetProxy = getSettings()->getValue("internet-proxy");
	int pos = internetProxy.find_first_of(":");
	if(pos!=string::npos){
		curl_easy_setopt(curl,CURLOPT_PROXYPORT,atoi(internetProxy.substr(pos+1).c_str()));
		internetProxy = internetProxy.substr(0,pos);
		curl_easy_setopt(curl,CURLOPT_PROXY,internetProxy.c_str());
	}

	curl_easy_setopt(curl,CURLOPT_URL,path.c_str());
	curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,writeData);
	curl_easy_setopt(curl,CURLOPT_WRITEDATA,destination);
	CURLcode res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	
	return (res==0);
}

size_t writeData(void *ptr, size_t size, size_t nmemb, void *stream){
	return fwrite(ptr, size, nmemb, (FILE *)stream);
}


bool extractFile(const string &fileName, const string &destination) {
	//Create the archive we're going to extract.
	archive* file=NULL;
	//Create the destination we're going to extract to.
	archive* dest=NULL;
	
	file=archive_read_new();
	dest=archive_write_disk_new();
	archive_write_disk_set_options(dest, ARCHIVE_EXTRACT_TIME);
	
	archive_read_support_format_zip(file);
	
	//Now read the archive.
	if(archive_read_open_file(file,fileName.c_str(),10240)) {
		cerr<<"Error while reading archive "+fileName<<endl;
		return false;
	}
	
	//Now write every entry to disk.
	int status;
	archive_entry* entry=NULL;
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

bool createDirectory(const char* path){
#ifdef WIN32
		return SHCreateDirectoryExA(NULL,path,NULL)!=0;
#else
		return mkdir(path,0777)==0;
#endif
}

bool removeDirectory(const char *path){
#ifdef WIN32
	WIN32_FIND_DATAA f;
	HANDLE h = FindFirstFileA((string(path)+"\\*").c_str(),&f);
#else
	//Open the directory that needs to be removed.
	DIR* d=opendir(path);
#endif
	//Get the path length
	size_t path_len = strlen(path);
	//Boolean if the directory is emptied.
	//True: succes		False: failure
	//Default is true because if the directory is empty it will never enter the while loop, but we still have success.
	bool r = true;

#ifdef WIN32
	if(h!=NULL && h!=INVALID_HANDLE_VALUE) {
#else
	//Check if the directory exists.
	if(d) {
		//Pointer to an entry of the directory.
		struct dirent* p;
#endif

#ifdef WIN32
		do{
#else
		//Loop the entries of the directory that needs to be removed as long as there's no error.
		while(r && (p=readdir(d))) {
#endif
			//Booleand if the entry is deleted.
			//True: succes		False: failure
			//Default is false.
			bool r2 = false;
			char* buf;
			size_t len;

			/* Skip the names "." and ".." as we don't want to recurse on them. */
#ifdef WIN32
			if (!strcmp(f.cFileName, ".") || !strcmp(f.cFileName, "..")) {
#else
			if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, "..")) {
#endif
				//The filename is . or .. so we continue to the next entry.
				continue;
			} else {

#ifdef WIN32
				//Get the length of the path + the directory entry name.
				len = path_len + strlen(f.cFileName) + 2; 
#else
				//Get the length of the path + the directory entry name.
				len = path_len + strlen(p->d_name) + 2; 
#endif
				buf = (char*) malloc(len);

				if(buf) {
#ifdef WIN32
					_snprintf(buf, len, "%s\\%s", path, f.cFileName);

					if(f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
						r2 = removeDirectory(buf);
					}else{
						r2 = unlink(buf)==0;
					}
#else
					struct stat statbuf;
					snprintf(buf, len, "%s/%s", path, p->d_name);

					if(!stat(buf, &statbuf)){
						//Check if the entry is a directory or a file.
						if (S_ISDIR(statbuf.st_mode)){
							//We call ourself(removeDirectory) recursively.
							//We return true on success.
							r2 = removeDirectory(buf);
						}else{
							//unlink() returns zero on succes so we set r2 to the unlink(buf)==0.
							r2 = unlink(buf)==0;
						}
					}
#endif
					//Free the buf.
					free(buf);
				}
				//We set r to r2 since r2 contains the status of the latest deletion.
				r = r2;
			}
#ifdef WIN32
		}while(r && FindNextFileA(h,&f));
		FindClose(h);
#else
		}
		//Close the directory.
		closedir(d);
#endif
	}
	
	//The while loop has ended, meaning we (tried) cleared the directory.
	//If r is true, meaning no errors we can delete the directory.
	if(r){
		//The return value of rmdir is 0 when it succeeded.
		r = rmdir(path)==0;
	}
	
	//Return the status.
	return r;
}


void copyData(archive* file, archive* dest) {
	int status;
	const void* buff;
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