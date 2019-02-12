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

#ifndef TREESTORAGENODE_H
#define TREESTORAGENODE_H

#include "ITreeStorage.h"
#include <map>
#include <vector>
#include <string>

//This class is used to store data in a tree-structured way.
//Every (TreeStorage)Node has a vector with subNodes and every Node contains a hashmap with attributes.
class TreeStorageNode:public ITreeStorageBuilder,public ITreeStorageReader{
private:
	//Iterator used to iterate the hashmap with attributes.
	//Used by the methods getNextAttribute and getNextNode.
	std::map<std::string,std::vector<std::string> >::iterator objAttrIterator;
public:
	//Vector containing the subnodes of the TreeStorageNode.
	std::vector<TreeStorageNode*> subNodes;
	//String containing the name of the TreeStorageNode.
	std::string name;
	//Vector containing the value(s) of the TreeStorageNode.
	std::vector<std::string> value;
	//Hashmap containing the attributes of the TreeStorageNode.
	std::map<std::string,std::vector<std::string> > attributes;
	
	//Constructor.
	TreeStorageNode(){}
	//Destructor.
	virtual ~TreeStorageNode();
	//This method is used to destroy the TreeStorageNode.
	//Also called when the deconstructor is called.
	void destroy();

	//Set the name of the TreeStorageNode.
	//name: The name to give.
	virtual bool setName(const std::string& name, const FilePosition& pos);
	//Sets the parameter name to the name of the TreeStorageNode.
	//name: The string to fill with the name;
 	virtual void getName(std::string& name);
	
	//Set the value of the TreeStorageNode.
	//value: The value to give.
	virtual bool setValue(const std::vector<std::string>& value, const std::vector<FilePosition>& pos);
	//Sets the parameter value to the value of the TreeStorageNode.
	//value: The string to fill with the name;
	virtual void getValue(std::vector<std::string>& value);
	
	//Creates a new node in the TreeStorageNode.
	//The new node will be added to the subnodes.
	//Returns: a pointer to the new node.
	virtual ITreeStorageBuilder* newNode();
	//Creates a new attribute in the TreeStorageNode.
	//The attribute will be added to the attributes map.
	//name: The name for the new attribute.
	//value: The value for the new attribute.
	virtual bool newAttribute(const std::string& name, const std::vector<std::string>& value, const FilePosition& namePos, const std::vector<FilePosition>& valuePos);


	//Method used for iterating through the attributes of the TreeStorageNode.
	//pUserData: Pointer to the iterator.
	//name: The string fill with the name of the attribute.
	//value: Vector to fill with the value(s) of the attribute.
	virtual void* getNextAttribute(void* pUserData,std::string& name,std::vector<std::string>& value);
	//Method used for iterating through the subnodes of the TreeStorageNode.
	//pUserData: Pointer to the iterator.
	//obj: Pointer that will be pointed to the nextNode, if present.
	virtual void* getNextNode(void* pUserData,ITreeStorageReader*& obj);

	//Calculate the MD5 of node based on the data structure.
	//places the message digest in md,
	//which must have space for 16 bytes of output (or NULL).
	unsigned char* calcMD5(unsigned char* md);
};
#endif
