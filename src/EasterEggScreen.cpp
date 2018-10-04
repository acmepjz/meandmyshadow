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
* Me And My Shadow is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Me and My Shadow.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "EasterEggScreen.h"
#include "SoundManager.h"
#include "Render.h"
#include "Globals.h"
#include "Functions.h"
#include "InputManager.h"
#include <SDL_mixer.h>
#include <SDL_ttf.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <string>
#include <map>

//Define this to enable fake "ls -la" screen.
#define SCAM

#ifdef SCAM

const unsigned int randMaxPlusOne = (unsigned int)(RAND_MAX) + 1;

struct FakeSoundEffectState {
	Sint16 buffer[4096];
	int state;
	int randContext;

	int rand() {
		randContext = (randContext * 1103515245 + 12345) & 0x7FFFFFFF;
		return ((randContext << 15) | (randContext >> 16)) & RAND_MAX;
	}
};

static void SDLCALL fakeSoundEffect(void *udata, Uint8 *stream_, int len) {
	FakeSoundEffectState *state = (FakeSoundEffectState*)udata;
	Sint16 *stream = (Sint16*)stream_;

	int len2 = len / 2; // NOTE: len is in bytes, not in samples !!!
	int len3 = len2;
	if (len3 > sizeof(state->buffer) / sizeof(state->buffer[0])) {
		len3 = sizeof(state->buffer) / sizeof(state->buffer[0]);
	}

	if (state->state & 0xFF) {
		state->state--;
	} else {
		state->state = int(float(state->rand()) / float(randMaxPlusOne) * 9.0f) << 8;
		if (state->state) state->state |= int(float(state->rand()) / float(randMaxPlusOne) * 10.0f);
		else state->state |= int(float(state->rand()) / float(randMaxPlusOne) * 80.0f);

		switch (state->state >> 8) {
		case 1:
			memcpy(state->buffer, stream, len3 * sizeof(Sint16));
			break;
		}
	}

	int i;

	switch (state->state >> 8) {
	case 0:
		break;
	case 1:
		memcpy(stream, state->buffer, len3 * sizeof(Sint16));
		break;
	case 2:
		for (i = 0; i < len2; i++) {
			stream[i] = stream[i] >= 0 ? 0x7FFF : 0x8000;
		}
		break;
	case 3:
		for (i = 1; i < len2; i++) {
			stream[i] ^= stream[i - 1];
		}
		break;
	case 4:
		for (i = 0; i < len2 / 2; i++) {
			std::swap(stream[i], stream[len2 - 1 - i]);
		}
		break;
	case 5:
		for (i = 0; i < len2; i++) {
			bool b = (stream[i] & 0x1000) != 0;
			if (stream[i] < 0) b = !b;
			stream[i] <<= 3;
			if (b) stream[i] = ~stream[i];
		}
		break;
	case 6:
		memcpy(state->buffer, stream, len3 * sizeof(Sint16));
		for (i = 0; i < len3; i += 2) {
			stream[i / 2] = state->buffer[i];
		}
		for (i = 1; i < len3; i += 2) {
			stream[(len3 + i) / 2] = state->buffer[i];
		}
	case 7:
		for (i = 0; i < len2; i++) {
			float f = float(stream[i]);
			stream[i] = Sint32(f * f * f / 1073741824.0f);
		}
		break;
	case 8:
		for (i = 0; i < len2; i++) {
			stream[i] = 0;
		}
		break;
	}
}

FakeSoundEffectState fakeSoundEffectState;

static void scamDrawText(ImageManager& imageManager, SDL_Renderer& renderer,
	const std::map<int, TexturePtr>& cache,
	int fontWidth, int fontHeight,
	int x, int y, const char* text)
{
	int color = 0, x0 = x;

	for (int i = 0;; i++) {
		int c = (int)(unsigned char)text[i];
		if (c == 0) break;
		if (c == '\n') {
			x = x0;
			y += fontHeight;
		} else if (c == ' ') {
			x += fontWidth;
		} else if (c >= 0x10 && c < 0x20) {
			color = c & 0xF;
		} else {
			const int key = c | (color << 8);
			auto it = cache.find(key);
			if (it != cache.end()) {
				applyTexture(x, y, const_cast<TexturePtr&>(it->second), renderer);
				x += fontWidth;
			}
		}
	}
}

