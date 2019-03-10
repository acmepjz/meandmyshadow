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

#include "FakeLuaLexer.h"
#include "UTF8Functions.h"
#include <string.h>
#include <stdio.h>

FakeLuaLexer::FakeLuaLexer()
	: buf(NULL)
	, posStart(ITreeStorageBuilder::FilePosition{ 1, 1 })
	, pos(ITreeStorageBuilder::FilePosition{ 1, 1 })
	, tokenType(EndOfFile)
	, storedByPOASerializer(false)
{
}

bool FakeLuaLexer::getNextToken() {
	char c;

	// skip whitespaces
	for (;;) {
		c = *buf;
		if (c == ' ' || c == '\r' || c == '\n' || c == '\t') {
			buf++; advanceByCharacter(c);
		} else break;
	}

	if (c == '_' || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
		// it's identifier
		posStart = pos;
		for (int i = 1;; i++) {
			c = buf[i];
			if (c == '_' || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
				// do nothing
			} else {
				tokenType = Identifier;
				token.assign(buf, buf + i);
				buf += i; pos.column += i;
				return true;
			}
		}
	} else if (c >= '0' && c <= '9') {
		// it's a number
		return parseNumber();
	}

	int length = 0;

	switch (c) {
	case '\0':
		// EOF
		tokenType = EndOfFile;
		token.clear();
		return false;
		break;
	case '.':
		if (buf[1] >= '0' && buf[1] <= '9') {
			// it's a number
			return parseNumber();
		} else {
			length = (buf[1] == '.') ? (buf[2] == '.' ? 3 : 2) : 1;
		}
		break;
	case '\'': case '\"':
		// short string
		buf++; advanceByCharacter(c);
		return parseShortString(c);
		break;
	case '-':
		// check if it's the beginning of a comment
		if (buf[1] == '-') {
			buf += 2; pos.column += 2;
			return parseComment();
		} else {
			length = 1;
		}
		break;
	case '+': case '*': case '%': case '^': case '#': case '&': case '|':
	case '(': case ')': case '{': case '}': case ']': case ';': case ',':
		length = 1;
		break;
	case '/': case ':':
		length = (buf[1] == c) ? 2 : 1;
		break;
	case '<': case '>':
		length = (buf[1] == c || buf[1] == '=') ? 2 : 1;
		break;
	case '~': case '=':
		length = (buf[1] == '=') ? 2 : 1;
		break;
	case '[':
		// check if it's the beginning of a long string
		length = checkOpeningLongBracket();
		if (length >= 0) {
			buf += length + 2; pos.column += length + 2;
			return parseLongString(length);
		} else {
			length = 1;
		}
		break;
	default:
		// invalid character
		break;
	}

	if (length > 0) {
		tokenType = Operator;
		token.assign(buf, buf + length);
		posStart = pos;
		buf += length; pos.column += length;
		return true;
	} else {
		tokenType = EndOfFile;
		token.clear();
		error = "Invalid character: '";
		error.push_back(c);
		error.push_back('\'');
		return false;
	}
}

