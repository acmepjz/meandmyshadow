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

#include "RecordPlayback.h"
#include "TreeStorageNode.h"
#include "POASerializer.h"
#include "InputManager.h"
#include "Functions.h"
#include "Render.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <algorithm>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <SDL_ttf_fontfallback.h>

#include "libs/tinyformat/tinyformat.h"

const int MAX_MOUSE_IDLE_TIME = 1 * 40;
const int MAX_RECENT_FRAMES = 64;
const int MAX_FRAME_PER_TICK = 200;

using namespace std;

RecordPlayback::RecordPlayback(SDL_Renderer& renderer, ImageManager& imageManager)
	: Game(renderer, imageManager)
	, lastMouseX(-1), lastMouseY(-1), mouseIdleTime(-MAX_MOUSE_IDLE_TIME)
	, replaySpeed(0x10), replayIdleTime(0), replayPaused(false)
	, clickedTime(-1), loadThisNextTime(NULL)
{
	guiTexture = imageManager.loadTexture(getDataPath() + "gfx/gui.png", renderer);

	objThemes.getCharacter(false)->createInstance(&walkingAnimation, "walkright");

	// We always show the cursor since Game hides it first.
	SDL_ShowCursor(SDL_ENABLE);
}

RecordPlayback::~RecordPlayback() {
}

enum RecordPlaybackButtons {
	BTN_RESTART,
	BTN_REWIND,
	BTN_PREV_FRAME,
	BTN_PLAY,
	BTN_NEXT_FRAME,
	BTN_FAST_FORWARD,
	BTN_SLOWER,
	BTN_FASTER,
};

void RecordPlayback::handleEvents(ImageManager& imageManager, SDL_Renderer& renderer) {
	// First let Game process events.
	Game::handleEvents(imageManager, renderer);

	int clickedButton = -1, t = -1;

	const InputManagerKeys keys[8] = {
		INPUTMGR_RESTART,
		INPUTMGR_LEFT,
		INPUTMGR_PREVIOUS,
		INPUTMGR_SPACE,
		INPUTMGR_NEXT,
		INPUTMGR_RIGHT,
		INPUTMGR_UP,
		INPUTMGR_DOWN,
	};

	for (int i = 0; i < 8; i++) {
		if (inputMgr.isKeyDownEvent(keys[i])) {
			clickedButton = i;
			break;
		}
	}

	if (event.type == SDL_MOUSEBUTTONDOWN) {
		const SDL_Rect r = { 100, SCREEN_HEIGHT - 144, SCREEN_WIDTH - 200, 72 };
		const SDL_Point p = { event.button.x, event.button.y };
		if (mouseIdleTime < 0 && SDL_PointInRect(&p, &r)) {
			if (p.y < r.y + 24) {
			} else if (p.y < r.y + 40) {
				// clicked the slider
				if (p.x >= r.x + 8 && p.x < r.x + r.w - 8) {
					int x = p.x - r.x - 12;
					if (x < 0) x = 0; else if (x > r.w - 24) x = r.w - 24;

					const int maxTime = player.getRecord()->size();

					t = int(float(x) * float(maxTime) / float(r.w - 24) + 0.5f);
					if (t < 0) t = 0;
					else if (t > maxTime) t = maxTime;

					mouseIdleTime = -MAX_MOUSE_IDLE_TIME;
				}
			} else if (p.y < r.y + 64) {
				// clicked the button bar
				if (p.x >= r.x + 8 && p.x < r.x + 152) {
					clickedButton = (p.x - r.x - 8) / 24;
				} else if (p.x < r.x + r.w - 104) {
				} else if (p.x < r.x + r.w - 80) {
					clickedButton = BTN_SLOWER;
				} else if (p.x < r.x + r.w - 32) {
				} else if (p.x < r.x + r.w - 8) {
					clickedButton = BTN_FASTER;
				}
			}
		} else {
			clickedButton = BTN_PLAY;
		}
	}

	if (clickedButton >= 0) {
		switch (clickedButton) {
		case BTN_RESTART:
			t = 0;
			break;
		case BTN_REWIND:
			t = time - ((time + 20) % 40) - 20;
			if (t < 0) t = 0;
			break;
		case BTN_PREV_FRAME:
			t = time - 1;
			if (t < 0) t = 0;
			break;
		case BTN_PLAY:
			replayPaused = !replayPaused;
			break;
		case BTN_NEXT_FRAME:
			t = time + 1;
			break;
		case BTN_FAST_FORWARD:
			t = time - ((time + 20) % 40) + 60;
			break;
		case BTN_SLOWER:
			if (replaySpeed > 0x4) replaySpeed >>= 1;
			break;
		case BTN_FASTER:
			if (replaySpeed < 0x40) replaySpeed <<= 1;
			break;
		}
		mouseIdleTime = -MAX_MOUSE_IDLE_TIME;
	}

	if (t >= 0 && t != time) {
		if (t > 0) clickedTime = t;
		else restart();
	}
}

