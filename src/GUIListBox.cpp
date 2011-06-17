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

#include "GUIListBox.h"
using namespace std;

GUIListBox::GUIListBox(int Left,int Top,int Width,int Height,bool Enabled,bool Visible):
GUIObject(Left,Top,Width,Height,0,NULL,-1,Enabled,Visible){
	State=-1;
	m_oScrollBar=new GUIScrollBar(0,0,16,0,1,0,0,0,0,0,true,false);
	ChildControls.push_back(m_oScrollBar);
}

bool GUIListBox::handle_events(int x,int y,bool enabled,bool visible,bool processed){
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
	x+=Left;
	y+=Top;
	//calc scroll bar pos
	m_oScrollBar->Left=Width-16;
	m_oScrollBar->Height=Height;
	int m=Item.size(),n=(Height-4)/24;
	if(m>n){
		m_oScrollBar->Max=m-n;
		m_oScrollBar->SmallChange=1;
		m_oScrollBar->LargeChange=n;
		m_oScrollBar->Visible=true;
		b=b||m_oScrollBar->handle_events(x,y,enabled,visible,b);
	}else{
		m_oScrollBar->Value=0;
		m_oScrollBar->Max=0;
		m_oScrollBar->Visible=false;
	}
	//
	State=-1;
	if(enabled&&visible&&!b){
		int i,j,k;
		k=SDL_GetMouseState(&i,&j);
		i-=x+2;
		j-=y+2;
		if(i>=0&&i<Width-4&&j>=0&&j<Height-4){
			int idx=j/24+m_oScrollBar->Value;
			if(idx>=0&&idx<(int)Item.size()){
				State=idx;
				if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT && Value!=idx){
					Value=idx;
					if(EventCallback){
						GUIEvent e={EventCallback,Name,this,GUIEventClick};
						GUIEventQueue.push_back(e);
					}
				}
			}
			if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_WHEELDOWN && m_oScrollBar->Enabled){
				m_oScrollBar->Value+=4;
				if(m_oScrollBar->Value > m_oScrollBar->Max) m_oScrollBar->Value = m_oScrollBar->Max;
			}else if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_WHEELUP && m_oScrollBar->Enabled){
				m_oScrollBar->Value-=4;
				if(m_oScrollBar->Value < 0) m_oScrollBar->Value = 0;
			}
		}
	}
	//process child controls event except for the scroll bar
	for(unsigned int i=1;i<ChildControls.size();i++){
		bool b1=ChildControls[i]->handle_events(x,y,enabled,visible,b);
		b=b||b1;
	}
	return b;
}

void GUIListBox::render(int x,int y){
	SDL_Rect r;
	if(!Visible) return;
	x+=Left;
	y+=Top;
	//border
	r.x=x;
	r.y=y;
	r.w=Width;
	r.h=Height;
	SDL_FillRect(screen,&r,0);
	r.x=x+1;
	r.y=y+1;
	r.w=Width-2;
	r.h=Height-2;
	SDL_FillRect(screen,&r,-1);
	//item
	int m=Item.size(),n=(Height-4)/24,i,j;
	if(m>m_oScrollBar->Value+n) m=m_oScrollBar->Value+n;
	for(i=m_oScrollBar->Value,j=y+2;i<m;i++,j+=24){
		int clr=-1;
		if(Value==i) clr=SDL_MapRGB(screen->format,192,192,192);
		r.x=x+2;
		r.y=j;
		r.w=Width-4;
		r.h=24;
		if(State==i){
			SDL_FillRect(screen,&r,0);
			r.x+=1;
			r.y+=1;
			r.w-=2;
			r.h-=2;
		}
		SDL_FillRect(screen,&r,clr);
		const char* s=Item[i].c_str();
		if(s && s[0]){
			SDL_Color black={0,0,0,0};
			SDL_Surface *bm=TTF_RenderText_Blended(font_small,s,black);
			r.x=x+4;
			r.y=j+12-bm->h/2;
			SDL_BlitSurface(bm,NULL,screen,&r);
			SDL_FreeSurface(bm);
		}
	}
	//
	for(unsigned int i=0;i<ChildControls.size();i++){
		ChildControls[i]->render(x,y);
	}
}

