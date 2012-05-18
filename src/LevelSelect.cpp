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

#include "libs/tinyformat/tinyformat.h"

using namespace std;

////////////////////NUMBER////////////////////////
Number::Number(){
	image=NULL;
	number=0;
	medal=0;
	selected=false;
	locked=false;
	
	//Set the default dimensions.
	box.x=0;
	box.y=0;
	box.h=50;
	box.w=50;
	
	//Load the medals image.
	background=loadImage(getDataPath()+"gfx/level.png");
	backgroundLocked=loadImage(getDataPath()+"gfx/levellocked.png");
	medals=loadImage(getDataPath()+"gfx/medals.png");
}

Number::~Number(){
	//We only need to free the SDLSurface.
	if(image) SDL_FreeSurface(image);
}

void Number::init(int number,SDL_Rect box){
	//First set the number and update our status.
	this->number=number;

	//Write our text, number+1 since the counting doens't start with 0, but with 1.
	std::stringstream text;
	number++;
	text<<number;

	//Create the text image.
	SDL_Color black={0,0,0};
	if(image) SDL_FreeSurface(image);
	//Create the text image.
	//Also check which font to use, if the number is higher than 100 use the small font.
	image=TTF_RenderUTF8_Blended(fontGUI,text.str().c_str(),black);

	//Set the new location of the number.
	this->box.x=box.x;
	this->box.y=box.y;
}

void Number::init(std::string text,SDL_Rect box){
	//First set the number and update our status.
	this->number=-1;

	//Create the text image.
	SDL_Color black={0,0,0};
	if(image) SDL_FreeSurface(image);
	image=TTF_RenderUTF8_Blended(fontGUI,text.c_str(),black);

	//Set the new location of the number.
	this->box.x=box.x;
	this->box.y=box.y;
}

void Number::show(int dy){
	//First draw the background, also apply the yOffset(dy).
	if(!locked)
		applySurface(box.x,box.y-dy,background,screen,NULL);
	else
		applySurface(box.x,box.y-dy,backgroundLocked,screen,NULL);
	//Now draw the text image over the background.
	//We draw it centered inside the box.
	applySurface((box.x+25-(image->w/2)),box.y+((TTF_FontAscent(fontGUI)+TTF_FontDescent(fontGUI))/2)-dy,image,screen,NULL);

	//Draw the selection mark.
	if(selected){
		drawGUIBox(box.x,box.y-dy,50,50,screen,0xFFFFFF23);
	}
	
	//Draw the medal.
	if(medal>0){
		SDL_Rect r={(medal-1)*30,0,30,30};
		applySurface(box.x+30,(box.y+30)-dy,medals,screen,&r);
	}
}

void Number::setLocked(bool locked){
	this->locked=locked;
}

void Number::setMedal(int medal){
	this->medal=medal;
}


/////////////////////LEVEL SELECT/////////////////////
LevelSelect::LevelSelect(string titleText,LevelPackManager::LevelPackLists packType){
	//clear the selected level
	selectedNumber=NULL;
	
	//Calculate the LEVELS_PER_ROW and LEVEL_ROWS if they aren't calculated already.
	calcRows();
	
	//Render the title.
	SDL_Color black={0,0,0};
	title=TTF_RenderUTF8_Blended(fontTitle,titleText.c_str(),black);
	
	//create GUI (test only)
	GUIObject* obj;
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}

	GUIObjectRoot=new GUIObject(0,0,SCREEN_WIDTH,SCREEN_HEIGHT);

	//the level select scroll bar
	levelScrollBar=new GUIScrollBar(SCREEN_WIDTH*0.9,184,16,SCREEN_HEIGHT-344,ScrollBarVertical,0,0,0,1,4,true,false);
	GUIObjectRoot->childControls.push_back(levelScrollBar);

	//level pack description
	levelpackDescription=new GUIObject(60,140,800,32,GUIObjectLabel);
	GUIObjectRoot->childControls.push_back(levelpackDescription);

	levelpacks=new GUISingleLineListBox((SCREEN_WIDTH-500)/2,104,500,32);
	levelpacks->name="cmdLvlPack";
	levelpacks->eventCallback=this;
	vector<string> v=getLevelPackManager()->enumLevelPacks(packType);
	levelpacks->item=v;
	levelpacks->value=0;

	//Check if we can find the lastlevelpack.
	for(vector<string>::iterator i=v.begin(); i!=v.end(); ++i){
		if(*i==getSettings()->getValue("lastlevelpack")){
			levelpacks->value=i-v.begin();
		}
	}
	
	//Get the name of the selected levelpack.
	string levelpackName=levelpacks->item[levelpacks->value];
	string s1=getUserPath(USER_DATA)+"progress/"+levelpackName+".progress";
	
	//Load the progress.
	levels=getLevelPackManager()->getLevelPack(v[levelpacks->value]);
	levels->loadProgress(s1);
	
	//And add the levelpack single line listbox to the GUIObjectRoot.
	GUIObjectRoot->childControls.push_back(levelpacks);
	
	obj=new GUIObject(20,20,-1,32,GUIObjectButton,_("Back"));
	obj->name="cmdBack";
	obj->eventCallback=this;
	GUIObjectRoot->childControls.push_back(obj);
	
	section=0;
}

LevelSelect::~LevelSelect(){
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}
	levelScrollBar=NULL;
	levelpackDescription=NULL;
	
	selectedNumber=NULL;
	
	//Free the rendered title surface.
	SDL_FreeSurface(title);
}

