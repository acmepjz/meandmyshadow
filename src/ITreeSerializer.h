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

#ifndef ITREESERIALIZER_H
#define ITREESERIALIZER_H

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include "ITreeStorage.h"

class ITreeSerializer{
public:
	virtual bool ReadNode(std::istream& fin,ITreeStorageBuilder* objOut,bool bLoadSubNodeOnly=false)=0;
	virtual void WriteNode(ITreeStorageReader* obj,std::ostream& fout,bool bWriteHeader=true,bool bSaveSubNodeOnly=false)=0;
public:
	bool LoadNodeFromFile(const char* FileName,ITreeStorageBuilder* objOut,bool bLoadSubNodeOnly=false){
		std::ifstream f(FileName);
		if(!f) return false;
		return ReadNode(f,objOut,bLoadSubNodeOnly);
	}
	bool SaveNodeToFile(const char* FileName,ITreeStorageReader* obj,bool bWriteHeader=true,bool bSaveSubNodeOnly=false){
		std::ofstream f(FileName);
		if(!f) return false;
		WriteNode(obj,f,bWriteHeader,bSaveSubNodeOnly);
		return true;
	}
};

#endif