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

#include "GUITextArea.h"
#include <cmath>
#include <ctype.h>
using namespace std;

GUITextArea::GUITextArea(int left,int top,int width,int height,bool enabled,bool visible):
	GUIObject(left,top,width,height,NULL,-1,enabled,visible),editable(true){
	
	key=-1;
	keyHoldTime=keyTime=0;
	
	//Set some default values.
	state=0;
	setFont(fontText);

	highlightLineStart=highlightLineEnd=0;
	highlightStartX=highlightEndX=0;
	highlightStart=highlightEnd=0;
	
	//Add empty text.
	lines.push_back("");
	linesCache.push_back(NULL);
	
	//Create scrollbar widget.
	scrollBar=new GUIScrollBar(width-16,0,16,height,1,0,0,0);
	childControls.push_back(scrollBar);
	
	scrollBarH=new GUIScrollBar(0,height-16,width-16,16,0,0,0,0,100,500,true,false);
	childControls.push_back(scrollBarH);
}

GUITextArea::~GUITextArea(){
	//Free cached images.
	for(unsigned int i=0;i<linesCache.size();i++){
		SDL_FreeSurface(linesCache[i]);
	}
	linesCache.clear();
}

void GUITextArea::setFont(TTF_Font* font){
	//NOTE: This fuction shouldn't be called after adding items, so no need to update the whole cache.
	widgetFont=font;
	fontHeight=TTF_FontHeight(font)+1;
}

