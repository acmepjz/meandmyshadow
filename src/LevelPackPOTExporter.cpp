/*
 * Copyright (C) 2019 Me and My Shadow
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

#include "LevelPackPOTExporter.h"
#include "Functions.h"
#include "FileManager.h"
#include "POASerializer.h"
#include "TreeStorageNode.h"
#include "FakeLuaLexer.h"
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <algorithm>
#include <iostream>

struct POTFileEntry {
	std::string msgctxt, msgid, msgid_plural;
	std::string comments, sources;
};

//Check if a string contains c-format specifier.
//Currently we only support %[+-]?[0-9]*[.]?[0-9]*[diufFeEgGaAxXoscp]
static bool isCFormat(const std::string& s) {
	for (int i = 0, m = s.size(); i < m; i++) {
		if (s[i] == '%') {
			i++;
			if (i < m) {
				switch (s[i]) {
				case '+': case '-':
					i++;
					break;
				}
			}
			int dotCount = 0;
			while (i < m) {
				switch (s[i]) {
				case '0': case '1': case '2': case '3': case '4':
				case '5': case '6': case '7': case '8': case '9':
					break;
				case '.':
					dotCount++;
					break;
				default:
					dotCount = 2;
					break;
				}
				if (dotCount >= 2) break;
				i++;
			}
			if (i < m) {
				switch (s[i]) {
				case 'd': case 'i': case 'u': case 'f': case 'F':
				case 'e': case 'E': case 'g': case 'G': case 'a': case 'A':
				case 'x': case 'X': case 'o': case 's': case 'c': case 'p':
					return true;
				}
			}
		}
	}

	return false;
}

static void writeComment(std::ostream& fout, const std::string& comments, const std::string& prefix) {
	std::string message;

	for (auto c : comments) {
		if (c != '\r') message.push_back(c);
	}

	//Trim the message.
	{
		size_t lps = message.find_first_not_of('\n'), lpe = message.find_last_not_of("\n \t");
		if (lps == std::string::npos || lpe == std::string::npos || lps > lpe) {
			message.clear(); // it's completely empty
		} else {
			message = message.substr(lps, lpe - lps + 1);
		}
	}

	if (!message.empty()) {
		message.push_back('\0');

		//Split the message into lines.
		for (int lps = 0;;) {
			// determine the end of line
			int lpe = lps;
			for (; message[lpe] != '\n' && message[lpe] != '\0'; lpe++);

			// output the line
			fout << prefix << message.substr(lps, lpe - lps) << std::endl;

			// break if the string ends
			if (message[lpe] == '\0') break;

			// point to the start of next line
			lps = lpe + 1;
		}
	}
}

static void writeHeader(std::ostream& fout) {
	time_t rawtime;
	struct tm *timeinfo;
	char buffer[256];

	time(&rawtime);
	timeinfo = gmtime(&rawtime);

	//FIXME: I can't make %z print timezone under Windows

	strftime(buffer, sizeof(buffer), "\"POT-Creation-Date: %Y-%m-%d %H:%M+0000\\n\"", timeinfo);

	fout << "# SOME DESCRIPTIVE TITLE." << std::endl;
	fout << "# Copyright (C) YEAR THE PACKAGE'S COPYRIGHT HOLDER" << std::endl;
	fout << "# This file is distributed under the same license as the PACKAGE package." << std::endl;
	fout << "# FIRST AUTHOR <EMAIL@ADDRESS>, YEAR." << std::endl;
	fout << "#" << std::endl;
	fout << "#, fuzzy" << std::endl;
	fout << "msgid \"\"" << std::endl;
	fout << "msgstr \"\"" << std::endl;
	fout << "\"Project-Id-Version: PACKAGE VERSION\\n\"" << std::endl;
	fout << "\"Report-Msgid-Bugs-To: \\n\"" << std::endl;
	fout << buffer << std::endl;
	fout << "\"PO-Revision-Date: YEAR-MO-DA HO:MI+ZONE\\n\"" << std::endl;
	fout << "\"Last-Translator: FULL NAME <EMAIL@ADDRESS>\\n\"" << std::endl;
	fout << "\"Language-Team: LANGUAGE <LL@li.org>\\n\"" << std::endl;
	fout << "\"Language: \\n\"" << std::endl;
	fout << "\"MIME-Version: 1.0\\n\"" << std::endl;
	fout << "\"Content-Type: text/plain; charset=UTF-8\\n\"" << std::endl;
	fout << "\"Content-Transfer-Encoding: 8bit\\n\"" << std::endl;
	fout << "\"Plural-Forms: nplurals=INTEGER; plural=EXPRESSION;\\n\"" << std::endl;
	fout << std::endl;
}

class POTFileEntries {
public:
	std::vector<POTFileEntry> entries;
	std::map<std::string, int> lookupTable;
public:
	//Add a msgid to the entries.
	void addEntry(const std::string& msgctxt, const std::string& msgid, const std::string& msgid_plural, const std::string& comments, const std::string& sources) {
		if (msgid.empty()) return;

		int index;
		{
			std::string key = msgctxt + "\x02" + msgid + "\x01" + msgid_plural;
			auto it = lookupTable.find(key);
			if (it == lookupTable.end()) {
				entries.emplace_back();
				lookupTable[key] = index = entries.size() - 1;
				entries[index].msgctxt = msgctxt;
				entries[index].msgid = msgid;
				entries[index].msgid_plural = msgid_plural;
			} else {
				index = it->second;
			}
		}

		if (!comments.empty()) {
			if (comments.back() == '\n') {
				entries[index].comments += comments;
			} else {
				entries[index].comments += comments + "\n";
			}
		}

		if (!sources.empty()) {
			if (sources.back() == '\n') {
				entries[index].sources += sources;
			} else {
				entries[index].sources += sources + "\n";
			}
		}
	}

	//Write all the entries to file.
	void writeEntry(std::ostream& fout) {
		for (auto &entry : entries) {
			//Check if the msgid contains special format strings.
			if (entry.msgid.find("{{{") != std::string::npos || entry.msgid.find("}}}") != std::string::npos) {
				entry.comments += "TRANSLATORS: Please keep words between '{{{' and '}}}' untranslated.\n";
			}

			//Write comments.
			writeComment(fout, entry.comments, "#. ");

			//Write sources.
			writeComment(fout, entry.sources, "#: ");

			//Check if it's c-format.
			if (isCFormat(entry.msgid) || isCFormat(entry.msgid_plural)) {
				fout << "#, c-format" << std::endl;
			}

			//Write msgctxt.
			if (!entry.msgctxt.empty()) {
				fout << "msgctxt \"" << escapeCString(entry.msgctxt) << "\"" << std::endl;
			}

			//Write msgids.
			fout << "msgid \"" << escapeCString(entry.msgid) << "\"" << std::endl;
			if (entry.msgid_plural.empty()) {
				fout << "msgstr \"\"" << std::endl;
			} else {
				fout << "msgid_plural \"" << escapeCString(entry.msgid_plural) << "\"" << std::endl;
				fout << "msgstr[0] \"\"" << std::endl;
				fout << "msgstr[1] \"\"" << std::endl;
			}

			//Add a newline.
			fout << std::endl;
		}
	}
};

static std::string formatSource(const std::string& fileName, const ITreeStorageBuilder::FilePosition& pos, bool showColumn = false) {
	char s[32];
	if (showColumn) {
		sprintf(s, ":%d:%d", pos.row, pos.column);
	} else {
		sprintf(s, ":%d", pos.row);
	}
	return fileName + s;
}

class LoadLevelListTreeStorageNode : public TreeStorageNode {
public:
	POTFileEntries *pot;
public:
	LoadLevelListTreeStorageNode(POTFileEntries *pot) : TreeStorageNode(), pot(pot) {}
	virtual bool newAttribute(const std::string& name, const std::vector<std::string>& value, const FilePosition& namePos, const std::vector<FilePosition>& valuePos) override {
		//Do our own stuff first.
		if (name == "name" && value.size() >= 1) {
			pot->addEntry("", value[0], "",
				"TRANSLATORS: This is the name of the level pack.",
				formatSource("levels.lst", valuePos[0]));
		} else if (name == "description" && value.size() >= 1) {
			pot->addEntry("", value[0], "",
				"TRANSLATORS: This is the description of the level pack.",
				formatSource("levels.lst", valuePos[0]));
		} else if (name == "congratulations" && value.size() >= 1) {
			pot->addEntry("", value[0], "",
				"TRANSLATORS: This will be shown when all the levels in the pack are finished.",
				formatSource("levels.lst", valuePos[0]));
		}

		//Do default stuff.
		return TreeStorageNode::newAttribute(name, value, namePos, valuePos);
	}
};

class FakeLuaParser {
public:
	POTFileEntries *pot;
	FakeLuaLexer& lexer;
	const std::string& fileName;
public:
	FakeLuaParser(POTFileEntries *pot, FakeLuaLexer& lexer, const std::string& fileName)
		: pot(pot), lexer(lexer), fileName(fileName)
	{
	}

	void parse() {
		comment.clear();

		bool skipOnce = false;

		for (;;) {
			if (skipOnce) skipOnce = false;
			else if (!getNextNonCommentToken()) return;

			// we only parse the following format
			// * ( '_' | '__' | 'gettext' ) ( <string> | '(' <string> ')' )
			// * 'pgettext' '(' <string> ',' <string> ')'
			// * ( 'ngettext' '(' | 'npgettext' '(' <string> ',' ) <string> ',' <string> ','

			if (lexer.tokenType == FakeLuaLexer::Identifier) {
				enum TokenType {
					TOKEN_GETTEXT,
					TOKEN_PGETTEXT,
					TOKEN_NGETTEXT,
					TOKEN_NPGETTEXT,
				} tokenType;

				if (lexer.token == "_" || lexer.token == "__" || lexer.token == "gettext") tokenType = TOKEN_GETTEXT;
				else if (lexer.token == "pgettext") tokenType = TOKEN_PGETTEXT;
				else if (lexer.token == "ngettext") tokenType = TOKEN_NGETTEXT;
				else if (lexer.token == "npgettext") tokenType = TOKEN_NPGETTEXT;
				else continue;

				if (!getNextNonCommentToken()) return;

				std::string msgctxt, msgid, msgid_plural;
				ITreeStorageBuilder::FilePosition pos;

				switch (tokenType) {
				case TOKEN_GETTEXT:
					if (lexer.tokenType == FakeLuaLexer::StringLiteral) {
						msgid = lexer.token;
						pos = lexer.posStart;
					} else if (lexer.tokenType == FakeLuaLexer::Operator && lexer.token == "(") {
						if (!getNextNonCommentToken()) return;
						if (lexer.tokenType == FakeLuaLexer::StringLiteral) {
							msgid = lexer.token;
							pos = lexer.posStart;
						} else {
							skipOnce = true;
							continue;
						}

						if (!getNextNonCommentToken()) return;
						if (!(lexer.tokenType == FakeLuaLexer::Operator && lexer.token == ")")) {
							skipOnce = true;
							continue;
						}
					} else {
						skipOnce = true;
						continue;
					}
					break;
				case TOKEN_PGETTEXT:
					if (!(lexer.tokenType == FakeLuaLexer::Operator && lexer.token == "(")) {
						skipOnce = true;
						continue;
					}

					if (!getNextNonCommentToken()) return;
					if (lexer.tokenType == FakeLuaLexer::StringLiteral) {
						msgctxt = lexer.token;
					} else {
						skipOnce = true;
						continue;
					}

					if (!getNextNonCommentToken()) return;
					if (!(lexer.tokenType == FakeLuaLexer::Operator && lexer.token == ",")) {
						skipOnce = true;
						continue;
					}

					if (!getNextNonCommentToken()) return;
					if (lexer.tokenType == FakeLuaLexer::StringLiteral) {
						msgid = lexer.token;
						pos = lexer.posStart;
					} else {
						skipOnce = true;
						continue;
					}

					if (!getNextNonCommentToken()) return;
					if (!(lexer.tokenType == FakeLuaLexer::Operator && lexer.token == ")")) {
						skipOnce = true;
						continue;
					}
					break;
				case TOKEN_NGETTEXT:
				case TOKEN_NPGETTEXT:
					if (!(lexer.tokenType == FakeLuaLexer::Operator && lexer.token == "(")) {
						skipOnce = true;
						continue;
					}

					if (tokenType == TOKEN_NPGETTEXT) {
						if (!getNextNonCommentToken()) return;
						if (lexer.tokenType == FakeLuaLexer::StringLiteral) {
							msgctxt = lexer.token;
						} else {
							skipOnce = true;
							continue;
						}

						if (!getNextNonCommentToken()) return;
						if (!(lexer.tokenType == FakeLuaLexer::Operator && lexer.token == ",")) {
							skipOnce = true;
							continue;
						}
					}

					if (!getNextNonCommentToken()) return;
					if (lexer.tokenType == FakeLuaLexer::StringLiteral) {
						msgid = lexer.token;
						pos = lexer.posStart;
					} else {
						skipOnce = true;
						continue;
					}

					if (!getNextNonCommentToken()) return;
					if (!(lexer.tokenType == FakeLuaLexer::Operator && lexer.token == ",")) {
						skipOnce = true;
						continue;
					}

					if (!getNextNonCommentToken()) return;
					if (lexer.tokenType == FakeLuaLexer::StringLiteral) {
						msgid_plural = lexer.token;
					} else {
						skipOnce = true;
						continue;
					}

					if (!getNextNonCommentToken()) return;
					if (!(lexer.tokenType == FakeLuaLexer::Operator && lexer.token == ",")) {
						skipOnce = true;
						continue;
					}
					break;
				}

				pot->addEntry(msgctxt, msgid, msgid_plural, comment, formatSource(fileName, pos));
				comment.clear();
			}
		}
	}

	bool getNextNonCommentToken() {
		bool isTranslatorsComment = false;

		for (;;) {
			if (!lexer.getNextToken()) {
				if (!lexer.error.empty()) {
					//Show error message.
					std::cerr << formatSource(fileName, lexer.pos, true) << ": ERROR: " << lexer.error << std::endl;
				}
				return false;
			}

			if (lexer.tokenType == FakeLuaLexer::Comment) {
				if (lexer.token.empty() || lexer.token.back() != '\n') lexer.token.push_back('\n');
				if (!isTranslatorsComment) {
					if (lexer.token.find("TRANSLATORS:") != std::string::npos) {
						isTranslatorsComment = true;
						comment.clear(); // remove last translators comment
					}
				}
				if (isTranslatorsComment) comment += lexer.token;
			} else {
				return true;
			}
		}
	}

private:
	std::string comment;
};

class LoadLevelMessageTreeStorageNode : public TreeStorageNode {
public:
	POTFileEntries *pot;
	const std::string& fileName;
public:
	LoadLevelMessageTreeStorageNode(POTFileEntries *pot, const std::string& fileName) : TreeStorageNode(), pot(pot), fileName(fileName) {}
	virtual ITreeStorageBuilder* newNode() override {
		return new LoadLevelMessageTreeStorageNode(pot, fileName);
	}
	virtual bool newAttribute(const std::string& name, const std::vector<std::string>& value, const FilePosition& namePos, const std::vector<FilePosition>& valuePos) override {
		//Do our own stuff first.
		if (name == "name" && value.size() >= 1) {
			pot->addEntry("", value[0], "",
				"TRANSLATORS: This is the name of a level.",
				formatSource(fileName, valuePos[0]));
		} else if (name == "message" && value.size() >= 1) {
			if (this->name == "tile" && this->value.size() >= 1 && this->value[0] == "NotificationBlock") {
				pot->addEntry("", unescapeNewline(value[0]), "", "",
					formatSource(fileName, valuePos[0]));
			} else {
				pot->addEntry("", value[0], "", "",
					formatSource(fileName, valuePos[0]));
			}
		} else if (name == "script" && value.size() >= 1) {
			//Now we extract strings from script.
			FakeLuaLexer lexer;

			lexer.buf = value[0].c_str();
			lexer.pos = valuePos[0];
			lexer.storedByPOASerializer = true;

			FakeLuaParser parser(pot, lexer, fileName);

			parser.parse();
		}

		//Do default stuff.
		return TreeStorageNode::newAttribute(name, value, namePos, valuePos);
	}
};

bool LevelPackPOTExporter::exportPOT(const std::string& levelpackPath) {
	//Open the level list.
	std::string levelListFile = levelpackPath + "levels.lst";
	std::ifstream fin(levelListFile.c_str());
	if (!fin) {
		std::cerr << "ERROR: Can't load level list " << levelListFile << std::endl;
		return false;
	}

	//Create the entries.
	POTFileEntries pot;

	//Load the level list file.
	LoadLevelListTreeStorageNode obj(&pot);
	if (!POASerializer().readNode(fin, &obj, true)){
		std::cerr << "ERROR: Invalid file format of level list " << levelListFile << std::endl;
		return false;
	}

	//Loop through the level list entries.
	for (unsigned int i = 0; i<obj.subNodes.size(); i++){
		TreeStorageNode* obj1 = obj.subNodes[i];
		if (obj1 == NULL)
			continue;
		if (!obj1->value.empty() && obj1->name == "levelfile") {
			std::string fileName = obj1->value[0];

			//The path to the file to open.
			std::string levelFile = levelpackPath + fileName;

			//Open the level file.
			LoadLevelMessageTreeStorageNode obj(&pot, fileName);
			if (!POASerializer().loadNodeFromFile(levelFile.c_str(), &obj, true)) {
				std::cerr << "ERROR: Can't load level file " << levelFile << std::endl;
				return false;
			}

			//Try to load the lua file.
			size_t lp = fileName.find_last_of('.');
			if (lp != std::string::npos) {
				fileName = fileName.substr(0, lp);
			}
			fileName += ".lua";
			std::ifstream fin((levelpackPath + fileName).c_str());
			if (fin) {
				fin.seekg(0, std::ios::end);
				auto size = fin.tellg();
				fin.seekg(0, std::ios::beg);

				std::vector<char> buf(size);
				fin.read(&(buf[0]), size);
				buf.push_back(0);

				//Extract strings from script.
				FakeLuaLexer lexer;

				lexer.buf = &(buf[0]);

				FakeLuaParser parser(&pot, lexer, fileName);

				parser.parse();
			}
		}
	}

	//Create the directory.
	createDirectory((levelpackPath + "locale").c_str());

	//Create the messages.pot
	std::string potFile = levelpackPath + "locale/messages.pot";
	std::ofstream fout(potFile.c_str());
	if (!fout) {
		std::cerr << "ERROR: Can't open the file " << potFile << " for save" << std::endl;
		return false;
	}

	//Write entries.
	writeHeader(fout);
	pot.writeEntry(fout);

	//Over.
	return true;
}
