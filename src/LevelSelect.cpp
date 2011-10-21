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
#include "GameState.h"
#include "Functions.h"
#include "FileManager.h"
#include "Globals.h"
#include "Objects.h"
#include "LevelSelect.h"
#include "GUIObject.h"
#include "GUIListBox.h"
#include "GUIScrollBar.h"
#include <SDL/SDL_ttf.h>
#include <SDL/SDL.h>
#include <string>
#include <sstream>
#include <iostream>
using namespace std;

////////////////////NUMBER////////////////////////
Number::Number(){
	image=NULL;
	background=NULL;
	number=0;

	//Set the default dimensions.
	box.x=0;
	box.y=0;
	box.h=50;
	box.w=50;
}

Number::~Number(){
	//We only need to free the SDLSurface.
	if(image) SDL_FreeSurface(image);
}

void Number::init(int number, SDL_Rect box){
	//First set the number and update our status.
	this->number=number;
	updateLock();

	//Write our text, number+1 since the counting doens't start with 0, but with 1.
	std::stringstream text;
	number++;
	text<<number;

	//Create the text image.
	SDL_Color black={0,0,0};
	if(image) SDL_FreeSurface(image);
	//Create the text image.
	//Also check which font to use, if the number is higher than 100 use the small font.
	image=TTF_RenderText_Blended(number>=100?fontSmall:font,text.str().c_str(),black);

	//Set the new location of the number.
	this->box.x=box.x;
	this->box.y=box.y;
}

void Number::show(int dy){
	//First draw the background, also apply the yOffset(dy).
	applySurface(box.x,box.y-dy,background,screen,NULL);
	//Now draw the text image over the background.
	//We draw it centered inside the box.
	applySurface((box.x+25-(image->w / 2)),(box.y+25-(image->h/2))-dy,image,screen,NULL);
}

void Number::updateLock(){
	//Check if the level is locked, if so change the background to the locked image.
	if(levels.getLocked(number)==false){
		background=loadImage(getDataPath()+"gfx/level.png");
	}else{
		background=loadImage(getDataPath()+"gfx/levellocked.png"); 
	}
}


/////////////////////LEVEL SELECT/////////////////////
static GUIScrollBar* levelScrollBar=NULL;
static GUIObject* levelpackDescription=NULL;

LevelSelect::LevelSelect(){
	background=loadImage(getDataPath()+"gfx/menu/levelselect.png");

	//create GUI (test only)
	GUIObject* obj;
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}

	GUIObjectRoot=new GUIObject(0,0,800,600);
	levelScrollBar=new GUIScrollBar(768,140,16,370,ScrollBarVertical,0,0,0,1,5,true,false);
	GUIObjectRoot->ChildControls.push_back(levelScrollBar);
	levelpackDescription=new GUIObject(60,96,800,32,GUIObjectLabel);
	GUIObjectRoot->ChildControls.push_back(levelpackDescription);

	GUISingleLineListBox* levelpacks=new GUISingleLineListBox(150,64,500,32);
	levelpacks->Name="cmdLvlPack";
	levelpacks->EventCallback=this;
	vector<string> v=enumAllDirs(getDataPath()+"levelpacks/");
	for(vector<string>::iterator i=v.begin(); i!=v.end(); ++i){
		levelpackLocations[*i]=getDataPath()+"levelpacks/"+*i;
	}
	vector<string> v2=enumAllDirs(getUserPath()+"levelpacks/");
	for(vector<string>::iterator i=v2.begin(); i!=v2.end(); ++i){
		levelpackLocations[*i]=getUserPath()+"levelpacks/"+*i;
	}
	v.insert(v.end(), v2.begin(), v2.end());
	levelpacks->Item=v;
	levelpacks->Value=0;
	GUIObjectRoot->ChildControls.push_back(levelpacks);
	
	obj=new GUIObject(20,540,175,32,GUIObjectButton,"Back");
	obj->Name="cmdBack";
	obj->EventCallback=this;
	GUIObjectRoot->ChildControls.push_back(obj);
	obj=new GUIObject(215,540,175,32,GUIObjectButton,"Clear progress");
	obj->Name="cmdReset";
	obj->EventCallback=this;
	GUIObjectRoot->ChildControls.push_back(obj);
	
	if(getSettings()->getBoolValue("internet")) {
		obj=new GUIObject(410,540,175,32,GUIObjectButton,"Addons");
		obj->Name="cmdAddon";
		obj->EventCallback=this;
		GUIObjectRoot->ChildControls.push_back(obj);
	}
	obj=new GUIObject(605,540,175,32,GUIObjectButton,"Levels");
	obj->Name="cmdLoadLv";
	obj->EventCallback=this;
	GUIObjectRoot->ChildControls.push_back(obj);

	//show level list
	refresh();
}

void LevelSelect::refresh(){
	int m=levels.getLevelCount();
	numbers.clear();

	for(int n=0; n<m; n++ ){
		numbers.push_back(Number());
	}

	for(int n=0; n<m; n++){
		SDL_Rect box={(n%10)*64+80,(n/10)*80+140,0,0};
		numbers[n].init( n, box );
	}

	if(m>50){
		levelScrollBar->Max=(m-41)/10;
		levelScrollBar->Visible=true;
	}else{
		levelScrollBar->Max=0;
		levelScrollBar->Visible=false;
	}
	levelpackDescription->Caption=levels.levelpackDescription;
	int width,height;
	TTF_SizeText(fontSmall,levels.levelpackDescription.c_str(),&width,&height);
	levelpackDescription->Left=(800-width)/2;
}

