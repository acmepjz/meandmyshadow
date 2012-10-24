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

#include "GameState.h"
#include "Globals.h"
#include "Functions.h"
#include "FileManager.h"
#include "GameObjects.h"
#include "ThemeManager.h"
#include "Objects.h"
#include "LevelPack.h"
#include "LevelEditor.h"
#include "TreeStorageNode.h"
#include "POASerializer.h"
#include "GUIListBox.h"
#include "GUITextArea.h"
#include "GUIOverlay.h"
#include "InputManager.h"
#include "StatisticsManager.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef WIN32
#include <windows.h>
#include <shlobj.h>
#else
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#endif

#include "libs/tinyformat/tinyformat.h"

using namespace std;

static int levelTime,levelRecordings;
static GUIObject *levelTimeProperty,*levelRecordingsProperty;

//Array containing translateble block names
static const char* blockNames[TYPE_MAX]={
	__("Block"),__("Player Start"),__("Shadow Start"),
	__("Exit"),__("Shadow Block"),__("Spikes"),
	__("Checkpoint"),__("Swap"),__("Fragile"),
	__("Moving Block"),__("Moving Shadow Block"),__("Moving Spikes"),
	__("Teleporter"),__("Button"),__("Switch"),
	__("Conveyor Belt"),__("Shadow Conveyor Belt"),__("Notification Block"),__("Collectable")
};

//Array indicates if block is configurable
static const bool isConfigurable[TYPE_MAX]={
	false,false,false,
	false,false,false,
	false,false,true,
	true,true,true,
	true,true,true,
	true,true,true,false
};

//Array indicates if block is linkable
static const bool isLinkable[TYPE_MAX]={
	false,false,false,
	false,false,false,
	false,false,false,
	true,true,true,
	true,true,true,
	false,false,false,false
};

/////////////////LevelEditorToolbox////////////////////////

class LevelEditorToolbox{
private:
	//The parent object
	LevelEditor* parent;

	//The position of window
	SDL_Rect rect;

	//Background surface
	SDL_Surface *background;

	//GUI image
	SDL_Surface *bmGUI;

public:
	int startRow,maxRow;
	bool visible;
	bool dragging;

public:
	SDL_Rect getRect(){
		return rect;
	}
	int width(){
		return 320;
	}
	int height(){
		return 180;
	}
	LevelEditorToolbox(LevelEditor* parent){
		this->parent=parent;

		visible=false;
		dragging=false;

		//calc row count
		startRow=0;
		maxRow=(LevelEditor::EDITOR_ORDER_MAX+4)/5;

		//set size
		rect.w=width();
		rect.h=height();

		//Load the gui images.
		bmGUI=loadImage(getDataPath()+"gfx/gui.png");

		//create background and draw something on it
		background=SDL_CreateRGBSurface(SDL_HWSURFACE,
			rect.w,rect.h,screen->format->BitsPerPixel,
			screen->format->Rmask,screen->format->Gmask,screen->format->Bmask,0);

		//background
		drawGUIBox(0,0,rect.w,rect.h,background,0xFFFFFFFFU);

		//caption
		{
			SDL_Rect captionRect={6,8,width()-16,32};
			//SDL_FillRect(background,&captionRect,0xCCCCCCU);

			SDL_Color fg={0,0,0};

			SDL_Surface *caption=TTF_RenderUTF8_Blended(fontGUI,_("Toolbox"),fg);

			applySurface(captionRect.x+(captionRect.w-caption->w)/2,
				captionRect.y+(captionRect.h-caption->h)/2,caption,background,NULL);

			SDL_FreeSurface(caption);
		}
	}
	~LevelEditorToolbox(){
		SDL_FreeSurface(background);
	}
	void move(int x,int y){
		if(x>SCREEN_WIDTH-rect.w) x=SCREEN_WIDTH-rect.w;
		else if(x<0) x=0;
		if(y>SCREEN_HEIGHT-rect.h) y=SCREEN_HEIGHT-rect.h;
		else if(y<0) y=0;
		rect.x=x;
		rect.y=y;
	}
	void render(){
		if(visible){
			applySurface(rect.x,rect.y,background,screen,NULL);

			//get mouse position
			int x,y;
			SDL_GetMouseState(&x,&y);
			SDL_Rect mouse={x,y,0,0};

			//draw close button
			{
				//check highlight
				SDL_Rect r={rect.x+rect.w-36,rect.y+12,24,24};

				if(checkCollision(mouse,r)){
					drawGUIBox(r.x,r.y,r.w,r.h,screen,0x999999FFU);
				}

				SDL_Rect r1={112,0,16,16};
				applySurface(rect.x+rect.w-32,rect.y+16,bmGUI,screen,&r1);
			}

			//the tool tip of item
			SDL_Rect tooltipRect;
			string tooltip;

			//draw avaliable item
			for(int i=0;i<2;i++){
				int j=startRow+i;
				if(j>=maxRow) j-=maxRow;

				for(int k=0;k<5;k++){
					int idx=j*5+k;
					if(idx<0 || idx>=LevelEditor::EDITOR_ORDER_MAX) break;

					SDL_Rect r={rect.x+k*60+10,rect.y+i*60+50,60,60};

					//check highlight
					if(checkCollision(mouse,r)){
						tooltipRect=r;
						tooltip=_(blockNames[LevelEditor::editorTileOrder[idx]]);

						if(parent->currentType==idx){
							drawGUIBox(r.x,r.y,r.w,r.h,screen,0x999999FFU);
						}else{
							SDL_FillRect(screen,&r,0xCCCCCC);
						}
					}else if(parent->currentType==idx){
						drawGUIBox(r.x,r.y,r.w,r.h,screen,0xCCCCCCFFU);
					}

					ThemeBlock* obj=objThemes.getBlock(LevelEditor::editorTileOrder[idx]);
					if(obj){
						obj->editorPicture.draw(screen,r.x+5,r.y+5);
					}
				}
			}

			//draw tooltip
			if(!tooltip.empty()){
				//The back and foreground colors.
				SDL_Color fg={0,0,0};

				//Tool specific text.
				SDL_Surface* tip=TTF_RenderUTF8_Blended(fontText,tooltip.c_str(),fg);

				//Draw only if there's a tooltip available
				if(tip!=NULL){
					if(tooltipRect.y+tooltipRect.h+tip->h>SCREEN_HEIGHT-20)
						tooltipRect.y-=tip->h;
					else
						tooltipRect.y+=tooltipRect.h;

					if(tooltipRect.x+tip->w>SCREEN_WIDTH-20)
						tooltipRect.x=SCREEN_WIDTH-20-tip->w;

					//Draw borders around text
					Uint32 color=0xFFFFFF00|230;
					drawGUIBox(tooltipRect.x-2,tooltipRect.y-2,tip->w+4,tip->h+4,screen,color);

					//Draw tooltip's text
					SDL_BlitSurface(tip,NULL,screen,&tooltipRect);
					SDL_FreeSurface(tip);
				}
			}
		}
	}
	void handleEvents(){
		if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button==SDL_BUTTON_LEFT){
			SDL_Rect mouse={event.button.x,event.button.y,0,0};

			//Check if item is clicked
			for(int i=0;i<2;i++){
				int j=startRow+i;
				if(j>=maxRow) j-=maxRow;

				for(int k=0;k<5;k++){
					int idx=j*5+k;
					if(idx<0 || idx>=LevelEditor::EDITOR_ORDER_MAX) break;

					SDL_Rect r={rect.x+k*60+10,rect.y+i*60+50,60,60};

					//check highlight
					if(checkCollision(mouse,r)){
						parent->currentType=idx;
						return;
					}
				}
			}

			//Now begin drag the toolbox
			dragging=true;
		}
		else if(event.type==SDL_MOUSEBUTTONUP && event.button.button==SDL_BUTTON_LEFT){
			//Stop dragging
			dragging=false;

			SDL_Rect mouse={event.button.x,event.button.y,0,0};

			//Check if close button clicked
			{
				SDL_Rect r={rect.x+rect.w-36,rect.y+12,24,24};
				if(checkCollision(mouse,r)){
					visible=false;
					return;
				}
			}
		}
		else if(event.type==SDL_MOUSEMOTION){
			if((event.motion.state & SDL_BUTTON_LMASK)==0){
				dragging=false;
			}else if(dragging){
				move(rect.x+event.motion.xrel,rect.y+event.motion.yrel);
			}
		}
	}
};

/////////////////LevelEditorSelectionPopup/////////////////

class LevelEditorSelectionPopup{
private:
	//The parent object
	LevelEditor* parent;

	//The position of window
	SDL_Rect rect;

	//GUI image
	SDL_Surface *bmGUI;

	//The selection
	std::vector<GameObject*> selection;

	//The scrollbar
	GUIScrollBar *scrollBar;

	//Highlighted object
	GameObject* highlightedObj;

	//Highlighted button index. 0=none 1=select/deselect 2=configure 3=link 4=delete
	int highlightedBtn;
public:
	int startRow,showedRow;
	bool dragging;

