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
#ifndef TREESTORAGENODE_H
#define TREESTORAGENODE_H

#include <string>
#include <map>
#include <vector>
#include "ITreeStorage.h"

class TreeStorageNode:public ITreeStorageBuilder,public ITreeStorageReader{
private:
	std::map<std::string,std::vector<std::string> >::iterator objAttrIterator;
public:
	std::vector<TreeStorageNode*> subNodes;
	std::string name;
	std::vector<std::string> value;
	std::map<std::string,std::vector<std::string> > attributes;
	
	TreeStorageNode(){}
	virtual ~TreeStorageNode();
	void destroy();

	virtual void setName(std::string& name);
	virtual void setValue(std::vector<std::string>& value);
	virtual ITreeStorageBuilder* newNode();
	virtual void endNode(){}
	virtual void newAttribute(std::string& name,std::vector<std::string>& value);

	virtual void getName(std::string& name);
	virtual void getValue(std::vector<std::string>& value);
	virtual void* getNextAttribute(void* lpUserData,std::string& sName,std::vector<std::string>& value);
	virtual void* getNextNode(void* lpUserData,ITreeStorageReader*& obj);
};

#endif