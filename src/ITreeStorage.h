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

//An abstract class which is used to transfer the data from file to TreeStorage.
//NOTE: Usually you should simply use the TreeStorageNode class.
//You only need to use this class when you want to store the data in a customized way.
//Another use case is you only want to read some of the property in the file, by using early exit feature.
class ITreeStorageBuilder{
public:
	//Represents a position in a text file.
	struct FilePosition {
		int row;
		int column;
		inline void advance() {
			column++;
		}
		inline void advanceByCharacter(int c) {
			if (c == '\n') {
				row++;
				column = 1;
			} else if (c == '\t') {
				//Assume tab width is 4
				column = ((column + 3) & (-4)) + 1;
			} else if (c >= 0 && c <= 255) {
				//TODO: Unicode support
				column++;
			}
		}
	};
public:
	//Destructor.
	virtual ~ITreeStorageBuilder(){}

	//Set the name of the TreeStorageNode.
	//name: The name to give.
	//return value: true means early exit, i.e. doesn't read the file further.
	virtual bool setName(const std::string& name, const FilePosition& pos) = 0;

	//Set the value of the TreeStorageNode.
	//value: The value to give.
	//return value: true means early exit, i.e. doesn't read the file further.
	virtual bool setValue(const std::vector<std::string>& value, const std::vector<FilePosition>& pos) = 0;
	
	//Method that should create a new node in the TreeStorageNode and add it to it's subnodes.
	//Returns a pointer to the new TreeStorageNode. NULL means early exit, i.e. doesn't read the file further.
	virtual ITreeStorageBuilder* newNode()=0;

	//Method that should add a new attribute to the TreeStorageNode.
	//name: The name of the new attribute.
	//value: The value(s) of the new attribute.
	//return value: true means early exit, i.e. doesn't read the file further.
	virtual bool newAttribute(const std::string& name, const std::vector<std::string>& value, const FilePosition& namePos, const std::vector<FilePosition>& valuePos) = 0;
};

//An abstract class which is used to transfer the data from TreeStorage to file.
//NOTE: Usually you should simply use the TreeStorageNode class.
//You only need to use this class when you want to store the data in a customized way.
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
	//pUserData: A user pointer, usually stores information about the iterator itself. NULL means get the first attribute.
	//name: The string fill with the name of the attribute.
	//value: Vector to fill with the value(s) of the attribute.
	//return value: The new value of the user pointer. NULL means there are no more attributes.
	virtual void* getNextAttribute(void* pUserData,std::string& name,std::vector<std::string>& value)=0;

	//Method used for iterating through the subnodes of the TreeStorageNode.
	//pUserData: A user pointer, usually stores information about the iterator itself. NULL means get the first node.
	//obj: Pointer that will be pointed to the nextNode, if present.
	//return value: The new value of the user pointer. NULL means there are no more nodes.
	virtual void* getNextNode(void* pUserData,ITreeStorageReader*& obj)=0;
};

#endif