	//If selection is dirty
	bool dirty;

public:
	SDL_Rect getRect(){
		return rect;
	}
	int width(){
		return rect.w;
	}
	int height(){
		return rect.h;
	}
	void updateScrollBar(){
		int m=selection.size()-showedRow;
		if(m>0){
			if(startRow<0) startRow=0;
			else if(startRow>m) startRow=m;

			if(scrollBar==NULL){
				scrollBar=new GUIScrollBar(0,0,16,rect.h-16,ScrollBarVertical,startRow,0,m,1,showedRow);
			}

			scrollBar->visible=true;
			scrollBar->maxValue=m;
			scrollBar->value=startRow;
		}else{
			startRow=0;
			if(scrollBar){
				scrollBar->visible=false;
				scrollBar->value=0;
			}
		}
	}
	void updateSelection(){
		if(parent!=NULL){
			std::vector<GameObject*>& v=parent->levelObjects;

			for(int i=selection.size()-1;i>=0;i--){
				if(find(v.begin(),v.end(),selection[i])==v.end()){
					selection.erase(selection.begin()+i);
				}
			}

			updateScrollBar();
		}
	}
	void dismiss(){
		if(parent!=NULL && parent->selectionPopup==this){
			parent->selectionPopup=NULL;
		}
		delete this;
	}
	LevelEditorSelectionPopup(LevelEditor* parent, std::vector<GameObject*>& selection, int x=0, int y=0){
		this->parent=parent;
		this->selection=selection;

		dirty=false;
		dragging=false;
		scrollBar=NULL;
		highlightedObj=NULL;
		highlightedBtn=0;

		//calc window size
		startRow=0;
		showedRow=selection.size();
		int m=SCREEN_HEIGHT/64-1;
		if(showedRow>m) showedRow=m;

		rect.w=320;
		rect.h=showedRow*64+16;

		if(x>SCREEN_WIDTH-rect.w) x=SCREEN_WIDTH-rect.w;
		else if(x<0) x=0;
		if(y>SCREEN_HEIGHT-rect.h) y=SCREEN_HEIGHT-rect.h;
		else if(y<0) y=0;
		rect.x=x;
		rect.y=y;

		updateScrollBar();

		//Load the gui images.
		bmGUI=loadImage(getDataPath()+"gfx/gui.png");
	}
	~LevelEditorSelectionPopup(){
		if(scrollBar) delete scrollBar;
	}
	void move(int x,int y){
		if(x>SCREEN_WIDTH-rect.w) x=SCREEN_WIDTH-rect.w;
		else if(x<0) x=0;
		if(y>SCREEN_HEIGHT-rect.h) y=SCREEN_HEIGHT-rect.h;
		else if(y<0) y=0;
		rect.x=x;
		rect.y=y;
	}
	void render(){
		//Check dirty
		if(dirty){
			updateSelection();
			if(selection.empty()){
				dismiss();
				return;
			}
			dirty=false;
		}

		//background
		drawGUIBox(rect.x,rect.y,rect.w,rect.h,screen,0xFFFFFFFFU);

		//get mouse position
		int x,y;
		SDL_GetMouseState(&x,&y);
		SDL_Rect mouse={x,y,0,0};

		//the tool tip of item
		SDL_Rect tooltipRect;
		string tooltip;

		if(scrollBar && scrollBar->visible){
			startRow=scrollBar->value;
		}

		highlightedObj=NULL;
		highlightedBtn=0;

		//draw avaliable item
		for(int i=0;i<showedRow;i++){
			int j=startRow+i;
			if(j>=(int)selection.size()) break;

			SDL_Rect r={rect.x+8,rect.y+i*64+8,rect.w-16,64};
			if(scrollBar && scrollBar->visible) r.w-=24;

			//check highlight
			if(checkCollision(mouse,r)){
				highlightedObj=selection[j];
				SDL_FillRect(screen,&r,0xCCCCCC);
			}

			int type=selection[j]->type;

			//draw tile picture
			ThemeBlock* obj=objThemes.getBlock(type);
			if(obj){
				obj->editorPicture.draw(screen,r.x+7,r.y+7);
			}

			//draw name
			SDL_Color fg={0,0,0};
			SDL_Surface* txt=TTF_RenderUTF8_Blended(fontText,_(blockNames[type]),fg);
			if(txt!=NULL){
				SDL_Rect r2={r.x+64,r.y+(64-txt->h)/2,0,0};
				SDL_BlitSurface(txt,NULL,screen,&r2);
				SDL_FreeSurface(txt);
			}

			if(parent!=NULL){
				//draw selected
				{
					std::vector<GameObject*> &v=parent->selection;
					bool isSelected=find(v.begin(),v.end(),selection[j])!=v.end();

					SDL_Rect r1={isSelected?16:0,0,16,16};
					SDL_Rect r2={r.x+r.w-96,r.y+20,24,24};
					if(checkCollision(mouse,r2)){
						drawGUIBox(r2.x,r2.y,r2.w,r2.h,screen,0x999999FFU);
						tooltipRect=r2;
						tooltip=_("Select");
						highlightedBtn=1;
					}
					r2.x+=4;
					r2.y+=4;
					SDL_BlitSurface(bmGUI,&r1,screen,&r2);
				}

				//draw configure
				if(isConfigurable[type]){
					SDL_Rect r1={112,16,16,16};
					SDL_Rect r2={r.x+r.w-72,r.y+20,24,24};
					if(checkCollision(mouse,r2)){
						drawGUIBox(r2.x,r2.y,r2.w,r2.h,screen,0x999999FFU);
						tooltipRect=r2;
						tooltip=_("Configure");
						highlightedBtn=2;
					}
					r2.x+=4;
					r2.y+=4;
					SDL_BlitSurface(bmGUI,&r1,screen,&r2);
				}

				//draw link
				if(isLinkable[type]){
					SDL_Rect r1={112,32,16,16};
					SDL_Rect r2={r.x+r.w-48,r.y+20,24,24};
					if(checkCollision(mouse,r2)){
						drawGUIBox(r2.x,r2.y,r2.w,r2.h,screen,0x999999FFU);
						tooltipRect=r2;
						tooltip=_("Link");
						highlightedBtn=3;
					}
					r2.x+=4;
					r2.y+=4;
					SDL_BlitSurface(bmGUI,&r1,screen,&r2);
				}

				//draw delete
				{
					SDL_Rect r1={112,0,16,16};
					SDL_Rect r2={r.x+r.w-24,r.y+20,24,24};
					if(checkCollision(mouse,r2)){
						drawGUIBox(r2.x,r2.y,r2.w,r2.h,screen,0x999999FFU);
						tooltipRect=r2;
						tooltip=_("Delete");
						highlightedBtn=4;
					}
					r2.x+=4;
					r2.y+=4;
					SDL_BlitSurface(bmGUI,&r1,screen,&r2);
				}
			}
		}

		//draw scrollbar
		if(scrollBar && scrollBar->visible){
			scrollBar->render(rect.x+rect.w-24,rect.y+8);
		}

		//draw tooltip
		if(!tooltip.empty()){
			//The back and foreground colors.
			SDL_Color fg={0,0,0};

			//Tool specific text.
			SDL_Surface* tip=TTF_RenderUTF8_Blended(fontText,tooltip.c_str(),fg);

			//Draw only if there's a tooltip available
			if(tip!=NULL){
				tooltipRect.y-=4;
				tooltipRect.h+=8;
				if(tooltipRect.y+tooltipRect.h+tip->h>SCREEN_HEIGHT-20)
					tooltipRect.y-=tip->h;
				else
					tooltipRect.y+=tooltipRect.h;

				if(tooltipRect.x+tip->w>SCREEN_WIDTH-20)
					tooltipRect.x=SCREEN_WIDTH-20-tip->w;

				//Draw borders around text
				Uint32 color=0xFFFFFF00|230;
				drawGUIBox(tooltipRect.x-2,tooltipRect.y-2,tip->w+4,tip->h+4,screen,color);

				//Draw tooltip's text
				SDL_BlitSurface(tip,NULL,screen,&tooltipRect);
				SDL_FreeSurface(tip);
			}
		}
	}
	void handleEvents(){
		//Check dirty
		if(dirty){
			updateSelection();
			if(selection.empty()){
				dismiss();
				return;
			}
			dirty=false;
		}

		//Check scrollbar event
		if(scrollBar && scrollBar->visible){
			if(scrollBar->handleEvents(rect.x+rect.w-24,rect.y+8)) return;
		}

		if(event.type==SDL_MOUSEBUTTONDOWN){
			//check mousewheel
			if(event.button.button==SDL_BUTTON_WHEELUP){
				startRow-=2;
				updateScrollBar();
				return;
			}else if(event.button.button==SDL_BUTTON_WHEELDOWN){
				startRow+=2;
				updateScrollBar();
				return;
			}

			//begin drag
			if(event.button.button==SDL_BUTTON_LEFT) dragging=true;
		}
		else if(event.type==SDL_MOUSEBUTTONUP && event.button.button==SDL_BUTTON_LEFT){
			//end drag
			dragging=false;

			SDL_Rect mouse={event.button.x,event.button.y,0,0};

			//Check if close it
			if(!checkCollision(mouse,rect)){
				dismiss();
				return;
			}

			//Check if item is clicked
			if(highlightedObj!=NULL && highlightedBtn>0 && parent!=NULL){
				std::vector<GameObject*>& v=parent->levelObjects;

				if(find(v.begin(),v.end(),highlightedObj)!=v.end()){
					switch(highlightedBtn){
					case 1:
						{
							std::vector<GameObject*>& v2=parent->selection;
							std::vector<GameObject*>::iterator it=find(v2.begin(),v2.end(),highlightedObj);

							if(it==v2.end()){
								v2.push_back(highlightedObj);
							}else{
								v2.erase(it);
							}
						}
						break;
					case 2:
						parent->tool=LevelEditor::CONFIGURE;
						parent->onEnterObject(highlightedObj);
						break;
					case 3:
						{
							std::vector<GameObject*>& v2=parent->selection;

							parent->tool=LevelEditor::CONFIGURE;
							parent->onRightClickObject(highlightedObj,find(v2.begin(),v2.end(),highlightedObj)!=v2.end());

							dismiss();
						}
						break;
					case 4:
						parent->removeObject(highlightedObj);
						break;
					}
				}
			}
		}
		else if(event.type==SDL_MOUSEMOTION){
			if((event.motion.state & SDL_BUTTON_LMASK)==0){
				dragging=false;
			}else if(dragging){
				move(rect.x+event.motion.xrel,rect.y+event.motion.yrel);
			}
		}
	}
};

/////////////////MovingPosition////////////////////////////
MovingPosition::MovingPosition(int x,int y,int time){
	this->x=x;
	this->y=y;
	this->time=time;
}

MovingPosition::~MovingPosition(){}

void MovingPosition::updatePosition(int x,int y){
	this->x=x;
	this->y=y;
}


/////////////////LEVEL EDITOR//////////////////////////////
LevelEditor::LevelEditor():Game(true){
	//Get the target time and recordings.
	levelTime=levels->getLevel()->targetTime;
	levelRecordings=levels->getLevel()->targetRecordings;

	//This will set some default settings.
	reset();

	//The level is loaded by the game, so do postLoad.
	postLoad();

	//Load the toolbar.
	toolbar=loadImage(getDataPath()+"gfx/menu/toolbar.png");
	SDL_Rect tmp={(SCREEN_WIDTH-460)/2,SCREEN_HEIGHT-50,460,50};
	toolbarRect=tmp;

	toolbox=NULL;
	selectionPopup=NULL;
	
	movingSpeedWidth=-1;

	//Load the selectionMark.
	selectionMark=loadImage(getDataPath()+"gfx/menu/selection.png");

	//Load the movingMark.
	movingMark=loadImage(getDataPath()+"gfx/menu/moving.png");

	//Create the semi transparent surface.
	placement=SDL_CreateRGBSurface(SDL_SWSURFACE|SDL_SRCALPHA,SCREEN_WIDTH,SCREEN_HEIGHT,32,RMASK,GMASK,BMASK,0);
	SDL_SetColorKey(placement,SDL_SRCCOLORKEY|SDL_RLEACCEL,SDL_MapRGB(placement->format,255,0,255));
	SDL_SetAlpha(placement,SDL_SRCALPHA,125);

	//Count the level editing time.
	statsMgr.startLevelEdit();
}

LevelEditor::~LevelEditor(){
	//Loop through the levelObjects and delete them.
	for(unsigned int i=0;i<levelObjects.size();i++)
		delete levelObjects[i];
	levelObjects.clear();
	selection.clear();

	//Free the placement surface.
	SDL_FreeSurface(placement);

	//Delete the toolbox (if any)
	if(toolbox){
		delete toolbox;
		toolbox=NULL;
	}

	//Delete the popup
	if(selectionPopup){
		delete selectionPopup;
		selectionPopup=NULL;
	}

	//Reset the camera.
	camera.x=0;
	camera.y=0;

	//Count the level editing time.
	statsMgr.endLevelEdit();
}

void LevelEditor::reset(){
	//Set some default values.
	playMode=false;
	tool=ADD;
	currentType=0;
	pressedShift=false;
	pressedLeftMouse=false;
	dragging=false;
	selectionDrag=false;
	dragCenter=NULL;
	if(LEVEL_WIDTH<SCREEN_WIDTH)
		camera.x=-(SCREEN_WIDTH-LEVEL_WIDTH)/2;
	else
		camera.x=0;
	if(LEVEL_HEIGHT<SCREEN_HEIGHT)
		camera.y=-(SCREEN_HEIGHT-LEVEL_HEIGHT)/2;
	else
		camera.y=0;
	cameraXvel=0;
	cameraYvel=0;
	objectProperty=NULL;
	secondObjectProperty=NULL;
	configuredObject=NULL;
	linking=false;
	linkingTrigger=NULL;
	currentId=0;
	movingBlock=NULL;
	moving=false;
	movingSpeed=10;
	tooltip=-1;

	//Set the player and shadow to their starting position.
	player.setPosition(player.fx,player.fy);
	shadow.setPosition(shadow.fx,shadow.fy);

	selection.clear();
	clipboard.clear();
	triggers.clear();
	movingBlocks.clear();
}

void LevelEditor::loadLevelFromNode(TreeStorageNode* obj, const std::string& fileName){
	//call the method of base class.
	Game::loadLevelFromNode(obj,fileName);

	//now do our own stuff.
	string s=editorData["time"];
	if(s.empty() || !(s[0]>='0' && s[0]<='9')){
		levelTime=-1;
	}else{
		levelTime=atoi(s.c_str());
	}

	s=editorData["recordings"];
	if(s.empty() || !(s[0]>='0' && s[0]<='9')){
		levelRecordings=-1;
	}else{
		levelRecordings=atoi(s.c_str());
	}
}

void LevelEditor::saveLevel(string fileName){
	//Create the output stream and check if it starts.
	std::ofstream save(fileName.c_str());
	if(!save) return;

	//The dimensions of the level.
	int maxX=0;
	int maxY=0;

	//The storageNode to put the level data in before writing it away.
	TreeStorageNode node;
	char s[64];

	//The name of the level.
	if(!levelName.empty()){
		node.attributes["name"].push_back(levelName);

		//Update the level name in the levelpack.
		levels->getLevel()->name=levelName;
	}

	//The leveltheme.
	if(!levelTheme.empty())
		node.attributes["theme"].push_back(levelTheme);

	//target time and recordings.
	{
		char c[32];
		if(levelTime>=0){
			sprintf(c,"%d",levelTime);
			node.attributes["time"].push_back(c);
		}
		if(levelRecordings>=0){
			sprintf(c,"%d",levelRecordings);
			node.attributes["recordings"].push_back(c);
		}
	}

	//The width of the level.
	maxX=LEVEL_WIDTH;
	sprintf(s,"%d",maxX);
	node.attributes["size"].push_back(s);

	//The height of the level.
	maxY=LEVEL_HEIGHT;
	sprintf(s,"%d",maxY);
	node.attributes["size"].push_back(s);

	//Loop through the gameObjects and save them.
	for(int o=0;o<(signed)levelObjects.size();o++){
		int objectType=levelObjects[o]->type;

		//Check if it's a legal gameObject type.
		if(objectType>=0 && objectType<TYPE_MAX){
			TreeStorageNode* obj1=new TreeStorageNode;
			node.subNodes.push_back(obj1);

			//It's a tile so name the node tile.
			obj1->name="tile";

			//Write away the type of the gameObject.
			sprintf(s,"%d",objectType);
			obj1->value.push_back(blockName[objectType]);

			//Get the box for the location of the gameObject.
			SDL_Rect box=levelObjects[o]->getBox(BoxType_Base);
			//Put the location in the storageNode.
			sprintf(s,"%d",box.x);
			obj1->value.push_back(s);
			sprintf(s,"%d",box.y);
			obj1->value.push_back(s);

			//Loop through the editor data and save it also.
			vector<pair<string,string> > obj;
			levelObjects[o]->getEditorData(obj);
			for(unsigned int i=0;i<obj.size();i++){
				if((!obj[i].first.empty()) && (!obj[i].second.empty())){
					obj1->attributes[obj[i].first].push_back(obj[i].second);
				}
			}
		}
	}

	//Create a POASerializer and write away the level node.
	POASerializer objSerializer;
	objSerializer.writeNode(&node,save,true,true);
}


