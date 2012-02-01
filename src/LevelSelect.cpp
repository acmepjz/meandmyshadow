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
#include "InputManager.h"
#include "Game.h"
#include <SDL/SDL_ttf.h>
#include <SDL/SDL.h>
#include <stdio.h>
#include <string>
#include <sstream>
#include <iostream>
using namespace std;

static SDL_Surface* playButtonImage=NULL;

////////////////////NUMBER////////////////////////
Number::Number(){
	image=NULL;
	background=NULL;
	number=0;
	medal=0;
	selected=false;
	
	//Set the default dimensions.
	box.x=0;
	box.y=0;
	box.h=50;
	box.w=50;
	
	//Load the medals image.
	medals=loadImage(getDataPath()+"gfx/medals.png");
}

Number::~Number(){
	//We only need to free the SDLSurface.
	if(image) SDL_FreeSurface(image);
}

void Number::init(int number,SDL_Rect box){
	//First set the number and update our status.
	this->number=number;
	update();

	//Write our text, number+1 since the counting doens't start with 0, but with 1.
	std::stringstream text;
	number++;
	text<<number;

	//Create the text image.
	SDL_Color black={0,0,0};
	if(image) SDL_FreeSurface(image);
	//Create the text image.
	//Also check which font to use, if the number is higher than 100 use the small font.
	image=TTF_RenderText_Blended(fontGUI,text.str().c_str(),black);

	//Set the new location of the number.
	this->box.x=box.x;
	this->box.y=box.y;
}

void Number::show(int dy){
	//First draw the background, also apply the yOffset(dy).
	applySurface(box.x,box.y-dy,background,screen,NULL);
	//Now draw the text image over the background.
	//We draw it centered inside the box.
	applySurface((box.x+25-(image->w/2)),box.y+((TTF_FontAscent(fontGUI)+TTF_FontDescent(fontGUI))/2)-dy,image,screen,NULL);

	//Draw the selection mark.
	if(selected){
		drawGUIBox(box.x,box.y-dy,50,50,screen,0x00000000);
	}
	
	//Draw the medal.
	if(medal>0){
		SDL_Rect r={(medal-1)*30,0,30,30};
		applySurface(box.x+30,(box.y+30)-dy,medals,screen,&r);
	}
}

void Number::update(){
	//Check if the level is locked, if so change the background to the locked image.
	if(levels.getLocked(number)==false){
		background=loadImage(getDataPath()+"gfx/level.png");
	}else{
		background=loadImage(getDataPath()+"gfx/levellocked.png"); 
	}
	
	//Set the medal.
	medal=levels.getLevel(number)->won;
	if(medal){
		if(levels.getLevel(number)->targetTime<0 || levels.getLevel(number)->time<=levels.getLevel(number)->targetTime)
			medal++;
		if(levels.getLevel(number)->targetRecordings<0 || levels.getLevel(number)->recordings<=levels.getLevel(number)->targetRecordings)
			medal++;
	}
}


/////////////////////LEVEL SELECT/////////////////////
static GUIScrollBar* levelScrollBar=NULL;
static GUIObject* levelpackDescription=NULL;
static string levelDescription,levelMedal,levelMedal2,levelMedal3;
static string bestTimeFilePath,bestRecordingFilePath;

