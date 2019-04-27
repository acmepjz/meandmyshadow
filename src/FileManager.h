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

#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include <string>
#include <vector>
#include <iostream>

struct archive;

//NOTE: All the methods work with processed pathnames.
//So %DATA%, %USER%, etc. can't be used.
//With exception of processFileName().

//A few strings that all have to do with file locations.
//userPath = The path the user files will be stored (addons, settings).
//exeName = The name of the executable.
//dataPath = The path the data files are located.
//appPath = The path where the executable is located.
extern std::string userPath,exeName,dataPath,appPath;

//The following two paths are for non-Windows systems only.
//userDataPath = The path for user created content and user downloaded content (addons).
//userCachePath = The path where temporary stuff will be stored.
#ifndef WIN32
extern std::string userDataPath,userCachePath;
#endif

//Enum containing the different userPath types.
//NOTE: They are only needed for the non-Windows platform..
enum UserPaths{
	//The userpath containing the config files.
	//Default $HOME/.config/meandmyshadow/
	USER_CONFIG,
	//The userpath containing the user data.
	//Default $HOME/.local/share/meandmyshadow/
	USER_DATA,
	//The userpath containing the temporary files.
	//Default $HOME/.cache/meandmyshadow/
	USER_CACHE
};

//Method for retrieving the userPath.
//type: The type of userpath to return, only used on non-Windows platforms.
//Returns: The userPath.
inline const std::string& getUserPath(int type=0){
#ifdef WIN32
	return userPath;
#else
	switch(type){
		case USER_CONFIG:
			return userPath;
			break;
		case USER_DATA:
			return userDataPath;
			break;
		case USER_CACHE:
			return userCachePath;
			break;
		default:
			std::cerr<<"WARNING: Illegal userpath type, returning user config path."<<std::endl;
			return userPath;
			break;
	}
#endif
}
//Method for retrieving the exeName.
//Returns: The exeName.
inline const std::string& getEXEName(){
	return exeName;
}
//Method for retrieving the dataPath.
//Returns: The dataPath.
inline const std::string& getDataPath(){
	return dataPath;
}
//Method for retrieving the appPath.
//Returns: The appPath.
inline const std::string& getAppPath(){
	return appPath;
}

//This method will try to find paths for the userPath, dataPath, appPath and exeName.
//Returns: True if nothing went wrong.
bool configurePaths();

//Method that returns a list of all the files in a given directory.
//path: The path to list the files of.
//extension: The extension the files must have.
//containsPath: Specifies if the return file name should contains path.
//Returns: A vector containing the names of the files.
std::vector<std::string> enumAllFiles(std::string path,const char* extension=NULL,bool containsPath=false);
//Method that returns a list of all the directories in a given directory.
//path: The path to list the directory of.
//containsPath: Specifies if the return file name should contains path.
//Returns: A vector containing the names of the directories.
std::vector<std::string> enumAllDirs(std::string path,bool containsPath=false);

//Method that will parse the string.
//It will convert %USER%, %DATA%, etc. to their according path.
//s: The string that needs to be processed.
//Returns: The processed string.
std::string processFileName(const std::string& s);
//Method that will parse the string.
//FIXME: The method name isn't misleading.
//It will convert the path to the proper domain %USER%, %DATA%, etc. if detected.
//s: The string that needs to be processed.
//Returns: The processed string.
std::string compressFileName(const std::string& s);
//Method used to retrieve the fileName from a full path.
//path: The path with the filename.
//webURL: Boolean if the path is a weburl.
//Returns: String containing the fileName.
std::string fileNameFromPath(const std::string &path, const bool webURL=false);
//Method used to retrieve the path without the fileName from a full path.
//filename: The path with the filename.
//Returns: String containing the path.
std::string pathFromFileName(const std::string &filename);

//Method that will extract an archive and places it in the destination folder.
//fileName: The name of the archive.
//destination: The destination location where the extracted files will come.
//Returns: True if it succeeds without errors.
bool extractFile(const std::string &fileName, const std::string &destination);
//Method used to read a data blcok from an archive and write it to an archive.
//file: The archive to read from.
//dest: The archive to write to.
void copyData(archive* file, archive* dest);

//Method that will check if a directory exists.
//dir: The directory to check.
//Returns: True if the directory exists.
bool dirExists(const char* dir);
//Method that will create a directory.
//path: The directory to create.
//Returns: True if it succeeds.
bool createDirectory(const char* path);
//Method that will remove a directory.
//path: The directory to remove.
//Returns: True if it succeeds.
bool removeDirectory(const char* path);
//Method that will rename a directory.
//oldPath: The folder path.
//newPath: The destination folder name.
//Returns: True if it succeeds.
bool renameDirectory(const char* oldPath,const char* newPath);

//Method that will check if a file exists.
//file: The filename of the file to check.
//Returns: True if the files exists.
bool fileExists(const char* file);
//Method that will create a file.
//file: The filename of the file to create.
//Returns: True if it succeeds.
bool createFile(const char* file);
//Method that will copy a file.
//source: The input file.
//dest: The output file.
//Returns: True if it succeeds.
bool copyFile(const char* source,const char* dest);
//Method that will remove a file.
//file: The file to remove.
//Returns: True if it succeeds.
bool removeFile(const char* file);

#endif
