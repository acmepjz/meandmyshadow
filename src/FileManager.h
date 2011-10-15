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
#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

//Included for the extractFile method.
#include <archive.h>

//A few strings that all have to do with file locations.
//userPath = The path the user files will be stored (addons, settings).
//exeName = The name of the executable??? TODO
//dataPath = The path the data files are located.
//appPath = The path where the executable is??? TODO
extern std::string userPath,exeName,dataPath,appPath;

//Method for retrieving the userPath.
//Returns: The userPath.
inline const std::string& getUserPath(){
	return userPath;
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

//
std::vector<std::string> EnumAllFiles(std::string sPath,const char* sExtension=NULL);
std::vector<std::string> EnumAllDirs(std::string sPath);

void setPathPrefix(std::string prefix);

std::string processFileName(const std::string& s);

std::string fileNameFromPath(const std::string &path);

bool extractFile(const std::string &fileName, const std::string &destination);

void copyData(archive *file, archive *dest);

bool removeDirectory(const char *path);

#endif