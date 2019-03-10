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
#ifndef HYPHENATE_HYPHENATOR_H
#define HYPHENATE_HYPHENATOR_H

#include <map>
#include <string>
#include <memory>
#include <vector>

namespace Hyphenate {
	class HyphenationTree;
	class HyphenationRule;

	class Hyphenator {
	private:
		std::auto_ptr<HyphenationTree> dictionary;

		std::string hyphenate_word
			(const std::string &word, const std::string &hyphen);

	public:
		/** Build a hyphenator from the patterns in the file provided. */
		Hyphenator(const char *filename);

		/** Destructor. */
		~Hyphenator();

		/** The actual workhorse. You'll want to call this function once
		* for each word (NEW: or complete string, not only word. The library
		* will do the word-splitting for you) you want hyphenated.
		*
		*  Usage example:
		*  	Hyphenate::Hyphenator hyphenator(Language("de-DE"));
		*  	hyphenator.hyphenate("Schifffahrt");
		*
		*  	yields "Schiff-fahrt", while
		*
		*  	Hyphenate::Hyphenator hyphenator(Language("en"));
		*  	hyphenator.hyphenate("example", "&shy;");
		*
		*  	yields "ex&shy;am&shy;ple".
		*
		*  \param word A single UTF-8 encoded word to be hyphenated.
		*  \param hyphen The string to put at each possible
		*                hyphenation point. The default is an ASCII dash.
		*/
		std::string hyphenate
			(const std::string &word,
			const std::string &hyphen = "-");

		/** Find a single hyphenation point in the string so that the first
		 *  part (including a hyphen) will be shorter or equal in length
		 *  to the parameter len. If this is not possible, choose the shortest
		 *  possible string.
		 *
		 *  The first element is the result, the second element the rest of
		 *  the string.
		 *
		 *  Example: To format a piece of text to width 60, use the following
		 *  loop:
		 *  string rest = text;
		 *  string result = "";
		 *  while ( ! rest.empty() ) {
		 *     pair<string,string> p = your_hyphenator.hyphenate_at(rest);
		 *     result += p.first + "\n"
		 *     rest = p.second;
		 *  }
		 **/
		std::pair<std::string, std::string> hyphenate_at
			(const std::string &word,
			const std::string &hyphen = "-",
			size_t len = std::string::npos);

		/** Just apply the hyphenation patterns to the word, but don't
		 *  hyphenate anything.
		 *
		 *  \returns A vector with the same size as the word with a non-NULL
		 *           entry for every hyphenation point. */
		std::auto_ptr<std::vector<const HyphenationRule*> >
			applyHyphenationRules(const std::string& word);
	};
}

#endif
