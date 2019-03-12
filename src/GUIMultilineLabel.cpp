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

#include "GUIMultilineLabel.h"
#include "ThemeManager.h"

void GUIMultilineLabel::refreshCache() {
	//Check if the caption changed or the caption, if so we need to clear the (old) cache.
	if (caption.compare(cachedCaption) != 0 || width <= 0 || width != cachedWidth || gravity != cachedGravity) {
		//TODO: Only change alpha if only enabled changes.
		//Free the cache.
		cacheTex.reset(nullptr);

		//And cache the new values.
		cachedEnabled = enabled;
		cachedCaption = caption;
		cachedWidth = width;
		cachedGravity = gravity;

		//Finally resize the widget
		if (autoWidth)
			width = -1;
	}
}

bool GUIMultilineLabel::handleEvents(SDL_Renderer&, int, int, bool, bool, bool processed){
	return processed;
}

void GUIMultilineLabel::render(SDL_Renderer& renderer, int x, int y, bool draw){
	//There's no need drawing the widget when it's invisible.
	if (!visible)
		return;

	//Check if the enabled state changed or the caption, if so we need to clear the (old) cache.
	refreshCache();

	//Get the absolute x and y location.
	x += left;
	y += top;

	//Update cache if needed.
	if (!cacheTex){
		SDL_Color color = objThemes.getTextColor(inDialog);

		std::vector<std::string> lines;

		bool temp = wrapper.wordWrap;
		wrapper.font = fontText;
		wrapper.maxWidth = width;
		if (width <= 0) wrapper.wordWrap = false;
		wrapper.addString(lines, caption);
		wrapper.wordWrap = temp;

		cacheTex = textureFromMultilineText(renderer, *fontText, lines, color, gravity);
		
		const SDL_Rect size = rectFromTexture(cacheTex);
		realWidth = size.w;
		realHeight = size.h;
		if (width <= 0) width = cachedWidth = size.w;
		if (height <= 0) height = size.h;
	}

	//Align the text properly and draw it.
	if (draw) {
		const SDL_Rect size = rectFromTexture(cacheTex);
		if (gravity == GUIGravityCenter)
			gravityX = (width - size.w) / 2;
		else if (gravity == GUIGravityRight)
			gravityX = width - size.w;
		else
			gravityX = 0;

		y += (height - size.h) / 2;
		x += gravityX;
		applyTexture(x, y, cacheTex, renderer);
	}
}
