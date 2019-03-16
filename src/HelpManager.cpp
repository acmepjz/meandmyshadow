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

#include "HelpManager.h"
#include "Functions.h"
#include "GUIWindow.h"
#include "GUIListBox.h"
#include "GUITextArea.h"
#include "WordWrapper.h"
#include "ThemeManager.h"
#include <iostream>
#include <fstream>
#include <algorithm>

#include "SDL_ttf_fontfallback.h"

class Chunk {
public:
	int cachedMaxWidth, cachedWidth, cachedNumberOfLines;

public:
	Chunk() : cachedMaxWidth(-1), cachedWidth(-1), cachedNumberOfLines(-1) {}
	virtual ~Chunk() {}

	void updateSize(int maxWidth) {
		if (maxWidth != cachedMaxWidth) updateSizeForced(maxWidth);
	}

	virtual void updateSizeForced(int maxWidth) = 0;
	virtual void createSurfaces(std::vector<SurfacePtr>& surfaces) = 0;
};

class ParagraphChunk : public Chunk {
public:
	std::string text;

	std::vector<std::string> cachedLines;

	virtual void updateSizeForced(int maxWidth) override {
		WordWrapper wrapper;
		wrapper.font = fontText;
		wrapper.wordWrap = true;
		wrapper.maxWidth = maxWidth;
		wrapper.hyphen = "-";
		wrapper.hyphenatorLanguage = "en";

		//TODO: verbatim support
		cachedLines.clear();
		cachedMaxWidth = maxWidth;
		cachedWidth = wrapper.addString(cachedLines, text);
		cachedNumberOfLines = cachedLines.size();
	}

	virtual void createSurfaces(std::vector<SurfacePtr>& surfaces) override {
		SDL_Color fg = objThemes.getTextColor(true);
		for (const std::string& s : cachedLines) {
			surfaces.emplace_back(TTF_RenderUTF8_Blended(fontText, s.c_str(), fg));
		}
	}
};

class ItemizeChunk : public Chunk {
public:
	std::vector<Chunk*> items;

	virtual ~ItemizeChunk() {
		for (auto item : items) {
			delete item;
		}
	}

	virtual void updateSizeForced(int maxWidth) override {
		cachedMaxWidth = maxWidth;
		cachedWidth = 0;
		cachedNumberOfLines = 0;

		for (auto item : items) {
			if (item) {
				item->updateSize(maxWidth - 16);
				cachedWidth = std::max(item->cachedWidth, cachedWidth);
				cachedNumberOfLines += item->cachedNumberOfLines;
			}
		}

		cachedWidth += 16;
	}

	virtual void createSurfaces(std::vector<SurfacePtr>& surfaces) override {
		auto bmGUI = getImageManager().loadImage(getDataPath() + "gfx/gui.png");

		for (auto item : items) {
			if (item) {
				std::vector<SurfacePtr> tempSurfaces;
				item->createSurfaces(tempSurfaces);

				for (int i = 0, m = tempSurfaces.size(); i < m; i++) {
					SDL_Surface *src = tempSurfaces[i].get();
					SurfacePtr surface = createSurface(src->w + 16, src->h);

					SDL_Rect srcrect = { 0, 0, src->w, src->h };
					SDL_Rect dstrect = { 16, 0, src->w, src->h };
					SDL_BlitSurface(src, &srcrect, surface.get(), &dstrect);

					if (i == 0) {
						srcrect = SDL_Rect{ 80, 64, 16, 16 };
						dstrect = SDL_Rect{ 0, (src->h - 16) / 2, 16, 16 };
						SDL_BlitSurface(bmGUI, &srcrect, surface.get(), &dstrect);
					}

					surfaces.push_back(std::move(surface));
				}
			}
		}
	}
};

class TableChunk : public Chunk {
public:
	int columns, rows;
	std::vector<Chunk*> items;
	std::vector<int> cachedRowWidth;

	static const int lineWidth = 16;