void LevelSelect::calcRows(){
	//Calculate the number of rows and the number of levels per row.
	LEVELS_PER_ROW=(SCREEN_WIDTH*0.8)/64;
	int LEVEL_ROWS=(SCREEN_HEIGHT-344)/64;
	LEVELS_DISPLAYED_IN_SCREEN=LEVELS_PER_ROW*LEVEL_ROWS;
}

void LevelSelect::selectNumberKeyboard(int x,int y){
	if(section==2){
		//Move selection
		int realNumber=selectedNumber->getNumber()+x+(y*LEVELS_PER_ROW);
		
		//If selection is outside of the map grid, change section
		if(realNumber<0 || realNumber>numbers.size()-1){
			section=1;
			for(int i=0;i<levels->getLevelCount();i++){
				numbers[i].selected=false;
			}
		}else{
			//If not, move selection
			if(!numbers[realNumber].getLocked()){
				selectNumber(realNumber,false);
				for(int i=0;i<levels->getLevelCount();i++){
					numbers[i].selected=(i==realNumber);
				}
			}
		}
	}else if(section==1){
		//Loop through levelpacks and update GUI
		levelpacks->value+=x;
		
		if(levelpacks->value<0){
			levelpacks->value=levelpacks->item.size()-1;
		}else if(levelpacks->value>levelpacks->item.size()-1){
			levelpacks->value=0;
		}
		
		GUIEventCallback_OnEvent("cmdLvlPack",(GUIObject*)levelpacks,0);
		
		//If up is pressed, change section
		if(y==1){
			section=2;
			selectNumber(0,false);
			numbers[0].selected=true;
		}
	}else{
		section=clamp(section+y,0,2);
	}
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
	
	//Check focus movement
	if(inputMgr.isKeyDownEvent(INPUTMGR_RIGHT)){
		selectNumberKeyboard(1,0);
	}else if(inputMgr.isKeyDownEvent(INPUTMGR_LEFT)){
		selectNumberKeyboard(-1,0);
	}else if(inputMgr.isKeyDownEvent(INPUTMGR_UP)){
		selectNumberKeyboard(0,-1);
	}else if(inputMgr.isKeyDownEvent(INPUTMGR_DOWN)){
		selectNumberKeyboard(0,1);
	}
	
	//Check if enter is pressed
	if(section==2 && inputMgr.isKeyUpEvent(INPUTMGR_SELECT)){
		selectNumber(selectedNumber->getNumber(),true);
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
	int x,y,dy=0,m=numbers.size();
	
	//Get the current mouse location.
	SDL_GetMouseState(&x,&y);

	//Check if there's a scrollbar, if so get the value.
	if(levelScrollBar)
		dy=levelScrollBar->value;
	//Upper bound of levels we'd like to display.
	if(m>dy*LEVELS_PER_ROW+LEVELS_DISPLAYED_IN_SCREEN)
		m=dy*LEVELS_PER_ROW+LEVELS_DISPLAYED_IN_SCREEN;
	y+=dy*64;

	SDL_Rect mouse={x,y,0,0};

	for(int n=dy*LEVELS_PER_ROW; n<m; n++){
		if(!numbers[n].getLocked()){
			if(checkCollision(mouse,numbers[n].box)==true){
				if(numbers[n].selected){
					selectNumber(n,true);
				}else{
					//Select current level
					for(int i=0;i<levels->getLevelCount();i++){
						numbers[i].selected=(i==n);
					}
					selectNumber(n,false);
				}
				section=2;
				break;
			}
		}
	}
}

void LevelSelect::logic(){}

void LevelSelect::render(){
	int x,y,dy=0,m=numbers.size();
	int idx=-1;
	
	//Get the current mouse location.
	SDL_GetMouseState(&x,&y);

	if(levelScrollBar)
		dy=levelScrollBar->value;
	//Upper bound of levels we'd like to display.
	if(m>dy*LEVELS_PER_ROW+LEVELS_DISPLAYED_IN_SCREEN)
		m=dy*LEVELS_PER_ROW+LEVELS_DISPLAYED_IN_SCREEN;
	y+=dy*64;

	SDL_Rect mouse={x,y,0,0};

	//Draw the menu background.
	applySurface(0,0,menuBackground,screen,NULL);
	//Draw the title.
	applySurface((SCREEN_WIDTH-title->w)/2,40,title,screen,NULL);
	
	//Loop through the level blocks and draw them.
	for(int n=dy*LEVELS_PER_ROW;n<m;n++){
		numbers[n].show(dy*64);
		if(numbers[n].getLocked()==false && checkCollision(mouse,numbers[n].box)==true)
			idx=n;
	}
	
	//Show the tool tip text.
	if(idx>=0){
		renderTooltip(idx,dy);
	}
}

void LevelSelect::resize(){
	calcRows();
	refresh();
	
	//NOTE: We don't need to recreate the listbox and the back button, only resize the list.
	levelpacks->left=(SCREEN_WIDTH-500)/2;
}

void LevelSelect::GUIEventCallback_OnEvent(std::string name,GUIObject* obj,int eventType){
	string s;
	if(name=="cmdLvlPack"){
		getSettings()->setValue("lastlevelpack",((GUISingleLineListBox*)obj)->item[obj->value]);
	}else if(name=="cmdBack"){
		setNextState(STATE_MENU);
		return;
	}else{
		return;
	}

	//new: reset the level list scroll bar
	if(levelScrollBar)
		levelScrollBar->value=0;

	string s1=getUserPath(USER_DATA)+"progress/"+((GUISingleLineListBox*)obj)->item[obj->value]+".progress";
	levels=getLevelPackManager()->getLevelPack(((GUISingleLineListBox*)obj)->item[obj->value]);
	//Load the progress file.
	levels->loadProgress(s1);
	
	//And refresh the numbers.
	refresh();
}