///////////////EVENT///////////////////
void LevelEditor::handleEvents(){
	//Check if we need to quit, if so we enter the exit state.
	if(event.type==SDL_QUIT){
		setNextState(STATE_EXIT);
	}

	//If playing/testing we should the game handle the events.
	if(playMode){
		Game::handleEvents();

		//Also check if we should exit the playMode.
		if(inputMgr.isKeyDownEvent(INPUTMGR_ESCAPE)){
			//Reset the game and disable playMode.
			Game::reset(true);
			playMode=false;
			camera.x=cameraSave.x;
			camera.y=cameraSave.y;

			//NOTE: To prevent the mouse to still "be pressed" we set it to false.
			pressedLeftMouse=false;
		}
	}else{
		//Also check if we should exit the editor.
		if(inputMgr.isKeyDownEvent(INPUTMGR_ESCAPE)){
			//Before we quit ask a make sure question.
			if(msgBox(_("Are you sure you want to quit?"),MsgBoxYesNo,_("Quit prompt"))==MsgBoxYes){
				//We exit the level editor.
				if(GUIObjectRoot){
					delete GUIObjectRoot;
					GUIObjectRoot=NULL;
				}
				setNextState(STATE_LEVEL_EDIT_SELECT);

				//Play the menu music again.
				getMusicManager()->playMusic("menu");
			}
		}

		//Check if we should redirect the event to selection popup
		if(selectionPopup!=NULL){
			if(event.type==SDL_MOUSEBUTTONDOWN
				|| event.type==SDL_MOUSEBUTTONUP
				|| event.type==SDL_MOUSEMOTION)
			{
				selectionPopup->handleEvents();
				return;
			}
		}

		//Check if toolbar is clicked.
		if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button==SDL_BUTTON_LEFT && tooltip>=0){
			int t=tooltip;

			if(t<NUMBER_TOOLS){
				tool=(Tools)t;

				//Check if we should show tool box
				if(tool==ADD){
					//show the tool box
					if(toolbox==NULL){
						toolbox=new LevelEditorToolbox(this);
						toolbox->move(event.button.x,event.button.y-toolbox->height()-20);
					}
					if(!toolbox->visible){
						toolbox->visible=true;
					}
				}
			}else{
				//The selected button isn't a tool.
				//Now check which button it is.
				if(t==NUMBER_TOOLS){
					playMode=true;
					cameraSave.x=camera.x;
					cameraSave.y=camera.y;

					if(tool==CONFIGURE){
						//Also stop linking or moving.
						if(linking){
							linking=false;
							linkingTrigger=NULL;
						}

						if(moving){
							//Write the path to the moving block.
							std::map<std::string,std::string> editorData;
							char s[64], s0[64];

							sprintf(s,"%d",int(movingBlocks[movingBlock].size()));
							editorData["MovingPosCount"]=s;
							//Loop through the positions.
							for(unsigned int o=0;o<movingBlocks[movingBlock].size();o++){
								sprintf(s0+1,"%d",o);
								sprintf(s,"%d",movingBlocks[movingBlock][o].x);
								s0[0]='x';
								editorData[s0]=s;
								sprintf(s,"%d",movingBlocks[movingBlock][o].y);
								s0[0]='y';
								editorData[s0]=s;
								sprintf(s,"%d",movingBlocks[movingBlock][o].time);
								s0[0]='t';
								editorData[s0]=s;
							}
							movingBlock->setEditorData(editorData);

							moving=false;
							movingBlock=NULL;
						}
					}
				}
				if(t==NUMBER_TOOLS+2){
					//Open up level settings dialog
					levelSettings();
				}
				if(t==NUMBER_TOOLS+4){
					//Go back to the level selection screen of Level Editor
					setNextState(STATE_LEVEL_EDIT_SELECT);
					//Change the music back to menu music.
					getMusicManager()->playMusic("menu");
				}
				if(t==NUMBER_TOOLS+3){
					//Save current level
					saveLevel(levelFile);
					//And give feedback to the user.
					if(levelName.empty())
						msgBox(tfm::format(_("Level \"%s\" saved"),fileNameFromPath(levelFile)),MsgBoxOKOnly,_("Saved"));
					else
						msgBox(tfm::format(_("Level \"%s\" saved"),levelName),MsgBoxOKOnly,_("Saved"));
				}
			}

			return;
		}

		//check if shift is pressed.
		if(inputMgr.isKeyDownEvent(INPUTMGR_SHIFT)){
			pressedShift=true;
		}
		if(inputMgr.isKeyUpEvent(INPUTMGR_SHIFT)){
			pressedShift=false;
		}

		//Check if delete is pressed.
		if(event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_DELETE){
			if(!selection.empty()){
				//Loop through the selected game objects.
				 while(!selection.empty()){
					//Remove the objects in the selection.
					removeObject(selection[0]);
				}

				//And clear the selection vector.
				selection.clear();
				dragCenter=NULL;
				selectionDrag=false;
			}
		}

		//Check for copy (Ctrl+c) or cut (Ctrl+x).
		if(event.type==SDL_KEYDOWN && (event.key.keysym.sym==SDLK_c || event.key.keysym.sym==SDLK_x) && (event.key.keysym.mod & KMOD_CTRL)){
			//Clear the current clipboard.
			clipboard.clear();

			//Check if the selection isn't empty.
			if(!selection.empty()){
				//Loop through the selection to find the left-top block.
				int x=selection[0]->getBox().x;
				int y=selection[0]->getBox().y;
				for(unsigned int o=1; o<selection.size(); o++){
					if(selection[o]->getBox().x<x || selection[o]->getBox().y<y){
						x=selection[o]->getBox().x;
						y=selection[o]->getBox().y;
					}
				}

				//Loop through the selection for the actual copying.
				for(unsigned int o=0; o<selection.size(); o++){
					//Get the editor data of the object.
					vector<pair<string,string> > obj;
					selection[o]->getEditorData(obj);

					//Loop through the editor data and convert it.
					map<string,string> objMap;
					for(unsigned int i=0;i<obj.size();i++){
						objMap[obj[i].first]=obj[i].second;
					}
					//Add some entries to the map.
					char s[64];
					sprintf(s,"%d",selection[o]->getBox().x-x);
					objMap["x"]=s;
					sprintf(s,"%d",selection[o]->getBox().y-y);
					objMap["y"]=s;
					sprintf(s,"%d",selection[o]->type);
					objMap["type"]=s;

					//Overwrite the id to prevent triggers, portals, buttons, movingblocks, etc. from malfunctioning.
					//We give an empty string as id, which is invalid and thus suitable.
					objMap["id"]="";
					//Do the same for destination if the type is portal.
					if(selection[o]->type==TYPE_PORTAL){
						objMap["destination"]="";
					}

					//And add the map to the clipboard vector.
					clipboard.push_back(objMap);

					if(event.key.keysym.sym==SDLK_x){
						//Cutting means deleting the game object.
						removeObject(selection[o]);
						o--;
					}
				}

				//Only clear the selection when Ctrl+x;
				if(event.key.keysym.sym==SDLK_x){
					selection.clear();
					dragCenter=NULL;
					selectionDrag=false;
				}
			}
		}

		//Check for paste (Ctrl+v).
		if(event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_v && (event.key.keysym.mod & KMOD_CTRL)){
			//First make sure that the clipboard isn't empty.
			if(!clipboard.empty()){
				//Clear the current selection.
				selection.clear();

				//Get the current mouse location.
				int x,y;
				SDL_GetMouseState(&x,&y);
				x+=camera.x;
				y+=camera.y;

				//Apply snap to grid.
				if(!pressedShift){
					snapToGrid(&x,&y);
				}else{
					x-=25;
					y-=25;
				}

				//Integers containing the diff of the x that occurs when placing a block outside the level size on the top or left.
				//We use it to compensate the corrupted x and y locations of the other clipboard blocks.
				int diffX=0;
				int diffY=0;


				//Loop through the clipboard.
				for(unsigned int o=0;o<clipboard.size();o++){
					Block* block=new Block(0,0,atoi(clipboard[o]["type"].c_str()),this);
					block->setPosition(atoi(clipboard[o]["x"].c_str())+x+diffX,atoi(clipboard[o]["y"].c_str())+y+diffY);
					block->setEditorData(clipboard[o]);

					if(block->getBox().x<0){
						//A block on the left side of the level, meaning we need to shift everything.
						//First calc the difference.
						diffX+=(0-(block->getBox().x));
					}
					if(block->getBox().y<0){
						//A block on the left side of the level, meaning we need to shift everything.
						//First calc the difference.
						diffY+=(0-(block->getBox().y));
					}

					//And add the object using the addObject method.
					addObject(block);

					//Also add the block to the selection.
					selection.push_back(block);
				}
			}
		}

		//Check if the return button is pressed.
		//If so run the configure tool.
		if(event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_RETURN){
			//Get the current mouse location.
			int x,y;
			SDL_GetMouseState(&x,&y);
			//Create the rectangle.
			SDL_Rect mouse={x+camera.x,y+camera.y,0,0};

			//Loop through the selected game objects.
			for(unsigned int o=0; o<levelObjects.size(); o++){
				//Check for collision.
				if(checkCollision(mouse,levelObjects[o]->getBox())){
					tool=CONFIGURE;
					//Invoke the onEnterObject.
					onEnterObject(levelObjects[o]);
					//Break out of the for loop.
					break;
				}
			}
		}

		//Check for the arrow keys, used for moving the camera when playMode=false.
		cameraXvel=0;
		cameraYvel=0;
		if(inputMgr.isKeyDown(INPUTMGR_RIGHT)){
			if(pressedShift){
				cameraXvel+=10;
			}else{
				cameraXvel+=5;
			}
		}
		if(inputMgr.isKeyDown(INPUTMGR_LEFT)){
			if(pressedShift){
				cameraXvel-=10;
			}else{
				cameraXvel-=5;
			}
		}
		if(inputMgr.isKeyDown(INPUTMGR_UP)){
			if(pressedShift){
				cameraYvel-=10;
			}else{
				cameraYvel-=5;
			}
		}
		if(inputMgr.isKeyDown(INPUTMGR_DOWN)){
			if(pressedShift){
				cameraYvel+=10;
			}else{
				cameraYvel+=5;
			}
		}

		//Check if the left mouse button is pressed/holded.
		if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button==SDL_BUTTON_LEFT){
			pressedLeftMouse=true;
		}
		if(event.type==SDL_MOUSEBUTTONUP && event.button.button==SDL_BUTTON_LEFT){
			pressedLeftMouse=false;

			//We also need to check if dragging is true.
			if(dragging){
				//Set dragging false and call the onDrop event.
				dragging=false;
				int x,y;
				SDL_GetMouseState(&x,&y);
				//We call the drop event.
				onDrop(x+camera.x,y+camera.y);
			}
		}

		//Check if the mouse is dragging.
		if(pressedLeftMouse && event.type==SDL_MOUSEMOTION){
			if(abs(event.motion.xrel)+abs(event.motion.yrel)>=2){
				//Check if this is the start of the dragging.
				if(!dragging){
					//The mouse is moved enough so let's set dragging true.
					dragging=true;
					//Get the current mouse location.
					int x,y;
					SDL_GetMouseState(&x,&y);
					//We call the dragStart event.
					onDragStart(x+camera.x,y+camera.y);
				}else{
					//Dragging was already true meaning we call onDrag() instead of onDragStart().
					onDrag(event.motion.xrel,event.motion.yrel);
				}
			}
		}

		//Get the current mouse location.
		int x,y;
		SDL_GetMouseState(&x,&y);
		//Create the rectangle.
		SDL_Rect mouse={x,y,0,0};

		//Check if mouse is in the tool box
		bool mouseInToolbox=(toolbox!=NULL && !playMode && tool==ADD && toolbox->visible
			&& (toolbox->dragging || checkCollision(mouse,toolbox->getRect())));

		//Check if we scroll up, meaning the currentType++;
		if((event.type==SDL_MOUSEBUTTONDOWN && event.button.button==SDL_BUTTON_WHEELUP) || inputMgr.isKeyDownEvent(INPUTMGR_NEXT)){
			switch(tool){
			case ADD:
				//Only change the current type when using the add tool.
				if(mouseInToolbox){
					if((--toolbox->startRow)<0){
						toolbox->startRow=toolbox->maxRow-1;
					}
				}else{
					currentType++;
					if(currentType>=EDITOR_ORDER_MAX){
						currentType=0;
					}
				}
				break;
			case CONFIGURE:
				//When in configure mode.
				movingSpeed++;
				//The movingspeed is capped at 100.
				if(movingSpeed>100){
					movingSpeed=100;
				}
				break;
			default:
				//When in other mode, just scrolling the map
				if(pressedShift) camera.x-=200;
				else camera.y-=200;
				break;
			}
		}
		//Check if we scroll down, meaning the currentType--;
		if((event.type==SDL_MOUSEBUTTONDOWN && event.button.button==SDL_BUTTON_WHEELDOWN) || inputMgr.isKeyDownEvent(INPUTMGR_PREVIOUS)){
			switch(tool){
			case ADD:
				//Only change the current type when using the add tool.
				if(mouseInToolbox){
					if((++toolbox->startRow)>=toolbox->maxRow){
						toolbox->startRow=0;
					}
				}else{
					currentType--;
					if(currentType<0){
						currentType=EDITOR_ORDER_MAX-1;
					}
				}
				break;
			case CONFIGURE:
				//When in configure mode.
				movingSpeed--;
				if(movingSpeed<=0){
					movingSpeed=1;
				}
				break;
			default:
				//When in other mode, just scrolling the map
				if(pressedShift) camera.x+=200;
				else camera.y+=200;
				break;
			}
		}

		//Check if we should enter playMode.
		if(event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_p){
			playMode=true;
			cameraSave.x=camera.x;
			cameraSave.y=camera.y;
		}
		//Check for tool shortcuts.
		if(event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_a){
			tool=ADD;
		}
		if(event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_s){
			tool=SELECT;
		}
		if(event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_d){
			//We clear the selection since that can't be used in the deletion tool.
			selection.clear();
			tool=REMOVE;
		}
		if(event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_w){
			tool=CONFIGURE;
		}

		//Check for certain events.

		//First make sure the mouse isn't above the toolbar.
		if(checkCollision(mouse,toolbarRect)==false){
			//Check if mouse is in the tool box
			if(mouseInToolbox){
				toolbox->handleEvents();
			}else{
				//We didn't hit the toolbar so convert the mouse location to ingame location.
				mouse.x+=camera.x;
				mouse.y+=camera.y;

				//Boolean if there's a click event fired.
				bool clickEvent=false;
				//Check if a mouse button is pressed.
				if(event.type==SDL_MOUSEBUTTONUP){
					std::vector<GameObject*> clickObjects;

					//Loop through the objects to check collision.
					for(unsigned int o=0; o<levelObjects.size(); o++){
						if(checkCollision(levelObjects[o]->getBox(),mouse)==true){
							clickObjects.push_back(levelObjects[o]);
						}
					}

					if(clickObjects.size()==1){
						//We have collision meaning that the mouse is above an object.
						std::vector<GameObject*>::iterator it;
						it=find(selection.begin(),selection.end(),clickObjects[0]);

						//Set event true since there's a click event.
						clickEvent=true;

						//Check if the clicked object is in the selection or not.
						bool isSelected=(it!=selection.end());
						if(event.button.button==SDL_BUTTON_LEFT){
							onClickObject(clickObjects[0],isSelected);
						}else if(event.button.button==SDL_BUTTON_RIGHT){
							onRightClickObject(clickObjects[0],isSelected);
						}
					}else if(clickObjects.size()>1){
						//There are more than one object under the mouse
						clickEvent=true;

						SDL_Rect r=clickObjects[0]->getBox();

						if(selectionPopup!=NULL) delete selectionPopup;
						selectionPopup=new LevelEditorSelectionPopup(this,clickObjects,
							r.x-camera.x,r.y-camera.y);
					}
				}

				//If event is false then we clicked on void.
				if(!clickEvent){
					if(event.type==SDL_MOUSEBUTTONUP){
						if(event.button.button==SDL_BUTTON_LEFT){
							//Left mouse button on void.
							onClickVoid(mouse.x,mouse.y);
						}else if(event.button.button==SDL_BUTTON_RIGHT && tool==CONFIGURE){
							//Stop linking.
							linking=false;
							linkingTrigger=NULL;

							//Write the path to the moving block.
							if(moving){
								std::map<std::string,std::string> editorData;
								char s[64], s0[64];

								sprintf(s,"%d",int(movingBlocks[movingBlock].size()));
								editorData["MovingPosCount"]=s;
								//Loop through the positions.
								for(unsigned int o=0;o<movingBlocks[movingBlock].size();o++){
									sprintf(s0+1,"%d",o);
									sprintf(s,"%d",movingBlocks[movingBlock][o].x);
									s0[0]='x';
									editorData[s0]=s;
									sprintf(s,"%d",movingBlocks[movingBlock][o].y);
									s0[0]='y';
									editorData[s0]=s;
									sprintf(s,"%d",movingBlocks[movingBlock][o].time);
									s0[0]='t';
									editorData[s0]=s;
								}
								movingBlock->setEditorData(editorData);

								//Stop moving.
								moving=false;
								movingBlock=NULL;
							}
						}
					}
				}
			}
		}

		//Check for backspace when moving to remove a movingposition.
		if(moving && event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_BACKSPACE){
			if(movingBlocks[movingBlock].size()>0){
				movingBlocks[movingBlock].pop_back();
			}
		}

		//Check for the tab key, level settings.
		if(inputMgr.isKeyDownEvent(INPUTMGR_TAB)){
			//Show the levelSettings.
			levelSettings();
		}

		//Check if we should a new level. (Ctrl+n)
		if(event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_n && (event.key.keysym.mod & KMOD_CTRL)){
			reset();
			//NOTE: We don't have anything to load from so we create an empty TreeStorageNode.
			Game::loadLevelFromNode(new TreeStorageNode,"");

			//Hide selection popup (if any)
			if(selectionPopup!=NULL){
				delete selectionPopup;
				selectionPopup=NULL;
			}
		}
		//Check if we should save the level (Ctrl+s) or save levelpack (Ctrl+Shift+s).
		if(event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_s && (event.key.keysym.mod & KMOD_CTRL)){
			saveLevel(levelFile);
			//And give feedback to the user.
			if(levelName.empty())
				msgBox(tfm::format(_("Level \"%s\" saved"),fileNameFromPath(levelFile)),MsgBoxOKOnly,_("Saved"));
			else
				msgBox(tfm::format(_("Level \"%s\" saved"),levelName),MsgBoxOKOnly,_("Saved"));
		}
	}
}

