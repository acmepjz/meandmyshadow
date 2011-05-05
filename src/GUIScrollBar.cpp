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

void GUIScrollBar::CalcPos(){
	float f,f1,f2;
	if(Value<Min) Value=Min;
	else if(Value>Max) Value=Max;
	if(Orientation){
		f=(float)(Top+16);
		f2=(float)(Height-32);
	}else{
		f=(float)(Left+16);
		f2=(float)(Width-32);
	}
	if(LargeChange<=0) f2=-1;
	if(f2>0){
		fValuePerPixel = (Max - Min + LargeChange) / f2;
		if(fValuePerPixel > 0.0001f) f1 = LargeChange / fValuePerPixel;
		if(f1 < 4 && f2 > 4){
			fValuePerPixel = (Max - Min) / (f2 - 4);
			f1 = 4;
		}
		fThumbStart = f + (Value - Min) / fValuePerPixel;
		fThumbEnd = fThumbStart + f1;
	}else{
		fValuePerPixel = -1;
		fThumbStart = f;
		fThumbEnd = f - 1;
	}
}

bool GUIScrollBar::handle_events(int x,int y,bool enabled,bool visible,bool processed){
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
	enabled=enabled && Enabled;
	visible=visible && Visible;
	//===
	if(event.type==SDL_MOUSEBUTTONUP || !(enabled&&visible)){
		State=0;
	}else if(event.type==SDL_MOUSEMOTION || event.type==SDL_MOUSEBUTTONDOWN){
		int i,j,k,f,f1,f2,f3;
		State&=~0xFF;
		k=SDL_GetMouseState(&i,&j);
		i-=x;
		j-=y;
		bool bInControl_0;
		if(Orientation){
			f=Top;
			f1=f+Height;
			bInControl_0=(i>=Left&&i<Left+Width);
			i=j;
		}else{
			f=Left;
			f1=f+Width;
			bInControl_0=(j>=Top&&j<Top+Height);
		}
		//===
		if((State&0x0000FF00)==0x300&&(k&SDL_BUTTON(1))&&event.type==SDL_MOUSEMOTION&&fValuePerPixel>0){
			//drag thumb
			State|=3;
			int val = nCriticalValue + (int)(((float)i - fStartDragPos) * fValuePerPixel + 0.5f);
			if(val<Min) val=Min;
			else if(val>Max) val=Max;
			if(Value!=val){
				Value=val;
				bChanged=true;
			}
			b=true;
		}else if(bInControl_0){
			if(fValuePerPixel > 0){
				f2=f+16;
				f3=f1-16;
			}else{
				f2=f3=(f+f1)/2;
			}
			if(i<f){ //do nothing
			}else if(i<f2){ //-smallchange
				State=(State&~0xFF)|1;
				if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) State=(State&~0x0000FF00)|((State&0xFF)<<8);
				else if((State&0x0000FF00)&&((State&0xFF)!=((State>>8)&0xFF))) State&=~0xFF;
				if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT){
					int val=Value-SmallChange;
					if(val<Min) val=Min;
					if(Value!=val){
						Value=val;
						bChanged=true;
					}
					nTimer=8;
				}
				b=true;
			}else if(i>=f3 && i<f1){ //+smallchange
				State=(State&~0xFF)|5;
				if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) State=(State&~0x0000FF00)|((State&0xFF)<<8);
				else if((State&0x0000FF00)&&((State&0xFF)!=((State>>8)&0xFF))) State&=~0xFF;
				if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT){
					int val=Value+SmallChange;
					if(val>Max) val=Max;
					if(Value!=val){
						Value=val;
						bChanged=true;
					}
					nTimer=8;
				}
				b=true;
			}else if(fValuePerPixel<=0){ //do nothing
			}else if(i<(int)fThumbStart){ //-largechange
				State=(State&~0xFF)|2;
				if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) State=(State&~0x0000FF00)|((State&0xFF)<<8);
				else if((State&0x0000FF00)&&((State&0xFF)!=((State>>8)&0xFF))) State&=~0xFF;
				if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT){
					int val=Value-LargeChange;
					if(val<Min) val=Min;
					if(Value!=val){
						Value=val;
						bChanged=true;
					}
					nTimer=8;
				}
				if(State&0xFF) nCriticalValue = Min + (int)(float(i - f2) * fValuePerPixel + 0.5f);
				b=true;
			}else if(i<(int)fThumbEnd){ //start drag
				State=(State&~0xFF)|3;
				if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) State=(State&~0x0000FF00)|((State&0xFF)<<8);
				else if((State&0x0000FF00)&&((State&0xFF)!=((State>>8)&0xFF))) State&=~0xFF;
				if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT){
					nCriticalValue=Value;
					fStartDragPos = (float)i;
				}
				b=true;
			}else if(i<f3){ //+largechange
				State=(State&~0xFF)|4;
				if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) State=(State&~0x0000FF00)|((State&0xFF)<<8);
				else if((State&0x0000FF00)&&((State&0xFF)!=((State>>8)&0xFF))) State&=~0xFF;
				if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT){
					int val=Value+LargeChange;
					if(val>Max) val=Max;
					if(Value!=val){
						Value=val;
						bChanged=true;
					}
					nTimer=8;
				}
				if(State&0xFF) nCriticalValue = Min - LargeChange + (int)(float(i - f2) * fValuePerPixel + 0.5f);
				b=true;
			}
		}
	}
	//===
	x+=Left;
	y+=Top;
	for(unsigned int i=0;i<ChildControls.size();i++){
		bool b1=ChildControls[i]->handle_events(x,y,enabled,visible,b);
		b=b||b1;
	}
	return b;
}

