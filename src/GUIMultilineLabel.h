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

#ifndef GUIMULTILINELABEL_H
#define GUIMULTILINELABEL_H

#include "GUIObject.h"
#include "WordWrapper.h"

class GUIMultilineLabel :public GUIObject {
protected:
	int cachedWidth, cachedGravity;

	void refreshCache();

public:
	//The word wrapper.
	WordWrapper wrapper;

	//The real width and height of the text.
	int realWidth, realHeight;

public:
	GUIMultilineLabel(ImageManager& imageManager, SDL_Renderer& renderer, int left = 0, int top = 0, int width = 0, int height = 0,
		const char* caption = NULL, int value = 0,
		bool enabled = true, bool visible = true, int gravity = 0) :
		GUIObject(imageManager, renderer, left, top, width, height, caption, value, enabled, visible, gravity),
		cachedWidth(-1), cachedGravity(-1), realWidth(0), realHeight(0)
	{}

	void clearCache();

	virtual bool handleEvents(SDL_Renderer&, int = 0, int = 0, bool = true, bool = true, bool processed = false);

	virtual void render(SDL_Renderer &renderer, int x = 0, int y = 0, bool draw = true);
};

#endif
