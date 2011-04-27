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

#include "POASerializer.h"
#include <sstream>
using namespace std;

static void ReadString(std::istream& fin,std::string& s){
	int c;
	c=fin.get();
	if(c=='\"'){
		while(!fin.eof()){
			c=fin.get();
			if(c=='\"'){
				c=fin.get();
				if(c!='\"'){
					fin.unget();
					return;
				}
			}
			s.push_back(c);
		}
	}else{
		do{
			switch(c){
			case EOF:
			case ' ':
			case '\r':
			case '\n':
			case '\t':
				return;
			case ',':
			case '=':
			case '(':
			case ')':
			case '{':
			case '}':
			case '#':
				fin.unget();
				return;
			default:
				s.push_back(c);
			}
			c=fin.get();
		}while(!fin.eof());
	}
}

static void SkipWhitespaces(std::istream& fin){
	int c;
	while(!fin.eof()){
		c=fin.get();
		switch(c){
		case EOF:
		case ' ':
		case '\r':
		case '\n':
		case '\t':
			break;
		default:
			fin.unget();
			return;
		}
	}
}

static void SkipComment(std::istream& fin){
	int c;
	while(!fin.eof()){
		c=fin.get();
		if(c=='\r'||c=='\n'){
			fin.unget();
			break;
		}
	}
}

bool POASerializer::ReadNode(std::istream& fin,ITreeStorageBuilder* objOut,bool bLoadSubNodeOnly){
	int c,nMode;
	if(!fin) return false;
	/*
	0=read name
	1=read attribute value
	2=read subnode value
	16=add attribure
	17=add subnode
	*/
	//---
	vector<ITreeStorageBuilder*> tStack;
	vector<string> Names,Values;
	//---
	if(bLoadSubNodeOnly) tStack.push_back(objOut);
	//---
	while(!fin.eof()){
		c=fin.get();
		switch(c){
		case EOF:
		case ' ':
		case '\r':
		case '\n':
		case '\t':
			//whitespaces
			break;
		case '#':
			//comment
			SkipComment(fin);
			break;
		case '}':
			if(tStack.size()==0) return false;
			if(objOut!=NULL) objOut->EndNode();
			tStack.pop_back();
			if(tStack.size()==0) return true;
			objOut=tStack.back();
			break;
		default:
			fin.unget();
			{
				Names.clear();
				Values.clear();
				nMode=0;
				//read names
				while(!fin.eof()){
					string s;
					SkipWhitespaces(fin);
					ReadString(fin,s);
					switch(nMode){
					case 0:
						Names.push_back(s);
						break;
					case 1:
					case 2:
						Values.push_back(s);
						break;
					}
					SkipWhitespaces(fin);
					//---
					c=fin.get();
					switch(c){
					case ',':
						break;
					case '=':
						if(nMode==0) nMode=1;
						else return false;
						break;
					case '(':
						if(nMode==0) nMode=2;
						else return false;
						break;
					case ')':
						if(nMode==2) nMode=17;
						else return false;
						break;
					case '{':
						fin.unget();
						nMode=17;
						break;
					default:
						fin.unget();
						nMode=16;
						break;
					}
					if(nMode>=16) break;
				}
				//check mode
				switch(nMode){
				case 16:
					if(tStack.size()==0) return false;
					if(objOut!=NULL){
						if(Names.size()==0) Names.push_back("");
						while(Values.size()<Names.size()) Values.push_back("");
						for(unsigned int i=0;i<Names.size()-1;i++){
							vector<string> v;
							v.push_back(Values[i]);
							objOut->NewAttribute(Names[i],v);
						}
						if(Names.size()>1) Values.erase(Values.begin(),Values.begin()+(Names.size()-1));
						objOut->NewAttribute(Names.back(),Values);
					}
					break;
				case 17:
					{
						if(Names.size()==0) Names.push_back("");
						else if(Names.size()>1){
							if(tStack.size()==0) return false;
							while(Values.size()<Names.size()) Values.push_back("");
							for(unsigned int i=0;i<Names.size()-1;i++){
								vector<string> v;
								v.push_back(Values[i]);
								objOut->NewAttribute(Names[i],v);
							}
							Values.erase(Values.begin(),Values.begin()+(Names.size()-1));
						}
						ITreeStorageBuilder *objNew=NULL;
						if(tStack.size()==0) objNew=objOut;
						else if(objOut!=NULL) objNew=objOut->NewNode();
						tStack.push_back(objNew);
						if(objNew!=NULL){
							objNew->SetName(Names.back());
							objNew->SetValue(Values);
						}
						objOut=objNew;
						//---
						SkipWhitespaces(fin);
						c=fin.get();
						if(c!='{'){
							fin.unget();
							if(objOut!=NULL) objOut->EndNode();
							tStack.pop_back();
							if(tStack.size()==0) return true;
							objOut=tStack.back();
						}
					}
					break;
				default:
					return false;
				}
			}
			break;
		}
	}
	return true;
}

