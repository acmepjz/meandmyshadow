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

#ifndef WORDWRAPPER_H
#define WORDWRAPPER_H

#include <vector>
#include <string>
#include <set>

// The forward declaration of TTF_Font is clunky
struct _TTF_Font;
typedef struct _TTF_Font TTF_Font;

class WordWrapper {
public:
	// The font.
	TTF_Font *font;

	// The maximal width (in pixels if font is not NULL, in characters if font is NULL)
	int maxWidth;

	// The hyphen used for the hyphenator. If it's empty, the hyphenator is disabled.
	std::string hyphen;

	// Enable word wrapping.
	bool wordWrap;

	// Don't hyphenate the words containing http:// or https://.
	bool reserveHyperlinks;

	// Don't hyphenate the following reserved words (case sensitive).
	std::set<std::string> reservedWords;

	// The language used for the hyphenator.
	// "" means use current language.
	std::string hyphenatorLanguage;

private:
	// Calculate text width (in pixels if font is not NULL, in characters if font is NULL).
	int getTextWidth(const std::string& s);

	// Calculate glyph width (in pixels if font is not NULL, in characters if font is NULL).
	int getGlyphWidth(int ch);

	// Check if a word is reserved.
	bool isReserved(const std::string& word);

	// Internal function.
	void addWord(std::vector<std::string>& output, std::string& line, int& lineWidth, const std::string& spaces, const std::string& nonSpaces);

public:
	WordWrapper();
	~WordWrapper();

	// Add a string (possible multiline) to output.
	void addString(std::vector<std::string>& output, const std::string& input);

	// Add a single line string (subject to wordwrap) to output.
	void addLine(std::vector<std::string>& output, const std::string& input);

	// Add a list of lines (assume each line is a single line string, subject to wordwrap) to output.
	void addLines(std::vector<std::string>& output, const std::vector<std::string>& input);
};

#endif