void LevelEditor::levelSettings(){
	//It isn't so open a popup asking for a name.
	//First delete any existing gui.
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}

	GUIObject* root=new GUIObject((SCREEN_WIDTH-600)/2,(SCREEN_HEIGHT-300)/2,600,300,GUIObjectFrame,_("Level settings"));
	GUIObject* obj;

	//NOTE: We reuse the objectProperty and secondProperty.
	obj=new GUIObject(40,50,240,36,GUIObjectLabel,_("Name:"));
	root->addChild(obj);
	obj=new GUIObject(140,50,410,36,GUIObjectTextBox,levelName.c_str());
	objectProperty=obj;
	root->addChild(obj);

	obj=new GUIObject(40,100,240,36,GUIObjectLabel,_("Theme:"));
	root->addChild(obj);
	obj=new GUIObject(140,100,410,36,GUIObjectTextBox,levelTheme.c_str());
	secondObjectProperty=obj;
	root->addChild(obj);

	//target time and recordings.
	{
		char c[32];

		if(levelTime>=0){
			sprintf(c,"%-.2f",levelTime/40.0f);
		}else{
			c[0]='\0';
		}
		obj=new GUIObject(40,150,240,36,GUIObjectLabel,_("Target time (s):"));
		root->addChild(obj);
		obj=new GUIObject(290,150,260,36,GUIObjectTextBox,c);
		levelTimeProperty=obj;
		root->addChild(obj);

		if(levelRecordings>=0){
			sprintf(c,"%d",levelRecordings);
		}else{
			c[0]='\0';
		}
		obj=new GUIObject(40,200,240,36,GUIObjectLabel,_("Target recordings:"));
		root->addChild(obj);
		obj=new GUIObject(290,200,260,36,GUIObjectTextBox,c);
		levelRecordingsProperty=obj;
		root->addChild(obj);
	}


	//Ok and cancel buttons.
	obj=new GUIObject(root->width*0.3,300-44,-1,36,GUIObjectButton,_("OK"),0,true,true,GUIGravityCenter);
	obj->name="lvlSettingsOK";
	obj->eventCallback=this;
	root->addChild(obj);
	obj=new GUIObject(root->width*0.7,300-44,-1,36,GUIObjectButton,_("Cancel"),0,true,true,GUIGravityCenter);
	obj->name="lvlSettingsCancel";
	obj->eventCallback=this;
	root->addChild(obj);

	//NOTE: We don't need to store a pointer since it will auto cleanup itself.
	new GUIOverlay(root);
}

void LevelEditor::postLoad(){
	//We need to find the triggers.
	for(unsigned int o=0;o<levelObjects.size();o++){
		//Get the editor data.
		vector<pair<string,string> > objMap;
		levelObjects[o]->getEditorData(objMap);

		//Check for the highest id.
		for(unsigned int i=0;i<objMap.size();i++){
			if(objMap[i].first=="id"){
				unsigned int id=atoi(objMap[i].second.c_str());
				if(id>=currentId){
					currentId=id+1;
				}
			}
		}

		switch(levelObjects[o]->type){
			case TYPE_BUTTON:
			case TYPE_SWITCH:
			{
				//Add the object to the triggers vector.
				vector<GameObject*> linked;
				triggers[levelObjects[o]]=linked;
				//Now loop through the levelObjects in search for objects with the same id.
				for(unsigned int oo=0;oo<levelObjects.size();oo++){
					//Check if it isn't the same object but has the same id.
					if(o!=oo && (dynamic_cast<Block*>(levelObjects[o]))->id==(dynamic_cast<Block*>(levelObjects[oo]))->id){
						//Add the object to the link vector of the trigger.
						triggers[levelObjects[o]].push_back(levelObjects[oo]);
					}
				}
				break;
			}
			case TYPE_PORTAL:
			{
				//Add the object to the triggers vector.
				vector<GameObject*> linked;
				triggers[levelObjects[o]]=linked;

				//If the destination is empty we return.
				if((dynamic_cast<Block*>(levelObjects[o]))->destination.empty()){
					break;
				}

				//Now loop through the levelObjects in search for objects with the same id as destination.
				for(unsigned int oo=0;oo<levelObjects.size();oo++){
					//Check if it isn't the same object but has the same id.
					if(o!=oo && (dynamic_cast<Block*>(levelObjects[o]))->destination==(dynamic_cast<Block*>(levelObjects[oo]))->id){
						//Add the object to the link vector of the trigger.
						triggers[levelObjects[o]].push_back(levelObjects[oo]);
					}
				}
				break;
			}
			case TYPE_MOVING_BLOCK:
			case TYPE_MOVING_SHADOW_BLOCK:
			case TYPE_MOVING_SPIKES:
			{
				//Add the object to the movingBlocks vector.
				vector<MovingPosition> positions;
				movingBlocks[levelObjects[o]]=positions;

				//Get the number of entries of the editor data.
				int m=objMap.size();

				//Check if the editor data isn't empty.
				if(m>0){
					//Integer containing the positions.
					int pos=0;
					int currentPos=0;

					//Get the number of movingpositions.
					pos=atoi(objMap[1].second.c_str());

					while(currentPos<pos){
						int x=atoi(objMap[currentPos*3+4].second.c_str());
						int y=atoi(objMap[currentPos*3+5].second.c_str());
						int t=atoi(objMap[currentPos*3+6].second.c_str());

						//Create a new movingPosition.
						MovingPosition position(x,y,t);
						movingBlocks[levelObjects[o]].push_back(position);

						//Increase currentPos by one.
						currentPos++;
					}
				}

				break;
			}
			default:
			  break;
		}
	}
}

void LevelEditor::snapToGrid(int* x,int* y){
	//Check if the x location is negative.
	if(*x<0){
		*x=-((abs(*x-50)/50)*50);
	}else{
		*x=(*x/50)*50;
	}

	//Now the y location.
	if(*y<0){
		*y=-((abs(*y-50)/50)*50);
	}else{
		*y=(*y/50)*50;
	}
}

void LevelEditor::onClickObject(GameObject* obj,bool selected){
	switch(tool){
	  //NOTE: We put CONFIGURE above ADD and SELECT to use the same method of selection.
	  //Meaning there's no break at the end of CONFIGURE.
	  case CONFIGURE:
	  {
	    //Check if we are linking.
	    if(linking){
			//Check if the obj is valid to link to.
			switch(obj->type){
				case TYPE_CONVEYOR_BELT:
				case TYPE_SHADOW_CONVEYOR_BELT:
				case TYPE_MOVING_BLOCK:
				case TYPE_MOVING_SHADOW_BLOCK:
				case TYPE_MOVING_SPIKES:
				{
					//It's only valid when not linking a portal.
					if(linkingTrigger->type==TYPE_PORTAL){
						//You can't link a portal to moving blocks, etc.
						//Stop linking and return.
						linkingTrigger=NULL;
						linking=false;
						return;
					}
					break;
				}
				case TYPE_PORTAL:
				{
					//Make sure that the linkingTrigger is also a portal.
					if(linkingTrigger->type!=TYPE_PORTAL){
						//The linkingTrigger isn't a portal so stop linking and return.
						linkingTrigger=NULL;
						linking=false;
						return;
					}
					break;
				}
				default:
					//It isn't valid so stop linking and return.
					linkingTrigger=NULL;
					linking=false;
					return;
				break;
			}

			//Check if the linkingTrigger can handle multiple or only one link.
			switch(linkingTrigger->type){
				case TYPE_PORTAL:
				{
					//Portals can only link to one so remove all existing links.
					triggers[linkingTrigger].clear();
					triggers[linkingTrigger].push_back(obj);
					break;
				}
				default:
				{
					//The most can handle multiple links.
					triggers[linkingTrigger].push_back(obj);
					break;
				}
			}

			//Check if it's a portal.
			if(linkingTrigger->type==TYPE_PORTAL){
				//Portals need to get the id of the other instead of give it's own id.
				vector<pair<string,string> > objMap;
				obj->getEditorData(objMap);
				int m=objMap.size();
				if(m>0){
					std::map<std::string,std::string> editorData;
					char s[64];
					sprintf(s,"%d",atoi(objMap[0].second.c_str()));
					editorData["destination"]=s;
					linkingTrigger->setEditorData(editorData);
				}
			}else{
				//Give the object the same id as the trigger.
				vector<pair<string,string> > objMap;
				linkingTrigger->getEditorData(objMap);
				int m=objMap.size();
				if(m>0){
					std::map<std::string,std::string> editorData;
					char s[64];
					sprintf(s,"%d",atoi(objMap[0].second.c_str()));
					editorData["id"]=s;
					obj->setEditorData(editorData);
				}
			}


			//We return to prevent configuring stuff like conveyor belts, etc...
			linking=false;
			linkingTrigger=NULL;
			return;
	    }

	    //If we're moving add a movingposition.
	    if(moving){
			//Get the current mouse location.
			int x,y;
			SDL_GetMouseState(&x,&y);
			x+=camera.x;
			y+=camera.y;

			//Apply snap to grid.
			if(!pressedShift){
				snapToGrid(&x,&y);
			}else{
				x-=25;
				y-=25;
			}

			x-=movingBlock->getBox().x;
			y-=movingBlock->getBox().y;

			//Calculate the length.
			//First get the delta x and y.
			int dx,dy;
			if(movingBlocks[movingBlock].empty()){
				dx=x;
				dy=y;
			}else{
				dx=x-movingBlocks[movingBlock].back().x;
				dy=y-movingBlocks[movingBlock].back().y;
			}

			double length=sqrt(double(dx*dx+dy*dy));
			movingBlocks[movingBlock].push_back(MovingPosition(x,y,(int)(length*(10/(double)movingSpeed))));
			return;
	    }
	    
	    //Now handle it as if the user pressed enter (show block properties dialog).
	    onEnterObject(obj);
	  }
	  case SELECT:
	  case ADD:
	  {
		//Check if object is already selected.
		if(!selected){
			//First check if shift is pressed or not.
			if(!pressedShift){
				//Clear the selection.
				selection.clear();
			}

			//Add the object to the selection.
			selection.push_back(obj);
		}
	    break;
	  }
	  case REMOVE:
	  {
	    //Remove the object.
	    removeObject(obj);
	    break;
	  }
	  default:
	    break;
	}
}