	TableChunk() : columns(0), rows(0) {}
	virtual ~TableChunk() {
		for (auto item : items) {
			delete item;
		}
	}

	virtual void updateSizeForced(int maxWidth) override {
		cachedMaxWidth = maxWidth;
		cachedWidth = 0;
		cachedNumberOfLines = 0;
		cachedRowWidth.clear();

		for (int i = 0; i < columns; i++) {
			int mw = (i < columns - 1) ? 0x40000000 : (maxWidth - lineWidth * 2 - cachedWidth);

			int w = 0;
			for (int j = 0; j < rows; j++) {
				auto item = items[j * columns + i];
				if (item) {
					item->updateSize(mw);
					w = std::max(item->cachedWidth, w);
				}
			}

			cachedRowWidth.push_back(w);
			cachedWidth += w + lineWidth;
		}
		cachedWidth += lineWidth;

		for (int j = 0; j < rows; j++) {
			int h = 0;
			for (int i = 0; i < columns; i++) {
				auto item = items[j * columns + i];
				if (item) {
					h = std::max(item->cachedNumberOfLines, h);
				}
			}
			cachedNumberOfLines += h;
		}
		cachedNumberOfLines += 3;
	}

	SurfacePtr createSurfaceWithGridLine(int height, bool drawHorizontal, int verticalTop, int verticalBottom) {
		SurfacePtr surface = createSurface(cachedWidth, height);

		SDL_Color fg = objThemes.getTextColor(true);
		Uint32 color = SDL_MapRGBA(surface->format, fg.r, fg.g, fg.b, 255);

		if (drawHorizontal) {
			SDL_Rect r = { lineWidth / 2, height / 2, cachedWidth - lineWidth, 1 };
			SDL_FillRect(surface.get(), &r, color);
		}
		if (verticalBottom > verticalTop) {
			SDL_Rect r = { lineWidth / 2, verticalTop, 1, verticalBottom - verticalTop };
			SDL_FillRect(surface.get(), &r, color);
			for (int w : cachedRowWidth) {
				r.x += w + lineWidth;
				SDL_FillRect(surface.get(), &r, color);
			}
		}

		return surface;
	}

	virtual void createSurfaces(std::vector<SurfacePtr>& surfaces) override {
		int height = TTF_FontHeight(fontText) + 1;

		for (int j = 0; j < rows; j++) {
			if (j == 0) surfaces.push_back(createSurfaceWithGridLine(height, true, height / 2, height));

			std::vector<std::vector<SurfacePtr> > tempSurfaces(columns);
			int h = 0;

			for (int i = 0; i < columns; i++) {
				auto item = items[j * columns + i];
				if (item) {
					item->createSurfaces(tempSurfaces[i]);
					h = std::max<int>(tempSurfaces[i].size(), h);
				}
			}

			for (int y = 0; y < h; y++) {
				SurfacePtr surface = createSurfaceWithGridLine(height, false, 0, height);
				int x = lineWidth;

				for (int i = 0; i < columns; i++) {
					if (y < (int)tempSurfaces[i].size()) {
						auto src = tempSurfaces[i][y].get();
						SDL_Rect srcrect = { 0, 0, src->w, src->h };
						SDL_Rect dstrect = { x, 0, src->w, src->h };
						SDL_BlitSurface(src, &srcrect, surface.get(), &dstrect);
					}
					x += cachedRowWidth[i] + lineWidth;
				}

				surfaces.push_back(std::move(surface));
			}

			if (j == 0) surfaces.push_back(createSurfaceWithGridLine(height, true, 0, height));
		}

		surfaces.push_back(createSurfaceWithGridLine(height, true, 0, height / 2));
	}
};

class CodeChunk : public Chunk {
public:
	std::vector<std::string> lines;

	virtual void updateSizeForced(int maxWidth) override {
		int width = 0;

		for (const std::string& s : lines) {
			int w = 0;
			TTF_SizeUTF8(fontMono, s.c_str(), &w, NULL);
			width = std::max(width, w);
		}

		cachedMaxWidth = maxWidth;
		cachedWidth = width;
		cachedNumberOfLines = lines.size() + 1;
	}

