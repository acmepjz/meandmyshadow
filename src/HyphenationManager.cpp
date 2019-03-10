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

#include "HyphenationManager.h"
#include "FileManager.h"
#include "Globals.h"

HyphenationManager hyphenationMgr;

HyphenationManager::HyphenationManager()
	: currentHyphenator(NULL)
{
}

HyphenationManager::~HyphenationManager() {
	for (auto it = hyphenators.begin(); it != hyphenators.end(); ++it) {
		delete it->second;
	}
}

void HyphenationManager::languageChanged() {
	currentHyphenator = NULL;
}

Hyphenate::Hyphenator* HyphenationManager::getHyphenator() {
	if (currentHyphenator == NULL) {
		auto files = enumAllFiles(getDataPath() + "hyphenate/");
		auto lng = dictionaryManager->get_language();

		std::string bestFileName = "en";
		int bestScore = 0;

		for (auto fn : files) {
			auto lng2 = tinygettext::Language::from_env(fn);
			int score = tinygettext::Language::match(lng, lng2);
			if (score > bestScore) {
				bestScore = score;
				bestFileName = fn;
			}
		}

		currentHyphenator = getHyphenator(bestFileName);
	}

	return currentHyphenator;
}

Hyphenate::Hyphenator* HyphenationManager::getHyphenator(const std::string& fileName) {
	auto it = hyphenators.find(fileName);
	if (it != hyphenators.end()) return it->second;

	auto hyphenator = new Hyphenate::Hyphenator((getDataPath() + "hyphenate/" + fileName).c_str());
	hyphenators[fileName] = hyphenator;
	return hyphenator;
}

HyphenationManager* getHyphenationManager() {
	return &hyphenationMgr;
}
