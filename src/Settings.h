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

#include <iostream>
#include <string>
#include <sstream>
#include <map>
#include <fstream>
#include <ctime>

class Settings
{
private:
	const std::string fileName;
	std::map<std::string,std::string> settings;
	
	void createFile();
	void parseLine(const std::string &line);
	bool validLine(const std::string &line);
	void unComment(std::string &line);
	bool empty(const std::string &line);
public:
	Settings(std::string fName);
	void parseFile();
	std::string getValue(const std::string &key);
	bool getBoolValue(const std::string &key);
	void setValue(const std::string &key, const std::string &value);
	void save();
};