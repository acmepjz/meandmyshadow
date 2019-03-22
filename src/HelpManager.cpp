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
#include "UTF8Functions.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <stdio.h>

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
	virtual void createSurfaces(std::vector<SurfacePtr>& surfaces, std::vector<GUITextArea::Hyperlink2>& links) = 0;
};

class ParagraphChunk : public Chunk {
public:
	std::string text;

	std::vector<std::string> cachedLines;

	class ParagraphWordWrapperCallback  : public WordWrapperCallback {
	public:
		bool isVerbatim;
		ParagraphWordWrapperCallback() : isVerbatim(false) {}
		~ParagraphWordWrapperCallback() {}
		void newLine() override {
			isVerbatim = false;
		}
		bool isBreakableSpace(int ch) override {
			if (ch == '`') isVerbatim = !isVerbatim;
			if (isVerbatim) return false;
			else return utf32IsBreakableSpace(ch);
		}
		int processWord(std::string& spaces, std::string& nonSpaces) override {
			size_t lps = nonSpaces.find_first_of('`'), lpe = nonSpaces.find_last_of('`');
			if (lps != std::string::npos && lpe != std::string::npos && lpe - lps >= 2) {
				std::string s1 = nonSpaces.substr(0, lps), s2 = nonSpaces.substr(lps + 1, lpe - lps - 1), s3 = nonSpaces.substr(lpe + 1);
				int w1 = 0, w2 = 0, w3 = 0;
				TTF_SizeUTF8(fontText, s1.c_str(), &w1, NULL);
				TTF_SizeUTF8(fontMono, s2.c_str(), &w2, NULL);
				TTF_SizeUTF8(fontText, s3.c_str(), &w3, NULL);
				nonSpaces = s1 + "\x01" + s2 + "\x01" + s3;
				return w1 + w2 + w3;
			} else {
				return 0;
			}
		}
	};

	virtual void updateSizeForced(int maxWidth) override {
		ParagraphWordWrapperCallback callback;
		WordWrapper wrapper;
		wrapper.font = fontText;
		wrapper.wordWrap = true;
		wrapper.maxWidth = maxWidth;
		wrapper.hyphen = "-";
		wrapper.hyphenatorLanguage = "en";
		wrapper.reserveHyperlinks = true;
		wrapper.callback = &callback;

		cachedLines.clear();
		cachedMaxWidth = maxWidth;
		cachedWidth = wrapper.addString(cachedLines, text);
		cachedNumberOfLines = cachedLines.size();
	}

	virtual void createSurfaces(std::vector<SurfacePtr>& surfaces, std::vector<GUITextArea::Hyperlink2>& links) override {
		SDL_Color fg = objThemes.getTextColor(true);

		int h = TTF_FontHeight(fontText);

		for (const std::string& s : cachedLines) {
			std::vector<SurfacePtr> tmpSurfaces;
			std::string tmp;
			bool isVerbatim = false;

			int w = 0;

			for (int i = 0, m = s.size(); i <= m; i++) {
				char c = (i < m) ? s[i] : 0x01;
				if (c == 0x01) {
					if (!tmp.empty()) {
						tmpSurfaces.emplace_back(TTF_RenderUTF8_Blended(isVerbatim ? fontMono : fontText, tmp.c_str(), fg));
						w += tmpSurfaces.back()->w;
					}
					isVerbatim = !isVerbatim;
					tmp.clear();
				} else {
					tmp.push_back(c);
				}
			}

			if (tmpSurfaces.empty()) {
				surfaces.emplace_back(TTF_RenderUTF8_Blended(fontText, " ", fg));
			} else {
				SurfacePtr surf2 = createSurface(w, h);

				w = 0;
				for (SurfacePtr& surf : tmpSurfaces) {
					SDL_Rect srcrect = { 0, 0, surf->w, surf->h };
					SDL_Rect dstrect = { w, (h - surf->h) / 2, surf->w, surf->h };
					SDL_BlitSurface(surf.get(), &srcrect, surf2.get(), &dstrect);
					w += surf->w;
				}

				surfaces.emplace_back(std::move(surf2));
			}
		}

		//TODO: extract hyperlinks
	}
};