LevelSelect::LevelSelect(){
	//clear the selected level
	selectedNumber=NULL;

	//Load the background image.
	background=loadImage(getDataPath()+"gfx/menu/background.png");

	if(playButtonImage==NULL){
		playButtonImage=loadImage(getDataPath()+"gfx/playbutton.png");
	}
	
	//Render the title.
	SDL_Color black={0,0,0};
	title=TTF_RenderText_Blended(fontTitle,"Select Level",black);
	
	//create GUI (test only)
	GUIObject* obj;
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}

	GUIObjectRoot=new GUIObject(0,0,800,600);

	//the level select scroll bar
	levelScrollBar=new GUIScrollBar(768,184,16,242,ScrollBarVertical,0,0,0,1,4,true,false);
	GUIObjectRoot->childControls.push_back(levelScrollBar);

	//level pack description
	levelpackDescription=new GUIObject(60,140,800,32,GUIObjectLabel);
	GUIObjectRoot->childControls.push_back(levelpackDescription);

	GUISingleLineListBox* levelpacks=new GUISingleLineListBox(150,104,500,32);
	levelpacks->name="cmdLvlPack";
	levelpacks->eventCallback=this;
	vector<string> v=enumAllDirs(getDataPath()+"levelpacks/");
	for(vector<string>::iterator i=v.begin(); i!=v.end(); ++i){
		levelpackLocations[*i]=getDataPath()+"levelpacks/"+*i;
	}
	vector<string> v2=enumAllDirs(getUserPath()+"levelpacks/");
	for(vector<string>::iterator i=v2.begin(); i!=v2.end(); ++i){
		levelpackLocations[*i]=getUserPath()+"levelpacks/"+*i;
	}
	vector<string> v3=enumAllDirs(getUserPath()+"custom/levelpacks/");
	for(vector<string>::iterator i=v3.begin(); i!=v3.end(); ++i){
		levelpackLocations[*i]=getUserPath()+"custom/levelpacks/"+*i;
	}
	v.insert(v.end(),v2.begin(),v2.end());
	v.insert(v.end(),v3.begin(),v3.end());
	
	//Now we add a special levelpack that will contain the levels not in a levelpack.
	v.push_back("Levels");
	
	levelpacks->item=v;
	levelpacks->value=0;

	//Check if we can find the lastlevelpack.
	for(vector<string>::iterator i=v.begin(); i!=v.end(); ++i){
		if(*i==getSettings()->getValue("lastlevelpack")){
			levelpacks->value=i-v.begin();
			string s1=getUserPath()+"progress/"+*i+".progress";
			
			//Check if this is the special Levels levelpack.
			if(*i=="Levels"){
				//Clear the current levels.
				levels.clear();
				levels.setCurrentLevel(0);
				
				//List the custom levels and add them one for one.
				vector<string> v=enumAllFiles(getUserPath()+"custom/levels/");
				for(vector<string>::iterator i=v.begin(); i!=v.end(); ++i){
					levels.addLevel(getUserPath()+"custom/levels/"+*i);
					levels.setLocked(levels.getLevelCount()-1);
				}
				//List the addon levels and add them one for one.
				v=enumAllFiles(getUserPath()+"levels/");
				for(vector<string>::iterator i=v.begin(); i!=v.end(); ++i){
					levels.addLevel(getUserPath()+"levels/"+*i);
					levels.setLocked(levels.getLevelCount()-1);
				}
			}else{
				//This isn't so load the levelpack in the normal way.
				if(!levels.loadLevels(levelpackLocations[*i]+"/levels.lst")){
					msgBox("Can't load level pack:\n"+*i,MsgBoxOKOnly,"Error");
				}
			}
			//Load the progress.
			levels.loadProgress(s1);
		}
	}
	GUIObjectRoot->childControls.push_back(levelpacks);
	
	obj=new GUIObject(20,540,240,32,GUIObjectButton,"Back");
	obj->name="cmdBack";
	obj->eventCallback=this;
	GUIObjectRoot->childControls.push_back(obj);
	obj=new GUIObject(280,540,240,32,GUIObjectButton,"Clear Progress");
	obj->name="cmdReset";
	obj->eventCallback=this;
	GUIObjectRoot->childControls.push_back(obj);
	
	if(getSettings()->getBoolValue("internet")) {
		obj=new GUIObject(560,540,240,32,GUIObjectButton,"Addons");
		obj->name="cmdAddon";
		obj->eventCallback=this;
		GUIObjectRoot->childControls.push_back(obj);
	}
	
	//show level list
	refresh();
}

void LevelSelect::refresh(){
	int m=levels.getLevelCount();
	numbers.clear();

	//clear the selected level
	if(selectedNumber!=NULL){
		delete selectedNumber;
		selectedNumber=NULL;
	}

	for(int n=0; n<m; n++){
		numbers.push_back(Number());
	}

	for(int n=0; n<m; n++){
		SDL_Rect box={(n%10)*64+80,(n/10)*64+184,0,0};
		numbers[n].init(n,box);
	}

	if(m>40){
		levelScrollBar->maxValue=(m-41)/10;
		levelScrollBar->visible=true;
	}else{
		levelScrollBar->maxValue=0;
		levelScrollBar->visible=false;
	}
	levelpackDescription->caption=levels.levelpackDescription;
	int width,height;
	TTF_SizeText(fontGUI,levels.levelpackDescription.c_str(),&width,&height);
	levelpackDescription->left=(800-width)/2;
}

LevelSelect::~LevelSelect(){
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}
	levelScrollBar=NULL;
	levelpackDescription=NULL;

	if(selectedNumber!=NULL) delete selectedNumber;
	
	//Free the rendered title surface.
	SDL_FreeSurface(title);
}