void LevelEditor::onRightClickObject(GameObject* obj,bool selected){
	switch(tool){
	  case CONFIGURE:
	  {
		//Make sure we aren't doing anything special.
		if(moving || linking)
			break;

		//Check if it's a trigger.
		if(obj->type==TYPE_PORTAL || obj->type==TYPE_BUTTON || obj->type==TYPE_SWITCH){
			//Set linking true.
			linking=true;
			linkingTrigger=obj;
		}

		//Check if it's a moving block.
		if(obj->type==TYPE_MOVING_BLOCK || obj->type==TYPE_MOVING_SHADOW_BLOCK || obj->type==TYPE_MOVING_SPIKES){
			//Set moving true.
			moving=true;
			movingBlock=obj;
		}
		break;
	  }
	  case SELECT:
	  case ADD:
	  {
		//We deselect the object if it's selected.
		if(selected){
			std::vector<GameObject*>::iterator it;
			it=find(selection.begin(),selection.end(),obj);

			//Remove the object from selection.
			if(it!=selection.end()){
				selection.erase(it);
			}
		}else{
			//It wasn't a selected object so switch to configure mode.
			//Check if it's the right type of object.
			if(obj->type==TYPE_MOVING_BLOCK || obj->type==TYPE_MOVING_SHADOW_BLOCK || obj->type==TYPE_MOVING_SPIKES ||
				obj->type==TYPE_PORTAL || obj->type==TYPE_BUTTON || obj->type==TYPE_SWITCH){
				tool=CONFIGURE;
				onRightClickObject(obj,selected);
			}

		}
		break;
	  }
	  default:
	    break;
	}
}

void LevelEditor::onClickVoid(int x,int y){
	switch(tool){
	  case SELECT:
	  {
	    //We need to clear the selection.
	    selection.clear();
	    break;
	  }
	  case ADD:
	  {
	      //We need to clear the selection.
	      selection.clear();

	      //Now place an object.
	      //Apply snap to grid.
	      if(!pressedShift){
			snapToGrid(&x,&y);
	      }else{
			x-=25;
			y-=25;
	      }
	      addObject(new Block(x,y,editorTileOrder[currentType],this));
	      break;
	  }
	  case CONFIGURE:
	  {
	      //We need to clear the selection.
	      selection.clear();

	      //If we're linking we should stop, user abort.
	      if(linking){
			linking=false;
			linkingTrigger=NULL;
			//And return.
			return;
	      }

	      //If we're moving we should add a point.
	      if(moving){
			//Apply snap to grid.
			if(!pressedShift){
				snapToGrid(&x,&y);
			}else{
				x-=25;
				y-=25;
			}

			x-=movingBlock->getBox().x;
			y-=movingBlock->getBox().y;

			//Calculate the length.
			//First get the delta x and y.
			int dx,dy;
			if(movingBlocks[movingBlock].empty()){
				dx=x;
				dy=y;
			}else{
				dx=x-movingBlocks[movingBlock].back().x;
				dy=y-movingBlocks[movingBlock].back().y;
			}

			double length=sqrt(double(dx*dx+dy*dy));
			movingBlocks[movingBlock].push_back(MovingPosition(x,y,(int)(length*(10/(double)movingSpeed))));

			//And return.
			return;
	      }
	      break;
	  }
	  default:
	    break;
	}
}

void LevelEditor::onDragStart(int x,int y){
	switch(tool){
	  case SELECT:
	  case ADD:
	  case CONFIGURE:
	  {
	    //We can drag the selection so check if the selection isn't empty.
	    if(!selection.empty()){
		//The selection isn't empty so search the dragCenter.
		//Create a mouse rectangle.
		SDL_Rect mouse={x,y,0,0};

		//Loop through the objects to check collision.
		for(unsigned int o=0; o<selection.size(); o++){
			if(checkCollision(selection[o]->getBox(),mouse)==true){
				//We have collision so set the dragCenter.
				dragCenter=selection[o];
				selectionDrag=true;
			}
		}
	    }
	    break;
	  }
	  default:
	    break;
	}
}

void LevelEditor::onDrag(int dx,int dy){
	switch(tool){
	  case REMOVE:
	  {
		//No matter what we delete the item the mouse is above.
		//Get the current mouse location.
		int x,y;
		SDL_GetMouseState(&x,&y);
		//Create the rectangle.
		SDL_Rect mouse={x+camera.x,y+camera.y,0,0};

		//Loop through the objects to check collision.
		for(unsigned int o=0; o<levelObjects.size(); o++){
			if(checkCollision(levelObjects[o]->getBox(),mouse)==true){
				//Remove the object.
				removeObject(levelObjects[o]);
			}
		}
	    break;
	  }
	  default:
	    break;
	}
}

void LevelEditor::onDrop(int x,int y){
	switch(tool){
	  case SELECT:
	  case ADD:
	  case CONFIGURE:
	  {
	      //Check if the drag center isn't null.
	      if(dragCenter==NULL) return;
	      //The location of the dragCenter.
	      SDL_Rect r=dragCenter->getBox();
	      //Apply snap to grid.
	      if(!pressedShift){
			snapToGrid(&x,&y);
	      }else{
			x-=25;
			y-=25;
	      }

	      //Loop through the selection.
	      for(unsigned int o=0; o<selection.size(); o++){
			SDL_Rect r1=selection[o]->getBox();
			//We need to place the object at his drop place.
			moveObject(selection[o],(r1.x-r.x)+x,(r1.y-r.y)+y);
	      }

	      //Make sure the dragCenter is null and set selectionDrag false.
	      dragCenter=NULL;
	      selectionDrag=false;
	      break;
	  }
	  default:
	    break;
	}
}

void LevelEditor::onCameraMove(int dx,int dy){
	switch(tool){
	  case REMOVE:
	  {
		//Only delete when the left mouse button is pressed.
		if(pressedLeftMouse){
			//Get the current mouse location.
			int x,y;
			SDL_GetMouseState(&x,&y);
			//Create the rectangle.
			SDL_Rect mouse={x+camera.x,y+camera.y,0,0};

			//Loop through the objects to check collision.
			for(unsigned int o=0; o<levelObjects.size(); o++){
				if(checkCollision(levelObjects[o]->getBox(),mouse)==true){
					//Remove the object.
					removeObject(levelObjects[o]);
				}
			}
		}
	    break;
	  }
	  default:
	    break;
	}
}