void RecordPlayback::restart() {
	player.loadStateInternal();
	shadow.loadStateInternal();
	loadGameOnlyStateInternal();

	player.playRecord();
	shadow.playRecord(); //???

	eventQueue.clear();

	won = false;
}

void RecordPlayback::logic(ImageManager& imageManager, SDL_Renderer& renderer) {
	// The number of ticks to advance.
	int m = 0;

	if (clickedTime > 0 && clickedTime != time) {
		if (clickedTime > time) { // fast forward
			if (won) {
				// the game replay is finished so we can't fast forward anymore
				clickedTime = -1;
			} else {
				// find a nearest cached frame if available
				loadThisNextTime = NULL;
				int t = -1;
				for (auto it = cachedFrames.begin(); it != cachedFrames.end(); ++it) {
					if (it->first > time && it->first <= clickedTime) {
						t = it->first;
						loadThisNextTime = &it->second;
					}
				}

				if (loadThisNextTime) {
					// we found a suitable cached state, load it through logic().
					Game::logic(imageManager, renderer);

					// sanity check.
					assert(time == t);
				}

				m = clickedTime - time;

				// set a maximal advance of frames per tick to prevent game lag.
				if (m > MAX_FRAME_PER_TICK) m = MAX_FRAME_PER_TICK;
				else clickedTime = -1;
			}
		} else { // rewind
			// find a nearest cached frame if available
			loadThisNextTime = NULL;
			int t = -1;
			for (auto it = cachedFrames.begin(); it != cachedFrames.end(); ++it) {
				if (it->first <= clickedTime) {
					t = it->first;
					loadThisNextTime = &it->second;
				}
			}

			if (loadThisNextTime) {
				// we found a suitable cached state, load it through logic().
				Game::logic(imageManager, renderer);

				// sanity check.
				assert(time == t);
			} else {
				// otherwide we need to restart from the beginning.
				restart();
			}

			m = clickedTime - time;

			// set a maximal advance of frames per tick to prevent game lag.
			if (m > MAX_FRAME_PER_TICK) m = MAX_FRAME_PER_TICK;
			else clickedTime = -1;
		}
	} else {
		clickedTime = -1;
	}

	if (m != 0) {
		replayIdleTime = 0;
	} else {
		if (won || replayPaused) {
			replayIdleTime = 0;
		} else if (replaySpeed >= 0x10) {
			m = replaySpeed >> 4;
			replayIdleTime = 0;
		} else {
			replayIdleTime += replaySpeed;
			if (replayIdleTime >= 0x10) {
				m = 1;
				replayIdleTime = 0;
			}
		}
	}

	// Let Game process logic.
	for (int i = 0; i < m; i++) {
		if (won) break;
		if (i == m - 1) saveStateNextTime = true;
		Game::logic(imageManager, renderer);
	}
}

