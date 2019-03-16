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

#include "WordWrapper.h"
#include "HyphenationManager.h"
#include "HyphenationRule.h"
#include "UTF8Functions.h"
#include <algorithm>

#include <assert.h>

#include <SDL_ttf_fontfallback.h>

int WordWrapper::getTextWidth(const std::string& s) {
	if (s.empty()) return 0;

	int w = 0;

	if (font) {
		TTF_SizeUTF8(font, s.c_str(), &w, NULL);
	} else {
		const size_t m = s.size();

		U8STRING_FOR_EACH_CHARACTER_DO_BEGIN(s, i, m, ch, REPLACEMENT_CHARACTER);
		w++;
		U8STRING_FOR_EACH_CHARACTER_DO_END();
	}

	return w;
}

int WordWrapper::getGlyphWidth(int ch) {
	if (font) {
		int w = 0;
		TTF_GlyphMetrics(font, ch, NULL, NULL, NULL, NULL, &w);
		return w;
	} else {
		return 1;
	}
}

WordWrapper::WordWrapper()
	: font(NULL)
	, maxWidth(0)
	, wordWrap(false)
	, reserveHyperlinks(false)
{
}

WordWrapper::~WordWrapper() {
}

bool WordWrapper::isReserved(const std::string& word) {
	if (reserveHyperlinks) {
		const char *s = word.c_str();
		const size_t m = word.size();
		for (size_t i = 0; i < m; i++) {
			// we only support http or https
			if ((s[i] == 'H' || s[i] == 'h')
				&& (s[i + 1] == 'T' || s[i + 1] == 't')
				&& (s[i + 2] == 'T' || s[i + 2] == 't')
				&& (s[i + 3] == 'P' || s[i + 3] == 'p'))
			{
				if (s[i + 4] == ':' && s[i + 5] == '/' && s[i + 6] == '/') {
					// http
					return true;
				} else if ((s[i + 4] == 'S' || s[i + 4] == 's') && s[i + 5] == ':' && s[i + 6] == '/' && s[i + 7] == '/') {
					// https
					return true;
				}
			}
		}
	}

	for (const std::string& s : reservedWords) {
		if (word == s) return true;
	}

	for (const std::string& s : reservedFragments) {
		if (word.find(s) != std::string::npos) return true;
	}

	return false;
}

int WordWrapper::addString(std::vector<std::string>& output, const std::string& input) {
	int mw = 0;
	std::string line;

	for (char c : input) {
		if (c == '\r') {
		} else if (c == '\n') {
			mw = std::max(addLine(output, line), mw);
			line.clear();
		} else {
			line.push_back(c);
		}
	}

	return std::max(addLine(output, line), mw);
}