	virtual void createSurfaces(std::vector<SurfacePtr>& surfaces) override {
		SDL_Color fg = objThemes.getTextColor(true);
		surfaces.emplace_back(TTF_RenderUTF8_Blended(fontText, "Copy code", fg));
		for (const std::string& s : lines) {
			surfaces.emplace_back(TTF_RenderUTF8_Blended(fontMono, s.c_str(), fg));
		}
	}
};

class HeaderChunk : public Chunk {
public:
	int level;
	Chunk *child;

	HeaderChunk() : level(0), child(NULL) {}
	virtual ~HeaderChunk() {
		delete child;
	}

	int getAdditionalWidth() const {
		return (level <= 1) ? 0 : (level == 2) ? 24 : 20;
	}

	int getAdditionalHeight() const {
		return (level <= 1) ? 1 : 0;
	}

	virtual void updateSizeForced(int maxWidth) override {
		const int additionalWidth = getAdditionalWidth();
		const int additionalHeight = getAdditionalHeight();

		cachedMaxWidth = maxWidth;
		child->updateSize(maxWidth - additionalWidth);
		cachedWidth = child->cachedWidth + additionalWidth;
		cachedNumberOfLines = child->cachedNumberOfLines + additionalHeight;
	}

	virtual void createSurfaces(std::vector<SurfacePtr>& surfaces) override {
		if (level <= 1) {
			child->createSurfaces(surfaces);

			int h = TTF_FontHeight(fontText);
			SurfacePtr surface = createSurface(cachedWidth, h);

			SDL_Color fg = objThemes.getTextColor(true);
			Uint32 color = SDL_MapRGBA(surface->format, fg.r, fg.g, fg.b, 255);

			if (level <= 0) {
				SDL_Rect r[2] = {
					{ 0, h / 2 - 3, cachedWidth, 2 },
					{ 0, h / 2 + 1, cachedWidth, 2 },
				};
				SDL_FillRects(surface.get(), r, 2, color);
			} else {
				SDL_Rect r = { 0, h / 2 - 1, cachedWidth, 2 };
				SDL_FillRect(surface.get(), &r, color);
			}

			surfaces.push_back(std::move(surface));
		} else {
			auto bmGUI = getImageManager().loadImage(getDataPath() + "gfx/gui.png");

			std::vector<SurfacePtr> tempSurfaces;
			child->createSurfaces(tempSurfaces);

			const int additionalWidth = getAdditionalWidth();

			for (int i = 0, m = tempSurfaces.size(); i < m; i++) {
				SDL_Surface *src = tempSurfaces[i].get();
				SurfacePtr surface = createSurface(src->w + additionalWidth, src->h);

				SDL_Rect srcrect = { 0, 0, src->w, src->h };
				SDL_Rect dstrect = { additionalWidth, 0, src->w, src->h };
				SDL_BlitSurface(src, &srcrect, surface.get(), &dstrect);

				if (i == 0) {
					if (level == 2) {
						srcrect = SDL_Rect{ 64, 0, 16, 16 };
					} else {
						srcrect = SDL_Rect{ 96, 16, 16, 16 };
					}
					dstrect = SDL_Rect{ (additionalWidth - 16) / 2, (src->h - 16) / 2, 16, 16 };
					SDL_BlitSurface(bmGUI, &srcrect, surface.get(), &dstrect);
				}

				surfaces.push_back(std::move(surface));
			}
		}
	}
};

class HelpPage {
public:
	int level;
	std::string title;
	std::vector<Chunk*> chunks;

	HelpPage() : level(0) {}
	~HelpPage() {
		for (auto chunk : chunks) {
			delete chunk;
		}
	}

