/* libhyphenate: A TeX-like hyphenation algorithm.
 * Copyright (C) 2007 Steve Wolter
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * If you have any questions, feel free to contact me:
 * http://swolter.sdf1.org
 **/
#include "Hyphenator.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <memory>
#include <ctype.h>
#include <stdlib.h>

#include "HyphenationRule.h"
#include "HyphenationTree.h"

#include "UTF8Functions.h"

#define UTF8_MAX 6

using namespace std;
using namespace Hyphenate;

/** The hyphenation table parser. */
static auto_ptr<HyphenationTree> read_hyphenation_table(const char *filename) {
	ifstream i(filename, fstream::in);
	auto_ptr<HyphenationTree> output(new HyphenationTree());
	output->loadPatterns(i);

	return output;
}

/** Build a hyphenator from the patterns in the file provided. */
Hyphenate::Hyphenator::Hyphenator(const char *filename) {
	dictionary = read_hyphenation_table(filename);
}

Hyphenator::~Hyphenator() {}

std::string Hyphenator::hyphenate
(const std::string &word, const std::string &hyphen)
{
	string result;
	unsigned int word_start = -1;

	/* Go through the input. All non-alpha characters are added to the
	 * output immediately, and words are hyphenated and then added. */
	for (unsigned int i = 0; i < word.size(); i++) {
		/* Skip UTF-8 tail bytes. */
		if ((word[i] & 0xC0) == 0x80)
			;
		else {
			bool isalpha = utf32IsAlpha(utf8GetCharacter(word.c_str() + i));

			if (word_start == string::npos && isalpha)
				word_start = i;
			else if (word_start != string::npos && !isalpha) {
				result +=
					hyphenate_word(word.substr(word_start, i - word_start), hyphen);
				word_start = string::npos;
			}
		}

		if (word_start == string::npos)
			result += word[i];
	}
	if (word_start != string::npos)
		result += hyphenate_word(word.substr(word_start), hyphen);

	return result;
}

std::string Hyphenator::hyphenate_word
(const std::string &word, const std::string &hyphen)
{
	auto_ptr<vector<const HyphenationRule*> > rules =
		dictionary->applyPatterns(word);

	/* Build our result string. Of course, we _could_ insert characters in
	 * w, but that would be highly inefficient. */
	string result;

	int acc_skip = 0;
	for (unsigned int i = 0; i < word.size(); i++) {
		if ((*rules)[i] != NULL)
			acc_skip += (*rules)[i]->apply(result, hyphen);

		if (acc_skip > 0)
			acc_skip--;
		else
			result += word[i];
	}

	return result;
}