void LevelEditor::onEnterObject(GameObject* obj){
	switch(tool){
	  case CONFIGURE:
	  {
	    //Check if the type is an moving block.
	    if(obj->type==TYPE_MOVING_BLOCK || obj->type==TYPE_MOVING_SHADOW_BLOCK || obj->type==TYPE_MOVING_SPIKES){
			//Open a message popup.
			//First delete any existing gui.
			if(GUIObjectRoot){
				delete GUIObjectRoot;
				GUIObjectRoot=NULL;
			}

			//Get the properties.
			vector<pair<string,string> > objMap;
			obj->getEditorData(objMap);
			int m=objMap.size();
			if(m>0){
				//Set the object we configure.
				configuredObject=obj;

				//Check if the moving block has a path..
				string s1;
				bool path=false;
				if(!movingBlocks[obj].empty()){
					s1=_("Defined");
					path=true;
				}else{
					s1=_("None");
				}

				//Now create the GUI.
				string s;
				switch(obj->type){
				  case TYPE_MOVING_BLOCK:
					s=_("Moving block");
				    break;
				  case TYPE_MOVING_SHADOW_BLOCK:
					s=_("Moving shadow block");
				    break;
				  case TYPE_MOVING_SPIKES:
					s=_("Moving spikes");
				    break;

				}
				GUIObject* root=new GUIObject((SCREEN_WIDTH-600)/2,(SCREEN_HEIGHT-200)/2,600,200,GUIObjectFrame,s.c_str());
				GUIObject* obj;

				obj=new GUIObject(70,50,280,36,GUIObjectCheckBox,_("Enabled"),(objMap[2].second!="1"));
				obj->name="cfgMovingBlockEnabled";
				obj->eventCallback=this;
				objectProperty=obj;
				root->addChild(obj);

				obj=new GUIObject(70,80,280,36,GUIObjectCheckBox,_("Loop"),(objMap[3].second!="0"));
				obj->name="cfgMovingBlockLoop";
				obj->eventCallback=this;
				secondObjectProperty=obj;
				root->addChild(obj);

				obj=new GUIObject(70,110,280,36,GUIObjectLabel,_("Path"));
				root->addChild(obj);
				GUIObject* label=new GUIObject(330,110,-1,36,GUIObjectLabel,s1.c_str());
				root->addChild(label);
				label->render(0,0,false);

				if(path){
					obj=new GUIObject(label->left+label->width,110,36,36,GUIObjectButton,"x");
					obj->name="cfgMovingBlockClrPath";
					obj->eventCallback=this;
					root->addChild(obj);
				}else{
					//NOTE: The '+' is translated 5 pixels down to align with the 'x'.
					obj=new GUIObject(label->left+label->width,115,36,36,GUIObjectButton,"+");
					obj->name="cfgMovingBlockMakePath";
					obj->eventCallback=this;
					root->addChild(obj);
				}

				obj=new GUIObject(root->width*0.3,200-44,-1,36,GUIObjectButton,_("OK"),0,true,true,GUIGravityCenter);
				obj->name="cfgMovingBlockOK";
				obj->eventCallback=this;
				root->addChild(obj);
				obj=new GUIObject(root->width*0.7,200-44,-1,36,GUIObjectButton,_("Cancel"),0,true,true,GUIGravityCenter);
				obj->name="cfgCancel";
				obj->eventCallback=this;
				root->addChild(obj);

				//Create the GUI overlay.
				//NOTE: We don't need to store a pointer since it will auto cleanup itself.
				new GUIOverlay(root);
			}
	    }

	    //Check which type of object it is.
	    if(obj->type==TYPE_NOTIFICATION_BLOCK){
			//Open a message popup.
			//First delete any existing gui.
			if(GUIObjectRoot){
				delete GUIObjectRoot;
				GUIObjectRoot=NULL;
			}

			//Get the properties.
			vector<pair<string,string> > objMap;
			obj->getEditorData(objMap);
			int m=objMap.size();
			if(m>0){
				//Set the object we configure.
				configuredObject=obj;

				//Now create the GUI.
				GUIObject* root=new GUIObject((SCREEN_WIDTH-600)/2,(SCREEN_HEIGHT-250)/2,600,250,GUIObjectFrame,_("Notification block"));
				GUIObject* obj;

				obj=new GUIObject(40,50,240,36,GUIObjectLabel,_("Enter message here:"));
				root->addChild(obj);
				obj=new GUITextArea(50,90,500,100);
				string tmp=objMap[1].second.c_str();
				//Change \n with the characters '\n'.
				while(tmp.find("\\n")!=string::npos){
					tmp=tmp.replace(tmp.find("\\n"),2,"\n");
				}
				obj->caption=tmp.c_str();
				//Set the textField.
				objectProperty=obj;
				root->addChild(obj);

				obj=new GUIObject(root->width*0.3,250-44,-1,36,GUIObjectButton,_("OK"),0,true,true,GUIGravityCenter);
				obj->name="cfgNotificationBlockOK";
				obj->eventCallback=this;
				root->addChild(obj);
				obj=new GUIObject(root->width*0.7,250-44,-1,36,GUIObjectButton,_("Cancel"),0,true,true,GUIGravityCenter);
				obj->name="cfgCancel";
				obj->eventCallback=this;
				root->addChild(obj);

				//Create the GUI overlay.
				//NOTE: We don't need to store a pointer since it will auto cleanup itself.
				new GUIOverlay(root);
			}
	    }
	    if(obj->type==TYPE_CONVEYOR_BELT || obj->type==TYPE_SHADOW_CONVEYOR_BELT){
			//Open a message popup.
			//First delete any existing gui.
			if(GUIObjectRoot){
				delete GUIObjectRoot;
				GUIObjectRoot=NULL;
			}

			//Get the properties and check if
			vector<pair<string,string> > objMap;
			obj->getEditorData(objMap);
			int m=objMap.size();
			if(m>0){
				//Set the object we configure.
				configuredObject=obj;

				//Now create the GUI.
				string s;
				if(obj->type==TYPE_CONVEYOR_BELT){
					s=_("Shadow Conveyor belt");
				}else{
				  	s=_("Conveyor belt");
				}

				GUIObject* root=new GUIObject((SCREEN_WIDTH-600)/2,(SCREEN_HEIGHT-200)/2,600,200,GUIObjectFrame,s.c_str());
				GUIObject* obj;

				obj=new GUIObject(40,60,220,36,GUIObjectCheckBox,_("Enabled"),(objMap[1].second!="1"));
				obj->name="cfgConveyorBlockEnabled";
				obj->eventCallback=this;
				objectProperty=obj;
				root->addChild(obj);

				obj=new GUIObject(40,100,240,36,GUIObjectLabel,_("Enter speed here:"));
				root->addChild(obj);
				obj=new GUIObject(240,100,320,36,GUIObjectTextBox,objMap[2].second.c_str());
				//Set the textField.
				secondObjectProperty=obj;
				root->addChild(obj);


				obj=new GUIObject(root->width*0.3,200-44,-1,36,GUIObjectButton,_("OK"),0,true,true,GUIGravityCenter);
				obj->name="cfgConveyorBlockOK";
				obj->eventCallback=this;
				root->addChild(obj);
				obj=new GUIObject(root->width*0.7,200-44,-1,36,GUIObjectButton,_("Cancel"),0,true,true,GUIGravityCenter);
				obj->name="cfgCancel";
				obj->eventCallback=this;
				root->addChild(obj);

				//Create the GUI overlay.
				//NOTE: We don't need to store a pointer since it will auto cleanup itself.
				new GUIOverlay(root);
			}
	    }

	    if(obj->type==TYPE_PORTAL){
			//Open a message popup.
			//First delete any existing gui.
			if(GUIObjectRoot){
				delete GUIObjectRoot;
				GUIObjectRoot=NULL;
			}

			//Get the properties and check if
			vector<pair<string,string> > objMap;
			obj->getEditorData(objMap);
			int m=objMap.size();
			if(m>0){
				//Set the object we configure.
				configuredObject=obj;

				//Check how many targets there are for this object.
				string s1;
				bool target=false;
				if(!triggers[obj].empty()){
					s1=_("Defined");
					target=true;
				}else{
					s1=_("None");
				}

				//Now create the GUI.
				GUIObject* root=new GUIObject((SCREEN_WIDTH-600)/2,(SCREEN_HEIGHT-200)/2,600,200,GUIObjectFrame,_("Portal"));
				GUIObject* obj;

				obj=new GUIObject(70,60,310,36,GUIObjectCheckBox,_("Activate on touch"),(objMap[1].second=="1"));
				obj->name="cfgPortalAutomatic";
				obj->eventCallback=this;
				objectProperty=obj;
				root->addChild(obj);

				obj=new GUIObject(70,100,240,36,GUIObjectLabel,_("Targets:"));
				root->addChild(obj);

				GUIObject* label=new GUIObject(360,100,-1,36,GUIObjectLabel,s1.c_str());
				root->addChild(label);
				label->render(0,0,false);

				//Check if there are targets defined.
				if(target){
					obj=new GUIObject(label->left+label->width,100,36,36,GUIObjectButton,"x");
					obj->name="cfgPortalUnlink";
					obj->eventCallback=this;
					root->addChild(obj);
				}else{
					//NOTE: The '+' is translated 5 pixels down to align with the 'x'.
					obj=new GUIObject(label->left+label->width,105,36,36,GUIObjectButton,"+");
					obj->name="cfgPortalLink";
					obj->eventCallback=this;
					root->addChild(obj);
				}

				obj=new GUIObject(root->width*0.3,200-44,-1,36,GUIObjectButton,_("OK"),0,true,true,GUIGravityCenter);
				obj->name="cfgPortalOK";
				obj->eventCallback=this;
				root->addChild(obj);
				obj=new GUIObject(root->width*0.7,200-44,-1,36,GUIObjectButton,_("Cancel"),0,true,true,GUIGravityCenter);
				obj->name="cfgCancel";
				obj->eventCallback=this;
				root->addChild(obj);

				//Create the GUI overlay.
				//NOTE: We don't need to store a pointer since it will auto cleanup itself.
				new GUIOverlay(root);
			}
	    }

	    if(obj->type==TYPE_BUTTON || obj->type==TYPE_SWITCH){
			//Open a message popup.
			//First delete any existing gui.
			if(GUIObjectRoot){
				delete GUIObjectRoot;
				GUIObjectRoot=NULL;
			}

			//Get the properties and check if
			vector<pair<string,string> > objMap;
			obj->getEditorData(objMap);
			int m=objMap.size();
			if(m>0){
				//Set the object we configure.
				configuredObject=obj;

				//Check how many targets there are for this object.
				string s1;
				bool targets=false;
				if(!triggers[obj].empty()){
					s1=tfm::format(_("%d Defined"),(int)triggers[obj].size());
					targets=true;
				}else{
					s1=_("None");
				}

				//Now create the GUI.
				string s;
				if(obj->type==TYPE_BUTTON){
					s=_("Button");
				}else{
					s=_("Switch");
				}
				GUIObject* root=new GUIObject((SCREEN_WIDTH-600)/2,(SCREEN_HEIGHT-200)/2,600,200,GUIObjectFrame,s.c_str());
				GUIObject* obj;

				obj=new GUIObject(70,60,240,36,GUIObjectLabel,_("Behaviour:"));
				root->addChild(obj);

				obj=new GUISingleLineListBox(250,60,300,36);
				obj->name="lstBehaviour";
				vector<string> v;
				v.push_back(_("On"));
				v.push_back(_("Off"));
				v.push_back(_("Toggle"));
				(dynamic_cast<GUISingleLineListBox*>(obj))->item=v;

				//Get the current behaviour.
				if(objMap[1].second=="on"){
					obj->value=0;
				}else if(objMap[1].second=="off"){
					obj->value=1;
				}else{
					//There's no need to check for the last one, since it's also the default.
					obj->value=2;
				}
				objectProperty=obj;
				root->addChild(obj);

				obj=new GUIObject(70,100,240,36,GUIObjectLabel,_("Targets:"));
				root->addChild(obj);

				GUIObject* label=new GUIObject(250,100,-1,36,GUIObjectLabel,s1.c_str());
				root->addChild(label);
				label->render(0,0,false);

				//NOTE: The '+' is translated 5 pixels down to align with the 'x'.
				obj=new GUIObject(label->left+label->width,105,36,36,GUIObjectButton,"+");
				obj->name="cfgTriggerLink";
				obj->eventCallback=this;
				root->addChild(obj);

				//Check if there are targets defined.
				if(targets){
					obj=new GUIObject(label->left+label->width+40,100,36,36,GUIObjectButton,"x");
					obj->name="cfgTriggerUnlink";
					obj->eventCallback=this;
					root->addChild(obj);
				}


				obj=new GUIObject(root->width*0.3,200-44,-1,36,GUIObjectButton,_("OK"),0,true,true,GUIGravityCenter);
				obj->name="cfgTriggerOK";
				obj->eventCallback=this;
				root->addChild(obj);
				obj=new GUIObject(root->width*0.7,200-44,-1,36,GUIObjectButton,_("Cancel"),0,true,true,GUIGravityCenter);
				obj->name="cfgCancel";
				obj->eventCallback=this;
				root->addChild(obj);

				//Create the GUI overlay.
				//NOTE: We don't need to store a pointer since it will auto cleanup itself.
				new GUIOverlay(root);
			}
	    }
	    if(obj->type==TYPE_FRAGILE){
			//First delete any existing gui.
			if(GUIObjectRoot){
				delete GUIObjectRoot;
				GUIObjectRoot=NULL;
			}

			//Get the properties and check if it contains the state data.
			vector<pair<string,string> > objMap;
			obj->getEditorData(objMap);
			int m=objMap.size();
			if(m>0){
				//Set the object we configure.
				configuredObject=obj;

				//Create the GUI.
				GUIObject* root=new GUIObject((SCREEN_WIDTH-600)/2,(SCREEN_HEIGHT-200)/2,600,200,GUIObjectFrame,_("Fragile"));
				GUIObject* obj;

				obj=new GUIObject(70,60,240,36,GUIObjectLabel,_("State:"));
				root->addChild(obj);

				obj=new GUISingleLineListBox(250,60,300,36);
				obj->name="lstBehaviour";
				vector<string> v;
				v.push_back(_("Complete"));
				v.push_back(_("One step"));
				v.push_back(_("Two steps"));
				v.push_back(_("Gone"));
				(dynamic_cast<GUISingleLineListBox*>(obj))->item=v;

				//Get the current state.
				obj->value=atoi(objMap[1].second.c_str());
				objectProperty=obj;
				root->addChild(obj);

				obj=new GUIObject(root->width*0.3,200-44,-1,36,GUIObjectButton,_("OK"),0,true,true,GUIGravityCenter);
				obj->name="cfgFragileOK";
				obj->eventCallback=this;
				root->addChild(obj);
				obj=new GUIObject(root->width*0.7,200-44,-1,36,GUIObjectButton,_("Cancel"),0,true,true,GUIGravityCenter);
				obj->name="cfgCancel";
				obj->eventCallback=this;
				root->addChild(obj);

				//Create the GUI overlay.
				//NOTE: We don't need to store a pointer since it will auto cleanup itself.
				new GUIOverlay(root);
			}
	    }

	    break;
	  }
	  default:
	    break;
	}
}

void LevelEditor::addObject(GameObject* obj){
    //Increase totalCollectables everytime we add a new collectable
	if (obj->type==TYPE_COLLECTABLE) {
		totalCollectables++;
	}

	//If it's a player or shadow start then we need to remove the previous one.
	if(obj->type==TYPE_START_PLAYER || obj->type==TYPE_START_SHADOW){
		//Loop through the levelObjects.
		for(unsigned int o=0; o<levelObjects.size(); o++){
			//Check if the type is the same.
			if(levelObjects[o]->type==obj->type){
				removeObject(levelObjects[o]);
			}
		}
	}

	//Add it to the levelObjects.
	levelObjects.push_back(obj);

	//Check if the object is inside the level dimensions, etc.
	//Just call moveObject() to perform this.
	moveObject(obj,obj->getBox().x,obj->getBox().y);

	//GameObject type specific stuff.
	switch(obj->type){
		case TYPE_BUTTON:
		case TYPE_SWITCH:
		case TYPE_PORTAL:
		{
			//Add the object to the triggers.
			vector<GameObject*> linked;
			triggers[obj]=linked;

			//Give it it's own id.
			std::map<std::string,std::string> editorData;
			char s[64];
			sprintf(s,"%d",currentId);
			currentId++;
			editorData["id"]=s;
			obj->setEditorData(editorData);
			break;
		}
		case TYPE_MOVING_BLOCK:
		case TYPE_MOVING_SHADOW_BLOCK:
		case TYPE_MOVING_SPIKES:
		{
			//Add the object to the moving blocks.
			vector<MovingPosition> positions;
			movingBlocks[obj]=positions;

			//Get the editor data.
			vector<pair<string,string> > objMap;
			obj->getEditorData(objMap);

			//Get the number of entries of the editor data.
			int m=objMap.size();

			//Check if the editor data isn't empty.
			if(m>0){
				//Integer containing the positions.
				int pos=0;
				int currentPos=0;

				//Get the number of movingpositions.
				pos=atoi(objMap[1].second.c_str());

				while(currentPos<pos){
					int x=atoi(objMap[currentPos*3+4].second.c_str());
					int y=atoi(objMap[currentPos*3+5].second.c_str());
					int t=atoi(objMap[currentPos*3+6].second.c_str());

					//Create a new movingPosition.
					MovingPosition position(x,y,t);
					movingBlocks[obj].push_back(position);

					//Increase currentPos by one.
					currentPos++;
				}
			}

			//Give it it's own id.
			std::map<std::string,std::string> editorData;
			char s[64];
			sprintf(s,"%d",currentId);
			currentId++;
			editorData["id"]=s;
			obj->setEditorData(editorData);
			break;
		}
		default:
		  break;
	}
}

void LevelEditor::moveObject(GameObject* obj,int x,int y){
	//Set the obj at it's new position.
	obj->setPosition(x,y);

	//Check if the object is inside the level dimensions.
	//If not let the level grow.
	if(obj->getBox().x+50>LEVEL_WIDTH){
		LEVEL_WIDTH=obj->getBox().x+50;
	}
	if(obj->getBox().y+50>LEVEL_HEIGHT){
		LEVEL_HEIGHT=obj->getBox().y+50;
	}
	if(obj->getBox().x<0 || obj->getBox().y<0){
		//A block on the left (or top) side of the level, meaning we need to shift everything.
		//First calc the difference.
		int diffx=(0-(obj->getBox().x));
		int diffy=(0-(obj->getBox().y));

		if(diffx<0) diffx=0;
		if(diffy<0) diffy=0;

		//Change the level size first.
		//The level grows with the difference, 0-(x+50).
		LEVEL_WIDTH+=diffx;
		LEVEL_HEIGHT+=diffy;
		//cout<<"x:"<<diffx<<",y:"<<diffy<<endl; //debug
		camera.x+=diffx;
		camera.y+=diffy;

		//Set the position of player and shadow
		//(although it's unnecessary if there is player and shadow start)
		player.setPosition(player.getBox().x+diffx,player.getBox().y+diffy);
		shadow.setPosition(shadow.getBox().x+diffx,shadow.getBox().y+diffy);

		for(unsigned int o=0; o<levelObjects.size(); o++){
			//FIXME: shouldn't recuesive call me (to prevent stack overflow bugs)
			moveObject(levelObjects[o],levelObjects[o]->getBox().x+diffx,levelObjects[o]->getBox().y+diffy);
		}
	}

	//If the object is a player or shadow start then change the start position of the player or shadow.
	if(obj->type==TYPE_START_PLAYER){
		//Center the player horizontally.
  		player.fx=obj->getBox().x+(50-23)/2;
		player.fy=obj->getBox().y;
		//Now reset the player to get him to it's new start position.
		player.reset(true);
	}
	if(obj->type==TYPE_START_SHADOW){
		//Center the shadow horizontally.
  		shadow.fx=obj->getBox().x+(50-23)/2;
		shadow.fy=obj->getBox().y;
		//Now reset the shadow to get him to it's new start position.
		shadow.reset(true);
	}
}

