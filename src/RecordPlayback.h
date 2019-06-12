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

#ifndef RECORDPLAYBACK_H
#define RECORDPLAYBACK_H

#include "Game.h"
#include "ThemeManager.h"

class FrameCache;

class RecordPlayback : public Game {
protected:
	int lastMouseX, lastMouseY, mouseIdleTime;

	// The replay speed. 0x10 means 1.0x, 0x20 means 2.0x, 0x08 means 0.5x, etc.
	int replaySpeed;

	// The replay idle time. When replaySpeed < 0x10 then each time it will be added to replayIdleTime,
	// and when replayIdleTime >= 0x10 the game tick increases.
	int replayIdleTime;

	// Boolean indicating if the replay is paused.
	bool replayPaused;

	//Texture containing the label of the replay speed.
	CachedTexture<int> replaySpeedTexture;

	//Texture containing the tool tip of buttons.
	CachedTexture<std::string> toolTipTexture;

	//Texture of gui.
	SharedTexture guiTexture;

	//The cached frames.
	FrameCache* cachedFrames;

	//The time we are going to jump to. Should be >0, otherwise it will be ignored.
	int clickedTime;

	//The walking animation used when navigating.
	ThemeBlockInstance walkingAnimation;

	//State that is set when we should load it on next logic update.
	GameSaveState *loadThisNextTime;

	//Max frame per tick in fast-forward mode. Will be measured dynamically.
	int maxFramePerTick;

	//The current and old camera mode when paused.
	CameraMode pausedCameraMode, oldCameraMode;

	//The current and old target for the camera when paused.
	SDL_Point pausedCameraTarget, oldCameraTarget;

	//Restart the game from time 0.
	void restart();

public:
	//Constructor.
	RecordPlayback(SDL_Renderer& renderer, ImageManager& imageManager);

	//Destructor.
	//It will call destroy();
	~RecordPlayback();

	//Inherited from GameState.
	void handleEvents(ImageManager& imageManager, SDL_Renderer& renderer) override;
	void logic(ImageManager& imageManager, SDL_Renderer& renderer) override;
	void render(ImageManager& imageManager, SDL_Renderer& renderer) override;

	//Check if we should save/load state, override it to provide seeking support.
	void checkSaveLoadState() override;

	//Load game record (and its level) from file and play it.
	//fileName: The filename of the recording file.
	//levelFileName: (Optional) The file name of the level file. It's only used when you want to test if the record works for another level.
	void loadRecord(ImageManager& imageManager, SDL_Renderer& renderer, const char* fileName, const char* levelFileName = NULL);
};

#endif
