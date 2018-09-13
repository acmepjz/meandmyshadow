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

#include "FontManager.h"
#include "FileManager.h"
#include "TreeStorageNode.h"
#include "POASerializer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL_ttf.h>

FontManager* fontMgr = NULL;

FontManager::FontManager() {
}

FontManager::~FontManager() {
	for (auto it = fonts.begin(); it != fonts.end(); ++it) {
		for (auto font : it->second) {
			if (font) TTF_CloseFont(font);
		}
	}
}

static TTF_Font* loadFont(const std::string& name, int size){
	TTF_Font* tmpFont;
	if (name.find('.') != std::string::npos) {
		tmpFont = TTF_OpenFont((getDataPath() + "font/" + name).c_str(), size);
	} else {
		tmpFont = TTF_OpenFont((getDataPath() + "font/" + name + ".ttf").c_str(), size);
	}
	if (tmpFont) {
		return tmpFont;
	} else{
		printf("ERROR: Unable to load font file '%s'!\n", name.c_str());
#if defined(ANDROID)
		//Android has built-in DroidSansFallback.ttf. (?)
		return TTF_OpenFont("/system/fonts/DroidSansFallback.ttf", size);
#else
		return TTF_OpenFont((getDataPath() + "font/DroidSansFallback.ttf").c_str(), size);
#endif
	}
}

void FontManager::loadFonts() {
	if (!fonts.empty()) return;

	TreeStorageNode root;
	POASerializer serializer;
	if (!serializer.loadNodeFromFile((getDataPath() + "font/fonts.list").c_str(), &root, true)) {
		fprintf(stderr, "FATAL ERROR: Failed to load the font list!\n");
		return;
	}

	for (auto node : root.subNodes) {
		if (node->name == "font" && node->value.size() >= 3) {
			if (fonts.find(node->value[0]) != fonts.end()) {
				printf("WARNING: The font named '%s' already exists!\n", node->value[0].c_str());
				continue;
			}

			const int size = atoi(node->value[2].c_str());
			if (size <= 0) {
				printf("FATAL ERROR: The font named '%s' has invalid size '%d'!\n", node->value[0].c_str(), size);
				continue;
			}

			TTF_Font *mainFont = loadFont(node->value[1], size);

			if (mainFont == NULL) {
				printf("FATAL ERROR: Failed to load the main font file for font named '%s'!\n", node->value[0].c_str());
				continue;
			}

			std::vector<TTF_Font*> &fontVector = fonts[node->value[0]];
			fontVector.push_back(mainFont);

			for (auto subnode : node->subNodes) {
				if (subnode->name == "fallback" && subnode->value.size() >= 1) {
					int newSize = size;
					if (subnode->value.size() >= 2) {
						if (subnode->value[1] == "relative" && subnode->value.size() >= 3) {
							newSize += atoi(subnode->value[2].c_str());
						} else if (subnode->value[1] == "scale" && subnode->value.size() >= 3) {
							double scale = atof(subnode->value[2].c_str());
							newSize = (int)floor(double(size) * scale + 0.5);
						} else {
							newSize = atoi(subnode->value[1].c_str());
						}
					}

					if (newSize <= 0) {
						printf("WARNING: Invalid font size '%d'!\n", newSize);
						continue;
					}

					TTF_Font *fallbackFont = loadFont(subnode->value[0], newSize);

					if (fallbackFont == NULL) continue;

					fontVector.push_back(fallbackFont);
				}
			}

			if (fontVector.size() > 1) {
				TTF_SetFontFallback(mainFont, fontVector.size() - 1, &(fontVector[1]));
			}
		}
	}
}

TTF_Font* FontManager::getFont(const std::string& name) {
	auto it = fonts.find(name);
	if (it == fonts.end() || it->second.empty() || it->second[0] == NULL) {
		fprintf(stderr, "FATAL ERROR: Can't find or failed to load font named '%s'!\n", name.c_str());
		return NULL;
	}
	return it->second[0];
}
