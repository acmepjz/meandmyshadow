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

#ifndef ITREESTORAGE_H
#define ITREESTORAGE_H

#include <string>
#include <vector>

class ITreeStorageBuilder{
public:
	//Destructor.
	virtual ~ITreeStorageBuilder(){}

	//Set the name of the TreeStorageNode.
	//name: The name to give.
	virtual void setName(std::string& name)=0;
	//Set the value of the TreeStorageNode.
	//value: The value to give.
	virtual void setValue(std::vector<std::string>& value)=0;
	
	//Method that should create a new node in the TreeStorageNode and add it to it's subnodes.
	//Returns a pointer to the new TreeStorageNode.
	virtual ITreeStorageBuilder* newNode()=0;
	//Method that should add a new attribute to the TreeStorageNode.
	//name: The name of the new attribute.
	//value: The value(s) of the new attribute.
	virtual void newAttribute(std::string& name,std::vector<std::string>& value)=0;

};

class ITreeStorageReader{
public:
	//Destructor.
	virtual ~ITreeStorageReader(){}	
  
	//Sets the parameter name to the name of the TreeStorageNode.
	//name: The string to fill with the name;
	virtual void getName(std::string& name)=0;
	//Sets the parameter value to the value(s) of the TreeStorageNode.
	//value: The vector to fill with the value(s);
	virtual void getValue(std::vector<std::string>& value)=0;
	
	//Method used for iterating through the attributes of the TreeStorageNode.
	//pUserData: Pointer TODO???
	//name: The string fill with the name of the attribute.
	//value: Vector to fill with the value(s) of the attribute.
	virtual void* getNextAttribute(void* pUserData,std::string& name,std::vector<std::string>& value)=0;
	//Method used for iterating through the subnodes of the TreeStorageNode.
	//pUserData: Pointer TODO???
	//obj: Pointer that will be pointed to the nextNode, if present.
	virtual void* getNextNode(void* pUserData,ITreeStorageReader*& obj)=0;
	
};
#endif