void RecordPlayback::checkSaveLoadState() {
	assert(time > 0 && eventQueue.empty());

	if (loadThisNextTime) {
		player.loadStateInternal(&loadThisNextTime->playerSaved);
		shadow.loadStateInternal(&loadThisNextTime->shadowSaved);
		loadGameOnlyStateInternal(&loadThisNextTime->gameSaved);

		player.playRecord(time - 1); //???
		shadow.playRecord(); //???

		won = false;
	} else {
		bool isKeyframe = (time % 40) == 0;
		if ((isKeyframe || saveStateNextTime) && cachedFrames.find(time) == cachedFrames.end()) {
			if (!isKeyframe) {
				if ((int)recentFrames.size() >= MAX_RECENT_FRAMES) {
					auto it = cachedFrames.find(recentFrames.front());
					if (it != cachedFrames.end()) cachedFrames.erase(it);
					recentFrames.pop();
				}
				recentFrames.push(time);
			}

			GameSaveState &state = cachedFrames[time];
			player.saveStateInternal(&state.playerSaved);
			shadow.saveStateInternal(&state.shadowSaved);
			saveGameOnlyStateInternal(&state.gameSaved);
		}
	}

	loadThisNextTime = NULL;
	saveStateNextTime = false;
}

void RecordPlayback::render(ImageManager& imageManager, SDL_Renderer& renderer) {
	// First let Game render.
	Game::render(imageManager, renderer);

	const SDL_Rect r = { 100, SCREEN_HEIGHT - 144, SCREEN_WIDTH - 200, 72 };
	SDL_Point p;

	// Check if mouse moves (FIXME: logic in render function)
	SDL_GetMouseState(&p.x, &p.y);
	if (lastMouseX != p.x || lastMouseY != p.y || SDL_PointInRect(&p, &r)) {
		lastMouseX = p.x;
		lastMouseY = p.y;
		mouseIdleTime = -MAX_MOUSE_IDLE_TIME;
	}

	// Now render the playback control.
	if (mouseIdleTime < 0) {
		std::string newToolTip;
		SDL_Rect toolTipRect = {};

		const SDL_Color fg = objThemes.getTextColor(true);
		const int alpha = std::max(mouseIdleTime, -15) * (-17);
		const int alpha2 = std::max(mouseIdleTime, -15) * (-10);

		drawGUIBox(r.x, r.y, r.w, r.h, renderer, 0xFFFFFF00 | alpha2, true);

		int y = r.y + 8;
		const int maxTime = player.getRecord()->size();
		const int tmp = time ^ (maxTime << 16) ^ 42;

		// Draw time text. FIXME: here we screwed up Game::timeTexture.
		if (timeTexture.needsUpdate(tmp)) {
			timeTexture.update(tmp, textureUniqueFromSurface(renderer, SurfacePtr(TTF_RenderUTF8_Blended(fontMono,
				tfm::format("%02d:%05.2f / %02d:%05.2f", time / 2400, (time % 2400) / 40.0, maxTime / 2400, (maxTime % 2400) / 40.0).c_str(),
				fg))));
		}
		SDL_SetTextureAlphaMod(timeTexture.get(), (Uint8)alpha);
		applyTexture(r.x + 8, y, *timeTexture.get(), renderer);
		y += 16;

		// Get the tool tip text of slider if mouse is over
		if (p.y >= y && p.y < y + 24 && p.x >= r.x + 8 && p.x < r.x + r.w - 8) {
			int x = p.x - r.x - 12;
			if (x < 0) x = 0; else if (x > r.w - 24) x = r.w - 24;
			toolTipRect = SDL_Rect{ r.x + x + 8, y, 8, -1 };

			int t = int(float(x) * float(maxTime) / float(r.w - 24) + 0.5f);
			if (t < 0) t = 0; else if (t > maxTime) t = maxTime;
			newToolTip = tfm::format("%02d:%05.2f", t / 2400, (t % 2400) / 40.0);
		}

		// Draw slider.
		int pos = maxTime > 0 ? int(float(r.w - 24) * float(std::min(time, maxTime)) / float(maxTime) + 0.5f) : 0;
		drawGUIBox(r.x + 8, y + 6, pos + 2, 4, renderer, 0x80808000 | alpha, true);
		drawGUIBox(r.x + pos + 14, y + 6, r.w - pos - 22, 4, renderer, 0xFFFFFF00 | alpha, true);
		drawGUIBox(r.x + pos + 8, y, 8, 16, renderer, 0xFFFFFF00 | alpha, true);
		y += 16;

		// Get the tool tip text of buttons if mouse is over
		if (p.y >= y && p.y < y + 24) {
			if (p.x < r.x + 8){
			} else if (p.x < r.x + 32) {
				newToolTip = _("Restart");
				newToolTip += " (" + InputManagerKeyCode::describeTwo(inputMgr.getKeyCode(INPUTMGR_RESTART, false), inputMgr.getKeyCode(INPUTMGR_RESTART, true)) + ")";
				toolTipRect = SDL_Rect{ r.x + 8, y, 24, 24 };
			} else if (p.x < r.x + 56) {
				newToolTip = _("Rewind");
				newToolTip += " (" + InputManagerKeyCode::describeTwo(inputMgr.getKeyCode(INPUTMGR_LEFT, false), inputMgr.getKeyCode(INPUTMGR_LEFT, true)) + ")";
				toolTipRect = SDL_Rect{ r.x + 32, y, 24, 24 };
			} else if (p.x < r.x + 80) {
				newToolTip = _("Step back");
				newToolTip += " (" + InputManagerKeyCode::describeTwo(inputMgr.getKeyCode(INPUTMGR_PREVIOUS, false), inputMgr.getKeyCode(INPUTMGR_PREVIOUS, true)) + ")";
				toolTipRect = SDL_Rect{ r.x + 56, y, 24, 24 };
			} else if (p.x < r.x + 104) {
				newToolTip = replayPaused ? _("Play") : _("Pause");
				newToolTip += " (" + InputManagerKeyCode::describeTwo(inputMgr.getKeyCode(INPUTMGR_SPACE, false), inputMgr.getKeyCode(INPUTMGR_SPACE, true)) + ")";
				toolTipRect = SDL_Rect{ r.x + 80, y, 24, 24 };
			} else if (p.x < r.x + 128) {
				newToolTip = _("Step forward");
				newToolTip += " (" + InputManagerKeyCode::describeTwo(inputMgr.getKeyCode(INPUTMGR_NEXT, false), inputMgr.getKeyCode(INPUTMGR_NEXT, true)) + ")";
				toolTipRect = SDL_Rect{ r.x + 104, y, 24, 24 };
			} else if (p.x < r.x + 152) {
				newToolTip = _("Fast forward");
				newToolTip += " (" + InputManagerKeyCode::describeTwo(inputMgr.getKeyCode(INPUTMGR_RIGHT, false), inputMgr.getKeyCode(INPUTMGR_RIGHT, true)) + ")";
				toolTipRect = SDL_Rect{ r.x + 128, y, 24, 24 };
			} else if (p.x < r.x + r.w - 104) {
			} else if (p.x < r.x + r.w - 80) {
				newToolTip = _("Slower");
				newToolTip += " (" + InputManagerKeyCode::describeTwo(inputMgr.getKeyCode(INPUTMGR_UP, false), inputMgr.getKeyCode(INPUTMGR_UP, true)) + ")";
				toolTipRect = SDL_Rect{ r.x + r.w - 104, y, 24, 24 };
			} else if (p.x < r.x + r.w - 32) {
			} else if (p.x < r.x + r.w - 8) {
				newToolTip = _("Faster");
				newToolTip += " (" + InputManagerKeyCode::describeTwo(inputMgr.getKeyCode(INPUTMGR_DOWN, false), inputMgr.getKeyCode(INPUTMGR_DOWN, true)) + ")";
				toolTipRect = SDL_Rect{ r.x + r.w - 32, y, 24, 24 };
			}
			
			if (toolTipRect.h > 0) {
				drawGUIBox(toolTipRect.x, toolTipRect.y, toolTipRect.w, toolTipRect.h, renderer, 0xFFFFFF00 | alpha, true);
			}
		}

		// Set the alpha of gui texture
		SDL_SetTextureAlphaMod(guiTexture.get(), (Uint8)alpha);

		// Draw buttons.
		SDL_Rect srcrect = { 32, 80, 16, 16 };
		SDL_Rect dstrect = { r.x + 12, y + 4, 16, 16 };
		SDL_RenderCopy(&renderer, guiTexture.get(), &srcrect, &dstrect); // restart
		srcrect.x += 16; dstrect.x += 24;
		SDL_RenderCopy(&renderer, guiTexture.get(), &srcrect, &dstrect); // rewind
		srcrect.x += 16; dstrect.x += 24;
		SDL_RenderCopy(&renderer, guiTexture.get(), &srcrect, &dstrect); // previous frame
		srcrect.x = replayPaused ? 96 : 112; srcrect.y = 64; dstrect.x += 24;
		SDL_RenderCopy(&renderer, guiTexture.get(), &srcrect, &dstrect); // play/pause
		srcrect.x = 80; srcrect.y = 80; dstrect.x += 24;
		SDL_RenderCopy(&renderer, guiTexture.get(), &srcrect, &dstrect); // next frame
		srcrect.x += 16; dstrect.x += 24;
		SDL_RenderCopy(&renderer, guiTexture.get(), &srcrect, &dstrect); // fast forward
		srcrect.x = 48; srcrect.y = 80; dstrect.x = r.x + r.w - 100;
		SDL_RenderCopy(&renderer, guiTexture.get(), &srcrect, &dstrect); // slower
		srcrect.x = 96; dstrect.x += 72;
		SDL_RenderCopy(&renderer, guiTexture.get(), &srcrect, &dstrect); // faster

		// Draw replay speed.
		if (replaySpeedTexture.needsUpdate(replaySpeed)) {
			replaySpeedTexture.update(replaySpeed, textureUniqueFromSurface(renderer, SurfacePtr(TTF_RenderUTF8_Blended(fontMono,
				tfm::format("%gx", float(replaySpeed) / 16.0f).c_str(),
				fg))));
		}
		SDL_Rect r1 = rectFromTexture(replaySpeedTexture);
		SDL_SetTextureAlphaMod(replaySpeedTexture.get(), (Uint8)alpha);
		applyTexture(r.x + r.w - 80 + (48 - r1.w) / 2, y + (24 - r1.h) / 2, *replaySpeedTexture.get(), renderer);

		// Reset alpha of gui texture since it is global
		SDL_SetTextureAlphaMod(guiTexture.get(), 255);

		y += 24;

		// Show tool tip if necessary.
		if (!newToolTip.empty()) {
			if (toolTipTexture.needsUpdate(newToolTip)) {
				toolTipTexture.update(newToolTip, textureUniqueFromSurface(renderer, SurfacePtr(TTF_RenderUTF8_Blended(fontText,
					newToolTip.c_str(), fg))));
			}
			SDL_Rect r1 = rectFromTexture(toolTipTexture);
			toolTipRect.y = toolTipRect.y - r1.h - 6;
			toolTipRect.w = r1.w + 4; toolTipRect.h = r1.h + 4;
			if (toolTipRect.x + toolTipRect.w > SCREEN_WIDTH - 2) toolTipRect.x = SCREEN_WIDTH - 2 - toolTipRect.w;

			drawGUIBox(toolTipRect.x, toolTipRect.y, toolTipRect.w, toolTipRect.h, renderer, 0xFFFFFF00 | alpha, true);

			SDL_SetTextureAlphaMod(toolTipTexture.get(), (Uint8)alpha);
			applyTexture(toolTipRect.x + 2, toolTipRect.y + 2, *toolTipTexture.get(), renderer);
		}
	}

	// Advance the mouse idle time. (FIXME: logic in render function)
	mouseIdleTime++;

	// Render the busy animation.
	if (clickedTime > 0) {
		drawGUIBox(SCREEN_WIDTH / 2 - 25, SCREEN_HEIGHT / 2 - 25, 50, 50, renderer, 0xFFFFFF00 | 150, true);
		walkingAnimation.draw(renderer, SCREEN_WIDTH / 2 - 11, SCREEN_HEIGHT / 2 - 20);
		walkingAnimation.updateAnimation();
	}

	// Render the replay finished tooltip.
	if (won) {
		// FIXME: here we screwed up Game::bmTipsRestartCheckpoint.
		// This shouldn't cause bugs since when playing from record, the save functionality is disabled.
		if (bmTipsRestartCheckpoint == NULL){
			//Draw string
			bmTipsRestartCheckpoint = textureFromText(renderer, *fontText,
				_("Game replay is done."),
				objThemes.getTextColor(true));
		}

		SDL_Texture* bm = bmTipsRestartCheckpoint.get();

		//Draw the tip.
		if (bm != NULL){
			const SDL_Rect textureSize = rectFromTexture(*bm);
			int x = (SCREEN_WIDTH - textureSize.w) / 2;
			int y = 32;
			drawGUIBox(x - 8, y - 8, textureSize.w + 16, textureSize.h + 14, renderer, 0xFFFFFFFF);
			applyTexture(x, y, *bm, renderer);
		}
	}
}