bool GUITextArea::handleEvents(int x,int y,bool enabled,bool visible,bool processed){
	//Boolean if the event is processed.
	bool b=processed;
	
	//The GUIObject is only enabled when he and his parent are enabled.
	enabled=enabled && this->enabled;
	//The GUIObject is only enabled when he and his parent are enabled.
	visible=visible && this->visible;
	
	//Get the absolute position.
	x+=left;
	y+=top;
	
	//Update the vertical scrollbar.
	b=b||scrollBar->handleEvents(x,y,enabled,visible,b);
	if(!editable)
		highlightLineStart=scrollBar->value;
	
	//NOTE: We don't reset the state to have a "focus" effect.
	//Only check for events when the object is both enabled and visible.
	if(enabled&&visible){
		//Check if there's a key press and the event hasn't been already processed.
		if(state==2 && event.type==SDL_KEYDOWN && !b && editable){
			//Get the keycode.
			int key=(int)event.key.keysym.unicode;

			//Check if the key is supported.
			if(key>=32&&key<=126){
				removeHighlight();
				string* str=&lines.at(highlightLineStart);
				str->insert((size_t)highlightEnd,1,char(key));
				highlightEnd++;
				highlightStart=highlightEnd;
				int advance;
				TTF_GlyphMetrics(widgetFont,char(key),NULL,NULL,NULL,NULL,&advance);
				highlightStartX=highlightEndX=highlightStartX+advance;

				//Update cache.
				SDL_Surface** c=&linesCache.at(highlightLineStart);
				if(*c) SDL_FreeSurface(*c);
				SDL_Color black={0,0,0,0};
				*c=TTF_RenderUTF8_Blended(widgetFont,str->c_str(),black);

				//Update view if needed.
				adjustView();

				//If there is an event callback then call it.
				if(eventCallback){
					GUIEvent e={eventCallback,name,this,GUIEventChange};
					GUIEventQueue.push_back(e);
				}
			}else if(event.key.keysym.sym==SDLK_BACKSPACE){
				//Set the key values correct.
				this->key=SDLK_BACKSPACE;
				keyHoldTime=0;
				keyTime=5;

				//Delete one character direct to prevent a lag.
				backspaceChar();
			}else if(event.key.keysym.sym==SDLK_DELETE){
				//Set the key values correct.
				this->key=SDLK_DELETE;
				keyHoldTime=0;
				keyTime=5;

				//Delete one character direct to prevent a lag.
				deleteChar();
			}else if(event.key.keysym.sym==SDLK_RETURN){
				removeHighlight();
				//Split the current line and update.
				string str2=lines.at(highlightLineEnd).substr(highlightStart);
				lines.at(highlightLineStart)=lines.at(highlightLineStart).substr(0,highlightStart);

				SDL_Surface** c=&linesCache.at(highlightLineStart);
				if(*c) SDL_FreeSurface(*c);
				SDL_Color black={0,0,0,0};
				*c=TTF_RenderUTF8_Blended(widgetFont,lines.at(highlightLineStart).c_str(),black);

				//Calculate indentation.
				int indent=0;
				for (int i=0; i<lines.at(highlightLineStart).length(); i++){
					if (isspace(lines.at(highlightLineStart)[i]))
						indent++;
					else
						break;
				}
				str2.insert(0,indent,' ');

				//Add the rest in a new line.
				highlightLineStart++;
				highlightStart=indent;
				highlightEnd=highlightStart;
				highlightLineEnd++;

				highlightStartX=0;
				for(int i=0; i<indent; i++){
					int advance;
					TTF_GlyphMetrics(widgetFont,str2.at(i),NULL,NULL,NULL,NULL,&advance);
					highlightStartX+=advance;
				}
				highlightEndX=highlightStartX;

				lines.insert(lines.begin()+highlightLineStart,str2);

				SDL_Surface* c2;
				c2=TTF_RenderUTF8_Blended(widgetFont,str2.c_str(),black);
				linesCache.insert(linesCache.begin()+highlightLineStart,c2);

				adjustView();

				//If there is an event callback then call it.
				if(eventCallback){
					GUIEvent e={eventCallback,name,this,GUIEventChange};
					GUIEventQueue.push_back(e);
				}
			}else if(event.key.keysym.sym==SDLK_TAB){
				removeHighlight();
				//Add a tabulator or here just 2 spaces to the string.
				string* str=&lines.at(highlightLineStart);
				str->insert((size_t)highlightStart,2,char(' '));

				int advance;
				TTF_GlyphMetrics(widgetFont,' ',NULL,NULL,NULL,NULL,&advance);
				highlightStart+=2;
				highlightStartX=advance*2;
				highlightEnd=highlightStart;
				highlightEndX=highlightStartX;
				
				//Update cache.
				SDL_Surface** c=&linesCache.at(highlightLineStart);
				if(*c) SDL_FreeSurface(*c);
				SDL_Color black={0,0,0,0};
				*c=TTF_RenderUTF8_Blended(widgetFont,str->c_str(),black);
				
				adjustView();
			}else if(event.key.keysym.sym==SDLK_RIGHT){
				//Set the key values correct.
				this->key=SDLK_RIGHT;
				keyHoldTime=0;
				keyTime=5;
				
				//Move the carrot once to prevent a lag.
				moveCarrotRight();
			}else if(event.key.keysym.sym==SDLK_LEFT){
				//Set the key values correct.
				this->key=SDLK_LEFT;
				keyHoldTime=0;
				keyTime=5;
				
				//Move the carrot once to prevent a lag.
				moveCarrotLeft();
			}else if(event.key.keysym.sym==SDLK_DOWN){
				//Set the key values correct.
				this->key=SDLK_DOWN;
				keyHoldTime=0;
				keyTime=5;
				
				//Move the carrot once to prevent a lag.
				moveCarrotDown();
			}else if(event.key.keysym.sym==SDLK_UP){
				//Set the key values correct.
				this->key=SDLK_UP;
				keyHoldTime=0;
				keyTime=5;
				
				//Move the carrot once to prevent a lag.
				moveCarrotUp();
			}
			
			//The event has been processed.
			b=true;
		}else if(state==2 && event.type==SDL_KEYUP && !b){
			//Check if released key is the same as the holded key.
			if(event.key.keysym.sym==key){
				//It is so stop the key.
				key=-1;
			}
		}
		
		//The mouse location (x=i, y=j) and the mouse button (k).
		int i,j,k;
		k=SDL_GetMouseState(&i,&j);
		
		//Check if the mouse is inside the GUIObject.
		if(i>=x&&i<x+width&&j>=y&&j<y+height){
			//We can only increase our state. (nothing->hover->focus).
			if(state!=2){
				state=1;
			}
			
			//Check for mouse wheel scrolling.
			//Scroll horizontally if mouse is over the horizontal scrollbar.
			//Otherwise scroll vertically.
			if(j>=y+height-16&&scrollBarH->visible){
				if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button==SDL_BUTTON_WHEELDOWN){
					scrollBarH->value+=20;
					if(scrollBarH->value>scrollBarH->maxValue)
						scrollBarH->value=scrollBarH->maxValue;
				}else if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button==SDL_BUTTON_WHEELUP){
					scrollBarH->value-=20;
					if(scrollBarH->value<0)
						scrollBarH->value=0;
				}
			}else{
				if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button==SDL_BUTTON_WHEELDOWN){
					scrollBar->value++;
					if(scrollBar->value>scrollBar->maxValue)
						scrollBar->value=scrollBar->maxValue;
				}else if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button==SDL_BUTTON_WHEELUP){
					scrollBar->value--;
					if(scrollBar->value<0)
						scrollBar->value=0;
				}
			}
			
			//When mouse is not over the scrollbar.
			if(i<x+width-16&&j<(scrollBarH->visible?y+height-16:y+height)&&editable){
				//Update the cursor type.
				currentCursor=CURSOR_CARROT;
				
				//Move carrot to the place clicked.
				int mouseLine=clamp((int)floor(float(j-y)/float(fontHeight))+scrollBar->value,0,lines.size()-1);
				string* str=&lines.at(mouseLine);
				value=str->length();
				
				int clickX=i-x+scrollBarH->value;
				int finalX=0;
				int finalPos=str->length();
				
				for(unsigned int i=0;i<str->length();i++){
					int advance;
					TTF_GlyphMetrics(widgetFont,str->at(i),NULL,NULL,NULL,NULL,&advance);
					finalX+=advance;
					
					if(clickX<finalX-advance/2){
						finalPos=i;
						finalX-=advance;
						break;
					}
				}
				//if(k&SDL_BUTTON(1)){
				if(event.type==SDL_MOUSEBUTTONUP){
					state=2;
					highlightEnd=finalPos;
					highlightEndX=finalX;
					highlightLineEnd=mouseLine;
				}else if(event.type==SDL_MOUSEBUTTONDOWN){
					state=2;
					highlightStart=highlightEnd=finalPos;
					highlightStartX=highlightEndX=finalX;
					highlightLineStart=mouseLine;
				}else if(event.type==SDL_MOUSEMOTION&&(k&SDL_BUTTON(1))){
					state=2;
					highlightEnd=finalPos;
					highlightEndX=finalX;
					highlightLineEnd=mouseLine;
				}
				//}
			}
		}else{
			//The mouse is outside the TextBox.
			//If we don't have focus but only hover we lose it.
			if(state==1){
				state=0;
			}
			
			//If it's a click event outside the textbox then we blur.
			if(event.type==SDL_MOUSEBUTTONUP && event.button.button==SDL_BUTTON_LEFT){
				//Set state to 0.
				state=0;
			}
		}
	}
	
	//Process child controls event except for the scrollbar.
	//That's why i starts at one.
	for(unsigned int i=1;i<childControls.size();i++){
		bool b1=childControls[i]->handleEvents(x,y,enabled,visible,b);
		
		//The event is processed when either our or the childs is true (or both).
		b=b||b1;
	}
	return b;
}