GUISingleLineListBox::GUISingleLineListBox(int Left,int Top,int Width,int Height,bool Enabled,bool Visible):
GUIObject(Left,Top,Width,Height,0,NULL,-1,Enabled,Visible){
}

bool GUISingleLineListBox::handle_events(int x,int y,bool enabled,bool visible,bool processed){
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
	x+=Left;
	y+=Top;
	State&=~0xF;
	if(enabled&&visible){
		int i,j,k;
		int idx=0;
		k=SDL_GetMouseState(&i,&j);
		i-=x;
		j-=y;
		if(i>=0&&i<Width&&j>=0&&j<Height){
			if(i<16 && i<Width/2){//left
				idx=1;
			}else if(i>=Width-16){//right
				idx=2;
			}
		}
		if(k&SDL_BUTTON(1)){
			if(((State>>4)&0xF)==idx) State|=idx;
		}else{
			State|=idx;
		}
		if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT && idx){
			State=idx|(idx<<4);
		}else if(event.type==SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT && idx && ((State>>4)&0xF)==idx){
			int m=(int)Item.size();
			if(m>0){
				if(idx==2){
					idx=Value+1;
					if(idx<0||idx>=m) idx=0;
					if(idx!=Value){
						Value=idx;
						if(EventCallback){
							GUIEvent e={EventCallback,Name,this,GUIEventClick};
							GUIEventQueue.push_back(e);
						}
					}
				}else if(idx==1){
					idx=Value-1;
					if(idx<0||idx>=m) idx=m-1;
					if(idx!=Value){
						Value=idx;
						if(EventCallback){
							GUIEvent e={EventCallback,Name,this,GUIEventClick};
							GUIEventQueue.push_back(e);
						}
					}
				}
			}
		}
		if(event.type==SDL_MOUSEBUTTONUP) State&=0xF;
	}else{
		State=0;
	}
	//process child controls event
	for(unsigned int i=0;i<ChildControls.size();i++){
		bool b1=ChildControls[i]->handle_events(x,y,enabled,visible,b);
		b=b||b1;
	}
	return b;
}

void GUISingleLineListBox::render(int x,int y){
	SDL_Rect r;
	if(!Visible) return;
	x+=Left;
	y+=Top;
	//border
	int clr_lightgray=SDL_MapRGB(screen->format,192,192,192);
	int clr_gray=SDL_MapRGB(screen->format,128,128,128);
	r.x=x;
	r.y=y;
	r.w=Width;
	r.h=Height;
	SDL_FillRect(screen,&r,0);
	r.x=x+1;
	r.y=y+1;
	r.w=Width-2;
	r.h=Height-2;
	SDL_FillRect(screen,&r,-1);
	//draw highlight
	if((State&0xF)==0x1){
		r.w=15;
		SDL_FillRect(screen,&r,(State&0xF0)?clr_gray:clr_lightgray);
	}
	if((State&0xF)==0x2){
		r.x=x+Width-16;
		r.w=15;
		SDL_FillRect(screen,&r,(State&0xF0)?clr_gray:clr_lightgray);
	}
	//draw text
	if(Value>=0 && Value<(int)Item.size()){
		const char* lp=Item[Value].c_str();
		if(lp!=NULL && lp[0]){
			SDL_Color black={0,0,0,0};
			SDL_Surface *bm=TTF_RenderText_Blended(font_small,lp,black);
			r.x=x+(Width - bm->w)/2;
			r.y=y+(Height - bm->h)/2;
			SDL_BlitSurface(bm,NULL,screen,&r);
			SDL_FreeSurface(bm);
		}
	}
	//draw arrow
	SDL_Rect r2={48,0,16,16};
	r.x=x;
	r.y=y+(Height-16)/2;
	SDL_BlitSurface(bmGUI,&r2,screen,&r);
	r2.x=64;
	r.x=x+Width-16;
	SDL_BlitSurface(bmGUI,&r2,screen,&r);
	//
	for(unsigned int i=0;i<ChildControls.size();i++){
		ChildControls[i]->render(x,y);
	}
}