	void show(SDL_Renderer& renderer, GUITextArea *textArea) {
		SDL_Color fg = objThemes.getTextColor(true);
		std::vector<SurfacePtr> surfaces;

		for (auto chunk : chunks) {
			if (chunk) {
				chunk->updateSize(textArea->width - 16);
				chunk->createSurfaces(surfaces);
				surfaces.emplace_back(TTF_RenderUTF8_Blended(fontText, " ", fg));
			}
		}

		textArea->setStringArray(renderer, surfaces);
	}
};

void parseText(const std::vector<std::string>& lines, size_t start, size_t end, size_t startOfNonSpaces, std::string& output) {
	bool init = false;

	for (size_t i = start; i < end; i++) {
		size_t lp = (i == start) ? startOfNonSpaces : 0;
		const std::string& line = lines[i];
		bool isSpace = true, isVerbatim = false;
		for (; lp < line.size(); lp++) {
			char c = line[lp];
			if (c == ' ' || c == '\t') {
				if (isVerbatim) output.push_back(' ');
				isSpace = true;
			} else {
				if (!isVerbatim && init && isSpace) output.push_back(' ');
				if (c == '`') isVerbatim = !isVerbatim;
				output.push_back(c);
				init = true;
				isSpace = false;
			}
		}
	}
}

ParagraphChunk* parseParagraph(const std::vector<std::string>& lines, size_t start, size_t end, size_t startOfNonSpaces) {
	auto ret = new ParagraphChunk;
	parseText(lines, start, end, startOfNonSpaces, ret->text);
	return ret;
}

CodeChunk* parseCode(const std::vector<std::string>& lines, size_t start, size_t end) {
	auto ret = new CodeChunk;
	for (size_t i = start; i < end; i++) {
		ret->lines.push_back(lines[i]);
	}
	return ret;
}

ItemizeChunk* parseItemize(const std::vector<std::string>& lines, size_t start, size_t end, char marker) {
	auto ret = new ItemizeChunk;
	size_t last = start, startOfNonSpaces = 0;

	for (size_t i = start; i <= end; i++) {
		bool isNewItem = false;
		size_t lp = 0;

		if (i < end) {
			lp = lines[i].find_first_not_of(" \t");
			if (lp != std::string::npos && lp + 1 < (int)lines[i].size() && lines[i][lp] == marker) {
				char c = lines[i][lp + 1];
				isNewItem = (c == ' ' || c == '\t');
			}
		} else {
			isNewItem = true;
		}

		if (isNewItem) {
			if (last != i) {
				ret->items.push_back(parseParagraph(lines, last, i, startOfNonSpaces));
			}
			last = i;
			startOfNonSpaces = lp + 1;
		}
	}
	return ret;
}

void parseRow(const std::string& text, std::vector<std::string>& output) {
	output.clear();

	size_t lps = text.find_first_not_of(" \t");
	if (lps == std::string::npos) return;
	if (text[lps] == '|') lps++;

	size_t lpe = text.find_last_not_of(" \t");
	if (lpe < lps) return;
	if (text[lpe] != '|') lpe++;

	std::string item;

	for (size_t lp = lps; lp <= lpe; lp++) {
		char c = (lp < lpe) ? text[lp] : '|';
		if (c == '|') {
			output.push_back(item);
			item.clear();
		} else {
			item.push_back(c);
		}
	}
}

TableChunk* parseTable(const std::vector<std::string>& lines, size_t start, size_t end) {
	auto ret = new TableChunk;

	std::vector<std::string> row;

	//determine the number of columns
	parseRow(lines[start], row);
	int columns = row.size(), rows = 0;

	if (columns > 0) {
		ret->columns = columns;
		ret->rows = rows = end - start - 1;
		ret->items.resize(columns * rows, NULL);

		for (int i = 0; i < columns; i++) {
			ret->items[i] = parseParagraph(row, i, i + 1, 0);
		}

		for (int j = 1; j < rows; j++) {
			parseRow(lines[start + j + 1], row);
			for (int i = 0, m = std::min<int>(row.size(), columns); i < m; i++) {
				ret->items[j * columns + i] = parseParagraph(row, i, i + 1, 0);
			}
		}
	}

	return ret;
}

