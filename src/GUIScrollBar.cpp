/****************************************************************************
** Copyright (C) 2011 Luka Horvat <redreaper132 at gmail.com>
** Copyright (C) 2011 Edward Lii <edward_iii at myway.com>
** Copyright (C) 2011 O. Bahri Gordebak <gordebak at gmail.com>
**
**
** This file may be used under the terms of the GNU General Public
** License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
**  You should have received a copy of the GNU General Public License
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/

#include "GUIScrollBar.h"
using namespace std;

void GUIScrollBar::calcPos(){
	float f,f1,f2;
	if(value<min) value=min;
	else if(value>max) value=max;
	if(orientation){
		f=(float)(top+16);
		f2=(float)(height-32);
	}else{
		f=(float)(left+16);
		f2=(float)(width-32);
	}
	if(largeChange<=0) f2=-1;
	if(f2>0){
		valuePerPixel = (max - min + largeChange) / f2;
		if(valuePerPixel > 0.0001f) f1 = largeChange / valuePerPixel;
		if(f1 < 4 && f2 > 4){
			valuePerPixel = (max - min) / (f2 - 4);
			f1 = 4;
		}
		thumbStart = f + (value - min) / valuePerPixel;
		thumbEnd = thumbStart + f1;
	}else{
		valuePerPixel = -1;
		thumbStart = f;
		thumbEnd = f - 1;
	}
}

bool GUIScrollBar::handleEvents(int x,int y,bool enabled,bool visible,bool processed){
	// ???
	if(event.type==SDL_QUIT){
		nextState=STATE_EXIT;
		if(GUIObjectRoot){
			delete GUIObjectRoot;
			GUIObjectRoot=NULL;
		}
		GUIEventQueue.clear();
		return true;
	}
	//===
	bool b=processed;
	enabled=this->enabled && enabled;
	visible=this->visible && visible;
	//===
	if(event.type==SDL_MOUSEBUTTONUP || !(enabled&&visible)){
		state=0;
	}else if(event.type==SDL_MOUSEMOTION || event.type==SDL_MOUSEBUTTONDOWN){
		int i,j,k,f,f1,f2,f3;
		state&=~0xFF;
		k=SDL_GetMouseState(&i,&j);
		i-=x;
		j-=y;
		bool bInControl_0;
		if(orientation){
			f=top;
			f1=f+height;
			bInControl_0=(i>=left&&i<left+width);
			i=j;
		}else{
			f=left;
			f1=f+width;
			bInControl_0=(j>=top&&j<top+height);
		}
		//===
		if((state&0x0000FF00)==0x300&&(k&SDL_BUTTON(1))&&event.type==SDL_MOUSEMOTION&&valuePerPixel>0){
			//drag thumb
			state|=3;
			int val = criticalValue + (int)(((float)i - startDragPos) * valuePerPixel + 0.5f);
			if(val<min) val=min;
			else if(val>max) val=max;
			if(value!=val){
				value=val;
				changed=true;
			}
			b=true;
		}else if(bInControl_0){
			if(valuePerPixel > 0){
				f2=f+16;
				f3=f1-16;
			}else{
				f2=f3=(f+f1)/2;
			}
			if(i<f){ //do nothing
			}else if(i<f2){ //-smallchange
				state=(state&~0xFF)|1;
				if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) state=(state&~0x0000FF00)|((state&0xFF)<<8);
				else if((state&0x0000FF00)&&((state&0xFF)!=((state>>8)&0xFF))) state&=~0xFF;
				if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT){
					int val=value-smallChange;
					if(val<min) val=min;
					if(value!=val){
						value=val;
						changed=true;
					}
					timer=8;
				}
				b=true;
			}else if(i>=f3 && i<f1){ //+smallchange
				state=(state&~0xFF)|5;
				if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) state=(state&~0x0000FF00)|((state&0xFF)<<8);
				else if((state&0x0000FF00)&&((state&0xFF)!=((state>>8)&0xFF))) state&=~0xFF;
				if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT){
					int val=value+smallChange;
					if(val>max) val=max;
					if(value!=val){
						value=val;
						changed=true;
					}
					timer=8;
				}
				b=true;
			}else if(valuePerPixel<=0){ //do nothing
			}else if(i<(int)thumbStart){ //-largechange
				state=(state&~0xFF)|2;
				if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) state=(state&~0x0000FF00)|((state&0xFF)<<8);
				else if((state&0x0000FF00)&&((state&0xFF)!=((state>>8)&0xFF))) state&=~0xFF;
				if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT){
					int val=value-largeChange;
					if(val<min) val=min;
					if(value!=val){
						value=val;
						changed=true;
					}
					timer=8;
				}
				if(state&0xFF) criticalValue = min + (int)(float(i - f2) * valuePerPixel + 0.5f);
				b=true;
			}else if(i<(int)thumbEnd){ //start drag
				state=(state&~0xFF)|3;
				if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) state=(state&~0x0000FF00)|((state&0xFF)<<8);
				else if((state&0x0000FF00)&&((state&0xFF)!=((state>>8)&0xFF))) state&=~0xFF;
				if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT){
					criticalValue=value;
					startDragPos = (float)i;
				}
				b=true;
			}else if(i<f3){ //+largechange
				state=(state&~0xFF)|4;
				if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) state=(state&~0x0000FF00)|((state&0xFF)<<8);
				else if((state&0x0000FF00)&&((state&0xFF)!=((state>>8)&0xFF))) state&=~0xFF;
				if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT){
					int val=value+largeChange;
					if(val>max) val=max;
					if(value!=val){
						value=val;
						changed=true;
					}
					timer=8;
				}
				if(state&0xFF) criticalValue = min - largeChange + (int)(float(i - f2) * valuePerPixel + 0.5f);
				b=true;
			}
		}
	}
	//===
	x+=left;
	y+=top;
	for(unsigned int i=0;i<childControls.size();i++){
		bool b1=childControls[i]->handleEvents(x,y,enabled,visible,b);
		b=b||b1;
	}
	return b;
}

void GUIScrollBar::renderScrollBarButton(int index,int x1,int y1,int x2,int y2,int srcleft,int srctop){
	if(x2<=x1||y2<=y1) return;
	int clr=-1;
	SDL_Rect r={x1,y1,x2-x1,y2-y1};
	if((state&0xFF)==index){
		if(((state>>8)&0xFF)==index){
			clr=SDL_MapRGB(screen->format,128,128,128);
		}else{
			clr=SDL_MapRGB(screen->format,192,192,192);
		}
	}
	SDL_FillRect(screen,&r,0);
	if(r.w>=2&&r.h>=2){
		r.x+=1;
		r.y+=1;
		r.w-=2;
		r.h-=2;
		SDL_FillRect(screen,&r,clr);
	}
	bool b;
	if(orientation) b=(y2-y1>=14);
	else b=(x2-x1>=14);
	if(b&&srcleft>=0&&srctop>=0){
		SDL_Rect r1={srcleft,srctop,16,16};
		r.x=(x1+x2)/2-8;
		r.y=(y1+y2)/2-8;
		SDL_BlitSurface(bmGUI,&r1,screen,&r);
	}
}

void GUIScrollBar::render(int x,int y){
	if(!visible) return;
	//timer event
	if(enabled){
		if((state&0xFF)==((state>>8)&0xFF)){
			switch(state&0xFF){
			case 1: //-smallchange
				if((--timer)<=0){
					int val=value-smallChange;
					if(val<min) val=min;
					if(value!=val){
						value=val;
						changed=true;
					}
					timer=2;
				}
				break;
			case 2: //-largechange
				if((--timer)<=0){
					if(value<criticalValue) state&=~0xFF;
					else{
						int val=value-largeChange;
						if(val<min) val=min;
						if(value!=val){
							value=val;
							changed=true;
						}
						timer=2;
					}
				}
				break;
			case 4: //+largechange
				if((--timer)<=0){
					if(value>criticalValue) state&=~0xFF;
					else{
						int val=value+largeChange;
						if(val>max) val=max;
						if(value!=val){
							value=val;
							changed=true;
						}
						timer=2;
					}
				}
				break;
			case 5: //+smallchange
				if((--timer)<=0){
					int val=value+smallChange;
					if(val>max) val=max;
					if(value!=val){
						value=val;
						changed=true;
					}
					timer=2;
				}
				break;
			}
		}
	}
	//changed?
	if(changed){
		if(eventCallback){
			GUIEvent e={eventCallback,name,this,GUIEventChange};
			GUIEventQueue.push_back(e);
		}
		changed=false;
	}
	//calc pos
	calcPos();
	//draw
	if(orientation){//vertical
		if(valuePerPixel>0){//5 buttons
			renderScrollBarButton(1,x+left,y+top,x+left+width,y+top+16,80,0);
			renderScrollBarButton(2,x+left,y+top+15,x+left+width,y+(int)thumbStart,-1,-1);
			renderScrollBarButton(3,x+left,y-1+(int)thumbStart,x+left+width,y+1+(int)thumbEnd,0,16);
			renderScrollBarButton(4,x+left,y+(int)thumbEnd,x+left+width,y+top+height-15,-1,-1);
			renderScrollBarButton(5,x+left,y+top+height-16,x+left+width,y+top+height,96,0);
		}else{//2 buttons
			int f=top+height/2;
			renderScrollBarButton(1,x+left,y+top,x+left+width,y+1+f,80,0);
			renderScrollBarButton(5,x+left,y+f,x+left+width,y+top+height,96,0);
		}
	}else{//horizontal
		if(valuePerPixel>0){//5 buttons
			renderScrollBarButton(1,x+left,y+top,x+left+16,y+top+height,48,0);
			renderScrollBarButton(2,x+left+15,y+top,x+(int)thumbStart,y+top+height,-1,-1);
			renderScrollBarButton(3,x-1+(int)thumbStart,y+top,x+1+(int)thumbEnd,y+top+height,16,16);
			renderScrollBarButton(4,x+(int)thumbEnd,y+top,x+left+width-15,y+top+height,-1,-1);
			renderScrollBarButton(5,x+left+width-16,y+top,x+left+width,y+top+height,64,0);
		}else{//2 buttons
			int f=left+width/2;
			renderScrollBarButton(1,x+left,y+top,x+1+f,y+top+height,48,0);
			renderScrollBarButton(5,x+f,y+top,x+left+width,y+top+height,64,0);
		}
	}
	//
	x+=left;
	y+=top;
	for(unsigned int i=0;i<childControls.size();i++){
		childControls[i]->render(x,y);
	}
}