void LevelSelect::handleEvents(){
	//Check for an SDL_QUIT event.
	if(event.type==SDL_QUIT){
		setNextState(STATE_EXIT);
	}
	
	//Check for a mouse click.
	if(event.type==SDL_MOUSEBUTTONUP && event.button.button==SDL_BUTTON_LEFT){
		checkMouse();
	}
	
	//Check if escape is pressed.
	if(inputMgr.isKeyUpEvent(INPUTMGR_ESCAPE)){
		setNextState(STATE_MENU);
	}
	
	//Check for scrolling down and up.
	if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button==SDL_BUTTON_WHEELDOWN && levelScrollBar){
		if(levelScrollBar->value<levelScrollBar->maxValue) levelScrollBar->value++;
		return;
	}else if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button==SDL_BUTTON_WHEELUP && levelScrollBar){
		if(levelScrollBar->value>0) levelScrollBar->value--;
		return;
	}
}

void LevelSelect::checkMouse(){
	int x,y,dy=0,m=levels.getLevelCount();
	
	//Get the current mouse location.
	SDL_GetMouseState(&x,&y);

	//Check if we should replay the record.
	if(selectedNumber!=NULL){
		SDL_Rect mouse={x,y,0,0};
		if(!bestTimeFilePath.empty()){
			SDL_Rect box={480,440,272,32};
			if(checkCollision(box,mouse)){
				Game::recordFile=bestTimeFilePath;
				levels.setCurrentLevel(selectedNumber->getNumber());
				setNextState(STATE_GAME);
				return;
			}
		}
		if(!bestRecordingFilePath.empty()){
			SDL_Rect box={480,472,272,32};
			if(checkCollision(box,mouse)){
				Game::recordFile=bestRecordingFilePath;
				levels.setCurrentLevel(selectedNumber->getNumber());
				setNextState(STATE_GAME);
				return;
			}
		}
	}
	
	//Check if there's a scrollbar, if so get the value.
	if(levelScrollBar)
		dy=levelScrollBar->value;
	if(m>dy*10+50)
		m=dy*10+50;
	y+=dy*64;

	SDL_Rect mouse={x,y,0,0};

	for(int n=dy*10; n<m; n++){
		if(levels.getLocked(n)==false){
			if(checkCollision(mouse,numbers[n].box)==true){
				if(numbers[n].selected){
					//current level was selected, so play it
					levels.setCurrentLevel(n);
					setNextState(STATE_GAME);
				}else{
					//select current level
					for(int i=0;i<levels.getLevelCount();i++){
						numbers[i].selected=(i==n);
					}
					//display level info
					displayLevelInfo(n);
				}
				break;
			}
		}
	}
}

void LevelSelect::displayLevelInfo(int number){
	//Update currently selected level
	if(selectedNumber==NULL){
		selectedNumber=new Number();
	}
	SDL_Rect box={40,440,50,50};
	selectedNumber->init(number,box);

	//show level description
	levelDescription=levels.getLevelName(number);

	//show level medal
	int medal=levels.getLevel(number)->won;
	int time=levels.getLevel(number)->time;
	int targetTime=levels.getLevel(number)->targetTime;
	int recordings=levels.getLevel(number)->recordings;
	int targetRecordings=levels.getLevel(number)->targetRecordings;

	if(medal){
		if(targetTime<0 && targetTime<0){
			medal=-1;
		}else{
			if(targetTime<0 || time<=targetTime)
				medal++;
			if(targetRecordings<0 || recordings<=targetRecordings)
				medal++;
		}
	}
	switch(medal){
	case 0:
		levelMedal="You haven't finished this level ";//"Medal: None";
		break;
	case 1:
		levelMedal="Medal: Bronze";
		break;
	case 2:
		levelMedal="Medal: Silver";
		break;
	case 3:
		levelMedal="Medal: Gold";
		break;
	default:
		levelMedal="Medal: Not avaliable for this level";
		break;
	}

	//show best time and recordings
	if(medal){
		char s[64];

		if(time>0)
			sprintf(s,"%-.2fs",time/40.0f);
		else
			s[0]='\0';
		levelMedal2=string("Time:        ")+s;

		if(recordings>=0)
			sprintf(s,"%d",recordings);
		else
			s[0]='\0';
		levelMedal3=string("Recordings: ")+s;
	}else{
		levelMedal2.clear();
		levelMedal3.clear();
	}

	//Check if there is auto record file
	levels.getLevelAutoSaveRecordPath(number,bestTimeFilePath,bestRecordingFilePath,false);
	//cout<<bestTimeFilePath<<endl; //debug
	if(!bestTimeFilePath.empty()){
		FILE *f;
		f=fopen(bestTimeFilePath.c_str(),"rb");
		if(f==NULL){
			bestTimeFilePath.clear();
		}else{
			fclose(f);
		}
	}
	if(!bestRecordingFilePath.empty()){
		FILE *f;
		f=fopen(bestRecordingFilePath.c_str(),"rb");
		if(f==NULL){
			bestRecordingFilePath.clear();
		}else{
			fclose(f);
		}
	}
}

