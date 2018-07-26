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

#ifndef GUITEXTAREA_H
#define GUITEXTAREA_H

#include "GUIObject.h"
#include "GUIScrollBar.h"

//Widget for multiline text input.
class GUITextArea:public GUIObject{
private:	
	//Method that will remove the last character of the text.
    void backspaceChar(SDL_Renderer &renderer);
    void deleteChar(SDL_Renderer &renderer);
	
	//Methods to move the carrot by one character/line.
	void moveCarrotLeft();
	void moveCarrotRight();
	void moveCarrotUp();
	void moveCarrotDown();

	// Remove all highlighted text.
    void removeHighlight(SDL_Renderer &renderer);

	// Input new text.
	void inputText(SDL_Renderer &renderer, const char* s);
	
	//Method to adjust view so carrot stays visible.
	void adjustView();
	
	//Pointer to the font used in the widget.
	TTF_Font* widgetFont;
	
	//Widget's text.
	//One line per vector element.
	std::vector<std::string> lines;
	
	//Cache for rendered lines.
	//Will be updated alongside with variable text.
    std::vector<TexturePtr> linesCache;
	
	//Variable for carrot position.
	int highlightLineStart;
	int highlightLineEnd;
	int highlightStart;
	int highlightStartX;
	int highlightEnd;
	int highlightEndX;

	//Height of the font.
	int fontHeight;
	
	//Scrollbar widget.
	GUIScrollBar* scrollBar;
	GUIScrollBar* scrollBarH;

	//A struct to save hyperlink.
	struct Hyperlink {
		int startX, endX;
		std::string url;
	};

	//Hyperlinks.
	std::vector<std::vector<Hyperlink> > hyperlinks;

    void drawHighlight(SDL_Renderer& renderer, int x, int y, SDL_Rect r, SDL_Color color);
public:
	//Constructor.
	//left: The relative x location of the GUITextArea.
	//top: The relative y location of the GUITextArea.
	//witdh: The width of the GUITextArea.
	//height: The height of the GUITextArea.
	//enabled: Boolean if the GUITextArea is enabled or not.
	//visible: Boolean if the GUITextArea is visisble or not.
    GUITextArea(ImageManager& imageManager, SDL_Renderer& renderer,int left=0,int top=0,int width=0,int height=0,bool enabled=true,bool visible=true);
	
	//Method used to change the font.
	//font: Pointer to the font
	void setFont(TTF_Font* font);
	
	//Method used to reposition scrollbars after a resize.
	void resize();
	
	//Method used to get widget's text in a single string.
	std::string getString();
	
	//Method used to set widget's text.
    void setString(SDL_Renderer& renderer, std::string input);
    void setStringArray(SDL_Renderer &renderer, std::vector<std::string> input);

	//Extract hyperlinks from text.
	//Currently only http and https links are extracted.
	void extractHyperlinks();
	
	//Bool if user can edit text in the widget.
	bool editable;
	
	//Method used to handle mouse and/or key events.
	//x: The x mouse location.
	//y: The y mouse location.
	//enabled: Boolean if the parent is enabled or not.
	//visible: Boolean if the parent is visible or not.
	//processed: Boolean if the event has been processed (by the parent) or not.
	//Returns: Boolean if the event is processed by the child.
    virtual bool handleEvents(SDL_Renderer&renderer, int x=0, int y=0, bool enabled=true, bool visible=true, bool processed=false);
	//Method that will render the GUITextArea.
	//x: The x location to draw the GUITextArea. (x+left)
	//y: The y location to draw the GUITextArea. (y+top)
    virtual void render(SDL_Renderer &renderer, int x=0, int y=0, bool draw=true);
};

#endif