// Parse number which is of form
//  ([0-9]+([.][0-9]*)?|[.][0-9]+)([Ee][+-]?[0-9]+)?
// or
//  0[Xx]([0-9A-Fa-f]+([.][0-9A-Fa-f]*)?|[.][0-9A-Fa-f]+)([Pp][+-]?[0-9]+)?
// returns true if it succeded,
bool FakeLuaLexer::parseNumber() {
	char c;
	int i = 0;
	bool isHex = false;

	c = buf[0];
	if (c == '0' && (buf[1] == 'X' || buf[1] == 'x')) {
		isHex = true;
		i = 2;
	}

	int len1 = 0, len2 = 0;

	// read the numbers before '.'
	for (;;) {
		c = buf[i];
		if ((c >= '0' && c <= '9') ||
			(isHex && ((c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f')))
			)
		{
			i++;
			len1++;
		} else {
			break;
		}
	}

	if (c == '.') {
		// read the numbers after '.'
		i++;
		for (;;) {
			c = buf[i];
			if ((c >= '0' && c <= '9') ||
				(isHex && ((c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f')))
				)
			{
				i++;
				len2++;
			} else {
				break;
			}
		}
	}

	if (len1 == 0 && len2 == 0) {
		// invalid
		tokenType = EndOfFile;
		token.clear();
		error = "At least one digits are expected";
		buf += i; pos.column += i;
		return false;
	}

	if (isHex ? (c == 'P' || c == 'p') : (c == 'E' || c == 'e')) {
		// read the exponents
		i++;
		c = buf[i];
		if (c == '+' || c == '-') {
			i++;
			c = buf[i];
		}

		int len3 = 0;
		for (;;) {
			c = buf[i];
			if (c >= '0' && c <= '9') {
				i++;
				len3++;
			} else {
				break;
			}
		}

		if (len3 == 0) {
			// invalid
			tokenType = EndOfFile;
			token.clear();
			error = "At least one digits are expected";
			buf += i; pos.column += i;
			return false;
		}
	}

	// done
	tokenType = NumberLiteral;
	token.assign(buf, buf + i);
	posStart = pos;
	buf += i; pos.column += i;
	return true;
}

// parse a short string which should ends with delim.
// before calling this function the buf should point to the first character of this string.
bool FakeLuaLexer::parseShortString(char delim) {
	tokenType = StringLiteral;
	token.clear();

	posStart = pos;

	char c;

	for (;;) {
		c = *buf;
		if (c == delim) {
			// over
			buf++; advanceByCharacter(c);
			return true;
		} else if (c == '\\') {
			buf++; pos.column++;
			c = *buf;
			switch (c) {
			case 'a': buf++; pos.column++; token.push_back('\a'); break;
			case 'b': buf++; pos.column++; token.push_back('\b'); break;
			case 'f': buf++; pos.column++; token.push_back('\f'); break;
			case 'n': buf++; pos.column++; token.push_back('\n'); break;
			case 'r': buf++; pos.column++; token.push_back('\r'); break;
			case 't': buf++; pos.column++; token.push_back('\t'); break;
			case 'v': buf++; pos.column++; token.push_back('\v'); break;
			case '\\': buf++; pos.column++; token.push_back('\\'); break;
			case '\"': buf++; advanceByCharacter(c); token.push_back('\"'); break;
			case '\'': buf++; pos.column++; token.push_back('\''); break;
			case '\r': case '\n':
				buf++; advanceByCharacter(c);
				if ((buf[0] == '\r' || buf[0] == '\n') && buf[0] != c) {
					c = buf[0];
					buf++; advanceByCharacter(c);
				}
				token.push_back('\n');
				break;
			case 'z':
				buf++; pos.column++;
				for (;;) {
					c = *buf;
					if (c == '\r' || c == '\n' || c == ' ' || c == '\t') {
						buf++; advanceByCharacter(c);
					} else {
						break;
					}
				}
				break;
			case 'u':
			{
				buf++; pos.column++;
				if (buf[0] != '{') {
					tokenType = EndOfFile;
					token.clear();
					error = "'{' expected";
					return false;
				}
				buf++; pos.column++;
				int ch = 0;
				for (;;) {
					int tmp = checkDigit(true);
					if (tmp < 0) {
						if (buf[0] == '}') {
							buf++; pos.column++;
							break;
						}
						tokenType = EndOfFile;
						token.clear();
						error = "Hexadecimal digit or '}' expected";
						return false;
					}
					tmp |= (ch << 4);
					if (tmp >= 0x110000) {
						tokenType = EndOfFile;
						token.clear();
						error = "Out of Unicode range";
						return false;
					}
					buf++; pos.column++;
					ch = tmp;
				}
				U8_ENCODE(ch, token.push_back);
				break;
			}
			case 'x':
			{
				buf++; pos.column++;
				int ch = 0;
				for (int i = 0; i < 2; i++) {
					int tmp = checkDigit(true);
					if (tmp < 0) {
						tokenType = EndOfFile;
						token.clear();
						error = "Hexadecimal digit expected";
						return false;
					}
					buf++; pos.column++;
					ch = (ch << 4) | tmp;
				}
				token.push_back(ch);
				break;
			}
			default:
				if (c >= '0' && c <= '9') {
					int ch = 0;
					for (int i = 0; i < 3; i++) {
						int tmp = checkDigit(false);
						if (tmp < 0) break;
						tmp += ch * 10;
						if (tmp > 255) break;
						buf++; pos.column++;
						ch = tmp;
					}
					token.push_back(ch);
				} else {
					// invalid character
					tokenType = EndOfFile;
					token.clear();
					error = "Invalid character: '";
					error.push_back(c);
					error.push_back('\'');
					return false;
				}
				break;
			}
		} else if (c == '\r' || c == '\n' || c == '\0') {
			tokenType = EndOfFile;
			token.clear();
			error = "Unexpected end of string literal";
			return false;
		} else {
			buf++; advanceByCharacter(c);
			token.push_back(c);
		}
	}
}

// parse a long string which should ends with closing long bracket of given level.
// before calling this function the buf should point to the first character of this string.
bool FakeLuaLexer::parseLongString(int level) {
	tokenType = StringLiteral;
	token.clear();

	char c;

	// skip initial newline
	c = *buf;
	if (c == '\r' || c == '\n') {
		buf++; advanceByCharacter(c);
		if ((buf[0] == '\r' || buf[0] == '\n') && buf[0] != c) {
			c = buf[0];
			buf++; advanceByCharacter(c);
		}
	}

	posStart = pos;

	for (;;) {
		c = *buf;
		if (c == ']' && checkClosingLongBracket() == level) {
			// over
			buf += level + 2; pos.column += level + 2;
			return true;
		} else if (c == '\r' || c == '\n') {
			buf++; advanceByCharacter(c);
			if ((buf[0] == '\r' || buf[0] == '\n') && buf[0] != c) {
				c = buf[0];
				buf++; advanceByCharacter(c);
			}
			token.push_back('\n');
		} else if (c == '\0') {
			tokenType = EndOfFile;
			token.clear();
			error = "Unexpected end of string literal";
			return false;
		} else {
			buf++; advanceByCharacter(c);
			token.push_back(c);
		}
	}
}

// parse a comment.
// before calling this function the buf should point to the first character of this comment.
bool FakeLuaLexer::parseComment() {
	int level = checkOpeningLongBracket();
	if (level >= 0) {
		buf += level + 2; pos.column += level + 2;
		if (parseLongString(level)) {
			tokenType = Comment;
			return true;
		} else {
			return false;
		}
	}

	tokenType = Comment;
	token.clear();

	posStart = pos;

	char c;

	for (;;) {
		c = *buf;
		if (c == '\r' || c == '\n' || c == '\0') {
			// over
			return true;
		} else {
			buf++; advanceByCharacter(c);
			token.push_back(c);
		}
	}
}

// check if the beginning of buf+offset is an opening long bracket.
// if yes it returns the level, otherwise it returns -1.
int FakeLuaLexer::checkOpeningLongBracket(int offset) {
	char c;

	c = buf[offset];
	if (c == '[') {
		for (int i = 1;; i++) {
			c = buf[offset + i];
			if (c == '[') return i - 1;
			else if (c != '=') return -1;
		}
	}

	return -1;
}

// check if the beginning of buf+offset is a closing long bracket.
// if yes it returns the level, otherwise it returns -1.
int FakeLuaLexer::checkClosingLongBracket(int offset) {
	char c;

	c = buf[offset];
	if (c == ']') {
		for (int i = 1;; i++) {
			c = buf[offset + i];
			if (c == ']') return i - 1;
			else if (c != '=') return -1;
		}
	}

	return -1;
}

// check if buf+offset is a digit.
// if yes it returns the number representation of the digit, otherwise it returns -1.
int FakeLuaLexer::checkDigit(bool isHex, int offset) {
	char c = buf[offset];

	if (c >= '0' && c <= '9') {
		return c - '0';
	} else {
		if (isHex) {
			if (c >= 'A' && c <= 'F') {
				return c - ('A' - 10);
			} else if (c >= 'a' && c <= 'f') {
				return c - ('a' - 10);
			}
		}
		return -1;
	}
}

void FakeLuaLexer::advanceByCharacter(char c) {
	// we need to advance by 2 since the \" stored by POASerializer is escaped
	if (c == '\"') pos.column += storedByPOASerializer ? 2 : 1;
	else pos.advanceByCharacter((int)(unsigned char)c);
}