void GUITextArea::removeHighlight(){
	if (highlightLineStart==highlightLineEnd) {
		int start=highlightStart, end=highlightEnd, startx=highlightStartX;
		if(highlightStart>highlightEnd){
			start=highlightEnd;
			end=highlightStart;
			startx=highlightEndX;
		}
		string *str=&lines.at(highlightLineStart);
		str->erase(start,end-start);

		highlightStart=highlightEnd=start;
		highlightStartX=highlightEndX=startx;

		// Update cache.
		SDL_Surface** c=&linesCache.at(highlightLineStart);
		if(*c) SDL_FreeSurface(*c);
		SDL_Color black={0,0,0,0};
		*c=TTF_RenderUTF8_Blended(widgetFont,str->c_str(),black);
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
		string *str=&lines.at(startLine);

		str->erase(start,str->length()-start);

		if(endLine-startLine>=2){
			for(int i=startLine+1; i < endLine; i++){
				SDL_Surface** c=&linesCache.at(i);
				if(*c) SDL_FreeSurface(*c);
				lines.erase(lines.begin()+i);
				linesCache.erase(linesCache.begin()+i);
				endLine--;
				i--;
			}
		}

		string *str2=&lines.at(endLine);

		str2->erase(0, end);
		str->append(*str2);

		SDL_Surface** c=&linesCache.at(endLine);
		if(*c) SDL_FreeSurface(*c);
		lines.erase(lines.begin()+endLine);
		linesCache.erase(linesCache.begin()+endLine);

		highlightLineStart=highlightLineEnd=startLine;
		highlightStart=highlightEnd=start;
		highlightStartX=highlightEndX=startx;

		// Update cache.
		c=&linesCache.at(startLine);
		if(*c) SDL_FreeSurface(*c);
		SDL_Color black={0,0,0,0};
		*c=TTF_RenderUTF8_Blended(widgetFont,str->c_str(),black);
	}
	adjustView();
}