void GUIScrollBar::pRenderScrollBarButton(int nIndex,int x1,int y1,int x2,int y2,int SrcLeft,int SrcTop){
	if(x2<=x1||y2<=y1) return;
	int clr=-1;
	SDL_Rect r={x1,y1,x2-x1,y2-y1};
	if((State&0xFF)==nIndex){
		if(((State>>8)&0xFF)==nIndex){
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
	if(Orientation) b=(y2-y1>=14);
	else b=(x2-x1>=14);
	if(b&&SrcLeft>=0&&SrcTop>=0){
		SDL_Rect r1={SrcLeft,SrcTop,16,16};
		r.x=(x1+x2)/2-8;
		r.y=(y1+y2)/2-8;
		SDL_BlitSurface(bmGUI,&r1,screen,&r);
	}
}

void GUIScrollBar::render(int x,int y){
	if(!Visible) return;
	//timer event
	if(Enabled){
		if((State&0xFF)==((State>>8)&0xFF)){
			switch(State&0xFF){
			case 1: //-smallchange
				if((--nTimer)<=0){
					int val=Value-SmallChange;
					if(val<Min) val=Min;
					if(Value!=val){
						Value=val;
						bChanged=true;
					}
					nTimer=2;
				}
				break;
			case 2: //-largechange
				if((--nTimer)<=0){
					if(Value<nCriticalValue) State&=~0xFF;
					else{
						int val=Value-LargeChange;
						if(val<Min) val=Min;
						if(Value!=val){
							Value=val;
							bChanged=true;
						}
						nTimer=2;
					}
				}
				break;
			case 4: //+largechange
				if((--nTimer)<=0){
					if(Value>nCriticalValue) State&=~0xFF;
					else{
						int val=Value+LargeChange;
						if(val>Max) val=Max;
						if(Value!=val){
							Value=val;
							bChanged=true;
						}
						nTimer=2;
					}
				}
				break;
			case 5: //+smallchange
				if((--nTimer)<=0){
					int val=Value+SmallChange;
					if(val>Max) val=Max;
					if(Value!=val){
						Value=val;
						bChanged=true;
					}
					nTimer=2;
				}
				break;
			}
		}
	}
	//changed?
	if(bChanged){
		if(EventCallback){
			GUIEvent e={EventCallback,Name,this,GUIEventChange};
			GUIEventQueue.push_back(e);
		}
		bChanged=false;
	}
	//calc pos
	CalcPos();
	//draw
	if(Orientation){//vertical
		if(fValuePerPixel>0){//5 buttons
			pRenderScrollBarButton(1,x+Left,y+Top,x+Left+Width,y+Top+16,80,0);
			pRenderScrollBarButton(2,x+Left,y+Top+15,x+Left+Width,y+(int)fThumbStart,-1,-1);
			pRenderScrollBarButton(3,x+Left,y-1+(int)fThumbStart,x+Left+Width,y+1+(int)fThumbEnd,0,16);
			pRenderScrollBarButton(4,x+Left,y+(int)fThumbEnd,x+Left+Width,y+Top+Height-15,-1,-1);
			pRenderScrollBarButton(5,x+Left,y+Top+Height-16,x+Left+Width,y+Top+Height,96,0);
		}else{//2 buttons
			int f=Top+Height/2;
			pRenderScrollBarButton(1,x+Left,y+Top,x+Left+Width,y+1+f,80,0);
			pRenderScrollBarButton(5,x+Left,y+f,x+Left+Width,y+Top+Height,96,0);
		}
	}else{//horizontal
		if(fValuePerPixel>0){//5 buttons
			pRenderScrollBarButton(1,x+Left,y+Top,x+Left+16,y+Top+Height,48,0);
			pRenderScrollBarButton(2,x+Left+15,y+Top,x+(int)fThumbStart,y+Top+Height,-1,-1);
			pRenderScrollBarButton(3,x-1+(int)fThumbStart,y+Top,x+1+(int)fThumbEnd,y+Top+Height,16,16);
			pRenderScrollBarButton(4,x+(int)fThumbEnd,y+Top,x+Left+Width-15,y+Top+Height,-1,-1);
			pRenderScrollBarButton(5,x+Left+Width-16,y+Top,x+Left+Width,y+Top+Height,64,0);
		}else{//2 buttons
			int f=Left+Width/2;
			pRenderScrollBarButton(1,x+Left,y+Top,x+1+f,y+Top+Height,48,0);
			pRenderScrollBarButton(5,x+f,y+Top,x+Left+Width,y+Top+Height,64,0);
		}
	}
	//
	x+=Left;
	y+=Top;
	for(unsigned int i=0;i<ChildControls.size();i++){
		ChildControls[i]->render(x,y);
	}
}