struct FakeTOCItem {
	std::string caption;
	int level, pageIndex;
};

class FakeTOCChunk : public Chunk {
public:
	std::vector<FakeTOCItem> items;

	virtual void updateSizeForced(int maxWidth) override {
		int width = 0;

		for (const FakeTOCItem& item : items) {
			int w = 0;
			TTF_SizeUTF8(fontText, item.caption.c_str(), &w, NULL);
			w += (item.level + 1) * 16;
			width = std::max(width, w);
		}

		cachedMaxWidth = maxWidth;
		cachedWidth = width;
		cachedNumberOfLines = items.size();
	}

	virtual void createSurfaces(std::vector<SurfacePtr>& surfaces, std::vector<GUITextArea::Hyperlink2>& links) override {
		SDL_Color fg = objThemes.getTextColor(true);
		auto bmGUI = getImageManager().loadImage(getDataPath() + "gfx/gui.png");

		for (const FakeTOCItem& item : items) {
			SurfacePtr surf1(TTF_RenderUTF8_Blended(fontText, item.caption.empty() ? " " : item.caption.c_str(), fg));
			SurfacePtr surf = createSurface(surf1->w + (item.level + 1) * 16, surf1->h);

			SDL_Rect srcrect = { 0, 0, surf1->w, surf1->h };
			SDL_Rect dstrect = { (item.level + 1) * 16, 0, surf1->w, surf1->h };
			SDL_BlitSurface(surf1.get(), &srcrect, surf.get(), &dstrect);

			srcrect = SDL_Rect{ 80, 64, 16, 16 };
			dstrect = SDL_Rect{ item.level * 16, (surf1->h - 16) / 2, 16, 16 };
			SDL_BlitSurface(bmGUI, &srcrect, surf.get(), &dstrect);

			if (item.pageIndex >= 0) {
				char s[32];
				sprintf(s, "page:%d", item.pageIndex);
				links.push_back(GUITextArea::Hyperlink2{ (int)surfaces.size(), (item.level + 1) * 16, surf->w, s });
			}

			surfaces.push_back(std::move(surf));
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

	virtual void createSurfaces(std::vector<SurfacePtr>& surfaces, std::vector<GUITextArea::Hyperlink2>& links) override {
		auto bmGUI = getImageManager().loadImage(getDataPath() + "gfx/gui.png");

		for (auto item : items) {
			if (item) {
				std::vector<SurfacePtr> tempSurfaces;
				const size_t oldLineCount = surfaces.size();
				const size_t oldLinkSize = links.size();
				item->createSurfaces(tempSurfaces, links);

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

				for (size_t i = oldLinkSize; i < links.size(); i++) {
					GUITextArea::Hyperlink2 &link = links[i];
					link.line += oldLineCount;
					link.startX += 16;
					link.endX += 16;
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

	virtual void createSurfaces(std::vector<SurfacePtr>& surfaces, std::vector<GUITextArea::Hyperlink2>& links) override {
		int height = TTF_FontHeight(fontText) + 1;

		for (int j = 0; j < rows; j++) {
			if (j == 0) surfaces.push_back(createSurfaceWithGridLine(height, true, height / 2, height));

			const size_t oldLineCount = surfaces.size();

			std::vector<std::vector<SurfacePtr> > tempSurfaces(columns);
			int h = 0;
			int x = lineWidth;

			for (int i = 0; i < columns; i++) {
				auto item = items[j * columns + i];
				if (item) {
					const size_t oldLinkSize = links.size();
					item->createSurfaces(tempSurfaces[i], links);
					h = std::max<int>(tempSurfaces[i].size(), h);

					for (size_t k = oldLinkSize; k < links.size(); k++) {
						GUITextArea::Hyperlink2 &link = links[k];
						link.line += oldLineCount;
						link.startX += x;
						link.endX += x;
					}
				}
				x += cachedRowWidth[i] + lineWidth;
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

	virtual void createSurfaces(std::vector<SurfacePtr>& surfaces, std::vector<GUITextArea::Hyperlink2>& links) override {
		SDL_Color fg = objThemes.getTextColor(true);
		surfaces.emplace_back(TTF_RenderUTF8_Blended(fontText, "Copy code", fg));

		links.push_back(GUITextArea::Hyperlink2{ (int)surfaces.size() - 1, 0, surfaces.back()->w, "code:" });

		int h = TTF_FontHeight(fontText);

		for (const std::string& s : lines) {
			SurfacePtr surf(TTF_RenderUTF8_Blended(fontMono, s.empty() ? " " : s.c_str(), fg));

			int y = (h - surf->h) / 2;
			SurfacePtr surf2 = createSurface(surf->w, y + surf->h);

			SDL_Rect srcrect = { 0, 0, surf->w, surf->h };
			SDL_Rect dstrect = { 0, y, surf->w, surf->h };

			SDL_BlitSurface(surf.get(), &srcrect, surf2.get(), &dstrect);

			surfaces.emplace_back(std::move(surf2));

			links.back().url += s + "\n";
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

	virtual void createSurfaces(std::vector<SurfacePtr>& surfaces, std::vector<GUITextArea::Hyperlink2>& links) override {
		if (level <= 1) {
			child->createSurfaces(surfaces, links);

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

			const size_t oldLineCount = surfaces.size();
			const size_t oldLinkSize = links.size();

			std::vector<SurfacePtr> tempSurfaces;
			child->createSurfaces(tempSurfaces, links);

			const int additionalWidth = getAdditionalWidth();

			for (size_t i = oldLinkSize; i < links.size(); i++) {
				GUITextArea::Hyperlink2 &link = links[i];
				link.line += oldLineCount;
				link.startX += additionalWidth;
				link.endX += additionalWidth;
			}

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
	std::string nameSpace;
	std::vector<Chunk*> chunks;

	static const int BIGGEST_LEVEL = 10;

	HelpPage() : level(0) {}
	~HelpPage() {
		for (auto chunk : chunks) {
			delete chunk;
		}
	}

	void show(SDL_Renderer& renderer, GUITextArea *textArea) {
		SDL_Color fg = objThemes.getTextColor(true);
		std::vector<SurfacePtr> surfaces;
		std::vector<GUITextArea::Hyperlink2> links;

		for (auto chunk : chunks) {
			if (chunk) {
				chunk->updateSize(textArea->width - 16);
				chunk->createSurfaces(surfaces, links);
				surfaces.emplace_back(TTF_RenderUTF8_Blended(fontText, " ", fg));
			}
		}

		textArea->setStringArray(renderer, surfaces);
		textArea->setHyperlinks(links);
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

class HelpWindow : public GUIWindow {
public:
	int currentPage;
	std::vector<int> history;
	int currentHistory;

public:
	HelpWindow(ImageManager& imageManager, SDL_Renderer& renderer, int left = 0, int top = 0, int width = 0, int height = 0,
		bool enabled = true, bool visible = true, const char* caption = NULL)
		: GUIWindow(imageManager, renderer, left, top, width, height, enabled, visible, caption)
		, currentPage(0)
		, currentHistory(0)
	{
	}
};

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

	{
		std::string library, nameSpace;
		HelpPage *page = new HelpPage;

		size_t start = 0;
		while (auto chunk = parseChunk(lines, start)) {
			if (HeaderChunk *header = dynamic_cast<HeaderChunk*>(chunk)) {
				//We parse a new header so add a new page.
				if (!page->chunks.empty()) {
					pages.push_back(page);
					page = new HelpPage;
				}

				page->level = header->level;

				//Get the title.
				if (auto par = dynamic_cast<ParagraphChunk*>(header->child)) {
					page->title = par->text;

					//Get the library name.
					if (page->title.find("library") != std::string::npos) {
						library.clear();
						nameSpace.clear();
						size_t lps = page->title.find_first_of('\"');
						if (lps != std::string::npos) {
							size_t lpe = page->title.find_last_of('\"');
							if (lpe > lps) {
								library = page->title.substr(lps + 1, lpe - lps - 1);
							}
						}
					}

					//Get the namespace.
					if (page->title.find("Global") != std::string::npos) {
						nameSpace.clear();
					} else if (page->title.find("Static") != std::string::npos) {
						nameSpace = library + ".";
					} else if (page->title.find("Member") != std::string::npos) {
						nameSpace = library + ":";
					}
				}
			}

			if (ItemizeChunk *itemize = library.empty() ? NULL : dynamic_cast<ItemizeChunk*>(chunk)) {
				//We parse a new header so add a new page.
				if (!page->chunks.empty()) {
					pages.push_back(page);
					page = new HelpPage;
				}

				page->level = HelpPage::BIGGEST_LEVEL;
				page->nameSpace = nameSpace;

				//Get the title.
				if (ParagraphChunk *par = itemize->items.empty() ? NULL : dynamic_cast<ParagraphChunk*>(itemize->items[0])) {
					page->title = par->text;
				}
			}

			page->chunks.push_back(chunk);
		}

		if (page->chunks.empty()) delete page;
		else pages.push_back(page);
	}

	//Normalize the title of each page
	for (auto page : pages) {
		if (page->title.find_first_of("(/") != std::string::npos || page->title.find("--") != std::string::npos) {
			std::string tmp = page->title;
			size_t lp = tmp.find("--");
			if (lp != std::string::npos) tmp = tmp.substr(0, lp);

			std::vector<std::string> names;

			std::istringstream stream(tmp);
			std::string line;
			while (std::getline(stream, line, '/')) {
				lp = line.find_first_of('(');
				if (lp != std::string::npos) line = line.substr(0, lp);
				lp = line.find_first_not_of(" \t");
				if (lp != std::string::npos) {
					line = line.substr(lp, line.find_last_not_of(" \t") - lp + 1);
					while ((lp = line.find_last_of('`')) != std::string::npos) {
						line.erase(line.begin() + lp);
					}
					if (std::find(names.begin(), names.end(), line) == names.end()) {
						names.push_back(line);
					}
				}
			}

			tmp.clear();
			for (const std::string& s : names) {
				if (!tmp.empty()) tmp.push_back('/');
				tmp += s;
			}

			page->title = tmp;
		}

		//Also normize the contents if necessary.
		if (ItemizeChunk *itemize = page->chunks.empty() ? NULL : dynamic_cast<ItemizeChunk*>(page->chunks[0])) {
			if (ParagraphChunk *par = itemize->items.empty() ? NULL : dynamic_cast<ParagraphChunk*>(itemize->items[0])) {
				size_t lp = par->text.find("--");
				if (lp != std::string::npos) {
					std::vector<std::string> temp;
					temp.push_back(par->text.substr(lp + 2));
					page->chunks.push_back(parseParagraph(temp, 0, 1, 0));

					lp = par->text.find_last_not_of(" \t", lp);
					if (lp != std::string::npos) par->text = par->text.substr(0, lp);
				}
			}
		}
	}

	//Add hyperlinks of subpages to each page
	for (size_t i = 0; i < pages.size(); i++) {
		auto page = pages[i];
		int minLevel = 0, maxLevel = -1;
		if (i == 0) {
			maxLevel = 1;
		} else {
			minLevel = page->level + 1;
			maxLevel = (page->level == 0) ? 1 : HelpPage::BIGGEST_LEVEL;
		}

		FakeTOCChunk *chunk = NULL;
		int prevLevel = -1;
		for (size_t j = i + 1; j < pages.size(); j++) {
			auto subpage = pages[j];
			if (subpage->level < minLevel) break;
			if (subpage->level > maxLevel) continue;

			int level;
			if (subpage->level == HelpPage::BIGGEST_LEVEL) {
				level = prevLevel + 1;
			} else {
				prevLevel = level = subpage->level - minLevel;
			}

			if (chunk == NULL) chunk = new FakeTOCChunk;
			chunk->items.push_back(FakeTOCItem{ subpage->title, level, (int)j });
		}

		if (chunk) page->chunks.push_back(chunk);
	}
}

HelpManager::~HelpManager() {
	for (auto page : pages) {
		delete page;
	}
}

void HelpManager::updateCurrentPage(ImageManager& imageManager, SDL_Renderer& renderer, HelpWindow *window, int currentPage, bool addToHistory) {
	auto sllb = dynamic_cast<GUISingleLineListBox*>(window->getChild("sllb"));
	auto textArea = dynamic_cast<GUITextArea*>(window->getChild("TextArea"));
	if (sllb && textArea && currentPage != window->currentPage) {
		//Set current page
		window->currentPage = currentPage;

		//Update history
		if (addToHistory) {
			window->currentHistory++;
			window->history.resize(window->currentHistory);
			window->history.push_back(currentPage);
		}

		auto page = pages[currentPage];

		//Show contents
		page->show(renderer, textArea);

		char s[32];
		sprintf(s, "%d/%d ", currentPage + 1, (int)pages.size());
		sllb->item[1].second = s + page->nameSpace + page->title;
		sllb->value = 1;
	}
}

GUIWindow* HelpManager::newWindow(ImageManager& imageManager, SDL_Renderer& renderer, int pageIndex) {
	//Create the GUI.
	HelpWindow* root = new HelpWindow(imageManager, renderer, (SCREEN_WIDTH - 600) / 2, (SCREEN_HEIGHT - 500) / 2, 600, 500, true, true, _("Scripting Help"));
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
	btn = new GUIButton(imageManager, renderer, root->width / 2 - BUTTON_SPACE, 60, -1, 36, _("Back"), 0, true, true, GUIGravityCenter);
	btn->gravityLeft = btn->gravityRight = GUIGravityCenter;
	btn->name = "Back";
	btn->smallFont = true;
	btn->eventCallback = root;
	root->addChild(btn);
	btn = new GUIButton(imageManager, renderer, root->width / 2 + BUTTON_SPACE, 60, -1, 36, _("Forward"), 0, true, true, GUIGravityCenter);
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

	//Some buttons for search mode
	btn = new GUIButton(imageManager, renderer, root->width / 2 - BUTTON_SPACE * 3, 60, -1, 36, _("Back"), 0, true, false, GUIGravityCenter);
	btn->gravityLeft = btn->gravityRight = GUIGravityCenter;
	btn->name = "Back2";
	btn->smallFont = true;
	btn->eventCallback = root;
	root->addChild(btn);
	btn = new GUIButton(imageManager, renderer, root->width / 2 + BUTTON_SPACE * 3, 60, -1, 36, _("Goto"), 0, true, false, GUIGravityCenter);
	btn->gravityLeft = btn->gravityRight = GUIGravityCenter;
	btn->name = "Goto";
	btn->smallFont = true;
	btn->eventCallback = root;
	root->addChild(btn);

	//Add a single line list box to select page directly
	GUISingleLineListBox *sllb = new GUISingleLineListBox(imageManager, renderer, 25, 100, 550, 36);
	sllb->gravityRight = GUIGravityRight;
	sllb->name = "sllb";
	sllb->item.resize(3);
	sllb->value = 1;
	sllb->eventCallback = root;
	root->addChild(sllb);

	//Add a text area.
	GUITextArea *textArea = new GUITextArea(imageManager, renderer, 25, 140, 550, 300);
	textArea->gravityRight = textArea->gravityBottom = GUIGravityRight;
	textArea->name = "TextArea";
	textArea->editable = false;
	textArea->eventCallback = root;
	root->addChild(textArea);

	//Some widgets for search mode
	GUITextBox *textBox = new GUITextBox(imageManager, renderer, 25, 100, 550, 36, NULL, 0, true, false);
	textBox->gravityRight = GUIGravityRight;
	textBox->name = "TextSearch";
	textBox->eventCallback = root;
	root->addChild(textBox);
	GUIListBox *listBox = new GUIListBox(imageManager, renderer, 25, 140, 550, 300, true, false);
	listBox->gravityRight = listBox->gravityBottom = GUIGravityRight;
	listBox->name = "List";
	root->addChild(listBox);

	//The close button
	btn = new GUIButton(imageManager, renderer, int(root->width*0.5f), 500 - 44, -1, 36, _("Close"), 0, true, true, GUIGravityCenter);
	btn->gravityLeft = btn->gravityRight = GUIGravityCenter;
	btn->gravityTop = btn->gravityBottom = GUIGravityRight;
	btn->name = "cfgCancel";
	btn->eventCallback = root;
	root->addChild(btn);

	//Show contents.
	if (pageIndex < 0 || pageIndex >= (int)pages.size()) pageIndex = 0;
	root->currentPage = -1;
	updateCurrentPage(imageManager, renderer, root, pageIndex, false);
	root->history.push_back(pageIndex);

	return root;
}

void HelpManager::GUIEventCallback_OnEvent(ImageManager& imageManager, SDL_Renderer& renderer, std::string name, GUIObject* obj, int eventType) {
	HelpWindow *window = dynamic_cast<HelpWindow*>(obj);

	if (name == "Homepage") {
		if (SDL_GetModState() & KMOD_CTRL) {
			GUIObjectRoot->addChild(newWindow(imageManager, renderer));
		} else {
			updateCurrentPage(imageManager, renderer, window, 0, true);
		}
		return;
	} else if (name == "Back") {
		if (window->currentHistory > 0) {
			window->currentHistory--;
			updateCurrentPage(imageManager, renderer, window, window->history[window->currentHistory], false);
		}
		return;
	} else if (name == "Forward") {
		if (window->currentHistory < (int)window->history.size() - 1) {
			window->currentHistory++;
			updateCurrentPage(imageManager, renderer, window, window->history[window->currentHistory], false);
		}
		return;
	} else if (name == "sllb") {
		auto sllb = dynamic_cast<GUISingleLineListBox*>(window->getChild("sllb"));
		if (sllb && sllb->value != 1) {
			int index = window->currentPage + sllb->value - 1;
			sllb->value = 1;
			if (index >= 0 && index < (int)pages.size()) {
				if (SDL_GetModState() & KMOD_CTRL) {
					GUIObjectRoot->addChild(newWindow(imageManager, renderer, index));
				} else {
					updateCurrentPage(imageManager, renderer, window, index, true);
				}
			}
		}
		return;
	} else if (name == "Search" || name == "Back2") {
		bool isSearch = name == "Search";

		if (auto o = obj->getChild("Homepage")) o->visible = !isSearch;
		if (auto o = obj->getChild("Back")) o->visible = !isSearch;
		if (auto o = obj->getChild("Forward")) o->visible = !isSearch;
		if (auto o = obj->getChild("Search")) o->visible = !isSearch;
		if (auto o = obj->getChild("sllb")) o->visible = !isSearch;
		if (auto o = obj->getChild("TextArea")) o->visible = !isSearch;
		if (auto o = obj->getChild("Back2")) o->visible = isSearch;
		if (auto o = obj->getChild("Goto")) o->visible = isSearch;

		auto textBox = obj->getChild("TextSearch");
		auto listBox = dynamic_cast<GUIListBox*>(obj->getChild("List"));
		if (textBox && listBox) {
			textBox->visible = isSearch;
			listBox->visible = isSearch;
			if (isSearch && listBox->item.empty()) {
				updateListBox(imageManager, renderer, listBox, textBox->caption);
			}
		}

		return;
	} else if (name == "Goto") {
		auto listBox = dynamic_cast<GUIListBox*>(obj->getChild("List"));
		auto textArea = dynamic_cast<GUITextArea*>(obj->getChild("TextArea"));
		if (listBox && textArea && listBox->value >= 0 && listBox->value < (int)listBox->item.size()) {
			const std::string& s = listBox->item[listBox->value];
			int index = atoi(s.c_str());
			if (index >= 0 && index < (int)pages.size()) {
				if (SDL_GetModState() & KMOD_CTRL) {
					GUIObjectRoot->addChild(newWindow(imageManager, renderer, index));
				} else {
					updateCurrentPage(imageManager, renderer, window, index, true);
					GUIEventQueue.push_back(GUIEvent{ this, "Back2", obj, GUIEventClick });
				}
			}
		}
		return;
	} else if (name == "TextArea") {
		auto textArea = dynamic_cast<GUITextArea*>(obj->getChild("TextArea"));
		if (textArea && textArea->clickedHyperlink.size() > 5) {
			std::string s = textArea->clickedHyperlink.substr(0, 5);
			if (s == "code:") {
				SDL_SetClipboardText(textArea->clickedHyperlink.c_str() + 5);
			} else if (s == "page:") {
				int index = atoi(textArea->clickedHyperlink.c_str() + 5);
				if (index >= 0 && index < (int)pages.size()) {
					if (SDL_GetModState() & KMOD_CTRL) {
						GUIObjectRoot->addChild(newWindow(imageManager, renderer, index));
					} else {
						updateCurrentPage(imageManager, renderer, window, index, true);
					}
				}
			}
		}
		return;
	} else if (name == "TextSearch") {
		auto textBox = obj->getChild("TextSearch");
		auto listBox = dynamic_cast<GUIListBox*>(obj->getChild("List"));
		if (textBox && listBox) {
			updateListBox(imageManager, renderer, listBox, textBox->caption);
		}
		return;
	}

	//Do the default.
	parent->GUIEventCallback_OnEvent(imageManager, renderer, name, obj, eventType);
}

void HelpManager::updateListBox(ImageManager& imageManager, SDL_Renderer& renderer, GUIListBox *listBox, const std::string& keyword) {
	std::vector<std::string> keywords;

	std::string line;
	for (char c : keyword) {
		if (c == ' ' || c == '\t') {
			if (!line.empty()) keywords.push_back(line);
			line.clear();
		} else {
			line.push_back(tolower((unsigned char)c));
		}
	}
	if (!line.empty()) keywords.push_back(line);

	int backup = listBox->value;

	listBox->clearItems();

	for (size_t i = 0; i < pages.size(); i++) {
		HelpPage *page = pages[i];

		bool match = true;
		std::string tmp;
		for (char c : page->title) {
			tmp.push_back(tolower((unsigned char)c));
		}

		for (const std::string& s : keywords) {
			if (tmp.find(s) == std::string::npos) {
				match = false;
				break;
			}
		}
		if (match) {
			SharedTexture tex;
			SDL_Color fg = objThemes.getTextColor(true);

			if (page->level == HelpPage::BIGGEST_LEVEL) {
				SDL_Color fg2 = { Uint8((255 + fg.r) / 2), Uint8((255 + fg.g) / 2), Uint8((255 + fg.b) / 2), Uint8(255) };

				SurfacePtr surf1(TTF_RenderUTF8_Blended(fontMono, (std::string(5, ' ') + page->nameSpace).c_str(), fg2));
				SurfacePtr surf2(TTF_RenderUTF8_Blended(fontMono, page->title.empty() ? " " : page->title.c_str(), fg));

				SurfacePtr surf = createSurface(surf1->w + surf2->w, std::max(surf1->h, surf2->h) + 4);

				SDL_Rect srcrect = { 0, 0, surf1->w, surf1->h };
				SDL_Rect dstrect = { 0, 2, surf1->w, surf1->h };
				SDL_BlitSurface(surf1.get(), &srcrect, surf.get(), &dstrect);
				srcrect = SDL_Rect{ 0, 0, surf2->w, surf2->h };
				dstrect = SDL_Rect{ surf1->w, 2, surf2->w, surf2->h };
				SDL_BlitSurface(surf2.get(), &srcrect, surf.get(), &dstrect);

				tex = textureFromSurface(renderer, std::move(surf));
			} else {
				tex = textureFromTextShared(renderer, *fontText,
					(std::string(page->level, ' ') + page->title).c_str(),
					fg);
			}

			char s[32];
			sprintf(s, "%d", (int)i);
			listBox->addItem(renderer, s, tex);
		}
	}

	listBox->value = (backup >= 0 && backup < (int)listBox->item.size()) ? backup : -1;
}