void GUITextArea::deleteChar(){
	if (highlightLineStart==highlightLineEnd && highlightStart==highlightEnd){
		highlightEnd++;
		if(highlightEnd>lines.at(highlightLineEnd).length()){
			if(highlightLineEnd==lines.size()-1){
				highlightEnd--;
			}else{
				highlightLineEnd++;
				highlightEnd=0;
			}
		}
	}
	removeHighlight();
	//If there is an event callback.
	if(eventCallback){
		GUIEvent e={eventCallback,name,this,GUIEventChange};
		GUIEventQueue.push_back(e);
	}
}

void GUITextArea::backspaceChar(){
	if(highlightLineStart==highlightLineEnd && highlightStart==highlightEnd){
		highlightStart--;
		if(highlightStart<0){
			if(highlightLineStart==0){
				highlightStart=0;
			}else{
				highlightLineStart--;
				highlightStart=lines.at(highlightLineStart).length();
				highlightStartX=0;
				SDL_Surface** c=&linesCache.at(highlightLineStart);
				if (*c) highlightStartX=(*c)->w;
				highlightEndX=highlightStartX;
			}
		}else{
			int advance;
			TTF_GlyphMetrics(widgetFont,lines.at(highlightLineStart).at(highlightStart),NULL,NULL,NULL,NULL,&advance);
			highlightStartX-=advance;
			highlightEndX=highlightStartX;
		}
	}
	removeHighlight();

	//If there is an event callback.
	if(eventCallback){
		GUIEvent e={eventCallback,name,this,GUIEventChange};
		GUIEventQueue.push_back(e);
	}
}

void GUITextArea::moveCarrotRight(){
	highlightEnd++;
	if (highlightEnd>lines.at(highlightLineEnd).length()){
		if (highlightLineEnd==lines.size()-1){
			highlightEnd--;
		}else{
			highlightEnd=0;
			highlightEndX=0;
			highlightLineEnd++;
		}
	}else{
		int advance;
		TTF_GlyphMetrics(widgetFont,lines.at(highlightLineEnd).at(highlightEnd-1),NULL,NULL,NULL,NULL,&advance);
		highlightEndX+=advance;
	}
	if((SDL_GetModState()&KMOD_SHIFT)==0){
        highlightLineStart=highlightLineEnd;
        highlightStart=highlightEnd;
        highlightStartX=highlightEndX;
    }
	adjustView();
}