void LevelSelect::logic(){}

void LevelSelect::render(){
	int x,y,dy=0,m=levels.getLevelCount();
	int idx=-1;
	
	//Get the current mouse location.
	SDL_GetMouseState(&x,&y);

	if(levelScrollBar)
		dy=levelScrollBar->value;
	if(m>dy*10+40)
		m=dy*10+40;
	y+=dy*64;

	SDL_Rect mouse={x,y,0,0};

	//Draw the background.
	applySurface(0,0,background,screen,NULL);
	//Draw the title.
	applySurface((800-title->w)/2,40,title,screen,NULL);
	
	//Loop through the level blocks and draw them.
	for(int n=dy*10; n<m;n++){
		numbers[n].show(dy*64);
		if(levels.getLocked(n)==false && checkCollision(mouse,numbers[n].box)==true)
			idx=n;
	}

	//Show currently selected level (if any)
	if(selectedNumber!=NULL){
		//Draw a background for the stats.
		drawGUIBox(0,420,SCREEN_WIDTH,SCREEN_HEIGHT-420,screen,0xFFFFFF80);

		selectedNumber->show(0);

		SDL_Color fg={0,0,0};
		SDL_Surface* bm;

		if(!levelDescription.empty()){
			bm=TTF_RenderText_Blended(fontText,levelDescription.c_str(),fg);
			applySurface(100,440,bm,screen,NULL);
			SDL_FreeSurface(bm);
		}

		if(!levelMedal.empty()){
			bm=TTF_RenderText_Blended(fontText,levelMedal.c_str(),fg);
			applySurface(40,504,bm,screen,NULL);
			SDL_FreeSurface(bm);
		}

		if(!bestTimeFilePath.empty()){
			SDL_Rect r={0,0,32,32};
			SDL_Rect box={480,440,272,32};

			if(checkCollision(box,mouse)){
				r.x=32;
				SDL_FillRect(screen,&box,0xFFCCCCCC);
			}

			applySurface(720,440,playButtonImage,screen,&r);
		}

		if(!bestRecordingFilePath.empty()){
			SDL_Rect r={0,0,32,32};
			SDL_Rect box={480,472,272,32};

			if(checkCollision(box,mouse)){
				r.x=32;
				SDL_FillRect(screen,&box,0xFFCCCCCC);
			}

			applySurface(720,472,playButtonImage,screen,&r);
		}

		if(!levelMedal2.empty()){
			bm=TTF_RenderText_Blended(fontText,levelMedal2.c_str(),fg);
			applySurface(480,440+(32-bm->h)/2,bm,screen,NULL);
			SDL_FreeSurface(bm);
		}

		if(!levelMedal3.empty()){
			bm=TTF_RenderText_Blended(fontText,levelMedal3.c_str(),fg);
			applySurface(480,472+(32-bm->h)/2,bm,screen,NULL);
			SDL_FreeSurface(bm);
		}
	}

	//Show the tool tip text.
	if(idx>=0){
		SDL_Color fg={0,0,0};
		char s[64];
		
		//Render the name of the level.
		SDL_Surface* name=TTF_RenderText_Blended(fontText,levels.getLevelName(idx).c_str(),fg);
		//The time it took.
		if(levels.getLevel(idx)->time>0)
			sprintf(s,"%-.2fs",levels.getLevel(idx)->time/40.0f);
		else
			s[0]='\0';
		SDL_Surface* time=TTF_RenderText_Blended(fontText,(string("Time:         ")+s).c_str(),fg);
		//The number of recordings it took.
		if(levels.getLevel(idx)->recordings>=0)
			sprintf(s,"%d",levels.getLevel(idx)->recordings);
		else
			s[0]='\0';
		SDL_Surface* recordings=TTF_RenderText_Blended(fontText,(string("Recordings:  ")+s).c_str(),fg);
		
		//Now draw a square the size of the three texts combined.
		SDL_Rect r=numbers[idx].box;
		r.y-=dy*80;
		r.w=(name->w)>time->w?(name->w)>recordings->w?name->w:recordings->w:(time->w)>recordings->w?time->w:recordings->w;
		r.h=name->h+5+time->h+recordings->h;
		
		//Make sure the tooltip doesn't go outside the window.
		if(r.y>SCREEN_HEIGHT-200){
			r.y-=name->h+4;
		}else{
			r.y+=numbers[idx].box.h+2;
		}
		if(r.x+r.w>SCREEN_WIDTH-50)
			r.x=SCREEN_WIDTH-50-r.w;
		
		//Draw a rectange
		Uint32 color=0xFFFFFF00|240;
		drawGUIBox(r.x-5,r.y-5,r.w+10,r.h+10,screen,color);
		
		//Calc the position to draw.
		SDL_Rect r2=r; //numbers[idx].box;
		/*r2.y-=dy*80;
		if(r2.y>SCREEN_HEIGHT-200){
			r2.y-=name->h+4;
		}else{
			r2.y+=numbers[idx].box.h+2;
		}
		if(r2.x+name->w>SCREEN_WIDTH-50)
			r2.x=SCREEN_WIDTH-50-name->w;
		*/
		
		//Now we render the name if the surface isn't null.
		if(name!=NULL){
			//Draw the name.
			SDL_BlitSurface(name,NULL,screen,&r2);
		}
		//Increase the height to leave a gap between name and stats.
		r2.y+=5;
		if(time!=NULL){
			//Now draw the time.
			r2.y+=name->h;
			SDL_BlitSurface(time,NULL,screen,&r2);
		}
		if(recordings!=NULL){
			//Now draw the recordings.
			r2.y+=time->h;
			SDL_BlitSurface(recordings,NULL,screen,&r2);
		}
		
		//And free the surfaces.
		SDL_FreeSurface(name);
		SDL_FreeSurface(time);
		SDL_FreeSurface(recordings);
	}
}