HeaderChunk* parseHeader(const std::vector<std::string>& lines, size_t start, size_t end, size_t startOfNonSpaces, int level) {
	auto ret = new HeaderChunk;
	ret->level = level;
	ret->child = parseParagraph(lines, start, end, startOfNonSpaces);
	return ret;
}

Chunk* parseChunk(const std::vector<std::string>& lines, size_t& start) {
	size_t end, startOfNonSpaces;

	// skip empty lines
	for (; start < lines.size(); start++) {
		startOfNonSpaces = lines[start].find_first_not_of(" \t");
		if (startOfNonSpaces != std::string::npos) break;
	}

	if (start >= lines.size()) return NULL;

	// check if it's code
	if (lines[start].find("~~~") == 0) {
		start++;
		if (start >= lines.size()) return NULL;
		for (end = start; end < lines.size(); end++) {
			if (lines[end].find("~~~") == 0) break;
		}
		auto ret = parseCode(lines, start, end);
		start = end + 1;
		return ret;
	} else {
		for (end = start; end < lines.size(); end++) {
			if (lines[end].find_first_not_of(" \t") == std::string::npos) break;
		}
	}

	if (end == start) return NULL;

	// check if it's itemize
	if (startOfNonSpaces + 1 < (int)lines[start].size()) {
		char c = lines[start][startOfNonSpaces];
		char c2 = lines[start][startOfNonSpaces + 1];
		if ((c == '*' || c == '+' || c == '-') && (c2 == ' ' || c2 == '\t')) {
			auto ret = parseItemize(lines, start, end, c);
			start = end;
			return ret;
		}
	}

	// check if it's table
	if (end - start >= 3 && lines[start].find_first_of('|') != std::string::npos && lines[start + 1].find_first_not_of(" \t-|") == std::string::npos) {
		auto ret = parseTable(lines, start, end);
		start = end;
		return ret;
	}

	// check if it's header of syntax "### title"
	if (lines[start][startOfNonSpaces] == '#') {
		int level = 0;
		for (; level < 5; level++) {
			if ((size_t)(startOfNonSpaces + level + 1) >= lines[start].size()) {
				level = -1;
				break;
			}
			char c = lines[start][startOfNonSpaces + level + 1];
			if (c == ' ' || c == '\t') break;
			else if (c != '#') {
				level = -1;
				break;
			}
		}
		if (level >= 0) {
			auto ret = parseHeader(lines, start, end, startOfNonSpaces + level + 1, level);
			start = end;
			return ret;
		}
	}

	// check if it's header of syntax "title\n==="
	if (end - start >= 2) {
		int level = -1;
		if (lines[end - 1].find_first_not_of(" \t=") == std::string::npos) level = 0;
		else if (lines[end - 1].find_first_not_of(" \t-") == std::string::npos) level = 1;
		if (level >= 0) {
			auto ret = parseHeader(lines, start, end - 1, startOfNonSpaces, level);
			start = end;
			return ret;
		}
	}

	// now assume it's a normal paragraph
	auto ret = parseParagraph(lines, start, end, startOfNonSpaces);
	start = end;
	return ret;
}

HelpManager::HelpManager(GUIEventCallback *parent)
	: parent(parent)
{
	std::vector<std::string> lines;

	{
		std::ifstream fin((getDataPath() + "../docs/ScriptAPI.md").c_str());
		if (fin) {
			//Loop the lines of the file.
			std::string line;
			char c;
			while (fin.get(c)) {
				if (c == '\r') {
				} else if (c == '\n') {
					lines.push_back(line);
					line.clear();
				} else {
					line.push_back(c);
				}
			}
			lines.push_back(line);
		} else {
			std::cerr << "ERROR: Unable to open the ScriptAPI.md file." << std::endl;
			lines.push_back("ERROR: Unable to open the ScriptAPI.md file.");
		}
	}

	//TEST ONLY! TODO:
	auto page = new HelpPage;
	pages.push_back(page);

	size_t start = 0;
	while (auto chunk = parseChunk(lines, start)) {
		page->chunks.push_back(chunk);
	}
}