// Add a word to line, output the line only if the line+newWord doesn't fit the width and in this case put the newWord to the line.
// Returns the maximal width required for this string.
int WordWrapper::addWord(std::vector<std::string>& output, std::string& line, int& lineWidth, const std::string& spaces, const std::string& nonSpaces) {
	int w1 = getTextWidth(spaces);

	{
		int w2 = getTextWidth(nonSpaces);

		//Check if it fits into current line.
		if (lineWidth + w1 + w2 <= maxWidth) {
			line += spaces + nonSpaces;
			lineWidth += w1 + w2;
			return lineWidth;
		}

		//Now it doesn't fit into current line.

		//Check if we should skip the hyphenation.
		if (hyphen.empty() || isReserved(nonSpaces)) {
			if (line.empty()) {
				//A line consists of at least one word, so we append it forcefully.
				line += spaces + nonSpaces;
				lineWidth += w1 + w2;
				return lineWidth;
			} else {
				//We output current line.
				output.push_back(line);

				//And add a new line consisting of new word (but we remove spaces in it).
				line = nonSpaces;
				int mw = std::max(lineWidth, w2);
				lineWidth = w2;
				return mw;
			}
		}
	}

	auto hm = getHyphenationManager();
	auto hyphenator = hyphenatorLanguage.empty() ? hm->getHyphenator() : hm->getHyphenator(hyphenatorLanguage);
	auto rules = hyphenator->applyHyphenationRules(nonSpaces);

	const size_t m = nonSpaces.size();

	std::string tmp, prev;
	int skip = 0, prevSkip = 0, prevWidth = 0;
	size_t prevIndex = 0;
	int mw = lineWidth;

	for (size_t i = 0;; i++) {
		const Hyphenate::HyphenationRule *rule = (i < m) ? (*rules)[i] : NULL;
		if (rule || i == m) {
			std::string tmp2 = tmp;
			if (rule) rule->apply_first(tmp2, hyphen);

			int newWidth = getTextWidth(tmp2);

			/*//debug
			printf("%-5d %s\n", newWidth, tmp2.c_str());*/

			//Check if we should output current line directly.
			if (lineWidth + w1 + newWidth > maxWidth && prev.empty() && !line.empty()) {
				//We output current line.
				output.push_back(line);
				mw = std::max(lineWidth, mw);

				line.clear();
				lineWidth = 0;
				w1 = 0;
			}

			//Check if the line is still too long.
			if (lineWidth + w1 + newWidth > maxWidth) {
				//Check if we have previous available hyphenation
				if (prev.empty()) {
					//Line is empty, we have to append it forcefully.
					assert(line.empty());

					if (w1 > 0) line += spaces;
					line += tmp2;
					if (i < m) {
						output.push_back(line);
						mw = std::max(lineWidth, mw);
						line.clear();
						lineWidth = 0;
						w1 = 0;
					} else {
						lineWidth += w1 + newWidth;
						mw = std::max(lineWidth, mw);
					}

					//Update buffer
					tmp.clear();
					if (rule) skip += rule->apply_second(tmp);
				} else {
					//We use previous available hyphenation
					if (w1 > 0) line += spaces;
					output.push_back(line + prev);
					mw = std::max(lineWidth + w1 + prevWidth, mw);
					line.clear();
					lineWidth = 0;
					w1 = 0;

					//Rewind
					prev.clear();
					prevWidth = 0;
					skip = prevSkip;
					i = prevIndex;

					//Update buffer
					tmp.clear();
					rule = (*rules)[i];
					assert(rule != NULL);
					skip += rule->apply_second(tmp);
				}
			} else if (i == m) {
				//Output last part
				if (w1 > 0) line += spaces;
				line += tmp2;
				lineWidth += w1 + newWidth;
				mw = std::max(lineWidth, mw);
			} else if (newWidth > prevWidth) {
				//Update prev hyphenation
				prev = tmp2;
				prevSkip = skip;
				prevWidth = newWidth;
				prevIndex = i;
			}
		}

		if (i >= m) break;

		if (skip > 0) skip--;
		else tmp.push_back(nonSpaces[i]);
	}

	return mw;
}

int WordWrapper::addLine(std::vector<std::string>& output, const std::string& input) {
	if (!wordWrap) {
		//Word wrap is not enabled, simply add it to output
		output.push_back(input);
		return getTextWidth(input);
	}

	const size_t m = input.size();

	std::string spaces, nonSpaces, line;
	int lineWidth = 0, mw = 0;

	bool prevIsCJK = false, prevIsCJKStarting = false;

	U8STRING_FOR_EACH_CHARACTER_DO_BEGIN(input, i, m, ch, REPLACEMENT_CHARACTER);

	//A word consists of a sequence of white spaces and a sequence of non-white-spaces.

	//For CJK should only read one CJK character (possibly with a punctuation mark)

	if (ch == '\r') {
	} else if (utf32IsSpace(ch)) {
		prevIsCJK = false;
		prevIsCJKStarting = false;
		if (!nonSpaces.empty()) {
			mw = std::max(addWord(output, line, lineWidth, spaces, nonSpaces), mw);
			spaces.clear();
			nonSpaces.clear();
		}
		U8_ENCODE(ch, spaces.push_back);
	} else {
		bool isCJK = utf32IsCJK(ch);
		bool isCJKStarting = utf32IsCJKStartingPunctuation(ch);
		if (prevIsCJK) {
			//Output the CJK character immediately unless current character can't be at start of line
			if (!utf32IsCJKEndingPunctuation(ch)) {
				mw = std::max(addWord(output, line, lineWidth, spaces, nonSpaces), mw);
				spaces.clear();
				nonSpaces.clear();
			}
		} else if (isCJK && !nonSpaces.empty()) {
			//Output the existing non-CJK character immediately unless it can't be at end of line
			if (!prevIsCJKStarting) {
				mw = std::max(addWord(output, line, lineWidth, spaces, nonSpaces), mw);
				spaces.clear();
				nonSpaces.clear();
			}
		}
		prevIsCJK = isCJK;
		prevIsCJKStarting = isCJKStarting;
		U8_ENCODE(ch, nonSpaces.push_back);
	}

	U8STRING_FOR_EACH_CHARACTER_DO_END();

	//FIXME: Here we temporarily ignore trailing spaces
	if (!nonSpaces.empty()) {
		mw = std::max(addWord(output, line, lineWidth, spaces, nonSpaces), mw);
	}

	//Output the remaining text.
	output.push_back(line);

	return mw;
}

int WordWrapper::addLines(std::vector<std::string>& output, const std::vector<std::string>& input) {
	int mw = 0;
	for (const std::string& s : input) {
		mw = std::max(addLine(output, s), mw);
	}
	return mw;
}