pair<std::string, std::string> Hyphenator::hyphenate_at
(const std::string &src, const std::string &hyphen, size_t len)
{
	/* First of all, find the word which needs to be hyphenated. */
	const char *cur = src.c_str();
	for (unsigned int i = 0; i < len; i++)
		cur = utf8GoToNextCharacter(cur);

	const char *next = cur;
	if (!utf32IsBreakableSpace(utf8GetCharacter(next)))
		next = utf8GoToNextCharacter(next);
	pair<string, string> result;

	if (utf32IsBreakableSpace(utf8GetCharacter(next))) {
		/* We are lucky: There is a space we can hyphenate at. */

		/* We leave no spaces at the end of a line: */
		while (utf32IsBreakableSpace(utf8GetCharacter(cur)))
			cur = utf8GoToPrevCharacter(cur);
		int len = cur - src.c_str() + 1;
		result.first = src.substr(0, len);

		/* Neither do we leave spaces at the beginning of the next. */
		while (utf32IsBreakableSpace(utf8GetCharacter(next)))
			next = utf8GoToNextCharacter(next);
		result.second = src.substr(next - src.c_str());

	} else {
		/* We can hyphenate at hyphenation points in words or at spaces, whatever
		 * comes earlier. We will check all words here in the loop. */
		const char *border = cur;
		while (true) {
			/* Find the start of a word first. */
			bool in_word = utf32IsAlpha(utf8GetCharacter(cur));
			const char *word_start = NULL;
			while (cur > src.c_str()) {
				cur = utf8GoToPrevCharacter(cur);
				int ch = utf8GetCharacter(cur);

				if (in_word && (!utf32IsAlpha(ch))) {
					/* If we have a word, try hyphenating it.*/
					word_start = utf8GoToNextCharacter(cur);
					break;
				} else if (utf32IsBreakableSpace(ch)) {
					break;
				} else if (!in_word && utf32IsAlpha(ch))
					in_word = true;

				if (cur == src.c_str() && in_word)
					word_start = cur;
			}

			/* There are two reasons why we may have left the previous loop with-
			 * out result:
			 * Either because our word goes all the way to the first character,
			 * or because we found whitespace. */
			/* In the first case, there is nothing really hyphenateable. */
			if (word_start != NULL) {
				/* We have the start of a word, now look for the character after
				* the end. */
				const char *word_end = word_start;
				while (utf32IsAlpha(utf8GetCharacter(word_end)))
					word_end = utf8GoToNextCharacter(word_end);

				/* Build the substring consisting of the word. */
				string word;
				for (const char *i = word_start; i < word_end; i++)
					word += *i;

				/* Hyphenate the word. */
				auto_ptr<vector<const HyphenationRule*> > rules =
					dictionary->applyPatterns(word);

				/* Determine the index of the latest hyphenation that will still
				* fit. */
				int latest_possible_hyphenation = -1;
				int earliest_hyphenation = -1;
				for (int i = 0; i < (int)rules->size(); i++)
					if ((*rules)[i] != NULL) {
						if (earliest_hyphenation == -1)
							earliest_hyphenation = i;
						if (word_start + i +
							(*rules)[i]->spaceNeededPreHyphen() + hyphen.length()
							<= border)
						{
							if (i > latest_possible_hyphenation) {
								latest_possible_hyphenation = i;
							}
						} else
							break;
					}

				bool have_space = false;
				for (const char *i = src.c_str(); i <= word_start;
					i = utf8GoToNextCharacter(i))
					if (utf32IsBreakableSpace(utf8GetCharacter(i))) {
						have_space = true;
						break;
					}
				if (latest_possible_hyphenation == -1 && !have_space)
					latest_possible_hyphenation = earliest_hyphenation;

				/* Apply the best hyphenation, if any. */
				if (latest_possible_hyphenation >= 0) {
					int i = latest_possible_hyphenation;
					result.first = src.substr(0, word_start - src.c_str() + i);
					(*rules)[i]->apply_first(result.first, hyphen);
					int skip = (*rules)[i]->apply_second(result.second);
					const char *after_hyphen = word_start + i + skip;
					result.second += string(after_hyphen);
					break;
				}
			}

			if (cur == src.c_str()) {
				/* We cannot hyphenate at all, so leave the first block standing
				 * and move to its end. */
				const char *eol = cur;
				while (*eol != 0 && !utf32IsBreakableSpace(utf8GetCharacter(eol)))
					eol = utf8GoToNextCharacter(eol);

				result.first = src.substr(0, eol - src.c_str() + 1);
				while (*eol != 0 && utf32IsBreakableSpace(utf8GetCharacter(eol)))
					eol = utf8GoToNextCharacter(eol);
				result.second = string(eol);
				break;
			} else if (utf32IsBreakableSpace(utf8GetCharacter(cur))) {
				/* eol is the end of the previous line, bol the start of the
				   * next. */
				const char *eol = cur, *bol = cur;
				while (utf32IsBreakableSpace(utf8GetCharacter(eol)))
					eol = utf8GoToPrevCharacter(eol);
				while (utf32IsBreakableSpace(utf8GetCharacter(bol)))
					bol = utf8GoToNextCharacter(bol);

				result.first = src.substr(0, eol - src.c_str() + 1);
				result.second = string(bol);
				break;
			}
		}
	}

	return result;
}

std::auto_ptr<std::vector<const HyphenationRule*> >
Hyphenate::Hyphenator::applyHyphenationRules(const std::string& word)
{
	return dictionary->applyPatterns(word);
}