void LevelEditor::removeObject(GameObject* obj){
	std::vector<GameObject*>::iterator it;
	std::map<GameObject*,vector<GameObject*> >::iterator mapIt;

	//Increase totalCollectables everytime we add a new collectable
	if (obj->type==TYPE_COLLECTABLE) {
		totalCollectables--;
	}

	//Check if the object is in the selection.
	it=find(selection.begin(),selection.end(),obj);
	if(it!=selection.end()){
		//It is so we delete it.
		selection.erase(it);
	}

	//Check if the object is in the triggers.
	mapIt=triggers.find(obj);
	if(mapIt!=triggers.end()){
		//It is so we remove it.
		triggers.erase(mapIt);
	}

	//Boolean if it could be a target.
	if(obj->type==TYPE_MOVING_BLOCK || obj->type==TYPE_MOVING_SHADOW_BLOCK || obj->type==TYPE_MOVING_SPIKES
		|| obj->type==TYPE_CONVEYOR_BELT || obj->type==TYPE_SHADOW_CONVEYOR_BELT || obj->type==TYPE_PORTAL){
		for(mapIt=triggers.begin();mapIt!=triggers.end();++mapIt){
			//Now loop the target vector.
			for(unsigned int o=0;o<(*mapIt).second.size();o++){
				//Check if the obj is in the target vector.
				if((*mapIt).second[o]==obj){
					(*mapIt).second.erase(find((*mapIt).second.begin(),(*mapIt).second.end(),obj));
					o--;
				}
			}
		}
	}

	//Check if the object is in the movingObjects.
	std::map<GameObject*,vector<MovingPosition> >::iterator movIt;
	movIt=movingBlocks.find(obj);
	if(movIt!=movingBlocks.end()){
		//It is so we remove it.
		movingBlocks.erase(movIt);
	}

	//Now we remove the object from the levelObjects.
	it=find(levelObjects.begin(),levelObjects.end(),obj);
	if(it!=levelObjects.end()){
		levelObjects.erase(it);
	}
	delete obj;
	obj=NULL;

	//Set dirty of selection popup
	if(selectionPopup!=NULL) selectionPopup->dirty=true;
}

void LevelEditor::GUIEventCallback_OnEvent(std::string name,GUIObject* obj,int eventType){
	//Check for GUI events.
	//Notification block configure events.
	if(name=="cfgNotificationBlockOK"){
		if(GUIObjectRoot){
			//Set the message of the notification block.
			std::map<std::string,std::string> editorData;
			editorData["message"]=objectProperty->caption;
			configuredObject->setEditorData(editorData);

			//And delete the GUI.
			objectProperty=NULL;
			secondObjectProperty=NULL;
			configuredObject=NULL;
			delete GUIObjectRoot;
			GUIObjectRoot=NULL;
		}
	}
	//Conveyor belt block configure events.
	if(name=="cfgConveyorBlockOK"){
		if(GUIObjectRoot){
			//Set the message of the notification block.
			std::map<std::string,std::string> editorData;
			editorData["speed"]=secondObjectProperty->caption;
			editorData["disabled"]=(objectProperty->value==0)?"1":"0";
			configuredObject->setEditorData(editorData);

			//And delete the GUI.
			objectProperty=NULL;
			secondObjectProperty=NULL;
			configuredObject=NULL;
			delete GUIObjectRoot;
			GUIObjectRoot=NULL;
		}
	}
	//Moving block configure events.
	if(name=="cfgMovingBlockOK"){
		if(GUIObjectRoot){
			//Set if the moving block is enabled/disabled.
			std::map<std::string,std::string> editorData;
			editorData["disabled"]=(objectProperty->value==0)?"1":"0";
			editorData["loop"]=(secondObjectProperty->value==1)?"1":"0";
			configuredObject->setEditorData(editorData);

			//And delete the GUI.
			objectProperty=NULL;
			secondObjectProperty=NULL;
			configuredObject=NULL;
			delete GUIObjectRoot;
			GUIObjectRoot=NULL;
		}
	}
	if(name=="cfgMovingBlockClrPath"){
		if(GUIObjectRoot){
			//Set the message of the notification block.
			std::map<std::string,std::string> editorData;
			editorData["MovingPosCount"]="0";
			configuredObject->setEditorData(editorData);

			std::map<GameObject*,vector<MovingPosition> >::iterator it;
			it=movingBlocks.find(configuredObject);
			if(it!=movingBlocks.end()){
				(*it).second.clear();
			}

			//And delete the GUI.
			objectProperty=NULL;
			secondObjectProperty=NULL;
			configuredObject=NULL;
			delete GUIObjectRoot;
			GUIObjectRoot=NULL;
		}
	}
	if(name=="cfgMovingBlockMakePath"){
		if(GUIObjectRoot){
			//Set moving.
			moving=true;
			movingBlock=configuredObject;

			//And delete the GUI.
			objectProperty=NULL;
			secondObjectProperty=NULL;
			configuredObject=NULL;
			delete GUIObjectRoot;
			GUIObjectRoot=NULL;
		}
	}
	//Portal block configure events.
	if(name=="cfgPortalOK"){
		if(GUIObjectRoot){
			//Set the message of the notification block.
			std::map<std::string,std::string> editorData;
			editorData["automatic"]=(objectProperty->value==1)?"1":"0";
			configuredObject->setEditorData(editorData);

			//And delete the GUI.
			objectProperty=NULL;
			secondObjectProperty=NULL;
			configuredObject=NULL;
			delete GUIObjectRoot;
			GUIObjectRoot=NULL;
		}
	}
	if(name=="cfgPortalLink"){
		//We set linking true.
		linking=true;
		linkingTrigger=configuredObject;

		//And delete the GUI.
		objectProperty=NULL;
		secondObjectProperty=NULL;
		configuredObject=NULL;
		if(GUIObjectRoot){
			delete GUIObjectRoot;
		}
		GUIObjectRoot=NULL;
	}
	if(name=="cfgPortalUnlink"){
		std::map<GameObject*,vector<GameObject*> >::iterator it;
		it=triggers.find(configuredObject);
		if(it!=triggers.end()){
			//Remove the targets.
			(*it).second.clear();
		}

		//We reset the destination.
		std::map<std::string,std::string> editorData;
		editorData["destination"]="";
		configuredObject->setEditorData(editorData);

		//And delete the GUI.
		objectProperty=NULL;
		secondObjectProperty=NULL;
		configuredObject=NULL;
		if(GUIObjectRoot){
			delete GUIObjectRoot;
		}
		GUIObjectRoot=NULL;
	}
	//Trigger block configure events.
	if(name=="cfgTriggerOK"){
		if(GUIObjectRoot){
			//Set the message of the notification block.
			std::map<std::string,std::string> editorData;
			editorData["behaviour"]=(dynamic_cast<GUISingleLineListBox*>(objectProperty))->item[objectProperty->value];
			configuredObject->setEditorData(editorData);

			//And delete the GUI.
			objectProperty=NULL;
			secondObjectProperty=NULL;
			configuredObject=NULL;
			delete GUIObjectRoot;
			GUIObjectRoot=NULL;
		}
	}
	if(name=="cfgTriggerLink"){
		//We set linking true.
		linking=true;
		linkingTrigger=configuredObject;

		//And delete the GUI.
		objectProperty=NULL;
		secondObjectProperty=NULL;
		configuredObject=NULL;
		if(GUIObjectRoot){
			delete GUIObjectRoot;
		}
		GUIObjectRoot=NULL;
	}
	if(name=="cfgTriggerUnlink"){
		std::map<GameObject*,vector<GameObject*> >::iterator it;
		it=triggers.find(configuredObject);
		if(it!=triggers.end()){
			//Remove the targets.
			(*it).second.clear();
		}

		//We give the trigger a new id to prevent activating unlinked targets.
		std::map<std::string,std::string> editorData;
		char s[64];
		sprintf(s,"%d",currentId);
		currentId++;
		editorData["id"]=s;
		configuredObject->setEditorData(editorData);

		//And delete the GUI.
		objectProperty=NULL;
		secondObjectProperty=NULL;
		configuredObject=NULL;
		if(GUIObjectRoot){
			delete GUIObjectRoot;
		}
		GUIObjectRoot=NULL;
	}

	//Fragile configuration.
	if(name=="cfgFragileOK"){
		std::map<std::string,std::string> editorData;
		char s[64];
		sprintf(s,"%d",objectProperty->value);
		editorData["state"]=s;
		configuredObject->setEditorData(editorData);

		//And delete the GUI.
		objectProperty=NULL;
		secondObjectProperty=NULL;
		configuredObject=NULL;
		if(GUIObjectRoot){
			delete GUIObjectRoot;
		}
		GUIObjectRoot=NULL;
	}

	//Cancel.
	if(name=="cfgCancel"){
		if(GUIObjectRoot){
			//Delete the GUI.
			objectProperty=NULL;
			secondObjectProperty=NULL;
			configuredObject=NULL;
			delete GUIObjectRoot;
			GUIObjectRoot=NULL;
		}
	}

	//LevelSetting events.
	if(name=="lvlSettingsOK"){
		levelName=objectProperty->caption;
		levelTheme=secondObjectProperty->caption;

		//target time and recordings.
		string s=levelTimeProperty->caption;
		if(s.empty() || !(s[0]>='0' && s[0]<='9')){
			levelTime=-1;
		}else{
			levelTime=int(atof(s.c_str())*40.0+0.5);
		}

		s=levelRecordingsProperty->caption;
		if(s.empty() || !(s[0]>='0' && s[0]<='9')){
			levelRecordings=-1;
		}else{
			levelRecordings=atoi(s.c_str());
		}

		//And delete the GUI.
		if(GUIObjectRoot){
			objectProperty=NULL;
			secondObjectProperty=NULL;
			delete GUIObjectRoot;
			GUIObjectRoot=NULL;
		}
	}
	if(name=="lvlSettingsCancel"){
		if(GUIObjectRoot){
			//Delete the GUI.
			objectProperty=NULL;
			secondObjectProperty=NULL;
			delete GUIObjectRoot;
			GUIObjectRoot=NULL;
		}
	}
}

////////////////LOGIC////////////////////
void LevelEditor::logic(){
	if(playMode){
		//PlayMode so let the game do it's logic.
		Game::logic();
	}else{
		//Move the camera.
		if(cameraXvel!=0 || cameraYvel!=0){
			camera.x+=cameraXvel;
			camera.y+=cameraYvel;
			//Call the onCameraMove event.
			onCameraMove(cameraXvel,cameraYvel);
		}
		//Move the camera with the mouse.
		{
			SDL_Rect r[3]={toolbarRect};
			int m=1;
			if(toolbox!=NULL && tool==ADD && toolbox->visible)
				r[m++]=toolbox->getRect();
			if(selectionPopup!=NULL)
				r[m++]=selectionPopup->getRect();
			setCamera(r,m);
		}

		//It isn't playMode so the mouse should be checked.
		tooltip=-1;
		//Get the mouse location.
		int x,y;
		SDL_GetMouseState(&x,&y);
		SDL_Rect mouse={x,y,0,0};

		//We loop through the number of tools + the number of buttons.
		for(int t=0; t<NUMBER_TOOLS+6; t++){
			SDL_Rect toolRect={(SCREEN_WIDTH-460)/2+(t*40)+((t+1)*10),SCREEN_HEIGHT-45,40,40};

			//Check for collision.
			if(checkCollision(mouse,toolRect)==true){
				//Set the tooltip tool.
				tooltip=t;
			}
		}
	}
}

