/*
 * Copyright (C) 2018 Me and My Shadow
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

#ifndef FONT_MANAGER_H
#define FONT_MANAGER_H

// The forward declaration of TTF_Font is clunky like this
// as it's forward declared like this in SDL_ttf_fontfallback.h
struct _TTF_Font;
typedef struct _TTF_Font TTF_Font;

#include <string>
#include <vector>
#include <map>

class FontManager {
private:
	std::map<std::string, std::vector<TTF_Font*> > fonts;

public:
	FontManager();
	~FontManager();

	void loadFonts();

	TTF_Font* getFont(const std::string& name);
};

extern FontManager* fontMgr;

#endif