HelpManager::~HelpManager() {
	for (auto page : pages) {
		delete page;
	}
}

GUIWindow* HelpManager::newWindow(ImageManager& imageManager, SDL_Renderer& renderer, int pageIndex) {
	//Create the GUI.
	GUIWindow* root = new GUIWindow(imageManager, renderer, (SCREEN_WIDTH - 600) / 2, (SCREEN_HEIGHT - 500) / 2, 600, 500, true, true, _("Scripting Help"));
	root->minWidth = root->width; root->minHeight = root->height;
	root->name = "scriptingHelpWindow";
	root->eventCallback = this;

	GUIButton* btn;

	const int BUTTON_SPACE = 70;

	//Some navigation buttons
	btn = new GUIButton(imageManager, renderer, root->width / 2 - BUTTON_SPACE * 3, 60, -1, 36, _("Homepage"), 0, true, true, GUIGravityCenter);
	btn->gravityLeft = btn->gravityRight = GUIGravityCenter;
	btn->name = "Homepage";
	btn->smallFont = true;
	btn->eventCallback = root;
	root->addChild(btn);
	btn = new GUIButton(imageManager, renderer, root->width / 2 - BUTTON_SPACE, 60, -1, 36, _("Back"), 0, false, true, GUIGravityCenter);
	btn->gravityLeft = btn->gravityRight = GUIGravityCenter;
	btn->name = "Back";
	btn->smallFont = true;
	btn->eventCallback = root;
	root->addChild(btn);
	btn = new GUIButton(imageManager, renderer, root->width / 2 + BUTTON_SPACE, 60, -1, 36, _("Forward"), 0, false, true, GUIGravityCenter);
	btn->gravityLeft = btn->gravityRight = GUIGravityCenter;
	btn->name = "Forward";
	btn->smallFont = true;
	btn->eventCallback = root;
	root->addChild(btn);
	btn = new GUIButton(imageManager, renderer, root->width / 2 + BUTTON_SPACE * 3, 60, -1, 36, _("Search"), 0, true, true, GUIGravityCenter);
	btn->gravityLeft = btn->gravityRight = GUIGravityCenter;
	btn->name = "Search";
	btn->smallFont = true;
	btn->eventCallback = root;
	root->addChild(btn);

	//Add a text area.
	GUITextArea* text = new GUITextArea(imageManager, renderer, 50, 100, 500, 340);
	text->gravityRight = text->gravityBottom = GUIGravityRight;
	text->name = "Text";
	text->editable = false;
	root->addChild(text);

	btn = new GUIButton(imageManager, renderer, int(root->width*0.5f), 500 - 44, -1, 36, _("Close"), 0, true, true, GUIGravityCenter);
	btn->gravityLeft = btn->gravityRight = GUIGravityCenter;
	btn->gravityTop = btn->gravityBottom = GUIGravityRight;
	btn->name = "cfgCancel";
	btn->eventCallback = root;
	root->addChild(btn);

	//Show contents.
	if (pageIndex < 0 || pageIndex >= (int)pages.size()) pageIndex = 0;
	if (pageIndex < (int)pages.size() && pages[pageIndex]) {
		pages[pageIndex]->show(renderer, text);
	}

	return root;
}

GUIWindow* HelpManager::newSearchWindow(ImageManager& imageManager, SDL_Renderer& renderer) {
	//TODO:
	return NULL;
}

void HelpManager::GUIEventCallback_OnEvent(ImageManager& imageManager, SDL_Renderer& renderer, std::string name, GUIObject* obj, int eventType) {
	if (name == "Homepage") {
		//TODO:
		return;
	} else if (name == "Back") {
		//TODO:
		return;
	} else if (name == "Forward") {
		//TODO:
		return;
	} else if (name == "Search") {
		//TODO:
		return;
	}

	//Do the default.
	parent->GUIEventCallback_OnEvent(imageManager, renderer, name, obj, eventType);
}