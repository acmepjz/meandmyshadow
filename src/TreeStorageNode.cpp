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

#include "TreeStorageNode.h"
using namespace std;

TreeStorageNode::~TreeStorageNode(){
	//The deconstructor will just calls destroy().
	destroy();
}

void TreeStorageNode::destroy(){
	//Loop through the subnodes and delete them.
	for(unsigned int i=0;i<subNodes.size();i++){
		delete subNodes[i];
	}
	
	//Now clear some stuff.
	name.clear();
	value.clear();
	attributes.clear();
	subNodes.clear();
}

void TreeStorageNode::setName(std::string& name){
	this->name=name;
}
void TreeStorageNode::getName(std::string& name){
	name=this->name;
}

void TreeStorageNode::setValue(std::vector<std::string>& value){
	this->value=value;
}
void TreeStorageNode::getValue(std::vector<std::string>& value){
	value=this->value;
}

ITreeStorageBuilder* TreeStorageNode::newNode(){
	TreeStorageNode* obj=new TreeStorageNode;
	subNodes.push_back(obj);
	return obj;
}

void TreeStorageNode::newAttribute(std::string& name,std::vector<std::string>& value){
	//Put the attribute in the attributes map.
	attributes[name]=value;
}

void* TreeStorageNode::getNextAttribute(void* pUserData,std::string& name,std::vector<std::string>& value){
	if(pUserData==NULL) objAttrIterator=attributes.begin();
	if(objAttrIterator!=attributes.end()){
		name=objAttrIterator->first;
		value=objAttrIterator->second;
		objAttrIterator++;
		return &objAttrIterator;
	}else{
		return NULL;
	}
}

void* TreeStorageNode::getNextNode(void* pUserData,ITreeStorageReader*& obj){
	unsigned int i=(intptr_t)pUserData;
	
	//Check if the pointer is in range of the subNodes vector.
	if(i<subNodes.size()){
		obj=subNodes[i];
		return (void*)(i+1);
	}else{
		return NULL;
	}
}