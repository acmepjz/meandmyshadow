/*
 * Copyright (C) 2011-2013 Me and My Shadow
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
#include "Game.h"
#include "TreeStorageNode.h"
#include "POASerializer.h"
#include "InputManager.h"
#include "StatisticsManager.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <algorithm>
#include <locale>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libs/tinyformat/tinyformat.h"

using namespace std;

const char* Game::blockName[TYPE_MAX]={"Block","PlayerStart","ShadowStart",
"Exit","ShadowBlock","Spikes",
"Checkpoint","Swap","Fragile",
"MovingBlock","MovingShadowBlock","MovingSpikes",
"Teleporter","Button","Switch",
"ConveyorBelt","ShadowConveyorBelt","NotificationBlock", "Collectable", "Pushable"
};

map<string,int> Game::blockNameMap;
map<int,string> Game::gameObjectEventTypeMap;
map<string,int> Game::gameObjectEventNameMap;
map<int,string> Game::levelEventTypeMap;
map<string,int> Game::levelEventNameMap;
string Game::recordFile;

Game::Game():isReset(false)
	,currentLevelNode(NULL)
	,customTheme(NULL)
	,background(NULL)
	,won(false)
	,interlevel(false)
	,gameTipIndex(0)
	,time(0),timeSaved(0)
	,recordings(0),recordingsSaved(0)
	,cameraMode(CAMERA_PLAYER),cameraModeSaved(CAMERA_PLAYER)
	,player(this),shadow(this),objLastCheckPoint(NULL),
	medalX(0),currentCollectables(0),totalCollectables(0),currentCollectablesSaved(0){

	saveStateNextTime=false;
	loadStateNextTime=false;

	recentSwap=recentSwapSaved=-10000;
	recentLoad=recentSave=0;

	//Reserve the memory for the GameObject tips.
	memset(bmTips,0,sizeof(bmTips));

	action=loadImage(getDataPath()+"gfx/actions.png");
	medals=loadImage(getDataPath()+"gfx/medals.png");
	//Get the collectable image from the theme.
	//NOTE: Isn't there a better way to retrieve the image?
	objThemes.getBlock(TYPE_COLLECTABLE)->createInstance(&collectable);

	//Hide the cursor if not in the leveleditor.
	if(stateID!=STATE_LEVEL_EDITOR)
		SDL_ShowCursor(SDL_DISABLE);
}

Game::~Game(){
	//Simply call our destroy method.
	destroy();
	
	//Before we leave make sure the cursor is visible.
	SDL_ShowCursor(SDL_ENABLE);
}

void Game::destroy(){
	//Loop through the levelObjects and delete them.
	for(unsigned int i=0;i<levelObjects.size();i++)
		delete levelObjects[i];
	//Done now clear the levelObjects vector.
	levelObjects.clear();
	
	//Loop through the backgroundLayers and delete them.
	std::map<std::string,std::vector<Scenery*> >::iterator it;
	for(it=backgroundLayers.begin();it!=backgroundLayers.end();++it){
		for(unsigned int i=0;i<it->second.size();i++)
			delete it->second[i];
	}
	backgroundLayers.clear();

	//Clear the name and the editor data.
	levelName.clear();
	levelFile.clear();
	editorData.clear();

	//Loop through the tips.
	for(int i=0;i<TYPE_MAX;i++){
		//If it exist free the SDL_Surface.
		if(bmTips[i])
			SDL_FreeSurface(bmTips[i]);
	}
	memset(bmTips,0,sizeof(bmTips));

	//Remove everything from the themeManager.
	background=NULL;
	if(customTheme)
		objThemes.removeTheme();
	customTheme=NULL;
	//If there's a (partial) theme bundled with the levelpack remove that as well.
	if(levels->customTheme)
		objThemes.removeTheme();

	//delete current level (if any)
	if(currentLevelNode){
		delete currentLevelNode;
		currentLevelNode=NULL;
	}

	//Reset the time.
	time=timeSaved=0;
	recordings=recordingsSaved=0;
	recentSwap=recentSwapSaved=-10000;
}

void Game::loadLevelFromNode(TreeStorageNode* obj,const string& fileName){
	//Make sure there's nothing left from any previous levels.
	//Not needed since loadLevelFromNode is only called from the changeState method, meaning it's a new instance of Game.
	//destroy();

	//set current level to loaded one.
	currentLevelNode=obj;

	//Temp var used for block locations.
	SDL_Rect box;

	//Set the level dimensions to the default, it will probably be changed by the editorData,
	//but 800x600 is a fallback.
	LEVEL_WIDTH=800;
	LEVEL_HEIGHT=600;

	currentCollectables=0;
	totalCollectables=0;
	currentCollectablesSaved=0;

	//Load the additional data.
	for(map<string,vector<string> >::iterator i=obj->attributes.begin();i!=obj->attributes.end();++i){
		if(i->first=="size"){
			//We found the size attribute.
			if(i->second.size()>=2){
				//Set the dimensions of the level.
				LEVEL_WIDTH=atoi(i->second[0].c_str());
				LEVEL_HEIGHT=atoi(i->second[1].c_str());
			}
		}else if(i->second.size()>0){
			//Any other data will be put into the editorData.
			editorData[i->first]=i->second[0];
		}
	}

	//Get the theme.
	{
		//If a theme is configured then load it.
		string theme=processFileName(getSettings()->getValue("theme"));

		//Check if level themes are enabled.
		if(getSettings()->getBoolValue("leveltheme")) {
			//Check for the theme to use.
			string &s=editorData["theme"];
			if(!s.empty()){
				customTheme=objThemes.appendThemeFromFile(processFileName(s)+"/theme.mnmstheme");
			}

			//Also check for bundled (partial) themes.
			if(levels->customTheme){
				if(objThemes.appendThemeFromFile(levels->levelpackPath+"/theme/theme.mnmstheme")==NULL){
					//The theme failed to load so set the customTheme boolean to false.
					levels->customTheme=false;
				}
			}
		}

		//Set the Appearance of the player and the shadow.
		objThemes.getCharacter(false)->createInstance(&player.appearance);
		objThemes.getCharacter(true)->createInstance(&shadow.appearance);
	}


	for(unsigned int i=0;i<obj->subNodes.size();i++){
		TreeStorageNode* obj1=obj->subNodes[i];
		if(obj1==NULL) continue;
		if(obj1->name=="tile"){
			Block* block=new Block(this);
			if(!block->loadFromNode(obj1)){
				delete block;
				continue;
			}

			//If the type is collectable, increase the number of totalCollectables
			if(block->type==TYPE_COLLECTABLE)
				totalCollectables++;

			//Add the block to the levelObjects vector.
			levelObjects.push_back(block);
		}else if(obj1->name=="backgroundlayer" && obj1->value.size()==1){
			//Loop through the sub nodes.
			for(unsigned int j=0;j<obj1->subNodes.size();j++){
				TreeStorageNode* obj2=obj1->subNodes[j];
				if(obj2==NULL) continue;

				if(obj2->name=="object"){
					//Load the scenery from node.
					Scenery* scenery=new Scenery(this);
					if(!scenery->loadFromNode(obj2)){
						delete scenery;
						continue;
					}
					
					backgroundLayers[obj1->value[0]].push_back(scenery);
				}
			}
		}else if(obj1->name=="script" && !obj1->value.empty()){
			map<string,int>::iterator it=Game::levelEventNameMap.find(obj1->value[0]);
			if(it!=Game::levelEventNameMap.end()){
				int eventType=it->second;
				const std::string& script=obj1->attributes["script"][0];
				if(!script.empty()) scripts[eventType]=script;
			}
		}
	}

	//Close exits if there are collectables
	if(totalCollectables>0){
		for(unsigned int i=0;i<levelObjects.size();i++){
			if(levelObjects[i]->type==TYPE_EXIT){
				levelObjects[i]->onEvent(GameObjectEvent_OnSwitchOff);
			}
		}
	}

	//Set the levelName to the name of the current level.
	levelName=editorData["name"];
	levelFile=fileName;

	//Some extra stuff only needed when not in the levelEditor.
	if(stateID!=STATE_LEVEL_EDITOR){
		//We create a text with the text "Level <levelno> <levelName>".
		//It will be shown in the left bottom corner of the screen.
		string s;
		if(levels->getLevelCount()>1){
			s=tfm::format(_("Level %d %s"),levels->getCurrentLevel()+1,_CC(levels->getDictionaryManager(),editorData["name"]));
		}

		SDL_Color fg={0,0,0,0};
		bmTips[0]=TTF_RenderUTF8_Blended(fontText,s.c_str(),fg);
	}

	//Get the background
	background=objThemes.getBackground(false);
	if(background)
		background->resetAnimation(true);

	//Reset the script environment.
	getScriptExecutor()->reset();

	//Send GameObjectEvent_OnCreate event to the script
	for(unsigned int i=0;i<levelObjects.size();i++){
		levelObjects[i]->onEvent(GameObjectEvent_OnCreate);
	}

	//Finally call the level's onCreate event.
	executeScript(LevelEvent_OnCreate);
}

void Game::loadLevel(string fileName){
	//Create a TreeStorageNode that will hold the loaded data.
	TreeStorageNode *obj=new TreeStorageNode();
	{
		POASerializer objSerializer;
		string s=fileName;

		//Parse the file.
		if(!objSerializer.loadNodeFromFile(s.c_str(),obj,true)){
			cout<<"Can't load level file "<<s<<endl;
			delete obj;
			return;
		}
	}

	//Now call another function.
	loadLevelFromNode(obj,fileName);
}

void Game::saveRecord(const char* fileName){
	//check if current level is NULL (which should be impossible)
	if(currentLevelNode==NULL) return;

	TreeStorageNode obj;
	POASerializer objSerializer;

	//put current level to the node.
	currentLevelNode->name="map";
	obj.subNodes.push_back(currentLevelNode);

	//serialize the game record using RLE compression.
#define PUSH_BACK \
			if(j>0){ \
				if(j>1){ \
					sprintf(c,"%d*%d",last,j); \
				}else{ \
					sprintf(c,"%d",last); \
				} \
				v.push_back(c); \
			}
	vector<string> &v=obj.attributes["record"];
	vector<int> *record=player.getRecord();
	char c[64];
	int i,j=0,last;
	for(i=0;i<(int)record->size();i++){
		int currentKey=(*record)[i];
		if(j==0 || currentKey!=last){
			PUSH_BACK;
			last=currentKey;
			j=1;
		}else{
			j++;
		}
	}
	PUSH_BACK;
#undef PUSH_BACK

#ifdef RECORD_FILE_DEBUG
	//add record file debug data.
	{
		obj.attributes["recordKeyPressLog"].push_back(player.keyPressLog());

		vector<SDL_Rect> &playerPosition=player.playerPosition();
		string s;
		char c[32];

		sprintf(c,"%d\n",int(playerPosition.size()));
		s=c;

		for(unsigned int i=0;i<playerPosition.size();i++){
			SDL_Rect& r=playerPosition[i];
			sprintf(c,"%d %d\n",r.x,r.y);
			s+=c;
		}

		obj.attributes["recordPlayerPosition"].push_back(s);
	}
#endif

	//save it
	objSerializer.saveNodeToFile(fileName,&obj,true,true);

	//remove current level from node to prevent delete it.
	obj.subNodes.clear();
}

void Game::loadRecord(const char* fileName){
	//Create a TreeStorageNode that will hold the loaded data.
	TreeStorageNode obj;
	{
		POASerializer objSerializer;
		string s=fileName;

		//Parse the file.
		if(!objSerializer.loadNodeFromFile(s.c_str(),&obj,true)){
			cout<<"Can't load record file "<<s<<endl;
			return;
		}
	}

	//find the node named 'map'.
	bool loaded=false;
	for(unsigned int i=0;i<obj.subNodes.size();i++){
		if(obj.subNodes[i]->name=="map"){
			//load the level. (fileName=???)
			loadLevelFromNode(obj.subNodes[i],"???");
			//remove this node to prevent delete it.
			obj.subNodes[i]=NULL;
			//over
			loaded=true;
			break;
		}
	}

	if(!loaded){
		cout<<"ERROR: Can't find subnode named 'map' from record file"<<endl;
		return;
	}

	//load the record.
	{
		vector<int> *record=player.getRecord();
		record->clear();
		vector<string> &v=obj.attributes["record"];
		for(unsigned int i=0;i<v.size();i++){
			string &s=v[i];
			string::size_type pos=s.find_first_of('*');
			if(pos==string::npos){
				//1 item only.
				int i=atoi(s.c_str());
				record->push_back(i);
			}else{
				//contains many items.
				int i=atoi(s.substr(0,pos).c_str());
				int j=atoi(s.substr(pos+1).c_str());
				for(;j>0;j--){
					record->push_back(i);
				}
			}
		}
	}

#ifdef RECORD_FILE_DEBUG
	//load the debug data
	{
		vector<string> &v=obj.attributes["recordPlayerPosition"];
		vector<SDL_Rect> &playerPosition=player.playerPosition();
		playerPosition.clear();
		if(!v.empty()){
			if(!v[0].empty()){
				stringstream st(v[0]);
				int m;
				st>>m;
				for(int i=0;i<m;i++){
					SDL_Rect r;
					st>>r.x>>r.y;
					r.w=0;
					r.h=0;
					playerPosition.push_back(r);
				}
			}
		}
	}
#endif

	//play the record.
	//TODO: tell the level manager don't save the level progress.
	player.playRecord();
	shadow.playRecord(); //???
}

/////////////EVENT///////////////
void Game::handleEvents(){
	//First of all let the player handle input.
	player.handleInput(&shadow);

	//Check for an SDL_QUIT event.
	if(event.type==SDL_QUIT){
		//We need to quit so enter STATE_EXIT.
		setNextState(STATE_EXIT);
	}

	//Check for the escape key.
	if(inputMgr.isKeyUpEvent(INPUTMGR_ESCAPE)){
		//Escape means we go one level up, to the level select state.
		setNextState(STATE_LEVEL_SELECT);
		//Save the progress.
		levels->saveLevelProgress();

		//And change the music back to the menu music.
		getMusicManager()->playMusic("menu");
	}

	//Check if 'r' is pressed.
	if(inputMgr.isKeyDownEvent(INPUTMGR_RESTART)){
		//Only set isReset true if this isn't a replay.
		if(!(player.isPlayFromRecord() && !interlevel))
			isReset=true;
	}

	//Check for the next level buttons when in the interlevel popup.
	if(inputMgr.isKeyDownEvent(INPUTMGR_SPACE) || (event.type==SDL_KEYDOWN && (event.key.keysym.sym==SDLK_RETURN || event.key.keysym.sym==SDLK_RCTRL))){
		if(interlevel){
			//The interlevel popup is shown so we need to delete it.
			if(GUIObjectRoot){
				delete GUIObjectRoot;
				GUIObjectRoot=NULL;
			}

			//Now goto the next level.
			gotoNextLevel();
		}
	}

	//Check if tab is pressed.
	if(inputMgr.isKeyDownEvent(INPUTMGR_TAB)){
		//Switch the camera mode.
		switch(cameraMode){
			case CAMERA_PLAYER:
				cameraMode=CAMERA_SHADOW;
				break;
			case CAMERA_SHADOW:
			case CAMERA_CUSTOM:
				cameraMode=CAMERA_PLAYER;
				break;
		}
	}
}

/////////////////LOGIC///////////////////
void Game::logic(){
	//Check if we should save/load state.
	if(saveStateNextTime){
		saveState();
	}else if(loadStateNextTime){
		loadState();
	}
	saveStateNextTime=false;
	loadStateNextTime=false;

	//Add one tick to the time.
	time++;

	//First prepare each gameObject for the new frame.
	//This includes resetting dx/dy and xVel/yVel.
	for(unsigned int o=0;o<levelObjects.size();o++)
		levelObjects[o]->prepareFrame();

	//Process any event in the queue.
	for(unsigned int idx=0;idx<eventQueue.size();idx++){
		//Get the event from the queue.
		typeGameObjectEvent &e=eventQueue[idx];

		//Check if the it has an id attached to it.
		if(e.target){
			//NOTE: Should we check if the target still exists???
			e.target->onEvent(e.eventType);
		}else if(e.flags|1){
			//Loop through the levelObjects and give them the event if they have the right id.
			for(unsigned int i=0;i<levelObjects.size();i++){
				if(e.objectType<0 || levelObjects[i]->type==e.objectType){
					if(levelObjects[i]->id==e.id){
						levelObjects[i]->onEvent(e.eventType);
					}
				}
			}
		}else{
			//Loop through the levelObjects and give them the event.
			for(unsigned int i=0;i<levelObjects.size();i++){
				if(e.objectType<0 || levelObjects[i]->type==e.objectType){
					levelObjects[i]->onEvent(e.eventType);
				}
			}
		}
	}
	//Done processing the events so clear the queue.
	eventQueue.clear();
	//Loop through the gameobjects to update them.
	for(unsigned int i=0;i<levelObjects.size();i++){
		//Send GameObjectEvent_OnEnterFrame event to the script
		levelObjects[i]->onEvent(GameObjectEvent_OnEnterFrame);
	}
	for(unsigned int i=0;i<levelObjects.size();i++){
		//Let the gameobject handle movement.
		levelObjects[i]->move();
	}
	//Also update the scenery.
	{
		std::map<std::string,std::vector<Scenery*> >::iterator it;
		for(it=backgroundLayers.begin();it!=backgroundLayers.end();++it){
			for(unsigned int i=0;i<it->second.size();i++)
				it->second[i]->move();
		}
	}

	//Let the player store his move, if recording.
	player.shadowSetState();
	//Let the player give his recording to the shadow, if configured.
	player.shadowGiveState(&shadow);
	//Let him move.
	player.move(levelObjects);

	//Now let the shadow decide his move, if he's playing a recording.
	shadow.moveLogic();
	//Let the shadow move.
	shadow.move(levelObjects);

	//Check collision and stuff for the shadow and player.
	player.otherCheck(&shadow);

	//Update the camera.
	switch(cameraMode){
		case CAMERA_PLAYER:
			player.setMyCamera();
			break;
		case CAMERA_SHADOW:
			shadow.setMyCamera();
			break;
		case CAMERA_CUSTOM:
			//NOTE: The target is (should be) screen size independent so calculate the real target x and y here. 
			int targetX=cameraTarget.x-(SCREEN_WIDTH/2);
			int targetY=cameraTarget.y-(SCREEN_HEIGHT/2);
			//Move the camera to the cameraTarget.
			if(camera.x>targetX){
				camera.x-=(camera.x-targetX)>>4;
				//Make sure we don't go too far.
				if(camera.x<targetX)
					camera.x=targetX;
			}else if(camera.x<targetX){
				camera.x+=(targetX-camera.x)>>4;
				//Make sure we don't go too far.
				if(camera.x>targetX)
					camera.x=targetX;
			}
			if(camera.y>targetY){
				camera.y-=(camera.y-targetY)>>4;
				//Make sure we don't go too far.
				if(camera.y<targetY)
					camera.y=targetY;
			}else if(camera.y<targetY){
				camera.y+=(targetY-camera.y)>>4;
				//Make sure we don't go too far.
				if(camera.y>targetY)
					camera.y=targetY;
			}
			break;
	}

	//Check if we won.
	if(won){
		//Check if it's playing from record
		if(player.isPlayFromRecord() && !interlevel){
			recordingEnded();
		}else{
			//the string to store auto-save record path.
			string bestTimeFilePath,bestRecordingFilePath;
			//and if we can't get test path.
			bool filePathError=false;

			//Get current level
			LevelPack::Level *level=levels->getLevel();

			//Now check if we should update statistics
			{
				//Get previous and current medal
				int oldMedal=level->won?1:0,newMedal=1;

				int bestTime=level->time;
				int targetTime=level->targetTime;
				int bestRecordings=level->recordings;
				int targetRecordings=level->targetRecordings;

				if(oldMedal){
					if(targetTime<0){
						oldMedal=3;
					}else{
						if(targetTime<0 || bestTime<=targetTime)
							oldMedal++;
						if(targetRecordings<0 || bestRecordings<=targetRecordings)
							oldMedal++;
					}
				}else{
					bestTime=time;
					bestRecordings=recordings;
				}

				if(bestTime==-1 || bestTime>time) bestTime=time;
				if(bestRecordings==-1 || bestRecordings>recordings) bestRecordings=recordings;

				if(targetTime<0){
					newMedal=3;
				}else{
					if(targetTime<0 || bestTime<=targetTime)
						newMedal++;
					if(targetRecordings<0 || bestRecordings<=targetRecordings)
						newMedal++;
				}

				//Check if we need to update statistics
				if(newMedal>oldMedal){
					switch(oldMedal){
					case 0:
						statsMgr.completedLevels++;
						break;
					case 2:
						statsMgr.silverLevels--;
						break;
					}

					switch(newMedal){
					case 2:
						statsMgr.silverLevels++;
						break;
					case 3:
						statsMgr.goldLevels++;
						break;
					}
				}
			}

			//Set the current level won.
			level->won=true;
			if(level->time==-1 || level->time>time){
				level->time=time;
				//save the best-time game record.
				if(bestTimeFilePath.empty()){
					getCurrentLevelAutoSaveRecordPath(bestTimeFilePath,bestRecordingFilePath,true);
				}
				if(bestTimeFilePath.empty()){
					cout<<"ERROR: Couldn't get auto-save record file path"<<endl;
					filePathError=true;
				}else{
					saveRecord(bestTimeFilePath.c_str());
				}
			}
			if(level->recordings==-1 || level->recordings>recordings){
				level->recordings=recordings;
				//save the best-recordings game record.
				if(bestRecordingFilePath.empty() && !filePathError){
					getCurrentLevelAutoSaveRecordPath(bestTimeFilePath,bestRecordingFilePath,true);
				}
				if(bestRecordingFilePath.empty()){
					cout<<"ERROR: Couldn't get auto-save record file path"<<endl;
					filePathError=true;
				}else{
					saveRecord(bestRecordingFilePath.c_str());
				}
			}

			//Set the next level unlocked if it exists.
			if(levels->getCurrentLevel()+1<levels->getLevelCount()){
				levels->setLocked(levels->getCurrentLevel()+1);
			}
			//And save the progress.
			levels->saveLevelProgress();

			//Now go to the interlevel screen.
			replayPlay();

			//Update achievements
			if(levels->levelpackName=="tutorial") statsMgr.updateTutorialAchievements();
			statsMgr.updateLevelAchievements();

			//NOTE: We set isReset false to prevent the user from getting a best time of 0.00s and 0 recordings.
		}
	}
	won=false;

	//Check if we should reset.
	if(isReset)
		//NOTE: In case of the interlevel popup the save data needs to be deleted so the restart behaviour is the same for key and button restart.
		reset(interlevel); 
	isReset=false;
}

/////////////////RENDER//////////////////
void Game::render(){
	//First of all render the background.
	{
		//Get a pointer to the background.
		ThemeBackground* bg=background;

		//Check if the background is null, but there are themes.
		if(bg==NULL && objThemes.themeCount()>0){
			//Get the background from the first theme in the stack.
			bg=objThemes[0]->getBackground(false);
		}

		//Check if the background isn't null.
		if(bg){
			//It isn't so draw it.
			bg->draw(screen);

			//And if it's the loaded background then also update the animation.
			//FIXME: Updating the animation in the render method?
			if(bg==background)
				bg->updateAnimation();
		}else{
			//There's no background so fill the screen with white.
			SDL_Rect r={0,0,SCREEN_WIDTH,SCREEN_HEIGHT};
			SDL_FillRect(screen,&r,-1);
		}
	}

	//Now draw the backgroundLayers.
	std::map<std::string,std::vector<Scenery*> >::iterator it;
	for(it=backgroundLayers.begin();it!=backgroundLayers.end();++it){
		for(unsigned int i=0;i<it->second.size();i++)
			it->second[i]->show();
	}

	//Now we draw the levelObjects.
	for(unsigned int o=0; o<levelObjects.size(); o++){
		levelObjects[o]->show();
	}

	//Followed by the player and the shadow.
	//NOTE: We draw the shadow second because he needs to be behind the player.
	shadow.show();
	player.show();

	//Show the levelName if it isn't the level editor.
	if(stateID!=STATE_LEVEL_EDITOR && bmTips[0]!=NULL && !interlevel){
		drawGUIBox(-2,SCREEN_HEIGHT-bmTips[0]->h-4,bmTips[0]->w+8,bmTips[0]->h+6,screen,0xFFFFFFFF);
		applySurface(2,SCREEN_HEIGHT-bmTips[0]->h,bmTips[0],screen,NULL);
	}

	//Check if there's a tooltip.
	//NOTE: gameTipIndex 0 is used for the levelName, 1 for shadow death, 2 for restart text, 3 for restart+checkpoint.
	if(gameTipIndex>3 && gameTipIndex<TYPE_MAX){
		//Check if there's a tooltip for the type.
		if(bmTips[gameTipIndex]==NULL){
			//There isn't thus make it.
			string s;
			string keyCode=_(inputMgr.getKeyCodeName(inputMgr.getKeyCode(INPUTMGR_ACTION,false)));
			transform(keyCode.begin(),keyCode.end(),keyCode.begin(),::toupper);
			switch(gameTipIndex){
			case TYPE_CHECKPOINT:
				/// TRANSLATORS: Please do not remove %s from your translation:
				///  - %s will be replaced with current action key
				s=tfm::format(_("Press %s key to save the game."),keyCode);
				break;
			case TYPE_SWAP:
				/// TRANSLATORS: Please do not remove %s from your translation:
				///  - %s will be replaced with current action key
				s=tfm::format(_("Press %s key to swap the position of player and shadow."),keyCode);
				break;
			case TYPE_SWITCH:
				/// TRANSLATORS: Please do not remove %s from your translation:
				///  - %s will be replaced with current action key
				s=tfm::format(_("Press %s key to activate the switch."),keyCode);
				break;
			case TYPE_PORTAL:
				/// TRANSLATORS: Please do not remove %s from your translation:
				///  - %s will be replaced with current action key
				s=tfm::format(_("Press %s key to teleport."),keyCode);
				break;
			}

			//If we have a string then it's a supported GameObject type.
			if(!s.empty()){
				SDL_Color fg={0,0,0,0};
				bmTips[gameTipIndex]=TTF_RenderUTF8_Blended(fontText,s.c_str(),fg);
			}
		}

		//We already have a gameTip for this type so draw it.
		if(bmTips[gameTipIndex]!=NULL){
			drawGUIBox(-2,-2,bmTips[gameTipIndex]->w+8,bmTips[gameTipIndex]->h+6,screen,0xFFFFFFFF);
			applySurface(2,2,bmTips[gameTipIndex],screen,NULL);
		}
	}
	//Set the gameTip to 0.
	gameTipIndex=0;

	//Pointer to the sdl surface that will contain a message, if any.
	SDL_Surface* bm=NULL;

	//Check if the player is dead, meaning we draw a message.
	if(player.dead){
		//Get user configured restart key
		string keyCodeRestart=inputMgr.getKeyCodeName(inputMgr.getKeyCode(INPUTMGR_RESTART,false));
		transform(keyCodeRestart.begin(),keyCodeRestart.end(),keyCodeRestart.begin(),::toupper);
		//The player is dead, check if there's a state that can be loaded.
		if(player.canLoadState()){
			//Now check if the tip is already made, if not make it.
			if(bmTips[3]==NULL){
				//Get user defined key for loading checkpoint
				string keyCodeLoad=inputMgr.getKeyCodeName(inputMgr.getKeyCode(INPUTMGR_LOAD,false));
				transform(keyCodeLoad.begin(),keyCodeLoad.end(),keyCodeLoad.begin(),::toupper);
				//Draw string
				SDL_Color fg={0,0,0,0},bg={255,255,255,0};
				bmTips[3]=TTF_RenderUTF8_Blended(fontText,
					/// TRANSLATORS: Please do not remove %s from your translation:
					///  - first %s means currently configured key to restart game
					///  - Second %s means configured key to load from last save
					tfm::format(_("Press %s to restart current level or press %s to load the game."),
						keyCodeRestart,keyCodeLoad).c_str(),
					fg);
			}
			bm=bmTips[3];
		}else{
			//Now check if the tip is already made, if not make it.
			if(bmTips[2]==NULL){
				SDL_Color fg={0,0,0,0};
				bmTips[2]=TTF_RenderUTF8_Blended(fontText,
					/// TRANSLATORS: Please do not remove %s from your translation:
					///  - %s will be replaced with currently configured key to restart game
					tfm::format(_("Press %s to restart current level."),keyCodeRestart).c_str(),
					fg);
			}
			bm=bmTips[2];
		}
	}

	//Check if the shadow has died (and there's no other notification).
	//NOTE: We use the shadow's jumptime as countdown, this variable isn't used when the shadow is dead.
	if(shadow.dead && bm==NULL && shadow.jumpTime>0){
		//Now check if the tip is already made, if not make it.
		if(bmTips[1]==NULL){
			SDL_Color fg={0,0,0,0},bg={255,255,255,0};
			bmTips[1]=TTF_RenderUTF8_Blended(fontText,
				_("Your shadow has died."),
				fg);
		}
		bm=bmTips[1];

		//NOTE: Logic in the render loop, we substract the shadow's jumptime by one.
		shadow.jumpTime--;
		
		//return view to player and keep it there
		cameraMode=CAMERA_PLAYER;
	}

	//Draw the tip.
	if(bm!=NULL){
		int x=(SCREEN_WIDTH-bm->w)/2;
		int y=32;
		drawGUIBox(x-8,y-8,bm->w+16,bm->h+14,screen,0xFFFFFFFF);
		applySurface(x,y,bm,screen,NULL);
	}

	//Show the number of collectables the user has collected if there are collectables in the level.
	//We hide this when interlevel.
	if(currentCollectables<=totalCollectables && totalCollectables!=0 && !interlevel && time>0){
		//Temp stringstream just to addup all the text nicely
		stringstream temp;
		temp << currentCollectables << "/" << totalCollectables;

		SDL_Rect r;
		SDL_Surface* bm=TTF_RenderText_Blended(fontText,temp.str().c_str(),themeTextColorDialog);

		//Align the text properly
		r.x=SCREEN_WIDTH-50-bm->w+22;
		r.y=SCREEN_HEIGHT-bm->h;
		
		//Draw background
		drawGUIBox(SCREEN_WIDTH-bm->w-34,SCREEN_HEIGHT-bm->h-4,bm->w+34+2,bm->h+4+2,screen,0xFFFFFFFF);
		
		//Draw the collectable icon
		collectable.draw(screen,SCREEN_WIDTH-50+12,SCREEN_HEIGHT-50+10,NULL);
		
		//Draw text
		SDL_BlitSurface(bm,NULL,screen,&r);
		SDL_FreeSurface(bm);
	}

	//show time and records used in level editor.
	if(stateID==STATE_LEVEL_EDITOR && time>0){
		SDL_Color fg={0,0,0,0},bg={255,255,255,0};
		SDL_Surface *bm;
		int y=SCREEN_HEIGHT;

		bm=TTF_RenderUTF8_Shaded(fontText,tfm::format(_("%d recordings"),recordings).c_str(),fg,bg);
		SDL_SetAlpha(bm,SDL_SRCALPHA,160);
		y-=bm->h;
		applySurface(0,y,bm,screen,NULL);
		SDL_FreeSurface(bm);

		char c[32];
		sprintf(c,"%-.2fs",time/40.0f);
		bm=TTF_RenderUTF8_Shaded(fontText,c,fg,bg);
		SDL_SetAlpha(bm,SDL_SRCALPHA,160);
		y-=bm->h;
		applySurface(0,y,bm,screen,NULL);
		SDL_FreeSurface(bm);
	}

	//Draw the current action in the upper right corner.
	if(player.record){
		applySurface(SCREEN_WIDTH-50,0,action,screen,NULL);
	}else if(shadow.state!=0){
		SDL_Rect r={50,0,50,50};
		applySurface(SCREEN_WIDTH-50,0,action,screen,&r);
	}

	//if the game is play from record then draw something indicates it
	if(player.isPlayFromRecord()){
		//Dim the screen if interlevel is true.
		if(interlevel){
			SDL_BlitSurface(screen,NULL,tempSurface,NULL);
			SDL_FillRect(screen,NULL,0);
			SDL_SetAlpha(tempSurface, SDL_SRCALPHA,191);
			SDL_BlitSurface(tempSurface,NULL,screen,NULL);

			//Check if the GUI isn't null.
			if(GUIObjectRoot){
				//==Create first box==

				//Create the title
				SDL_Rect r;
				/// TRANSLATORS: This is caption for finished level
				SDL_Surface* bm=TTF_RenderUTF8_Blended(fontGUI,_("You've finished:"),themeTextColorDialog);

				//Recreate the level string.
				string s;
				if (levels->getLevelCount()>0){
					/// TRANSLATORS: Please do not remove %s or %d from your translation:
					///  - %d means the level number in a levelpack
					///  - %s means the name of current level
					s=tfm::format(_("Level %d %s"),levels->getCurrentLevel()+1,_CC(levels->getDictionaryManager(),levelName));
				}

				SDL_Surface* bm2=TTF_RenderUTF8_Blended(fontText,s.c_str(),themeTextColorDialog);

				//Now draw the first gui box so that it's bigger than longer text.
				int width;
				if(bm->w>bm2->w)
					width=bm->w+32;
				else
					width=bm2->w+32;
				drawGUIBox((SCREEN_WIDTH-width)/2,4,width,68,screen,0xFFFFFFBF);

				// Now draw title.
				r.x=(SCREEN_WIDTH-bm->w)/2;
				r.y=8-GUI_FONT_RAISE;
				SDL_BlitSurface(bm,NULL,screen,&r);

				// And then level name.
				r.x=(SCREEN_WIDTH-bm2->w)/2;
				r.y=44;
				SDL_BlitSurface(bm2,NULL,screen,&r);

				//Free drawed texts
				SDL_FreeSurface(bm);
				SDL_FreeSurface(bm2);

				//==Create second box==

				//Now draw the second gui box.
				drawGUIBox(GUIObjectRoot->left,GUIObjectRoot->top,GUIObjectRoot->width,GUIObjectRoot->height,screen,0xFFFFFFBF);

				//Draw the medal.
				int medal=GUIObjectRoot->value;
				r.x=(medal-1)*30;
				r.y=0;
				r.w=30;
				r.h=30;
				applySurface(GUIObjectRoot->left+16,GUIObjectRoot->top+92,medals,screen,&r);
				applySurface(GUIObjectRoot->left+medalX,GUIObjectRoot->top+92,medals,screen,&r);
			}
		}else if((time & 0x10)==0x10){
			SDL_Rect r={50,0,50,50};
			applySurface(0,0,action,screen,&r);
			applySurface(0,SCREEN_HEIGHT-50,action,screen,&r);
			applySurface(SCREEN_WIDTH-50,SCREEN_HEIGHT-50,action,screen,&r);
		}
	}else if(player.objNotificationBlock){
		//If the player is in front of a notification block show the message.
		//And it isn't a replay.
		std::string &untranslated_message=player.objNotificationBlock->message;
		std::string message=_CC(levels->getDictionaryManager(),untranslated_message);
		std::vector<char> string_data(message.begin(), message.end());
		string_data.push_back('\0');

		int maxWidth = 0;
		int y = 20;
		vector<SDL_Surface*> lines;

		//Now process the prompt.
		{
			//Pointer to the string.
			char* lps=&string_data[0];
			//Pointer to a character.
			char* lp=NULL;

			//We keep looping forever.
			//The only way out is with the break statement.
			for(;;){
				//As long as it's still the same sentence we continue.
				//It will stop when there's a newline or end of line.
				for(lp=lps;*lp!='\n'&&*lp!='\r'&&*lp!=0;lp++);

				//Store the character we stopped on. (End or newline)
				char c=*lp;
				//Set the character in the string to 0, making lps a string containing one sentence.
				*lp=0;

				//Integer used to center the sentence horizontally.
				int x;
				TTF_SizeText(fontText,lps,&x,NULL);
				
				//Find out largest width
				if(x>maxWidth)
					maxWidth=x;
				
				x=(SCREEN_WIDTH-x)/2;

				lines.push_back(TTF_RenderUTF8_Blended(fontText,lps,themeTextColorDialog));
				//Increase y with 25, about the height of the text.
				y+=25;

				//Check the stored character if it was a stop.
				if(c==0){
					//It was so break out of the for loop.
					lps=lp;
					break;
				}
				//It wasn't meaning more will follow.
				//We set lps to point after the "newline" forming a new string.
				lps=lp+1;
			}
		}
		maxWidth+=SCREEN_WIDTH*0.15;
		drawGUIBox((SCREEN_WIDTH-maxWidth)/2,SCREEN_HEIGHT-y-25,maxWidth,y+20,screen,0xFFFFFFBF);
		while(!lines.empty()){
			SDL_Surface* bm=lines[0];

			if(bm!=NULL){
				applySurface(100+((SCREEN_WIDTH-200-bm->w)/2),SCREEN_HEIGHT-y,bm,screen,NULL);
				SDL_FreeSurface(bm);
			}

			y-=25;
			lines.erase(lines.begin());

		}
	}
}

void Game::resize(){
	//Check if the interlevel popup is shown.
	if(interlevel && GUIObjectRoot){
		GUIObjectRoot->left=(SCREEN_WIDTH-GUIObjectRoot->width)/2;
	}
}

void Game::replayPlay(){
	//Reset the number of collectables
	currentCollectables=currentCollectablesSaved=0;
	  
	//We show the interlevel popup so interlevel must be true.
	interlevel=true;
	
	//Make the cursor visible when the interlevel popup is up.
	SDL_ShowCursor(SDL_ENABLE);
	
	//Create the gui if it isn't already done.
	if(!GUIObjectRoot){
		GUIObjectRoot=new GUIObject(0,SCREEN_HEIGHT-140,570,135,GUIObjectNone);
		//NOTE: We put the medal in the value of the GUIObjectRoot.
		
		//Make child widgets change color properly according to theme.
		GUIObjectRoot->inDialog=true;

		//The different values.
		int bestTime=levels->getLevel()->time;
		int targetTime=levels->getLevel()->targetTime;
		int bestRecordings=levels->getLevel()->recordings;
		int targetRecordings=levels->getLevel()->targetRecordings;

		int medal=1;
		if(targetTime<0){
			medal=3;
		}else{
			if(targetTime<0 || bestTime<=targetTime)
				medal++;
			if(targetRecordings<0 || bestRecordings<=targetRecordings)
				medal++;
		}
		//Add it to the GUIObjectRoot.
		GUIObjectRoot->value=medal;
		
		int maxWidth=0;
		int x=20;
		
		//Is there a target time for this level?
		int timeY=0;
		bool isTargetTime=true;
		if(targetTime<=0){
			isTargetTime=false;
			timeY=12;
		}

		//Create the labels with the time and best time.
		/// TRANSLATORS: Please do not remove %-.2f from your translation:
		///  - %-.2f means time in seconds
		///  - s is shortened form of a second. Try to keep it so.
		GUIObject* obj=new GUIObject(x,10+timeY,-1,36,GUIObjectLabel,tfm::format(_("Time: %-.2fs"),time/40.0f).c_str());
		GUIObjectRoot->addChild(obj);
		
		obj->render(0,0,false);
		maxWidth=obj->width;

		/// TRANSLATORS: Please do not remove %-.2f from your translation:
		///  - %-.2f means time in seconds
		///  - s is shortened form of a second. Try to keep it so.
		obj=new GUIObject(x,34+timeY,-1,36,GUIObjectLabel,tfm::format(_("Best time: %-.2fs"),bestTime/40.0f).c_str());
		GUIObjectRoot->addChild(obj);
		
		obj->render(0,0,false);
		if(obj->width>maxWidth)
			maxWidth=obj->width;

		/// TRANSLATORS: Please do not remove %-.2f from your translation:
		///  - %-.2f means time in seconds
		///  - s is shortened form of a second. Try to keep it so.
		if(isTargetTime){
			obj=new GUIObject(x,58,-1,36,GUIObjectLabel,tfm::format(_("Target time: %-.2fs"),targetTime/40.0f).c_str());
			GUIObjectRoot->addChild(obj);
			
			obj->render(0,0,false);
			if(obj->width>maxWidth)
				maxWidth=obj->width;
		}
		
		x+=maxWidth+20;
		
		//Is there target recordings for this level?
		int recsY=0;
		bool isTargetRecs=true;
		if(targetRecordings<0){
			isTargetRecs=false;
			recsY=12;
		}

		//Now the ones for the recordings.
		/// TRANSLATORS: Please do not remove %d from your translation:
		///  - %d means the number of recordings user has made
		obj=new GUIObject(x,10+recsY,-1,36,GUIObjectLabel,tfm::format(_("Recordings: %d"),recordings).c_str());
		GUIObjectRoot->addChild(obj);
		
		obj->render(0,0,false);
		maxWidth=obj->width;

		/// TRANSLATORS: Please do not remove %d from your translation:
		///  - %d means the number of recordings user has made
		obj=new GUIObject(x,34+recsY,-1,36,GUIObjectLabel,tfm::format(_("Best recordings: %d"),bestRecordings).c_str());
		GUIObjectRoot->addChild(obj);
		
		obj->render(0,0,false);
		if(obj->width>maxWidth)
			maxWidth=obj->width;

		/// TRANSLATORS: Please do not remove %d from your translation:
		///  - %d means the number of recordings user has made
		if(isTargetRecs){
			obj=new GUIObject(x,58,-1,36,GUIObjectLabel,tfm::format(_("Target recordings: %d"),targetRecordings).c_str());
			GUIObjectRoot->addChild(obj);
			
			obj->render(0,0,false);
			if(obj->width>maxWidth)
				maxWidth=obj->width;
		}
		
		x+=maxWidth;

		//The medal that is earned.
		/// TRANSLATORS: Please do not remove %s from your translation:
		///  - %s will be replaced with name of a prize medal (gold, silver or bronze)
		string s1=tfm::format(_("You earned the %s medal"),(medal>1)?(medal==3)?_("GOLD"):_("SILVER"):_("BRONZE"));
		obj=new GUIObject(50,92,-1,36,GUIObjectLabel,s1.c_str(),0,true,true,GUIGravityCenter);
		GUIObjectRoot->addChild(obj);
		
		obj->render(0,0,false);
		if(obj->left+obj->width>x){
			x=obj->left+obj->width+30;
		}else{
			obj->left=20+(x-20-obj->width)/2;
		}
		
		medalX=x-24;

		//Create the three buttons, Menu, Restart, Next.
		/// TRANSLATORS: used as return to the level selector menu
		GUIObject* b1=new GUIObject(x,10,-1,36,GUIObjectButton,_("Menu"),0,true,true,GUIGravityCenter);
		b1->name="cmdMenu";
		b1->eventCallback=this;
		GUIObjectRoot->addChild(b1);
		b1->render(0,0,true);

		/// TRANSLATORS: used as restart level
		GUIObject* b2=new GUIObject(x,50,-1,36,GUIObjectButton,_("Restart"),0,true,true,GUIGravityCenter);
		b2->name="cmdRestart";
		b2->eventCallback=this;
		GUIObjectRoot->addChild(b2);
		b2->render(0,0,true);

		/// TRANSLATORS: used as next level
		GUIObject* b3=new GUIObject(x,90,-1,36,GUIObjectButton,_("Next"),0,true,true,GUIGravityCenter);
		b3->name="cmdNext";
		b3->eventCallback=this;
		GUIObjectRoot->addChild(b3);
		b3->render(0,0,true);
		
		maxWidth=b1->width;
		if(b2->width>maxWidth)
			maxWidth=b2->width;
		if(b3->width>maxWidth)
			maxWidth=b3->width;
		
		b1->left=b2->left=b3->left=x+maxWidth/2;
		
		x+=maxWidth;
		GUIObjectRoot->width=x;
		GUIObjectRoot->left=(SCREEN_WIDTH-GUIObjectRoot->width)/2;
	}

	//We only need to reset a few things so we don't call reset().
	for(unsigned int i=0;i<levelObjects.size();i++){
		levelObjects[i]->reset(true);
	}
	//Also reset the background animation, if any.
	if(background)
		background->resetAnimation(true);
	//The script environment.
	getScriptExecutor()->reset();
	//And call the onCreate scripts.
	for(unsigned int i=0;i<levelObjects.size();i++){
		levelObjects[i]->onEvent(GameObjectEvent_OnCreate);
	}
	
	//Close exit(s) if there are any collectables
	if(totalCollectables>0){
		for(unsigned int i=0;i<levelObjects.size();i++){
			if(levelObjects[i]->type==TYPE_EXIT){
				levelObjects[i]->onEvent(GameObjectEvent_OnSwitchOff);
			}
		}
	}

	//Execute the onCreate event, if any.
	//NOTE: Do we need an onReset event???
	//executeScript(LevelEvent_OnReset);
	executeScript(LevelEvent_OnCreate);

	//Make a copy of the playerButtons.
	vector<int> recordCopy=player.recordButton;
	player.reset(true);
	shadow.reset(true);
	player.recordButton=recordCopy;

	//Now play the recording.
	player.playRecord();
}

void Game::recordingEnded(){
	//Check if it's a normal replay, if so just stop.
	if(!interlevel){
		//Show the cursor so that the user can press the ok button.
		SDL_ShowCursor(SDL_ENABLE);
		//Now show the message box.
		msgBox(_("Game replay is done."),MsgBoxOKOnly,_("Game Replay"));
		//Go to the level select menu.
		setNextState(STATE_LEVEL_SELECT);

		//And change the music back to the menu music.
		getMusicManager()->playMusic("menu");
	}else{
		//Instead of directly replaying we set won true to let the Game handle the replaying at the end of the update cycle.
		won=true;
	}
}

bool Game::canSaveState(){
	return (player.canSaveState() && shadow.canSaveState());
}

bool Game::saveState(){
	//Check if the player and shadow can save the current state.
	if(canSaveState()){
		//Let the player and the shadow save their state.
		player.saveState();
		shadow.saveState();

		//Save the stats.
		timeSaved=time;
		recordingsSaved=recordings;
		recentSwapSaved=recentSwap;

		//Save the camera mode and target.
		cameraModeSaved=cameraMode;
		cameraTargetSaved=cameraTarget;

		//Save the current collectables
		currentCollectablesSaved=currentCollectables;

		//Save other state, for example moving blocks.
		for(unsigned int i=0;i<levelObjects.size();i++){
			levelObjects[i]->saveState();
		}

		//Also save the background animation, if any.
		if(background)
			background->saveAnimation();

		if(!player.isPlayFromRecord() && !interlevel){
			//Update achievements
			Uint32 t=SDL_GetTicks()+5000; //Add a bias to prevent bugs
			if(recentSave+1000>t){
				statsMgr.newAchievement("panicSave");
			}
			recentSave=t;

			//Update statistics.
			statsMgr.saveTimes++;
			
			//Update achievements
			switch(statsMgr.saveTimes){
			case 1000:
				statsMgr.newAchievement("save1k");
				break;
			}
		}

		//Execute the onSave event.
		executeScript(LevelEvent_OnSave);

		//Return true.
		return true;
	}

	//We can't save the state so return false.
	return false;
}

bool Game::loadState(){
	//Check if there's a state that can be loaded.
	if(player.canLoadState() && shadow.canLoadState()){
		//Let the player and the shadow load their state.
		player.loadState();
		shadow.loadState();

		//Load the stats.
		time=timeSaved;
		recordings=recordingsSaved;
		recentSwap=recentSwapSaved;

		//Load the camera mode and target.
		cameraMode=cameraModeSaved;
		cameraTarget=cameraTargetSaved;

		//Load the current collactbles
		currentCollectables=currentCollectablesSaved;

		//Load other state, for example moving blocks.
		for(unsigned int i=0;i<levelObjects.size();i++){
			levelObjects[i]->loadState();
		}

		//Also load the background animation, if any.
		if(background)
			background->loadAnimation();

		if(!player.isPlayFromRecord() && !interlevel){
			//Update achievements.
			Uint32 t=SDL_GetTicks()+5000; //Add a bias to prevent bugs
			if(recentLoad+1000>t){
				statsMgr.newAchievement("panicLoad");
			}
			recentLoad=t;

			//Update statistics.
			statsMgr.loadTimes++;
			
			//Update achievements
			switch(statsMgr.loadTimes){
			case 1000:
				statsMgr.newAchievement("load1k");
				break;
			}
		}

		//Execute the onLoad event, if any.
		executeScript(LevelEvent_OnLoad);

		//Return true.
		return true;
	}

	//We can't load the state so return false.
	return false;
}

void Game::reset(bool save){
	//We need to reset the game so we also reset the player and the shadow.
	player.reset(save);
	shadow.reset(save);

	saveStateNextTime=false;
	loadStateNextTime=false;

	//Reset the stats.
	time=0;
	recordings=0;

	recentSwap=-10000;
	if(save) recentSwapSaved=-10000;

	//Reset the camera.
	cameraMode=CAMERA_PLAYER;
	if(save) cameraModeSaved=CAMERA_PLAYER;
	cameraTarget.x=cameraTarget.y=cameraTarget.w=cameraTarget.h=0;
	if(save) cameraTargetSaved.x=cameraTargetSaved.y=cameraTargetSaved.w=cameraTargetSaved.h=0;

	//Reset the number of collectables
	currentCollectables=0;
	if(save)
		currentCollectablesSaved=0;

	//There is no last checkpoint so set it to NULL.
	if(save)
		objLastCheckPoint=NULL;

	//Clear the event queue, since all the events are from before the reset.
	eventQueue.clear();

	//Reset other state, for example moving blocks.
	for(unsigned int i=0;i<levelObjects.size();i++){
		levelObjects[i]->reset(save);
	}
	//Also reset the background animation, if any.
	if(background)
		background->resetAnimation(save);

	//Reset the script environment
	//NOTE: The scriptExecutor will only be reset between levels.
	//getScriptExecutor()->reset();

	//Send GameObjectEvent_OnCreate event to the script
	for(unsigned int i=0;i<levelObjects.size();i++){
		levelObjects[i]->onEvent(GameObjectEvent_OnCreate);
	}
		
	//Close exit(s) if there are any collectables
	if(totalCollectables>0){
		for(unsigned int i=0;i<levelObjects.size();i++){
			if(levelObjects[i]->type==TYPE_EXIT){
				levelObjects[i]->onEvent(GameObjectEvent_OnSwitchOff);
			}
		}
	}

	//Execute the onCreate event, if any.
	//NOTE: Do we need an onReset event???
	//executeScript(LevelEvent_OnReset);
	executeScript(LevelEvent_OnCreate);

	//Check if interlevel is true, if so we might need to delete the gui.
	if(interlevel){
		if(GUIObjectRoot){
			delete GUIObjectRoot;
			GUIObjectRoot=NULL;
		}
	}

	//Also set interlevel to false.
	interlevel=false;
	
	//Hide the cursor (if not the leveleditor).
	if(stateID!=STATE_LEVEL_EDITOR)
		SDL_ShowCursor(SDL_DISABLE);
}

void Game::executeScript(int eventType){
	map<int,string>::iterator it;
	
	//Check if there's a script for the given event.
	it=scripts.find(eventType);
	if(it!=scripts.end()){
		//There is one so execute it.
		getScriptExecutor()->executeScript(it->second);
	}
}

void Game::broadcastObjectEvent(int eventType,int objectType,const char* id,GameObject* target){
	//Create a typeGameObjectEvent that can be put into the queue.
	typeGameObjectEvent e;
	//Set the event type.
	e.eventType=eventType;
	//Set the object type.
	e.objectType=objectType;
	//By default flags=0.
	e.flags=0;
	//Unless there's an id.
	if(id){
		//Set flags to 0x1 and set the id.
		e.flags|=1;
		e.id=id;
	}
	//Or there's a target given.
	if(target)
		e.target=target;
	else
		e.target=NULL;

	//Add the event to the queue.
	eventQueue.push_back(e);
}

void Game::getCurrentLevelAutoSaveRecordPath(std::string &bestTimeFilePath,std::string &bestRecordingFilePath,bool createPath){
	levels->getLevelAutoSaveRecordPath(-1,bestTimeFilePath,bestRecordingFilePath,createPath);
}

void Game::gotoNextLevel(){
	//Goto the next level.
	levels->nextLevel();

	//Check if the level exists.
	if(levels->getCurrentLevel()<levels->getLevelCount()){
		setNextState(STATE_GAME);

		//Don't forget the music.
		getMusicManager()->pickMusic();
	}else{
		if(!levels->congratulationText.empty()){
			msgBox(_CC(levels->getDictionaryManager(),levels->congratulationText),MsgBoxOKOnly,_("Congratulations"));
		}else{
			msgBox(_("You have finished the levelpack!"),MsgBoxOKOnly,_("Congratulations"));
		}
		//Now go back to the levelselect screen.
		setNextState(STATE_LEVEL_SELECT);
		//And set the music back to menu.
		getMusicManager()->playMusic("menu");
	}
}

void Game::GUIEventCallback_OnEvent(string name,GUIObject* obj,int eventType){
	if(name=="cmdMenu"){
		setNextState(STATE_LEVEL_SELECT);

		//And change the music back to the menu music.
		getMusicManager()->playMusic("menu");
	}else if(name=="cmdRestart"){
		//Clear the gui.
		if(GUIObjectRoot){
			delete GUIObjectRoot;
			GUIObjectRoot=NULL;
		}

		//And reset the game.
		//new: we don't need to clear the save game because
		//in level replay the game won't be saved
		//TODO: it doesn't work; better leave for next release
		reset(/*false*/ true);
	}else if(name=="cmdNext"){
		//No matter what, clear the gui.
		if(GUIObjectRoot){
			delete GUIObjectRoot;
			GUIObjectRoot=NULL;
		}

		//And goto the next level.
		gotoNextLevel();
	}
}
