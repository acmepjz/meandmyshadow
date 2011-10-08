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

#ifndef ITREESTORAGE_H
#define ITREESTORAGE_H

#include <string>
#include <vector>

class ITreeStorageBuilder{
public:
	virtual void setName(std::string& name)=0;
	virtual void setValue(std::vector<std::string>& value)=0;
	virtual ITreeStorageBuilder* newNode()=0;
	virtual void endNode()=0;
	virtual void newAttribute(std::string& name,std::vector<std::string>& value)=0;
	virtual ~ITreeStorageBuilder(){}
};

class ITreeStorageReader{
public:
	virtual void getName(std::string& name)=0;
	virtual void getValue(std::vector<std::string>& value)=0;
	virtual void* getNextAttribute(void* lpUserData,std::string& name,std::vector<std::string>& value)=0;
	virtual void* getNextNode(void* lpUserData,ITreeStorageReader*& obj)=0;
	virtual ~ITreeStorageReader(){}
};

#endif