/////////////////RENDER//////////////////////
void LevelEditor::render(){
	//Always let the game render the game.
	Game::render();

	//Only render extra stuff like the toolbar, selection, etc.. when not in playMode.
	if(!playMode){
		//Render the selectionmarks.
		//TODO: Check if block is in sight.
		for(unsigned int o=0; o<selection.size(); o++){
			//Get the location to draw.
			SDL_Rect r=selection[o]->getBox();
			r.x-=camera.x;
			r.y-=camera.y;

			//Draw the selectionMarks.
			applySurface(r.x,r.y,selectionMark,screen,NULL);
			applySurface(r.x+r.w-5,r.y,selectionMark,screen,NULL);
			applySurface(r.x,r.y+r.h-5,selectionMark,screen,NULL);
			applySurface(r.x+r.w-5,r.y+r.h-5,selectionMark,screen,NULL);
		}

		//Clear the placement surface.
		SDL_FillRect(placement,NULL,0x00FF00FF);
		
		Uint32 color=SDL_MapRGB(placement->format,themeTextColor.r,themeTextColor.g,themeTextColor.b);

		//Draw the dark areas marking the outside of the level.
		SDL_Rect r;
		if(camera.x<0){
			//Draw left side.
			r.x=0;
			r.y=0;
			r.w=0-camera.x;
			r.h=SCREEN_HEIGHT;
			SDL_FillRect(placement,&r,color);
		}
		if(camera.x>LEVEL_WIDTH-SCREEN_WIDTH){
			//Draw right side.
			r.x=LEVEL_WIDTH-camera.x;
			r.y=0;
			r.w=SCREEN_WIDTH-(LEVEL_WIDTH-camera.x);
			r.h=SCREEN_HEIGHT;
			SDL_FillRect(placement,&r,color);
		}
		if(camera.y<0){
			//Draw the top.
			r.x=0;
			r.y=0;
			r.w=SCREEN_WIDTH;
			r.h=0-camera.y;
			SDL_FillRect(placement,&r,color);
		}
		if(camera.y>LEVEL_HEIGHT-SCREEN_HEIGHT){
			//Draw the bottom.
			r.x=0;
			r.y=LEVEL_HEIGHT-camera.y;
			r.w=SCREEN_WIDTH;
			r.h=SCREEN_HEIGHT-(LEVEL_HEIGHT-camera.y);
			SDL_FillRect(placement,&r,color);
		}

		//Check if we should draw on the placement surface.
		if(selectionDrag){
			showSelectionDrag();
		}else{
			if(tool==ADD){
				showCurrentObject();
			}
			if(tool==CONFIGURE){
				showConfigure();
			}
		}

		//Draw the level borders.
		drawRect(-camera.x,-camera.y,LEVEL_WIDTH,LEVEL_HEIGHT,screen);

		//Render the placement surface.
		applySurface(0,0,placement,screen,NULL);

		//Render the hud layer.
		renderHUD();

		//Render tool box (if any)
		if(toolbox!=NULL && tool==ADD && toolbox->visible){
			toolbox->render();
		}

		//On top of all render the toolbar.
		applySurface(toolbarRect.x,toolbarRect.y,toolbar,screen,NULL);
		//Now render a tooltip.
		if(tooltip>=0){
			//The back and foreground colors.
			SDL_Color fg={0,0,0};

			//Tool specific text.
			SDL_Surface* tip=NULL;
			switch(tooltip){
				case 0:
					tip=TTF_RenderUTF8_Blended(fontText,_("Select"),fg);
					break;
				case 1:
					tip=TTF_RenderUTF8_Blended(fontText,_("Add"),fg);
					break;
				case 2:
					tip=TTF_RenderUTF8_Blended(fontText,_("Delete"),fg);
					break;
				case 3:
					tip=TTF_RenderUTF8_Blended(fontText,_("Configure"),fg);
					break;
				case 4:
					tip=TTF_RenderUTF8_Blended(fontText,_("Play"),fg);
					break;
				case 6:
					tip=TTF_RenderUTF8_Blended(fontText,_("Level settings"),fg);
					break;
				case 7:
					tip=TTF_RenderUTF8_Blended(fontText,_("Save level"),fg);
					break;
				case 8:
					tip=TTF_RenderUTF8_Blended(fontText,_("Back to menu"),fg);
					break;
				default:
					break;
			}

			//Draw only if there's a tooltip available
			if(tip!=NULL){
				SDL_Rect r={(SCREEN_WIDTH-440)/2+(tooltip*40)+(tooltip*10),SCREEN_HEIGHT-45,40,40};
				r.y=SCREEN_HEIGHT-50-tip->h;
				if(r.x+tip->w>SCREEN_WIDTH-50)
					r.x=SCREEN_WIDTH-50-tip->w;

				//Draw borders around text
				Uint32 color=0xFFFFFF00|230;
				drawGUIBox(r.x-2,r.y-2,tip->w+4,tip->h+4,screen,color);

				//Draw tooltip's text
				SDL_BlitSurface(tip,NULL,screen,&r);
				SDL_FreeSurface(tip);
			}
		}

		//Draw a rectangle around the current tool.
		color=0xFFFFFF00;
		drawGUIBox((SCREEN_WIDTH-440)/2+(tool*40)+(tool*10),SCREEN_HEIGHT-46,42,42,screen,color);

		//Render selection popup (if any)
		if(selectionPopup!=NULL){
			if(linking){
				//If we switch to linking mode then delete it
				delete selectionPopup;
				selectionPopup=NULL;
			}else{
				selectionPopup->render();
			}
		}
	}
}

void LevelEditor::renderHUD(){
	//Switch the tool.
	switch(tool){
	case CONFIGURE:
		//If moving show the moving speed in the top right corner.
		if(moving){
			//Calculate width of text "Movespeed: 100" to keep the same position with every value
			if (movingSpeedWidth==-1){
				int w;
				TTF_SizeUTF8(fontText,tfm::format(_("Movespeed: %s"),100).c_str(),&w,NULL);
				movingSpeedWidth=w+4;
			}
		
			//Now render the text.
			SDL_Color black={0,0,0,0};
			SDL_Surface* bm=TTF_RenderUTF8_Blended(fontText,tfm::format(_("Movespeed: %s"),movingSpeed).c_str(),black);

			//Draw the text in box and free the surface.
			drawGUIBox(SCREEN_WIDTH-movingSpeedWidth-2,-2,movingSpeedWidth+8,bm->h+6,screen,0xDDDDDDDD);
			applySurface(SCREEN_WIDTH-movingSpeedWidth,2,bm,screen,NULL);
			SDL_FreeSurface(bm);
		}
		break;
	default:
		break;
	}
}

void LevelEditor::showCurrentObject(){
	//Get the current mouse location.
	int x,y;
	SDL_GetMouseState(&x,&y);
	x+=camera.x;
	y+=camera.y;

	//Check if we should snap the block to grid or not.
	if(!pressedShift){
		snapToGrid(&x,&y);
	}else{
		x-=25;
		y-=25;
	}

	//Check if the currentType is a legal type.
	if(currentType>=0 && currentType<EDITOR_ORDER_MAX){
		ThemeBlock* obj=objThemes.getBlock(editorTileOrder[currentType]);
		if(obj){
			obj->editorPicture.draw(placement,x-camera.x,y-camera.y);
		}
	}
}

void LevelEditor::showSelectionDrag(){
	//Get the current mouse location.
	int x,y;
	SDL_GetMouseState(&x,&y);
	//Create the rectangle.
	x+=camera.x;
	y+=camera.y;

	//Check if we should snap the block to grid or not.
	if(!pressedShift){
		snapToGrid(&x,&y);
	}else{
		x-=25;
		y-=25;
	}

	//Check if the drag center isn't null.
	if(dragCenter==NULL) return;
	//The location of the dragCenter.
	SDL_Rect r=dragCenter->getBox();

	//Loop through the selection.
	//TODO: Check if block is in sight.
	for(unsigned int o=0; o<selection.size(); o++){
		ThemeBlock* obj=objThemes.getBlock(selection[o]->type);
		if(obj){
			SDL_Rect r1=selection[o]->getBox();
			obj->editorPicture.draw(placement,(r1.x-r.x)+x-camera.x,(r1.y-r.y)+y-camera.y);
		}
	}
}

void LevelEditor::showConfigure(){
	//arrow animation value. go through 0-65535 and loops.
	static unsigned short arrowAnimation=0;
	arrowAnimation++;
	
	//By default use black color for arrows.
	Uint32 color=0x00000000;
	
	//Theme can change the color.
	//TODO: use the actual color from the theme.
	if(themeTextColor.r>128 && themeTextColor.g>128 && themeTextColor.b>128)
		color=0xffffffff;

	//Draw the trigger lines.
	{
		map<GameObject*,vector<GameObject*> >::iterator it;
		for(it=triggers.begin();it!=triggers.end();++it){
			//Check if the trigger has linked targets.
			if(!(*it).second.empty()){
				//The location of the trigger.
				SDL_Rect r=(*it).first->getBox();

				//Loop through the targets.
				for(unsigned int o=0;o<(*it).second.size();o++){
					//Get the location of the target.
					SDL_Rect r1=(*it).second[o]->getBox();

					//Draw the line from the center of the trigger to the center of the target.
					drawLineWithArrow(r.x-camera.x+25,r.y-camera.y+25,r1.x-camera.x+25,r1.y-camera.y+25,placement,color,32,arrowAnimation%32);

					//Also draw two selection marks.
					applySurface(r.x-camera.x+25-2,r.y-camera.y+25-2,selectionMark,screen,NULL);
					applySurface(r1.x-camera.x+25-2,r1.y-camera.y+25-2,selectionMark,screen,NULL);
				}
			}
		}

		//Draw a line to the mouse from the linkingTrigger when linking.
		if(linking){
			//Get the current mouse location.
			int x,y;
			SDL_GetMouseState(&x,&y);

			//Draw the line from the center of the trigger to mouse.
			drawLineWithArrow(linkingTrigger->getBox().x-camera.x+25,linkingTrigger->getBox().y-camera.y+25,x,y,placement,color,32,arrowAnimation%32);
		}
	}

	//Draw the moving positions.
	map<GameObject*,vector<MovingPosition> >::iterator it;
	for(it=movingBlocks.begin();it!=movingBlocks.end();++it){
		//Check if the block has positions.
		if(!(*it).second.empty()){
			//The location of the moving block.
			SDL_Rect block=(*it).first->getBox();
			block.x+=25-camera.x;
			block.y+=25-camera.y;

			//The location of the previous position.
			//The first time it's the moving block's position self.
			SDL_Rect r=block;

			//Loop through the positions.
			for(unsigned int o=0;o<(*it).second.size();o++){
				//Draw the line from the center of the previous position to the center of the position.
				//x and y are the coordinates for the current moving position.
				int x=block.x+(*it).second[o].x;
				int y=block.y+(*it).second[o].y;

				//Check if we need to draw line
				double dx=r.x-x;
				double dy=r.y-y;
				double d=sqrt(dx*dx+dy*dy);
				if(d>0.001f){
					if(it->second[o].time>0){
						//Calculate offset to contain the moving speed.
						int offset=int(d*arrowAnimation/it->second[o].time)%32;
						drawLineWithArrow(r.x,r.y,x,y,placement,color,32,offset);
					}else{
						//time==0 ???? so don't draw arrow at all
						drawLine(r.x,r.y,x,y,placement);
					}
				}

				//And draw a marker at the end.
				applySurface(x-13,y-13,movingMark,screen,NULL);

				//Get the box of the previous position.
				SDL_Rect tmp={x,y,0,0};
				r=tmp;
			}
		}
	}

	//Draw a line to the mouse from the previous moving pos.
	if(moving){
		//Get the current mouse location.
		int x,y;
		SDL_GetMouseState(&x,&y);

		//Check if we should snap the block to grid or not.
		if(!pressedShift){
			x+=camera.x;
			y+=camera.y;
			snapToGrid(&x,&y);
			x-=camera.x;
			y-=camera.y;
		}else{
			x-=25;
			y-=25;
		}

		int posX,posY;

		//Check if there are moving positions for the moving block.
		if(!movingBlocks[movingBlock].empty()){
			//Draw the line from the center of the previouse moving positions to mouse.
			posX=movingBlocks[movingBlock].back().x;
			posY=movingBlocks[movingBlock].back().y;

			posX-=camera.x;
			posY-=camera.y;

			posX+=movingBlock->getBox().x;
			posY+=movingBlock->getBox().y;
		}else{
			//Draw the line from the center of the movingblock to mouse.
			posX=movingBlock->getBox().x-camera.x;
			posY=movingBlock->getBox().y-camera.y;
		}

		//Calculate offset to contain the moving speed.
		int offset=int(double(arrowAnimation)*movingSpeed/10.0)%32;

		drawLineWithArrow(posX+25,posY+25,x+25,y+25,placement,color,32,offset);
		applySurface(x+12,y+12,movingMark,screen,NULL);
	}

}

void LevelEditor::resize(){
	//Call the resize method of the Game.
	Game::resize();

	//Now update the placement surface.
	if(placement)
		SDL_FreeSurface(placement);
	placement=SDL_CreateRGBSurface(SDL_SWSURFACE|SDL_SRCALPHA,SCREEN_WIDTH,SCREEN_HEIGHT,32,RMASK,GMASK,BMASK,0);
	SDL_SetColorKey(placement,SDL_SRCCOLORKEY|SDL_RLEACCEL,SDL_MapRGB(placement->format,255,0,255));
	SDL_SetAlpha(placement,SDL_SRCALPHA,125);
	
	//Move the toolbar's position rect used for collision.
	toolbarRect.x=(SCREEN_WIDTH-460)/2;
	toolbarRect.y=SCREEN_HEIGHT-50;
}

//Filling the order array
const int LevelEditor::editorTileOrder[EDITOR_ORDER_MAX]={
	TYPE_BLOCK,
	TYPE_SHADOW_BLOCK,
	TYPE_SPIKES,
	TYPE_FRAGILE,
	TYPE_MOVING_BLOCK,
	TYPE_MOVING_SHADOW_BLOCK,
	TYPE_MOVING_SPIKES,
	TYPE_CONVEYOR_BELT,
	TYPE_SHADOW_CONVEYOR_BELT,
	TYPE_BUTTON,
	TYPE_SWITCH,
	TYPE_PORTAL,
	TYPE_SWAP,
	TYPE_CHECKPOINT,
	TYPE_NOTIFICATION_BLOCK,
	TYPE_START_PLAYER,
	TYPE_START_SHADOW,
	TYPE_EXIT,
	TYPE_COLLECTABLE
};
