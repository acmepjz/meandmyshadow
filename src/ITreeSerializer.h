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

#ifndef ITREESERIALIZER_H
#define ITREESERIALIZER_H

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include "ITreeStorage.h"

//An abstract class which is used to read/write data from TreeStorage to various file formats.
//Currently only one file format is implemented, i.e. POASerializer. (Theoretically you can write a XMLSerializer.)
class ITreeSerializer{
public:
	//Method used to read a node from an inputstream.
	//fin: The inputstream to read from.
	//objOut: The TreeStorageNode in which the result will come.
	//loadSubNodeOnly: Boolean if only the subNodes should be loaded.
	//Returns: False if there's an error while reading the node.
	virtual bool readNode(std::istream& fin,ITreeStorageBuilder* objOut,bool loadSubNodeOnly=false)=0;

	//Method used to write to an outputstream.
	//obj: Pointer to the TreeStorageNode containing the data.
	//fout: The output stream to write to.
	//writeHeader: Write the header to the file (e.g. XML file; not that POA file doesn't have a header).
	//saveSubNodeOnly: Boolean if only the subNodes should be saved.
	virtual void writeNode(ITreeStorageReader* obj,std::ostream& fout,bool writeHeader=true,bool saveSubNodeOnly=false)=0;
public:
	//Method used to load a node from a file.
	//fileName: The file to load from.
	//objOut: The object to place the result in.
	//loadSubNodeOnly: Boolean if only the subNodes should be loaded.
	//Returns: False if there's an error while reading the node.
	bool loadNodeFromFile(const char* fileName,ITreeStorageBuilder* objOut,bool loadSubNodeOnly=false){
		//Open an inputstream.
		std::ifstream f(fileName);
		//If it failed then we return.
		if(!f)
			return false;
		
		//It didn't fail so let the readNode method handle the rest.
		return readNode(f,objOut,loadSubNodeOnly);
	}

	//Method used to write a node to a file.
	//fileName: The file to save to.
	//obj: Pointer to the TreeStorageNode containing the data.
	//writeHeader: Write the header to the file (e.g. XML file; not that POA file doesn't have a header).
	//saveSubNodeOnly: Boolean if only the subNodes should be saved.
	bool saveNodeToFile(const char* fileName,ITreeStorageReader* obj,bool writeHeader=true,bool saveSubNodeOnly=false){
		//Open an outputstream.
		std::ofstream f(fileName);
		//If it failed then we return.
		if(!f)
			return false;
		
		//It didn't fail so let the writeNode method handle the rest.
		writeNode(obj,f,writeHeader,saveSubNodeOnly);
		return true;
	}
};

#endif
