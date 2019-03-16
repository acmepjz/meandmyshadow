/*
 * Copyright (C) 2011-2012 Me and My Shadow
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

#include "Functions.h"
#include "UTF8Functions.h"
#include "GUITextArea.h"
#include "ThemeManager.h"
#include "WordWrapper.h"
#include <cmath>
#include <algorithm>
#include <assert.h>
#include <ctype.h>
#include <SDL_ttf_fontfallback.h>

using namespace std;

#define SPACE_PER_TAB 2

GUITextArea::GUITextArea(ImageManager& imageManager, SDL_Renderer& renderer,int left,int top,int width,int height,bool enabled,bool visible):
    GUIObject(imageManager,renderer,left,top,width,height,NULL,-1,enabled,visible),editable(true){
	
	//Set some default values.
	state=0;
	setFont(fontText);

	highlightLineStart=highlightLineEnd=0;
	highlightStartX=highlightEndX=0;
	highlightStart=highlightEnd=0;
	
	//Add empty text.
	lines.push_back("");
    linesCache.push_back(nullptr);
	
	//Create scrollbar widget.
    scrollBar=new GUIScrollBar(imageManager,renderer,width-16,0,16,height,1,0,0,0);
	childControls.push_back(scrollBar);
	
    scrollBarH=new GUIScrollBar(imageManager,renderer,0,height-16,width-16,16,0,0,0,0,100,500,true,false);
	childControls.push_back(scrollBarH);
}

void GUITextArea::setFont(TTF_Font* font){
	//NOTE: This fuction shouldn't be called after adding items, so no need to update the whole cache.
	widgetFont=font;
	fontHeight=TTF_FontHeight(font)+1;
}

void GUITextArea::inputText(SDL_Renderer &renderer, const char* s) {
	if (s && s[0]) {
		//Split into lines.
		vector<string> newLines;
		newLines.push_back(std::string());
		for (int i = 0; s[i]; i++) {
			if (s[i] == '\r') continue;
			if (s[i] == '\n') {
				newLines.push_back(std::string());
				continue;
			}
			if (s[i] == '\t') {
				// Replace tabs by spaces.
				newLines.back() += std::string(SPACE_PER_TAB, ' ');
				continue;
			}
			newLines.back().push_back(s[i]);
		}

		const int m = newLines.size();

		if (m == 1 && newLines[0].empty()) return;

		//Remove selected text.
		removeHighlight(renderer);

		//Calculate the width of the last line.
		int advance = 0;
		{
			const char* lastLine = newLines[m - 1].c_str();
			for (int i = 0;;) {
				int a = 0;
				int ch = utf8ReadForward(lastLine, i);
				if (ch <= 0) break;
				TTF_GlyphMetrics(widgetFont, ch, NULL, NULL, NULL, NULL, &a);
				advance += a;
			}
		}

		if (m > 1) {
			//Multiple lines.
			highlightEnd = newLines[m - 1].size();
			highlightStartX = highlightEndX = advance;

			newLines[m - 1] += lines[highlightLineStart].substr(highlightStart);
			lines[highlightLineStart] = lines[highlightLineStart].substr(0, highlightStart) + newLines[0];

			lines.insert(lines.begin() + (highlightLineStart + 1), newLines.begin() + 1, newLines.end());
			for (int i = 0; i < m - 1; i++) {
				linesCache.insert(linesCache.begin() + (highlightLineStart + 1), nullptr);
			}

			highlightStart = highlightEnd;
		} else {
			//Single line.
			highlightEnd = highlightStart + newLines[0].size();

			lines[highlightLineStart].insert(highlightStart, newLines[0]);

			highlightStart = highlightEnd;
			highlightStartX = highlightEndX = highlightStartX + advance;
		}

		//Update cache.
		highlightLineEnd = highlightLineStart + m - 1;
		for (int i = highlightLineStart; i <= highlightLineEnd; i++) {
			linesCache[i] = textureFromText(renderer, *widgetFont, lines[i].c_str(), objThemes.getTextColor(true));
		}
		highlightLineStart = highlightLineEnd;

		//Update view if needed.
		adjustView();

		//If there is an event callback then call it.
		if (eventCallback){
			GUIEvent e = { eventCallback, name, this, GUIEventChange };
			GUIEventQueue.push_back(e);
		}
	}
}

void GUITextArea::scrollScrollbar(int dx, int dy) {
	if (dx && scrollBarH->visible){
		scrollBarH->value = clamp(scrollBarH->value + dx, 0, scrollBarH->maxValue);
	}
	if (dy) {
		scrollBar->value = clamp(scrollBar->value + dy, 0, scrollBar->maxValue);
	}
}

bool GUITextArea::handleEvents(SDL_Renderer& renderer,int x,int y,bool enabled,bool visible,bool processed){
	//Boolean if the event is processed.
	bool b=processed;
	
	//The GUIObject is only enabled when he and his parent are enabled.
	enabled=enabled && this->enabled;
	//The GUIObject is only enabled when he and his parent are enabled.
	visible=visible && this->visible;
	
	//Get the absolute position.
	x+=left;
	y+=top;
	
	//Update the scrollbars.
	b = b || scrollBar->handleEvents(renderer, x, y, enabled, visible, b);
	b = b || scrollBarH->handleEvents(renderer, x, y, enabled, visible, b);

	//NOTE: We don't reset the state to have a "focus" effect.
	//Only check for events when the object is both enabled and visible.
	if(enabled&&visible){
		//Check if there's a key press and the event hasn't been already processed.
		if(state==2 && event.type==SDL_KEYDOWN && !b && editable){
			if ((event.key.keysym.mod & KMOD_CTRL) == 0) {
				//Check if the key is supported.
				if (event.key.keysym.sym == SDLK_BACKSPACE){
					//Delete one character direct to prevent a lag.
					backspaceChar(renderer);
				} else if (event.key.keysym.sym == SDLK_DELETE){
					//Delete one character direct to prevent a lag.
					deleteChar(renderer);
				} else if (event.key.keysym.sym == SDLK_RETURN){
					removeHighlight(renderer);
					//Split the current line and update.
					string str2 = lines.at(highlightLineEnd).substr(highlightStart);
					lines.at(highlightLineStart) = lines.at(highlightLineStart).substr(0, highlightStart);

					linesCache.at(highlightLineStart) =
						textureFromText(renderer, *widgetFont, lines.at(highlightLineStart).c_str(), objThemes.getTextColor(true));

					//Calculate indentation.
					int indent = 0;
					for (int i = 0; i < (int)lines.at(highlightLineStart).length(); i++){
						if (isspace(lines.at(highlightLineStart)[i]))
							indent++;
						else
							break;
					}
					str2.insert(0, indent, ' ');

					//Add the rest in a new line.
					highlightLineStart++;
					highlightStart = indent;
					highlightEnd = highlightStart;
					highlightLineEnd++;

					highlightStartX = 0;
					for (int i = 0; i < indent; i++){
						int advance;
						TTF_GlyphMetrics(widgetFont, str2.at(i), NULL, NULL, NULL, NULL, &advance);
						highlightStartX += advance;
					}
					highlightEndX = highlightStartX;

					lines.insert(lines.begin() + highlightLineStart, str2);

					auto tex = textureFromText(renderer, *widgetFont, str2.c_str(), objThemes.getTextColor(true));
					linesCache.insert(linesCache.begin() + highlightLineStart, std::move(tex));


					adjustView();

					//If there is an event callback then call it.
					if (eventCallback){
						GUIEvent e = { eventCallback, name, this, GUIEventChange };
						GUIEventQueue.push_back(e);
					}
				} else if (event.key.keysym.sym == SDLK_TAB){
					//Calculate the width of a space.
					int advance;
					TTF_GlyphMetrics(widgetFont, ' ', NULL, NULL, NULL, NULL, &advance);

					int start = highlightLineStart, end = highlightLineEnd;
					if (start > end) std::swap(start, end);

					for (int line = start; line <= end; line++) {
						int count = 0;
						std::string &s = lines[line];
						if (event.key.keysym.mod & KMOD_SHIFT) {
							// remove spaces
							for (; count < SPACE_PER_TAB; count++) {
								if (s.c_str()[count] != ' ') break;
							}
							if (count > 0) {
								s.erase(0, count);
								count = -count;
							}
						} else {
							// add spaces
							count = SPACE_PER_TAB;
							s.insert(0, count, ' ');
						}

						//Update cache.
						if (count) {
							linesCache.at(line) = textureFromText(renderer, *widgetFont, s.c_str(), objThemes.getTextColor(true));
						}

						//Update selection.
						if (line == highlightLineStart) {
							highlightStart += count;
							highlightStartX += count*advance;
							if (highlightStart <= 0) {
								highlightStart = 0;
								highlightStartX = 0;
							}
						}
						if (line == highlightLineEnd) {
							highlightEnd += count;
							highlightEndX += count*advance;
							if (highlightEnd <= 0) {
								highlightEnd = 0;
								highlightEndX = 0;
							}
						}
					}

					adjustView();
				} else if (event.key.keysym.sym == SDLK_RIGHT){
					//Move the carrot once to prevent a lag.
					moveCarrotRight();
				} else if (event.key.keysym.sym == SDLK_LEFT){
					//Move the carrot once to prevent a lag.
					moveCarrotLeft();
				} else if (event.key.keysym.sym == SDLK_DOWN){
					//Move the carrot once to prevent a lag.
					moveCarrotDown();
				} else if (event.key.keysym.sym == SDLK_UP){
					//Move the carrot once to prevent a lag.
					moveCarrotUp();
				}
			} else {
				//Check hotkey.
				if (event.key.keysym.sym == SDLK_a) {
					//Select all.
					highlightLineStart = 0;
					highlightStart = 0;
					highlightStartX = 0;
					highlightLineEnd = lines.size() - 1;
					highlightEnd = lines.back().size();
					highlightEndX = 0;
					if (highlightEnd > 0) {
						TTF_SizeUTF8(widgetFont, lines.back().c_str(), &highlightEndX, NULL);
					}
				} else if (event.key.keysym.sym == SDLK_x || event.key.keysym.sym == SDLK_c) {
					//Cut or copy.
					int startLine = highlightLineStart, endLine = highlightLineEnd;
					int start = highlightStart, end = highlightEnd;
					if (startLine > endLine || (startLine == endLine && start > end)) {
						std::swap(startLine, endLine);
						std::swap(start, end);
					}

					std::string s;

					if (startLine < endLine) {
						//Multiple lines.
						s = lines[startLine].substr(start);
						s.push_back('\n');
						for (int i = startLine + 1; i < endLine; i++) {
							s += lines[i];
							s.push_back('\n');
						}
						s += lines[endLine].substr(0, end);
					} else {
						//Single line.
						s = lines[startLine].substr(start, end - start);
					}

					if (!s.empty()) {
						SDL_SetClipboardText(s.c_str());
						if (event.key.keysym.sym == SDLK_x) {
							//Cut.
							removeHighlight(renderer);

							//If there is an event callback then call it.
							if (eventCallback){
								GUIEvent e = { eventCallback, name, this, GUIEventChange };
								GUIEventQueue.push_back(e);
							}
						}
					}
				} else if (event.key.keysym.sym == SDLK_v) {
					//Paste.
					if (SDL_HasClipboardText()) {
						char *s = SDL_GetClipboardText();
						inputText(renderer, s);
						SDL_free(s);
					}
				}
			}
			
			//The event has been processed.
			b=true;
		} else if (state == 2 && event.type == SDL_TEXTINPUT && !b && editable){
			inputText(renderer, event.text.text);
		} else if (state == 2 && event.type == SDL_TEXTEDITING && !b && editable){
			// TODO: process SDL_TEXTEDITING event
		}
		
		if (event.type == SDL_MOUSEMOTION || event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP || event.type == SDL_MOUSEWHEEL) {
			//The mouse location (x=i, y=j) and the mouse button (k).
			int i, j, k;
			k = SDL_GetMouseState(&i, &j);

			//Check if the mouse is inside the GUIObject.
			if (i >= x && i < x + width && j >= y && j < y + height && !b){
				//We can only increase our state. (nothing->hover->focus).
				if (state != 2){
					state = 1;
				}

				//Check for mouse wheel scrolling.
				//Scroll horizontally if mouse is over the horizontal scrollbar.
				//Otherwise scroll vertically.
				if (event.type == SDL_MOUSEWHEEL && event.wheel.y) {
					if (j >= y + height - 16 && scrollBarH->visible){
						scrollScrollbar(event.wheel.y < 0 ? 20 : -20, 0);
					} else{
						scrollScrollbar(0, event.wheel.y < 0 ? 1 : -1);
					}
				}

				//When mouse is not over the scrollbar.
				if (i < x + width - 16 && j < (scrollBarH->visible ? y + height - 16 : y + height)){
					if (editable) {
						//Update the cursor type.
						currentCursor = CURSOR_CARROT;

						if (((event.type == SDL_MOUSEBUTTONUP || event.type == SDL_MOUSEBUTTONDOWN) && event.button.button == 1)
							|| (event.type == SDL_MOUSEMOTION && (k & SDL_BUTTON(1))))
						{
							//Move carrot to the place clicked.
							const int mouseLine = clamp((int)floor(float(j - y) / float(fontHeight)) + scrollBar->value, 0, lines.size() - 1);

							string* str = &lines.at(mouseLine);
							value = str->length();

							const int clickX = i - x + scrollBarH->value;
							int finalX = 0;
							int finalPos = str->length();

							for (int i = 0;;){
								int advance = 0;

								int i0 = i;
								int ch = utf8ReadForward(str->c_str(), i);
								if (ch <= 0) break;
								TTF_GlyphMetrics(widgetFont, ch, NULL, NULL, NULL, NULL, &advance);
								finalX += advance;

								if (clickX < finalX - advance / 2){
									finalPos = i0;
									finalX -= advance;
									break;
								}
							}

							if (event.type == SDL_MOUSEBUTTONUP){
								state = 2;
								highlightEnd = finalPos;
								highlightEndX = finalX;
								highlightLineEnd = mouseLine;
							} else if (event.type == SDL_MOUSEBUTTONDOWN){
								state = 2;
								highlightStart = highlightEnd = finalPos;
								highlightStartX = highlightEndX = finalX;
								highlightLineStart = highlightLineEnd = mouseLine;
							} else if (event.type == SDL_MOUSEMOTION){
								state = 2;
								highlightEnd = finalPos;
								highlightEndX = finalX;
								highlightLineEnd = mouseLine;
							}
						}
					} else {
						const int mouseLine = (int)floor(float(j - y) / float(fontHeight)) + scrollBar->value;
						if (mouseLine >= 0 && mouseLine < (int)hyperlinks.size()) {
							const int clickX = i - x + scrollBarH->value;
							for (const Hyperlink& lnk : hyperlinks[mouseLine]) {
								if (clickX >= lnk.startX && clickX < lnk.endX) {
									currentCursor = CURSOR_POINTING_HAND;
									if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == 1) {
										openWebsite(lnk.url);
									}
									break;
								}
							}
						}
					}
				}

				//Event has been processed as long as this is a mouse event and the mouse is inside the widget.
				b = true;
			} else{
				//The mouse is outside the TextBox.
				//If we don't have focus but only hover we lose it.
				if (state == 1){
					state = 0;
				}

				//If it's a click event outside the textbox then we blur.
				if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT){
					//Set state to 0.
					state = 0;
				}
			}
		}
	}

    if(!editable)
        highlightLineStart=scrollBar->value;
	
	//Process child controls event except for the scrollbars.
	//That's why i ends at 2.
	for (int i = childControls.size() - 1; i >= 2; i--) {
		bool b1 = childControls[i]->handleEvents(renderer, x, y, enabled, visible, b);
		
		//The event is processed when either our or the childs is true (or both).
		b=b||b1;
	}
	return b;
}

void GUITextArea::removeHighlight(SDL_Renderer& renderer){
	if (highlightLineStart==highlightLineEnd) {
		if (highlightStart == highlightEnd) return;
		int start=highlightStart, end=highlightEnd, startx=highlightStartX;
		if(highlightStart>highlightEnd){
			start=highlightEnd;
			end=highlightStart;
			startx=highlightEndX;
		}
        std::string& str=lines.at(highlightLineStart);
        str.erase(start,end-start);

		highlightStart=highlightEnd=start;
		highlightStartX=highlightEndX=startx;

		// Update cache.
        linesCache.at(highlightLineStart) = textureFromText(renderer,*widgetFont,str.c_str(),objThemes.getTextColor(true));
	}else{
		int startLine=highlightLineStart, endLine=highlightLineEnd,
			start=highlightStart, end=highlightEnd, startx=highlightStartX;
		if(startLine>endLine){
			startLine=highlightLineEnd;
			endLine=highlightLineStart;
			start=highlightEnd;
			end=highlightStart;
			startx=highlightEndX;
		}

		lines[startLine] = lines[startLine].substr(0, start) + lines[endLine].substr(end);

		lines.erase(lines.begin() + startLine + 1, lines.begin() + endLine + 1);
		linesCache.erase(linesCache.begin() + startLine + 1, linesCache.begin() + endLine + 1);

		highlightLineStart=highlightLineEnd=startLine;
		highlightStart=highlightEnd=start;
		highlightStartX=highlightEndX=startx;

		// Update cache.
		linesCache.at(startLine) = textureFromText(renderer, *widgetFont, lines[startLine].c_str(), objThemes.getTextColor(true));
	}
	adjustView();
}

void GUITextArea::deleteChar(SDL_Renderer& renderer){
	if (highlightLineStart==highlightLineEnd && highlightStart==highlightEnd){
		if(highlightEnd>=(int)lines.at(highlightLineEnd).length()){
			if(highlightLineEnd<(int)lines.size()-1){
				highlightLineEnd++;
				highlightEnd=0;
			}
		} else {
			utf8ReadForward(lines.at(highlightLineEnd).c_str(), highlightEnd);
		}
	}
    removeHighlight(renderer);
	//If there is an event callback.
	if(eventCallback){
		GUIEvent e={eventCallback,name,this,GUIEventChange};
		GUIEventQueue.push_back(e);
	}
}

void GUITextArea::backspaceChar(SDL_Renderer& renderer){
	if(highlightLineStart==highlightLineEnd && highlightStart==highlightEnd){
		if(highlightStart<=0){
			if(highlightLineStart==0){
				highlightStart=0;
			}else{
				highlightLineStart--;
				highlightStart=lines.at(highlightLineStart).length();
				highlightStartX=0;
				if (highlightStart > 0) {
					TexturePtr& t = linesCache.at(highlightLineStart);
					if (t) highlightStartX = textureWidth(*t);
				}
			}
		}else{
			int advance = 0;

			int ch = utf8ReadBackward(lines.at(highlightLineStart).c_str(), highlightStart);
			if (ch > 0) TTF_GlyphMetrics(widgetFont, ch, NULL, NULL, NULL, NULL, &advance);

			highlightStartX -= advance;
		}
	}
    removeHighlight(renderer);

	//If there is an event callback.
	if(eventCallback){
		GUIEvent e={eventCallback,name,this,GUIEventChange};
		GUIEventQueue.push_back(e);
	}
}

void GUITextArea::moveCarrotRight(){
	if (highlightEnd>=(int)lines.at(highlightLineEnd).length()){
		if (highlightLineEnd<(int)lines.size()-1){
			highlightEnd=0;
			highlightEndX=0;
			highlightLineEnd++;
		}
	}else{
		int advance = 0;

		int ch = utf8ReadForward(lines.at(highlightLineEnd).c_str(), highlightEnd);
		if (ch > 0) TTF_GlyphMetrics(widgetFont, ch, NULL, NULL, NULL, NULL, &advance);

		highlightEndX += advance;
	}
	if((SDL_GetModState()&KMOD_SHIFT)==0){
        highlightLineStart=highlightLineEnd;
        highlightStart=highlightEnd;
        highlightStartX=highlightEndX;
    }
	adjustView();
}

void GUITextArea::moveCarrotLeft(){
	if (highlightEnd<=0){
		if (highlightLineEnd==0){
			highlightEnd=0;
		}else{
			highlightLineEnd--;
			highlightEnd=lines.at(highlightLineEnd).length();
			highlightEndX=0;
			if (highlightEnd > 0) {
				TexturePtr& t = linesCache.at(highlightLineEnd);
				if (t) highlightEndX = textureWidth(*t);
			}
		}
	}else{
		int advance = 0;

		int ch = utf8ReadBackward(lines.at(highlightLineEnd).c_str(), highlightEnd);
		if (ch > 0) TTF_GlyphMetrics(widgetFont, ch, NULL, NULL, NULL, NULL, &advance);

		highlightEndX -= advance;
	}
	if((SDL_GetModState()&KMOD_SHIFT)==0){
        highlightLineStart=highlightLineEnd;
        highlightStart=highlightEnd;
        highlightStartX=highlightEndX;
    }
	adjustView();
}

void GUITextArea::moveCarrotUp(){
	if(highlightLineEnd==0){
		highlightEnd=0;
		highlightEndX=0;
	}else{
		highlightLineEnd--;
        const std::string& str=lines.at(highlightLineEnd);

		//Find out closest match.
		int xPos=0;
		int i=0;
		for (;;){
			int advance = 0;

			int i0 = i;
			int ch = utf8ReadForward(str.c_str(), i);
			if (ch <= 0) break;
			TTF_GlyphMetrics(widgetFont, ch, NULL, NULL, NULL, NULL, &advance);
			xPos += advance;

			if(highlightEndX<xPos-advance/2){
				highlightEnd=i=i0;
				highlightEndX=xPos-advance;
				break;
			}
		}
		if (i == 0) {
			highlightEnd = highlightEndX = 0;
		} else if (i == str.length()){
            highlightEnd=str.length();
			highlightEndX=0;
			if (highlightEnd > 0) {
				TexturePtr& t = linesCache.at(highlightLineEnd);
				if (t) highlightEndX = textureWidth(*t);
			}
		}
	}
	if((SDL_GetModState()&KMOD_SHIFT)==0){
        highlightStart=highlightEnd;
        highlightStartX=highlightEndX;
        highlightLineStart=highlightLineEnd;
    }
	adjustView();
}

void GUITextArea::moveCarrotDown(){
	if(highlightLineEnd==lines.size()-1){
		highlightEnd=lines.at(highlightLineEnd).length();
		highlightEndX=0;
		if (highlightEnd > 0) {
			TexturePtr& t = linesCache.at(highlightLineEnd);
			if (t) highlightEndX = textureWidth(*t);
		}
	}else{
		highlightLineEnd++;
		string* str=&lines.at(highlightLineEnd);

		//Find out closest match.
		int xPos=0;
		int i = 0;
		for (;;){
			int advance = 0;

			int i0 = i;
			int ch = utf8ReadForward(str->c_str(), i);
			if (ch <= 0) break;
			TTF_GlyphMetrics(widgetFont, ch, NULL, NULL, NULL, NULL, &advance);
			xPos += advance;

			if (highlightEndX<xPos - advance / 2){
				highlightEnd = i = i0;
				highlightEndX = xPos - advance;
				break;
			}
		}
		if (i == 0) {
			highlightEnd = highlightEndX = 0;
		} else if (i == str->length()){
			highlightEnd=str->length();
			highlightEndX=0;
            TexturePtr& t = linesCache.at(highlightLineEnd);
            if(t) highlightEndX=textureWidth(*t);
		}
	}
	if((SDL_GetModState()&KMOD_SHIFT)==0){
        highlightStart=highlightEnd;
        highlightStartX=highlightEndX;
        highlightLineStart=highlightLineEnd;
    }
	adjustView();
}

void GUITextArea::adjustView(){
	//Adjust view to current line.
	if(fontHeight*(highlightLineEnd-scrollBar->value)+4>height-4)
		scrollBar->value=highlightLineEnd-3;
	else if(highlightLineEnd-scrollBar->value<0)
		scrollBar->value=highlightLineEnd;

	//Find out the lenght of the longest line.
	int maxWidth=0;
    for(const TexturePtr& tex: linesCache){
        if(tex) {
            const int texWidth = textureWidth(*tex.get());
            if(texWidth>width-16&&texWidth>maxWidth)
                maxWidth=texWidth;
        }
	}
	
	//We need the horizontal scrollbar if any line is too long.
	if(maxWidth>0){
		scrollBar->height=height-16;
		scrollBarH->visible=true;
		scrollBarH->maxValue=maxWidth-width+24;
	}else{
		scrollBar->height=height;
		scrollBarH->visible=false;
		scrollBarH->value=0;
		scrollBarH->maxValue=0;
	}
	
	//Adjust the horizontal view.
	int carrotX=0;
	for(int n=0;n<highlightEnd;){
		int advance = 0;

		int ch = utf8ReadForward(lines.at(highlightLineEnd).c_str(), n);
		if (ch <= 0) break;
		TTF_GlyphMetrics(widgetFont, ch, NULL, NULL, NULL, NULL, &advance);
		carrotX += advance;
	}
	if(carrotX>width-24)
		scrollBarH->value=scrollBarH->maxValue;
	else
		scrollBarH->value=0;
	
	//Update vertical scrollbar.
	int rh=height-(scrollBarH->visible?16:0);
	int m=lines.size(),n=(int)floor((float)rh/(float)fontHeight);
	if(m>n){
		scrollBar->maxValue=m-n;
		scrollBar->smallChange=1;
		scrollBar->largeChange=n;
	}else{
		scrollBar->value=0;
		scrollBar->maxValue=0;
	}
}

void GUITextArea::drawHighlight(SDL_Renderer& renderer, int x,int y,SDL_Rect r,SDL_Color color){
    if(r.x<x) {
        int tmp_w = r.w - x + r.x;
        if(tmp_w<=0) return;
        r.w = tmp_w;
        r.x = x;
    }
    if(r.x+r.w > x+width){
        int tmp_w=width-r.x+x;
        if(tmp_w<=0) return;
        r.w=tmp_w;
    }
    if(r.y<y){
        int tmp_h=r.h - y + r.y;
        if(tmp_h<=0) return;
        r.h=tmp_h;
		r.y = y;
    }
    if(r.y+r.h > y+height){
        int tmp_h=height-r.y+y;
        if(tmp_h<=0) return;
        r.h=tmp_h;
    }
    SDL_SetRenderDrawColor(&renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(&renderer, &r);
}

void GUITextArea::render(SDL_Renderer& renderer, int x,int y,bool draw){	
	//There's no need drawing the GUIObject when it's invisible.
	if(!visible||!draw)
		return;
	
	//Get the absolute x and y location.
	x+=left;
	y+=top;
	
    {
	//Draw the box.
        const Uint32 color=0xFFFFFFFF;
        drawGUIBox(x,y,width,height,renderer,color);
    }

	//Place the highlighted area.
	SDL_Rect r;
    const SDL_Color color{128,128,128,255};
	if (editable) {
		if (highlightLineStart == highlightLineEnd){
			r.x = x - scrollBarH->value;
			r.y = y + ((highlightLineStart - scrollBar->value)*fontHeight);
			r.h = fontHeight;
			if (highlightStart < highlightEnd){
				r.x += highlightStartX;
				r.w = highlightEndX - highlightStartX;
			} else{
				r.x += highlightEndX;
				r.w = highlightStartX - highlightEndX;
			}
			drawHighlight(renderer, x, y, r, color);
		} else if (highlightLineStart < highlightLineEnd){
			int lnc = highlightLineEnd - highlightLineStart;
			for (int i = 0; i <= lnc; i++){
				r.x = x - scrollBarH->value;
				r.y = y + ((i + highlightLineStart - scrollBar->value)*fontHeight);
				r.w = width + scrollBarH->maxValue;
				r.h = fontHeight;
				if (i == 0){
					r.x += highlightStartX;
					r.w -= highlightStartX;
				} else if (i == lnc){
					r.w = highlightEndX;
				}
				if (lines.at(i + highlightLineStart).empty()){
					r.w = fontHeight / 4;
				}
				drawHighlight(renderer, x, y, r, color);
			}
		} else{
			int lnc = highlightLineStart - highlightLineEnd;
			for (int i = 0; i <= lnc; i++){
				r.x = x - scrollBarH->value;
				r.y = y + ((i + highlightLineEnd - scrollBar->value)*fontHeight);
				r.w = width + scrollBarH->maxValue;
				r.h = fontHeight;
				if (i == 0){
					r.x += highlightEndX;
					r.w -= highlightEndX;
				} else if (i == lnc){
					r.w = highlightStartX;
				}
				if (lines.at(i + highlightLineEnd).empty()){
					r.w = fontHeight / 4;
				}
				drawHighlight(renderer, x, y, r, color);
			}
		}
	}

	//Draw text.
	int lineY=0;
    for(int line=scrollBar->value;line<(int)linesCache.size();line++){
		TexturePtr& it = linesCache[line];
		if(it){
			if(lineY<height){
				SDL_Rect r = { scrollBarH->value, 0, std::min(width - 17, textureWidth(*it.get()) - scrollBarH->value), textureHeight(*it.get()) };
				int over=-height+lineY+fontHeight;
				if(over>0) r.h-=over;
                const SDL_Rect dstRect={x+1,y+1+lineY,r.w,r.h};
                if(r.w>0 && r.h>0) SDL_RenderCopy(&renderer,it.get(),&r,&dstRect);

				// draw hyperlinks
				if (!editable && line<(int)hyperlinks.size()) {
					r.y = lineY + fontHeight - 1;
					if (r.y < height){
						r.y += y + 1;
						r.h = 1;
						for (const Hyperlink& lnk : hyperlinks[line]) {
							r.x = clamp(lnk.startX - scrollBarH->value, 0, width - 17);
							r.w = clamp(lnk.endX - scrollBarH->value, 0, width - 17);
							if (r.w > r.x) {
								r.w -= r.x;
								r.x += x + 1;
								SDL_SetRenderDrawColor(&renderer, 0, 0, 0, 255);
								SDL_RenderFillRect(&renderer, &r);
							}
						}
					}
				}
			}else{
				break;
			}
		}
		lineY+=fontHeight;
	}
	
	//Only draw the carrot when focus.
	if(state==2&&editable){
		r.x=x-scrollBarH->value+highlightEndX;
		r.y=y+4+fontHeight*(highlightLineEnd-scrollBar->value);
		r.w=2;
		r.h=fontHeight-4;
		
		//Make sure that the carrot is inside the textbox.
		if((r.y<y+height-4)&&(r.y>y)&&(r.x>x-1)&&(r.x<x+width-16)){
            drawHighlight(renderer,x,y,r,SDL_Color{0,0,0,127});
		}
	}
	
	//We now need to draw all the children of the GUIObject.
	for(unsigned int i=0;i<childControls.size();i++){
        childControls[i]->render(renderer,x,y,draw);
	}
}

void GUITextArea::setString(SDL_Renderer& renderer, const std::string& input, bool wordWrap) {
	WordWrapper wrapper;
	wrapper.wordWrap = wordWrap;
	setString(renderer, input, wrapper);
}

void GUITextArea::setString(SDL_Renderer& renderer, const std::string& input, WordWrapper& wrapper) {
	//Clear previous content if any.
	//Delete every line.
	lines.clear();
	linesCache.clear();

	//Copy values.
	wrapper.maxWidth = width - 16;
	wrapper.font = widgetFont;
	wrapper.addString(lines, input);

	//Render and cache text.
	for (const std::string& s : lines) {
		linesCache.push_back(textureFromText(renderer, *widgetFont, s.c_str(), objThemes.getTextColor(true)));
	}

	adjustView();
}

void GUITextArea::setStringArray(SDL_Renderer& renderer, const std::vector<std::string>& input, bool wordWrap) {
	WordWrapper wrapper;
	wrapper.wordWrap = wordWrap;
	setStringArray(renderer, input, wrapper);
}

void GUITextArea::setStringArray(SDL_Renderer& renderer, const std::vector<std::string>& input, WordWrapper& wrapper) {
	//Free cached images.
	linesCache.clear();
	lines.clear();

	//Copy values.
	wrapper.maxWidth = width - 16;
	wrapper.font = widgetFont;
	wrapper.addLines(lines, input);

	//Render and cache text.
    for(const std::string& s: lines) {
        linesCache.push_back(textureFromText(renderer,*widgetFont,s.c_str(),objThemes.getTextColor(true)));
    }
	
	adjustView();
}

void GUITextArea::setStringArray(SDL_Renderer &renderer, std::vector<SurfacePtr>& surfaces) {
	//Free cached images.
	linesCache.clear();
	lines.clear();

	//Copy values.
	lines.resize(surfaces.size());
	for (SurfacePtr& surface : surfaces) {
		linesCache.emplace_back(SDL_CreateTextureFromSurface(&renderer, surface.get()));
	}

	adjustView();
}

void GUITextArea::extractHyperlinks() {
	const int lm = lines.size();
	hyperlinks.clear();

	if (lm <= 0) return;
	hyperlinks.resize(lm);

	for (int l = 0; l < lm; l++) {
		const char* s = lines[l].c_str();
		for (int i = 0, m = lines[l].size(); i < m; i++) {
			const int lps = i;
			std::string url;

			// we only support http or https
			if ((s[i] == 'H' || s[i] == 'h')
				&& (s[i + 1] == 'T' || s[i + 1] == 't')
				&& (s[i + 2] == 'T' || s[i + 2] == 't')
				&& (s[i + 3] == 'P' || s[i + 3] == 'p'))
			{
				if (s[i + 4] == ':' && s[i + 5] == '/' && s[i + 6] == '/') {
					// http
					i += 7;
					url = "http://";
				} else if ((s[i + 4] == 'S' || s[i + 4] == 's') && s[i + 5] == ':' && s[i + 6] == '/' && s[i + 7] == '/') {
					// https
					i += 8;
					url = "https://";
				} else {
					continue;
				}
				for (; i < m; i++) {
					char c = s[i];
					// url ends with following character
					if (c == '\0' || c == ' ' || c == ')' || c == ']' || c == '}' || c == '>' || c == '\r' || c == '\n' || c == '\t') {
						break;
					}
					url.push_back(c);
				}
			} else {
				continue;
			}

			const int lpe = i;

			Hyperlink hyperlink = {};
			TTF_SizeUTF8(widgetFont, lines[l].substr(0, lps).c_str(), &hyperlink.startX, NULL);
			TTF_SizeUTF8(widgetFont, lines[l].substr(0, lpe).c_str(), &hyperlink.endX, NULL);
			hyperlink.url = lines[l].substr(lps, lpe - lps);

			hyperlinks[l].push_back(hyperlink);
		}
	}
}

string GUITextArea::getString(){
	string tmp;
	for(vector<string>::iterator it=lines.begin();it!=lines.end();++it){
		//Append a newline only if not the first line.
		if(it!=lines.begin())
			tmp.append(1,'\n');
		//Append the line.
		tmp.append(*it);
	}
	return tmp;
}

void GUITextArea::onResize(){
	scrollBar->left=width-16;
	scrollBar->height=height;
	
	if(scrollBarH->visible)
		scrollBar->height-=16;
	
	scrollBarH->top=height-16;
	scrollBarH->width=width-16;
	
	adjustView();
}