void GUITextArea::moveCarrotLeft(){
	highlightEnd--;
	if (highlightEnd<0){
		if (highlightLineEnd==0){
			highlightEnd++;
		}else{
			highlightLineEnd--;
			highlightEnd=lines.at(highlightLineEnd).length();
			highlightEndX=0;
			SDL_Surface** c=&linesCache.at(highlightLineEnd);
			if(*c) highlightEndX=(*c)->w;
		}
	}else{
		int advance;
		TTF_GlyphMetrics(widgetFont,lines.at(highlightLineEnd).at(highlightEnd),NULL,NULL,NULL,NULL,&advance);
		highlightEndX-=advance;
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
		string* str=&lines.at(highlightLineEnd);

		//Find out closest match.
		int xPos=0;
		size_t i;
		for(i=0;i<str->length();i++){
			int advance;
			TTF_GlyphMetrics(widgetFont,str->at(i),NULL,NULL,NULL,NULL,&advance);
			xPos+=advance;

			if(highlightEndX<xPos-advance/2){
				highlightEnd=i;
				highlightEndX=xPos-advance;
				break;
			}
		}
		if(i==str->length()){
			highlightEnd=str->length();
			highlightEndX=0;
			SDL_Surface** c=&linesCache.at(highlightLineEnd);
			if(*c) highlightEndX=(*c)->w;
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
		SDL_Surface** c=&linesCache.at(highlightLineEnd);
		if(*c) highlightEndX=(*c)->w;
	}else{
		highlightLineEnd++;
		string* str=&lines.at(highlightLineEnd);

		//Find out closest match.
		int xPos=0;
		size_t i;
		for(i=0;i<str->length();i++){
			int advance;
			TTF_GlyphMetrics(widgetFont,str->at(i),NULL,NULL,NULL,NULL,&advance);
			xPos+=advance;

			if(highlightEndX<xPos-advance/2){
				highlightEnd=i;
				highlightEndX=xPos-advance;
				break;
			}
		}
		if(i==str->length()){
			highlightEnd=str->length();
			highlightEndX=0;
			SDL_Surface** c=&linesCache.at(highlightLineEnd);
			if(*c) highlightEndX=(*c)->w;
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
	for(vector<SDL_Surface*>::iterator it=linesCache.begin();it!=linesCache.end();++it){
		if((*it)&&(*it)->w>width-16&&(*it)->w>maxWidth)
			maxWidth=(*it)->w;
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
	for(int n=0;n<highlightEnd;n++){
		int advance;
		TTF_GlyphMetrics(widgetFont,lines.at(highlightLineEnd).at(n),NULL,NULL,NULL,NULL,&advance);
		carrotX+=advance;
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

void GUITextArea::drawHighlight(int x,int y,SDL_Rect* r,Uint32 color){
	if(r->x<x) {
		int tmp_w = r->w - x + r->x;
		if(tmp_w<0) return;
		r->w = tmp_w;
		r->x = left;
	}
	if(r->x+r->w > x+width){
		int tmp_w=width-r->x+x;
		if(tmp_w<=0) return;
		r->w=tmp_w;
	}
	if(r->y<y){
		int tmp_h=r->h - y + r->y;
		if(tmp_h<=0) return;
		r->h=tmp_h;
	}
	if(r->y+r->h > y+height){
		int tmp_h=height-r->y+y;
		if(tmp_h<=0) return;
		r->h=tmp_h;
	}
	SDL_FillRect(screen,r,color);
}

void GUITextArea::render(int x,int y,bool draw){
	//FIXME: Logic in the render method since that is update constant.
	if(key!=-1){
		//Increase the key time.
		keyHoldTime++;
		//Make sure the deletionTime isn't to short.
		if(keyHoldTime>=keyTime){
			keyHoldTime=0;
			keyTime--;
			if(keyTime<1)
				keyTime=1;
			
			//Now check the which key it was.
			switch(key){
				case SDLK_BACKSPACE:
					backspaceChar();
					break;
				case SDLK_DELETE:
					deleteChar();
					break;
				case SDLK_LEFT:
					moveCarrotLeft();
					break;
				case SDLK_RIGHT:
					moveCarrotRight();
					break;
				case SDLK_UP:
					moveCarrotUp();
					break;
				case SDLK_DOWN:
					moveCarrotDown();
					break;
			}
		}
	}
	
	//There's no need drawing the GUIObject when it's invisible.
	if(!visible||!draw)
		return;
	
	//Get the absolute x and y location.
	x+=left;
	y+=top;
	
	//Draw the box.
	Uint32 color=0xFFFFFFFF;
	drawGUIBox(x,y,width,height,screen,color);

	//Place the highlighted area.
	SDL_Rect r;
	color=SDL_MapRGB(screen->format,128,128,128);
	if(highlightLineStart==highlightLineEnd){
		r.x=x-scrollBarH->value;
		r.y=y+((highlightLineStart-scrollBar->value)*fontHeight);
		r.h=fontHeight;
		if(highlightStart<highlightEnd){
			r.x+=highlightStartX;
			r.w=highlightEndX-highlightStartX;
		}else{
			r.x+=highlightEndX;
			r.w=highlightStartX-highlightEndX;
		}
		drawHighlight(x,y,&r,color);
	}else if(highlightLineStart<highlightLineEnd){
		int lnc=highlightLineEnd-highlightLineStart;
		for(int i=0;i<=lnc;i++){
			r.x=x-scrollBarH->value;
			r.y=y+((i+highlightLineStart-scrollBar->value)*fontHeight);
			r.w=width+scrollBarH->maxValue;
			r.h=fontHeight;
			if(i==0){
				r.x+=highlightStartX;
				r.w-=highlightStartX;
			}else if(i==lnc){
				r.w=highlightEndX;
			}
			if(lines.at(i+highlightLineStart).empty()){
				r.w=fontHeight/4;
			}
			drawHighlight(x,y,&r,color);
		}
	}else{
		int lnc=highlightLineStart-highlightLineEnd;
		for(int i=0;i<=lnc;i++){
			r.x=x-scrollBarH->value;
			r.y=y+((i+highlightLineEnd-scrollBar->value)*fontHeight);
			r.w=width+scrollBarH->maxValue;
			r.h=fontHeight;
			if(i==0){
				r.x+=highlightEndX;
				r.w-=highlightEndX;
			}else if(i==lnc){
				r.w=highlightStartX;
			}
			if(lines.at(i+highlightLineEnd).empty()){
				r.w=fontHeight/4;
			}
			drawHighlight(x,y,&r,color);
		}
	}

	//Draw text.
	int lineY=0;
	for(std::vector<SDL_Surface*>::iterator it=linesCache.begin()+scrollBar->value;it!=linesCache.end();++it){
		if(*it){
			if(lineY<height){
				SDL_Rect r={scrollBarH->value,0,width-17,(*it)->h};
				int over=-height+lineY+fontHeight;
				if(over>0) r.h-=over;
				applySurface(x+1,y+1+lineY,*it,screen,&r);
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
			//SDL_FillRect(screen,&r,0);
			drawHighlight(x,y,&r,0x00000000);
		}
	}
	
	//We now need to draw all the children of the GUIObject.
	for(unsigned int i=0;i<childControls.size();i++){
		childControls[i]->render(x,y,draw);
	}
}

void GUITextArea::setString(std::string input){
	//Clear previous content if any.
	//Delete every line.
	lines.clear();
	//Free cached images.
	for(unsigned int i=0;i<linesCache.size();i++){
		SDL_FreeSurface(linesCache[i]);
	}
	linesCache.clear();
	
	size_t linePos=0,lineLen=0;
	SDL_Color black={0,0,0,0};
	SDL_Surface* bm=NULL;
	
	//Loop through the input string.
	for(size_t i=0;i<input.length();++i){
		//Check when we come in end of a line.
		if(input.at(i)=='\n'){
			//Check if the line is empty.
			if(lineLen==0){
				lines.push_back("");
				linesCache.push_back(NULL);
			}else{
				//Read the whole line.
				string line=input.substr(linePos,lineLen);
				lines.push_back(line);
				
				//Render and cache text.
				bm=TTF_RenderUTF8_Blended(widgetFont,line.c_str(),black);
				linesCache.push_back(bm);
			}
			//Skip '\n' in end of the line.
			linePos=i+1;
			lineLen=0;
		}else{
			lineLen++;
		}
	}
	
	//The string might not end with a newline.
	//That's why we're going to add end rest of the string as one line.
	string line=input.substr(linePos);
	lines.push_back(line);
	
	bm=TTF_RenderUTF8_Blended(widgetFont,line.c_str(),black);
	linesCache.push_back(bm);
	
	adjustView();
}

void GUITextArea::setStringArray(std::vector<std::string> input){
	//Free cached images.
	for(unsigned int i=0;i<linesCache.size();i++){
		SDL_FreeSurface(linesCache[i]);
	}
	linesCache.clear();
	
	//Copy values.
	lines=input;
	
	//Draw new strings.
	SDL_Color black={0,0,0,0};
	for(vector<string>::iterator it=lines.begin();it!=lines.end();++it){
		SDL_Surface* bm=TTF_RenderUTF8_Blended(widgetFont,(*it).c_str(),black);
		linesCache.push_back(bm);
	}
	
	adjustView();
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

void GUITextArea::resize(){
	scrollBar->left=width-16;
	scrollBar->height=height;
	
	if(scrollBarH->visible)
		scrollBar->height-=16;
	
	scrollBarH->top=height-16;
	scrollBarH->width=width-16;
	
	adjustView();
}
