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

#ifndef POASERIALIZER_H
#define POASERIALIZER_H

#include "ITreeSerializer.h"

//This class is used for reading/writing of TreeStorageNodes to/from POAformat-files.
class POASerializer:public ITreeSerializer{
public:
	//This method will read the inputstream and parses it.
	//The result will be placed in a TreeStorageNode.
	//fin: The input stream which will be read from.
	//objOut: The TreeStorageNode the result will be put in.
	//loadSubNodesOnly: Boolean if only the subNodes need to be loaded.
	//Returns: true if the reading and parsing succeeded.
	virtual bool readNode(std::istream& fin,ITreeStorageBuilder* objOut,bool loadSubNodesOnly=false);
	
	//This method will write a TreeStorageNode to a outputstream.
	//obj: The TreeStorageNode to save.
	//fout: The output stream which will be writen to.
	//saveSubNodesOnly: Boolean if only the subNodes need to be saved.
	//Returns: true if the writing succeeded.
	virtual void writeNode(ITreeStorageReader* obj,std::ostream& fout,bool bWriteHeader=true,bool saveSubNodeOnly=false);
};
#endif
