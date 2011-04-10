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
#include "GUIObject.h"

GUIObject *GUIObjectRoot=NULL;

struct GUIEvent{
	GUIEventCallback *EventCallback;
	std::string Name;
	GUIObject* obj;
	int nEventType;
};

std::vector<GUIEvent> GUIEventQueue;

void GUIObjectHandleEvents(){
	if(GUIObjectRoot) GUIObjectRoot->handle_events();
	for(unsigned int i=0;i<GUIEventQueue.size();i++){
		if(GUIEventQueue[i].EventCallback){
			GUIEventQueue[i].EventCallback->GUIEventCallback_OnEvent(GUIEventQueue[i].Name,GUIEventQueue[i].obj,GUIEventQueue[i].nEventType);
		}
	}
	GUIEventQueue.clear();
}

GUIObject::~GUIObject(){
	for(unsigned int i=0;i<ChildControls.size();i++){
		delete ChildControls[i];
	}
	ChildControls.clear();
}

bool GUIObject::handle_events(int x,int y,bool enabled,bool visible,bool processed){
	bool b=processed;
	enabled=enabled && Enabled;
	visible=visible && Visible;
	x+=Left;
	y+=Top;
	//
	switch(Type){
	case GUIObjectButton:
		State=0;
		if(enabled&&visible){
			int i,j,k;
			k=SDL_GetMouseState(&i,&j);
			if(i>=x&&i<x+Width&&j>=y&&j<y+Height){
				State=1;
				if(k&SDL_BUTTON(1)) State=2;
				if(event.type==SDL_MOUSEBUTTONUP && event.button.button==SDL_BUTTON_LEFT && !b){
					if(EventCallback){
						GUIEvent e={EventCallback,Name,this,GUIEventClick};
						GUIEventQueue.push_back(e);
					}
					b=true;
				}
			}
		}
		break;
	case GUIObjectTextBox:
		State=0;
		if(enabled&&visible){
			int i,j;
			SDL_GetMouseState(&i,&j);
			if(i>=x&&i<x+Width&&j>=y&&j<y+Height){
				State=1;
				if(event.type==SDL_KEYDOWN && !b){
					int key=(int)event.key.keysym.unicode;
					if(key>=32&&key<=126){
						Caption+=char(key);
						if(EventCallback){
							GUIEvent e={EventCallback,Name,this,GUIEventClick};
							GUIEventQueue.push_back(e);
						}
					}else if(event.key.keysym.sym==SDLK_BACKSPACE||event.key.keysym.sym==SDLK_DELETE){
						if(Caption.length()>0){
							Caption=Caption.substr(0,Caption.length()-1);
							if(EventCallback){
								GUIEvent e={EventCallback,Name,this,GUIEventClick};
								GUIEventQueue.push_back(e);
							}
						}
					}
					b=true;
				}
			}
		}
		break;
	}
	//
	for(unsigned int i=0;i<ChildControls.size();i++){
		bool b1=ChildControls[i]->handle_events(x,y,enabled,visible,b);
		b=b||b1;
	}
	return b;
}

void GUIObject::render(int x,int y){
	SDL_Rect r;
	if(!Visible) return;
	x+=Left;
	y+=Top;
	//
	switch(Type){
	case GUIObjectLabel:
		{
			r.x=x;
			r.y=y;
			r.w=Width;
			r.h=Height;
			//SDL_FillRect(screen,&r,-1); //label is transparent
			const char* lp=Caption.c_str();
			if(lp!=NULL && lp[0]){
				SDL_Color black={0,0,0,0};
				SDL_Surface *bm=TTF_RenderText_Blended(font_small,lp,black);
				r.x=x;
				r.y=y+(Height - bm->h)/2;
				SDL_BlitSurface(bm,NULL,screen,&r);
				SDL_FreeSurface(bm);
			}
		}
		break;
	case GUIObjectButton:
		{
			int clr=-1;
			if(State==1) clr=SDL_MapRGB(screen->format,192,192,192);
			else if(State==2) clr=SDL_MapRGB(screen->format,128,128,128);
			r.x=x;
			r.y=y;
			r.w=Width;
			r.h=Height;
			SDL_FillRect(screen,&r,0);
			r.x=x+1;
			r.y=y+1;
			r.w=Width-2;
			r.h=Height-2;
			SDL_FillRect(screen,&r,clr);
			const char* lp=Caption.c_str();
			if(lp!=NULL && lp[0]){
				SDL_Color black={0,0,0,0};
				SDL_Surface *bm=TTF_RenderText_Blended(font_small,lp,black);
				r.x=x+(Width - bm->w)/2;
				r.y=y+(Height - bm->h)/2;
				SDL_BlitSurface(bm,NULL,screen,&r);
				SDL_FreeSurface(bm);
			}
		}
		break;
	case GUIObjectTextBox:
		{
			int clr=-1;
			if(State==1) clr=SDL_MapRGB(screen->format,192,192,192);
			r.x=x;
			r.y=y;
			r.w=Width;
			r.h=Height;
			SDL_FillRect(screen,&r,0);
			r.x=x+1;
			r.y=y+1;
			r.w=Width-2;
			r.h=Height-2;
			SDL_FillRect(screen,&r,clr);
			const char* lp=Caption.c_str();
			if(lp!=NULL){
				if(lp[0]){
					SDL_Color black={0,0,0,0};
					SDL_Surface *bm=TTF_RenderText_Blended(font_small,lp,black);
					r.x=x+2;
					r.y=y+(Height - bm->h)/2;
					SDL_BlitSurface(bm,NULL,screen,&r);
					if(State==1){
						r.x=x+4+bm->w;
						r.y=y+4;
						r.w=2;
						r.h=Height-8;
						SDL_FillRect(screen,&r,0);
					}
					SDL_FreeSurface(bm);
				}else{
					if(State==1){
						r.x=x+4;
						r.y=y+4;
						r.w=2;
						r.h=Height-8;
						SDL_FillRect(screen,&r,0);
					}
				}
			}
		}
		break;
	case GUIObjectFrame:
		{
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
			const char* lp=Caption.c_str();
			if(lp!=NULL && lp[0]){
				SDL_Color black={0,0,0,0};
				SDL_Color white={255,255,255,255};
				SDL_Surface *bm=TTF_RenderText_Shaded(font,lp,black,white);
				r.x=x+(Width - bm->w)/2;
				r.y=y - (int(bm->h))/2;
				SDL_BlitSurface(bm,NULL,screen,&r);
				SDL_FreeSurface(bm);
			}
		}
		break;
	}
	//
	for(unsigned int i=0;i<ChildControls.size();i++){
		ChildControls[i]->render(x,y);
	}
}