static void WriteString(std::ostream& fout,std::string& s){
	int c;
	if(s.find_first_of(" \r\n\t,=(){}#\"")!=string::npos){
		fout<<'\"';
		for(unsigned int i=0;i<s.size();i++){
			c=s[i];
			if(c=='\"'){
				fout<<"\"\"";
			}else{
				fout<<(char)c;
			}
		}
		fout<<'\"';
	}else{
		fout<<s;
	}
}

static void WriteStringArray(std::ostream& fout,std::vector<std::string>& s){
	for(unsigned int i=0;i<s.size();i++){
		if(i>0) fout<<',';
		WriteString(fout,s[i]);
	}
}

static void pWriteNode(ITreeStorageReader* obj,std::ostream& fout,int nIndent,bool bSaveSubNodeOnly){
	bool bHaveSubNode=false;
	void *lpUserData=NULL;
	ITreeStorageReader* objSubNode=NULL;
	string s;
	vector<string> v;
	//---
	if(obj==NULL) return;
	//---
	if(!bSaveSubNodeOnly){
		for(int i=0;i<nIndent;i++) fout<<'\t';
		s.clear();
		obj->GetName(s);
		WriteString(fout,s);
		fout<<'(';
		v.clear();
		obj->GetValue(v);
		WriteStringArray(fout,v);
		fout<<')';
		nIndent++;
	}
	//attributes
	lpUserData=NULL;
	for(;;){
		s.clear();
		v.clear();
		lpUserData=obj->GetNextAttribute(lpUserData,s,v);
		if(lpUserData==NULL) break;
		if(!bHaveSubNode && !bSaveSubNodeOnly) fout<<"{\n";
		bHaveSubNode=true;
		for(int i=0;i<nIndent;i++) fout<<'\t';
		WriteString(fout,s);
		fout<<'=';
		WriteStringArray(fout,v);
		fout<<'\n';
	}
	//subnodes
	lpUserData=NULL;
	for(;;){
		lpUserData=obj->GetNextNode(lpUserData,objSubNode);
		if(lpUserData==NULL) break;
		if(objSubNode!=NULL){
			if(!bHaveSubNode && !bSaveSubNodeOnly) fout<<"{\n";
			bHaveSubNode=true;
			pWriteNode(objSubNode,fout,nIndent,false);
		}
	}
	//---
	if(!bSaveSubNodeOnly){
		nIndent--;
		if(bHaveSubNode){
			for(int i=0;i<nIndent;i++) fout<<'\t';
			fout<<'}';
		}
		fout<<'\n';
	}
}

void POASerializer::WriteNode(ITreeStorageReader* obj,std::ostream& fout,bool bWriteHeader,bool bSaveSubNodeOnly){
	if(!fin) return;
	pWriteNode(obj,fout,0,bSaveSubNodeOnly);
}
