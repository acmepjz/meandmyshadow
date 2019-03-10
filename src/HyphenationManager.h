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

#ifndef HYPHENATIONMANAGER_H
#define HYPHENATIONMANAGER_H

#include "Hyphenator.h"
#include <map>
#include <string>

class HyphenationManager {
private:
	std::map<std::string, Hyphenate::Hyphenator*> hyphenators;
	Hyphenate::Hyphenator *currentHyphenator;

public:
	HyphenationManager();
	~HyphenationManager();

	//Call this when language changed, which will reset the default hyphenator.
	void languageChanged();

	//Get the hyphenator for the current language.
	Hyphenate::Hyphenator* getHyphenator();

	//Get the hyphenator for the specified file.
	Hyphenate::Hyphenator* getHyphenator(const std::string& fileName);
};

HyphenationManager* getHyphenationManager();

#endif