//Show a fake never-ending "ls -la" screen unless the user press Ctrl+C.
bool easterEggScreen(ImageManager& imageManager, SDL_Renderer& renderer) {
	//Some colors.
	SDL_Color colors[] = {
		{ 0xC0, 0xC0, 0xC0, 0xFF }, //lightgray
		{ 0x00, 0xFF, 0x00, 0xFF }, //green
		{ 0x00, 0x00, 0xFF, 0xFF }, //blue
		{ 0x00, 0xFF, 0xFF, 0xFF }, //cyan
		{ 0xFF, 0x00, 0xFF, 0xFF }, //magenta
	};
	const int numberOfColors = sizeof(colors) / sizeof(colors[0]);

	int fontWidth = 0;
	TTF_GlyphMetrics(fontMono, 'W', NULL, NULL, NULL, NULL, &fontWidth);

	//Initialize some textures.
	std::map<int, TexturePtr> cache;
	for (int i = 0; i < numberOfColors; i++) {
		for (int c = 33; c <= 126; c++) {
			const char s[2] = { c, 0 };
			const int key = c | (i << 8);
			cache[key] = textureFromText(renderer, *fontMono, s, colors[i]);
		}
	}

	const char* extensions[] = {
		"\x11sh", "\x11py",
		"\x14png", "\x14jpg",
		"txt", "c", "cpp", "h", "map", "lst", "lua",
	};
	const int numberOfExtensions = sizeof(extensions) / sizeof(extensions[0]);

	const char* users[] = {
		"root", "user", "me", "shadow"
	};
	const int numberOfUsers = sizeof(users) / sizeof(users[0]);

	const char* months[12] = {
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
	};

	const int numOfDays[12] = {
		31, 28, 31, 30, 31, 30,
		31, 31, 30, 31, 30, 31,
	};

	const int fontHeight = TTF_FontHeight(fontMono);

	//Keep the last resize event, this is to only process one.
	SDL_Event lastResize = {};

	bool ret = false, isRunning = true;

	char s0[72] = {}, s1[72] = {}, s2[256] = {};
	for (int i = 0; i < 60; i++) {
		s0[i] = int(float(rand()) / float(randMaxPlusOne) * 15.0f);
	}

	if (getSettings()->getBoolValue("music")) {
		fakeSoundEffectState.state = int(float(rand()) / float(randMaxPlusOne) * 80.0f);
		fakeSoundEffectState.randContext = rand() ^ (rand() << 16);
		Mix_SetPostMix(fakeSoundEffect, &fakeSoundEffectState);
	}

	for (int t = 0; isRunning; t++) {
		while (SDL_PollEvent(&event)) {
			//Check if we need to quit, if so enter the exit state.
			if (event.type == SDL_QUIT){
				setNextState(STATE_EXIT);
				isRunning = false;
			}

			//Check for a resize event.
			if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
				lastResize = event;
				continue;
			}

			//Check Ctrl+C.
			if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_c
				&& (event.key.keysym.mod & KMOD_CTRL) != 0 && (event.key.keysym.mod & ~KMOD_CTRL) == 0)
			{
				ret = true;
				isRunning = false;
			}

			//Set the cursor type to the default one, the GUI can change that if needed.
			currentCursor = CURSOR_POINTER;

			//Let the input manager handle the events.
			inputMgr.updateState(true);
		}

		//Process the resize event.
		if (lastResize.type == SDL_WINDOWEVENT){
			//TODO - used to be SDL_VIDEORESIZE
			// so this may trigger on more events than intended
			event = lastResize;
			onVideoResize(imageManager, renderer);

			//After resize we erase the event type
			//TODO - used to be SDL_NOEVENT
			lastResize.type = SDL_FIRSTEVENT;
		}

		//update input state (??)
		inputMgr.updateState(false);

		//Don't update the screen when the sound glitches.
		if (fakeSoundEffectState.state >> 8) {
			SDL_Delay(1000 / FPS);
			continue;
		}

		//Clear the screen.
		SDL_SetRenderDrawColor(&renderer, 0, 0, 0, 255);
		SDL_RenderClear(&renderer);

		const int rows = SCREEN_HEIGHT / fontHeight - 1;

		for (int row = 0; row < rows; row++) {
			//Generate some random text.
			int m = int(float(rand()) / float(randMaxPlusOne) * 56.0f);
			int carry = 0;
			for (int i = 0; i < 60; i++) {
				int c = s0[59 - i] + carry;
				if (i <= m) c += int(float(rand()) / float(randMaxPlusOne) * 15.0f);
				carry = 0;
				while (c >= 15) {
					c -= 15;
					carry++;
				}
				s0[59 - i] = c;
			}

			int lp;
			for (lp = 0; s0[lp]; lp++) {
				char c = s0[lp] - 4;
				if (c >= 0 && c < 10) {
					s1[lp] = c + '0';
				} else if (c == 10) {
					s1[lp] = '_';
				} else {
					break;
				}
			}
			s1[lp] = 0;
			if (lp < 60) s0[lp] = 4;

			//Choose a random extension.
			int extension = int(float(rand()) / float(randMaxPlusOne) * 20.0f);
			const char* ext = NULL;
			int color = 0;
			if (extension < numberOfExtensions) {
				ext = extensions[extension];
				if (ext[0] >= 0x10 && ext[0] < 0x20) {
					color = ext[0] & 0xF;
					ext++;
				}
			}
			if (ext) {
				s1[lp] = '.';
				s1[lp + 1] = 0;
				strcat(s1 + lp, ext);
				lp = strlen(s1);
			}

			//Choose a random color.
			if (extension >= numberOfExtensions) {
				int r = int(float(rand()) / float(randMaxPlusOne) * 10.0f);
				if (r <= 2) color = r;
			}
			
			bool isLink = int(float(rand()) / float(randMaxPlusOne) * 10.0f) == 0;

			//Choose a random user.
			const char* user = users[int(float(rand()) / float(randMaxPlusOne) * float(numberOfUsers))];

			//Choose a random permission
			const char* permission = NULL;
			switch (int(float(rand()) / float(randMaxPlusOne) * 10.0f)) {
			case 0:
				permission = (color == 1 || color == 2) ? "rwxrwxrwx" : "rw-rw-rw-";
				break;
			case 1:
				permission = (color == 1 || color == 2) ? "rwx------" : "rw-------";
				break;
			default:
				permission = (color == 1 || color == 2) ? "rwxr-xr-x" : "rw-r--r--";
				break;
			}

			//Choose a random size
			int size = 0;
			if (isLink) {
				size = 3 + lp;
			} else if (color == 2) {
				size = 4096;
			} else {
				size = int(float(rand()) / float(randMaxPlusOne) * 10000.0f);
			}

			int num = 1;
			if (color == 2) {
				num = int(float(rand()) / float(randMaxPlusOne) * 10.0f);
			}

			//Choose a random date
			char date[8];
			{
				int d = int(float(rand()) / float(randMaxPlusOne) * 365.0f);
				int m = 0;
				while (d >= numOfDays[m]) {
					d -= numOfDays[m];
					m++;
				}
				sprintf(date, "%s %2d", months[m], d + 1);
			}

			//Choose a random year or time
			char year[8];
			if (int(float(rand()) / float(randMaxPlusOne) * 10.0f) == 0) {
				sprintf(year, "%d", 1970 + int(float(rand()) / float(randMaxPlusOne) * 100.0f));
			} else {
				sprintf(year, "%02d:%02d", int(float(rand()) / float(randMaxPlusOne) * 24.0f), int(float(rand()) / float(randMaxPlusOne) * 60.0f));
			}

			//Put them together
			if (isLink) {
				sprintf(s2, "l%s %d %-6s %-6s %4d %s %5s \x13.%s\x10 -> %c../%s",
					permission, num, user, user, size, date, year, s1, 0x10 + color, s1
					);
			} else {
				sprintf(s2, "%c%s %d %-6s %-6s %4d %s %5s %c.%s",
					color == 2 ? 'd' : '-', permission, num, user, user, size, date, year, 0x10 + color, s1
					);
			}

			//Show text
			scamDrawText(imageManager, renderer, cache, fontWidth, fontHeight, 0, row * fontHeight, s2);
		}

		//Show a caret.
		if (t & 0x10) {
			SDL_Rect r = { 0, rows * fontHeight, fontWidth, fontHeight };
			SDL_SetRenderDrawColor(&renderer, 0x80, 0xFF, 0, 0xFF);
			SDL_RenderDrawRect(&renderer, &r);
		}

		//display it
		flipScreen(renderer);
		SDL_Delay(1000 / FPS);
	}

	Mix_SetPostMix(NULL, NULL);

	return ret;
}

#else

// Only play a sound.
bool easterEggScreen(ImageManager& imageManager, SDL_Renderer& renderer) {
	//play a sound effect
	getSoundManager()->playSound("hit");

	return true;
}

#endif
