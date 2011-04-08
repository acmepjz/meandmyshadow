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
	Destroy();
}

void TreeStorageNode::Destroy(){
	/*
	for(map<string,TreeStorageNode*>::iterator i=SubNodes.begin();i!=SubNodes.end();i++){
		delete i->second;
	}
	*/
	for(unsigned int i=0;i<SubNodes.size();i++){
		delete SubNodes[i];
	}
	SubNodes.clear();
	Name.clear();
	Value.clear();
	Attributes.clear();
}

void TreeStorageNode::SetName(std::string& sName){
	Name=sName;
}

void TreeStorageNode::SetValue(std::vector<std::string>& sValue){
	if(sValue.size()>0) Value=sValue[0];
	else Value="";
}

ITreeStorageBuilder* TreeStorageNode::NewNode(){
	TreeStorageNode* obj=new TreeStorageNode;
	SubNodes.push_back(obj);
	return obj;
}

void TreeStorageNode::NewAttribute(std::string& sName,std::vector<std::string>& sValue){
	if(sValue.size()>0){
		Attributes[sName]=sValue[0];
	}else{
		Attributes[sName]="";
	}
}

void TreeStorageNode::GetName(std::string& sName){
	sName=Name;
}

void TreeStorageNode::GetValue(std::vector<std::string>& sValue){
	sValue.push_back(Value);
}

void* TreeStorageNode::GetNextAttribute(void* lpUserData,std::string& sName,std::vector<std::string>& sValue){
	if(lpUserData==NULL) objAttrIterator=Attributes.begin();
	if(objAttrIterator!=Attributes.end()){
		sName=objAttrIterator->first;
		sValue.push_back(objAttrIterator->second);
		objAttrIterator++;
		return &objAttrIterator;
	}else{
		return NULL;
	}
}

void* TreeStorageNode::GetNextNode(void* lpUserData,ITreeStorageReader*& obj){
	unsigned int i=(unsigned int)lpUserData;
	if(i<SubNodes.size()){
		obj=SubNodes[i];
		return (void*)(i+1);
	}else{
		return NULL;
	}
}

