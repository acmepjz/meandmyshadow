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
	destroy();
}

/**
 * Destroy the TreeStorageNode including it's subNodes.
 */
void TreeStorageNode::destroy(){
	for(unsigned int i=0;i<subNodes.size();i++){
		delete subNodes[i];
	}
	subNodes.clear();
	name.clear();
	value.clear();
	attributes.clear();
}

/**
 * Set the name of the TreeStorageNode.
 * name: The name to give.
 */
void TreeStorageNode::setName(std::string& name){
	this->name=name;
}

/**
 * Set the value of the TreeStorageNode.
 * value: The value to give.
 */
void TreeStorageNode::setValue(std::vector<std::string>& value){
	this->value=value;
}

/**
 * Creates a new node and adds it to the subNodes.
 * returns: ??? 
 */
ITreeStorageBuilder* TreeStorageNode::newNode(){
	TreeStorageNode* obj=new TreeStorageNode;
	subNodes.push_back(obj);
	return obj;
}

/**
 * Give the TreeStorageNode a new attribute.
 * This will create a new attribute in the TreeStorageNode.
 * name: The name of the new attribute.
 * value: The value for the newly added atrribute.
 */
void TreeStorageNode::newAttribute(std::string& name,std::vector<std::string>& value){
	attributes[name]=value;
}

/**
 * Sets the parameter name to the name of the TreeStorageNode.
 * name: The string to fill with the name;
 */
void TreeStorageNode::getName(std::string& name){
	name=this->name;
}

/**
 * Sets the parameter value to the value of the TreeStorageNode.
 * value: The string to fill with the value.
 */
void TreeStorageNode::getValue(std::vector<std::string>& value){
	value=this->value;
}

/**
 * Method used for iterating through the attributes of the TreeStorageNode.
 * lpUserData: Pointer ???
 * name: The string to set to the name of the attribute.
 * value: Vector to set to the value(s) of the attribute.
 */
void* TreeStorageNode::getNextAttribute(void* lpUserData,std::string& name,std::vector<std::string>& value){
	if(lpUserData==NULL) objAttrIterator=attributes.begin();
	if(objAttrIterator!=attributes.end()){
		name=objAttrIterator->first;
		value=objAttrIterator->second;
		objAttrIterator++;
		return &objAttrIterator;
	}else{
		return NULL;
	}
}

/**
 * Method used for iterating through the subNodes of the TreeStorageNode.
 * lpUserData: Pointer ???
 * obj: ???
 */
void* TreeStorageNode::getNextNode(void* lpUserData,ITreeStorageReader*& obj){
	unsigned int i=(intptr_t)lpUserData;
	if(i<subNodes.size()){
		obj=subNodes[i];
		return (void*)(i+1);
	}else{
		return NULL;
	}
}