LevelSelect::~LevelSelect(){
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}
	levelScrollBar=NULL;
	levelpackDescription=NULL;
}

void LevelSelect::handleEvents(){
	if(event.type==SDL_QUIT){
		setNextState(STATE_EXIT);
	}

	if(event.type==SDL_MOUSEBUTTONUP && event.button.button==SDL_BUTTON_LEFT){
		checkMouse();
	}

	if(event.type==SDL_KEYUP && event.key.keysym.sym==SDLK_ESCAPE){
		setNextState(STATE_MENU);
	}

	if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button==SDL_BUTTON_WHEELDOWN && levelScrollBar){
		if(levelScrollBar->Value<levelScrollBar->Max) levelScrollBar->Value++;
		return;
	}else if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button==SDL_BUTTON_WHEELUP && levelScrollBar){
		if(levelScrollBar->Value>0) levelScrollBar->Value--;
		return;
	}
}

void LevelSelect::checkMouse(){
	int x,y,dy=0,m=levels.getLevelCount();

	SDL_GetMouseState(&x,&y);

	if(levelScrollBar) dy=levelScrollBar->Value;
	if(m>dy*10+50) m=dy*10+50;
	y+=dy*80;

	SDL_Rect mouse = { x,y,0,0};

	for(int n=dy*10; n<m; n++){
		if(levels.getLocked(n)==false){
			if(checkCollision(mouse,numbers[n].box)==true){
				levels.setLevel(n);
				setNextState(STATE_GAME);
			}
		}
	}
}

void LevelSelect::logic(){}

void LevelSelect::render(){
	int x,y,dy=0,m=levels.getLevelCount();
	int idx=-1;

	SDL_GetMouseState(&x,&y);

	if(levelScrollBar) dy=levelScrollBar->Value;
	if(m>dy*10+50) m=dy*10+50;
	y+=dy*80;

	SDL_Rect mouse = { x,y,0,0};

	applySurface(0,0,background,screen,NULL);

	for(int n = dy*10; n < m; n++ ){
		numbers[n].show(dy*80);
		if(levels.getLocked(n)==false && checkCollision(mouse,numbers[n].box)==true) idx=n;
	}
	//show tool tip text
	if(idx>=0){
		SDL_Color bg={255,255,255},fg={0,0,0};
		SDL_Surface *s=TTF_RenderText_Shaded(fontSmall, levels.getLevelName(idx).c_str(), fg, bg);
		if(s!=NULL){
			SDL_Rect r=numbers[idx].box;
			r.y-=dy*80;
			if(r.y>SCREEN_HEIGHT-200){
				r.y-=s->h+4;
			}else{
				r.y+=r.h+4;
			}
			if(r.x+s->w>SCREEN_WIDTH-50) r.x=SCREEN_WIDTH-50-s->w;
			SDL_BlitSurface(s,NULL,screen,&r);
			r.x--;
			r.y--;
			r.w=s->w+1;
			r.h=1;
			SDL_FillRect(screen,&r,0);
			SDL_Rect r1={r.x,r.y,1,s->h+1};
			SDL_FillRect(screen,&r1,0);
			r1.x+=r.w;
			SDL_FillRect(screen,&r1,0);
			r.y+=r1.h;
			SDL_FillRect(screen,&r,0);
			SDL_FreeSurface(s);
		}
	}
}

void LevelSelect::GUIEventCallback_OnEvent(std::string Name,GUIObject* obj,int nEventType){
	string s;
	if(Name=="cmdLvlPack"){
		s=levelpackLocations[((GUISingleLineListBox*)obj)->Item[obj->Value]];
	}else if(Name=="cmdLoadLv"){
		if(fileDialog(s,"Load Level","map","%DATA%/levels/\nMain levels\n%USER%/levels/\nAddon levels",false,true)){
			levels.clear();
			levels.addLevel(fileNameFromPath(s),"");
			levels.levelpackPath=pathFromFileName(processFileName(s));
			levels.setLevel(0);
			setNextState(STATE_GAME);
		}
		return;
	}else if(Name=="cmdBack"){
		setNextState(STATE_MENU);
		return;
	}else if(Name=="cmdReset"){
		if(msgBox("Do you really want to reset level progress?",MsgBoxYesNo,"Warning")==MsgBoxYes){
			for(int i=0;i<levels.getLevelCount();i++){
				levels.setLocked(i,i>0?true:false);
				numbers[i].updateLock();
			}
			levels.saveLevelProgress();
		}
		return;
	}else if(Name=="cmdAddon"){
		setNextState(STATE_ADDONS);
		return;
	}else{
		return;
	}
	string s1;
	if(s.compare(0,6,"%DATA%")==0){
		int i=s.find_last_of("/\\");
		if(i!=string::npos) s1=s.substr(i+1);
		else s1=s.substr(6);
		s1="%USER%/progress/"+s1+".progress";
	}else{
 		int i=s.find_last_of("/\\");
		if(i!=string::npos) s1=s.substr(i+1);
		else s1=s.substr(6);
		s1="%USER%/progress/"+s1+".progress";
	}
	//load file
	if(!levels.loadLevels(s+"/levels.lst",s1)){
		msgBox("Can't load level pack:\n"+s,MsgBoxOKOnly,"Error");
	}
	refresh();
}