void RecordPlayback::loadRecord(ImageManager& imageManager, SDL_Renderer& renderer, const char* fileName, const char* levelFileName) {
	//Create a TreeStorageNode that will hold the loaded data.
	TreeStorageNode obj;
	{
		POASerializer objSerializer;

		//Parse the file.
		if(!objSerializer.loadNodeFromFile(fileName,&obj,true)){
			cerr<<"ERROR: Can't load record file "<<fileName<<endl;
			return;
		}
	}

	//Load the seed of psuedo-random number generator.
	prngSeed.clear();
	{
		auto it = obj.attributes.find("seed");
		if (it != obj.attributes.end() && !it->second.empty()) {
			prngSeed = it->second[0];
		}
	}

	bool loaded=false;

	//substitute the level if the level file name is specified.
	if (levelFileName) {
		TreeStorageNode *obj = new TreeStorageNode();
		if (!POASerializer().loadNodeFromFile(levelFileName, obj, true)) {
			cerr << "ERROR: Can't load level file " << levelFileName << endl;
			delete obj;
		} else {
			//also replace the script by the external one.
			std::string s = levelFileName;
			size_t lp = s.find_last_of('.');
			if (lp != std::string::npos) {
				s = s.substr(0, lp);
			}
			s += ".lua";

			loadLevelFromNode(imageManager, renderer, obj, "?record?", s);
			loaded = true;
		}
	} else {
		//find the node named 'map'.
		for (unsigned int i = 0; i < obj.subNodes.size(); i++) {
			if (obj.subNodes[i]->name == "map") {
				//load the script from record file.
				std::string s;
				auto it = obj.attributes.find("script");
				if (it != obj.attributes.end() && !it->second.empty()) {
					s = "?" + it->second[0];
				}

				//load the level. (fileName=???)
				loadLevelFromNode(imageManager, renderer, obj.subNodes[i], "?record?", s);
				//remove this node to prevent delete it.
				obj.subNodes[i] = NULL;
				//over
				loaded = true;
				break;
			}
		}
	}

	if(!loaded){
		cerr<<"ERROR: Can't find subnode named 'map' from record file"<<endl;
		return;
	}

	//load the record.
	{
		vector<int> *record=player.getRecord();
		record->clear();
		vector<string> &v=obj.attributes["record"];
		for(unsigned int i=0;i<v.size();i++){
			string &s=v[i];
			string::size_type pos=s.find_first_of('*');
			if(pos==string::npos){
				//1 item only.
				int i=atoi(s.c_str());
				record->push_back(i);
			}else{
				//contains many items.
				int i=atoi(s.substr(0,pos).c_str());
				int j=atoi(s.substr(pos+1).c_str());
				for(;j>0;j--){
					record->push_back(i);
				}
			}
		}
	}

#ifdef RECORD_FILE_DEBUG
	//load the debug data
	{
		vector<string> &v=obj.attributes["recordPlayerPosition"];
		vector<SDL_Rect> &playerPosition=player.playerPosition();
		playerPosition.clear();
		if(!v.empty()){
			if(!v[0].empty()){
				stringstream st(v[0]);
				int m;
				st>>m;
				for(int i=0;i<m;i++){
					SDL_Rect r;
					st>>r.x>>r.y;
					r.w=0;
					r.h=0;
					playerPosition.push_back(r);
				}
			}
		}
	}
#endif

	//some sanity check
	assert(eventQueue.empty());

	//play the record.
	player.playRecord();
	shadow.playRecord(); //???

	//Save the game state for time 0.
	player.saveStateInternal();
	shadow.saveStateInternal();
	saveGameOnlyStateInternal();

	// We always show the cursor since Game hides it first.
	SDL_ShowCursor(SDL_ENABLE);
}