void LevelSelect::GUIEventCallback_OnEvent(std::string Name,GUIObject* obj,int nEventType){
	string s;
	if(Name=="cmdLvlPack"){
		s=levelpackLocations[((GUISingleLineListBox*)obj)->item[obj->value]];
		getSettings()->setValue("lastlevelpack",((GUISingleLineListBox*)obj)->item[obj->value]);
	}else if(Name=="cmdBack"){
		setNextState(STATE_MENU);
		return;
	}else if(Name=="cmdReset"){
		if(msgBox("Do you really want to reset level progress?",MsgBoxYesNo,"Warning")==MsgBoxYes){
			//clear the selected level
			if(selectedNumber!=NULL){
				delete selectedNumber;
				selectedNumber=NULL;
			}

			//clear the progress
			if(getSettings()->getValue("lastlevelpack")!="Levels"){
				for(int i=0;i<levels.getLevelCount();i++){
					levels.resetLevel(i);
					numbers[i].selected=false;
					numbers[i].update();
				}
			}else{
				for(int i=0;i<levels.getLevelCount();i++){
					levels.resetLevel(i);
					levels.setLocked(i,false);
					numbers[i].selected=false;
					numbers[i].update();
				}
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

	string s1=getUserPath()+"progress/"+((GUISingleLineListBox*)obj)->item[obj->value]+".progress";
	
	//Check if this is the special Levels levelpack.
	if(((GUISingleLineListBox*)obj)->item[obj->value]=="Levels"){
		//Clear the current levels.
		levels.clear();
		levels.setCurrentLevel(0);
		
		//List the custom levels and add them one for one.
		vector<string> v=enumAllFiles(getUserPath()+"custom/levels/");
		for(vector<string>::iterator i=v.begin(); i!=v.end(); ++i){
			levels.addLevel(getUserPath()+"custom/levels/"+*i);
			levels.setLocked(levels.getLevelCount()-1);
		}
		//List the addon levels and add them one for one.
		v=enumAllFiles(getUserPath()+"levels/");
		for(vector<string>::iterator i=v.begin(); i!=v.end(); ++i){
			levels.addLevel(getUserPath()+"levels/"+*i);
			levels.setLocked(levels.getLevelCount()-1);
		}
	}else{
		//This isn't so load the levelpack in the normal way.
		if(!levels.loadLevels(levelpackLocations[((GUISingleLineListBox*)obj)->item[obj->value]]+"/levels.lst")){
			msgBox("Can't load level pack:\n"+((GUISingleLineListBox*)obj)->item[obj->value],MsgBoxOKOnly,"Error");
		}
	}
	//Load the progress file.
	levels.loadProgress(s1);
	
	//And refresh the numbers.
	refresh();
}
