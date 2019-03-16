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

#ifndef HELPMANAGER_H
#define HELPMANAGER_H

#include "GUIObject.h"

class GUIWindow;
class HelpPage;

class HelpManager : public GUIEventCallback {
private:
	GUIEventCallback *parent;
	std::vector<HelpPage*> pages;

public:
	HelpManager(GUIEventCallback *parent);
	~HelpManager();

	GUIWindow* newWindow(ImageManager& imageManager, SDL_Renderer& renderer, int pageIndex = 0);

	GUIWindow* newSearchWindow(ImageManager& imageManager, SDL_Renderer& renderer);

	void GUIEventCallback_OnEvent(ImageManager& imageManager, SDL_Renderer& renderer, std::string name, GUIObject* obj, int eventType) override;
};

#endif
