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

#include "Block.h"
#include "GameState.h"
#include "Functions.h"
#include "GameObjects.h"
#include "ThemeManager.h"
#include "Game.h"
#include "TreeStorageNode.h"
#include "POASerializer.h"
#include "InputManager.h"
#include "MusicManager.h"
#include "Render.h"
#include "StatisticsManager.h"
#include "ScriptExecutor.h"
#include "MD5.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <algorithm>
#include <assert.h>
#include <stdio.h>
#include <SDL_ttf.h>

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

//An internal function.
static void copyCompiledScripts(lua_State *state, const std::map<int, int>& src, std::map<int, int>& dest) {
	//Clear the existing scripts.
	for (auto it = dest.begin(); it != dest.end(); ++it) {
		luaL_unref(state, LUA_REGISTRYINDEX, it->second);
	}
	dest.clear();

	//Copy the source to the destination.
	for (auto it = src.begin(); it != src.end(); ++it) {
		lua_rawgeti(state, LUA_REGISTRYINDEX, it->second);
		dest[it->first] = luaL_ref(state, LUA_REGISTRYINDEX);
	}
}

//An internal function.
static void copyLevelObjects(const std::vector<Block*>& src, std::vector<Block*>& dest, bool setActive) {
	//Clear the existing objects.
	for (auto o : dest) delete o;
	dest.clear();

	//Copy the source to the destination.
	for (auto o : src) {
		if (o == NULL || o->isDelete) continue;
		Block* o2 = new Block(*o);
		dest.push_back(o2);
		if (setActive) o2->setActive();
	}
}

Game::Game(SDL_Renderer &renderer, ImageManager &imageManager):isReset(false)
	, scriptExecutor(new ScriptExecutor())
	,currentLevelNode(NULL)
	,customTheme(NULL)
	,background(NULL)
	, levelRect(SDL_Rect{ 0, 0, 0, 0 }), levelRectSaved(SDL_Rect{ 0, 0, 0, 0 }), levelRectInitial(SDL_Rect{ 0, 0, 0, 0 })
	,won(false)
	,interlevel(false)
	,gameTipIndex(0)
	,time(0),timeSaved(0)
	,recordings(0),recordingsSaved(0)
	,cameraMode(CAMERA_PLAYER),cameraModeSaved(CAMERA_PLAYER)
	,player(this),shadow(this),objLastCheckPoint(NULL)
	, currentCollectables(0), currentCollectablesSaved(0), currentCollectablesInitial(0)
	, totalCollectables(0), totalCollectablesSaved(0), totalCollectablesInitial(0)
{

	saveStateNextTime=false;
	loadStateNextTime=false;

	recentSwap=recentSwapSaved=-10000;
	recentLoad=recentSave=0;

    action=imageManager.loadTexture(getDataPath()+"gfx/actions.png", renderer);
    medals=imageManager.loadTexture(getDataPath()+"gfx/medals.png", renderer);
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
	delete scriptExecutor;
	scriptExecutor = NULL;

	//Loop through the levelObjects, etc. and delete them.
	for (auto o : levelObjects) delete o;
	levelObjects.clear();

	for (auto o : levelObjectsSave) delete o;
	levelObjectsSave.clear();

	for (auto o : levelObjectsInitial) delete o;
	levelObjectsInitial.clear();

	//Loop through the sceneryLayers and delete them.
	for(auto it=sceneryLayers.begin();it!=sceneryLayers.end();++it){
		delete it->second;
	}
	sceneryLayers.clear();

	//Clear the name and the editor data.
	levelName.clear();
	levelFile.clear();
	editorData.clear();

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

	//Set the music list back to the configured list.
	getMusicManager()->setMusicList(getSettings()->getValue("musiclist"));
}

void Game::reloadMusic() {
	//NOTE: level music is always enabled.

	//Check if the levelpack has a prefered music list.
	if (levels && !levels->levelpackMusicList.empty())
		getMusicManager()->setMusicList(levels->levelpackMusicList);

	//Check for the music to use.
	string &s = editorData["music"];
	if (!s.empty()) {
		getMusicManager()->playMusic(s);
	} else {
		getMusicManager()->pickMusic();
	}
}

void Game::loadLevelFromNode(ImageManager& imageManager,SDL_Renderer& renderer,TreeStorageNode* obj,const string& fileName){
	//Make sure there's nothing left from any previous levels.
	assert(levelObjects.empty() && levelObjectsSave.empty() && levelObjectsInitial.empty());

	//set current level to loaded one.
	currentLevelNode=obj;

	//Set the level dimensions to the default, it will probably be changed by the editorData,
	//but 800x600 is a fallback.
	levelRect = levelRectSaved = levelRectInitial = SDL_Rect{ 0, 0, 800, 600 };

	currentCollectables = currentCollectablesSaved = currentCollectablesInitial = 0;
	totalCollectables = totalCollectablesSaved = totalCollectablesInitial = 0;

	//Load the additional data.
	for(map<string,vector<string> >::iterator i=obj->attributes.begin();i!=obj->attributes.end();++i){
		if(i->first=="size"){
			//We found the size attribute.
			if(i->second.size()>=2){
				//Set the dimensions of the level.
				int w = atoi(i->second[0].c_str()), h = atoi(i->second[1].c_str());
				levelRect = levelRectSaved = levelRectInitial = SDL_Rect{ 0, 0, w, h };
			}
		}else if(i->second.size()>0){
			//Any other data will be put into the editorData.
			editorData[i->first]=i->second[0];
		}
	}

	//Get the theme.
	{
		//NOTE: level themes are always enabled.

		//Check for bundled (partial) themes for level pack.
		if (levels->customTheme){
			if (objThemes.appendThemeFromFile(levels->levelpackPath + "/theme/theme.mnmstheme", imageManager, renderer) == NULL){
				//The theme failed to load so set the customTheme boolean to false.
				levels->customTheme = false;
			}
		}

		//Check for the theme to use for this level. This has higher priority.
		//Examples: %DATA%/themes/classic or %USER%/themes/Orange
		string &s = editorData["theme"];
		if (!s.empty()){
			customTheme = objThemes.appendThemeFromFile(processFileName(s) + "/theme.mnmstheme", imageManager, renderer);
		}

		//Set the Appearance of the player and the shadow.
		objThemes.getCharacter(false)->createInstance(&player.appearance, "standright");
		player.appearanceInitial = player.appearanceSave = player.appearance;
		objThemes.getCharacter(true)->createInstance(&shadow.appearance, "standright");
		shadow.appearanceInitial = shadow.appearanceSave = shadow.appearance;
	}

	//Get the music.
	reloadMusic();

	//Load the data from the level node.
	for(unsigned int i=0;i<obj->subNodes.size();i++){
		TreeStorageNode* obj1=obj->subNodes[i];
		if(obj1==NULL) continue;
		if(obj1->name=="tile"){
			Block* block=new Block(this);
            if(!block->loadFromNode(imageManager,renderer,obj1)){
				delete block;
				continue;
			}

			//If the type is collectable, increase the number of totalCollectables
			if (block->type == TYPE_COLLECTABLE) {
				totalCollectablesSaved = totalCollectablesInitial = ++totalCollectables;
			}

			//Add the block to the levelObjects vector.
			levelObjects.push_back(block);
		}else if(obj1->name=="scenerylayer" && obj1->value.size()==1){
			//Check if the layer exists.
			if (sceneryLayers[obj1->value[0]] == NULL) {
				sceneryLayers[obj1->value[0]] = new SceneryLayer();
			}

			//Load contents from node.
			sceneryLayers[obj1->value[0]]->loadFromNode(this, imageManager, renderer, obj1);
		}else if(obj1->name=="script" && !obj1->value.empty()){
			map<string,int>::iterator it=Game::levelEventNameMap.find(obj1->value[0]);
			if(it!=Game::levelEventNameMap.end()){
				int eventType=it->second;
				const std::string& script=obj1->attributes["script"][0];
				if(!script.empty()) scripts[eventType]=script;
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
		if(levels->getLevelCount()>1 && levels->type!=COLLECTION){
			s=tfm::format(_("Level %d %s"),levels->getCurrentLevel()+1,_CC(levels->getDictionaryManager(),editorData["name"]));
		} else {
			s = _CC(levels->getDictionaryManager(), editorData["name"]);
		}

        bmTips[0]=textureFromText(renderer, *fontText,s.c_str(),objThemes.getTextColor(true));
	}

	//Get the background
	background=objThemes.getBackground(false);

	//Now the loading is finished, we reset all objects to their initial states.
	//Before doing that we swap the levelObjects to levelObjectsInitial.
	std::swap(levelObjects, levelObjectsInitial);
	reset(true, stateID == STATE_LEVEL_EDITOR);
}

void Game::loadLevel(ImageManager& imageManager,SDL_Renderer& renderer,std::string fileName){
	//Create a TreeStorageNode that will hold the loaded data.
	TreeStorageNode *obj=new TreeStorageNode();
	{
		POASerializer objSerializer;
		string s=fileName;

		//Parse the file.
		if(!objSerializer.loadNodeFromFile(s.c_str(),obj,true)){
			cerr<<"ERROR: Can't load level file "<<s<<endl;
			delete obj;
			return;
		}
	}

	//Now call another function.
    loadLevelFromNode(imageManager,renderer,obj,fileName);
}

void Game::saveRecord(const char* fileName){
	//check if current level is NULL (which should be impossible)
	if(currentLevelNode==NULL) return;

	TreeStorageNode obj;
	POASerializer objSerializer;

	//put current level to the node.
	currentLevelNode->name="map";
	obj.subNodes.push_back(currentLevelNode);

	//put the random seed into the attributes.
	obj.attributes["seed"].push_back(prngSeed);

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

void Game::loadRecord(ImageManager& imageManager, SDL_Renderer& renderer, const char* fileName){
	//Create a TreeStorageNode that will hold the loaded data.
	TreeStorageNode obj;
	{
		POASerializer objSerializer;

		//Parse the file.
		if(!objSerializer.loadNodeFromFile(fileName,&obj,true)){
			cerr<<"ERROR: Can't load record file "<<fileName<<endl;
			return;
		}
	}

	//Load the seed of psuedo-random number generator.
	prngSeed.clear();
	{
		auto it = obj.attributes.find("seed");
		if (it != obj.attributes.end() && !it->second.empty()) {
			prngSeed = it->second[0];
		}
	}

	//find the node named 'map'.
	bool loaded=false;
	for(unsigned int i=0;i<obj.subNodes.size();i++){
		if(obj.subNodes[i]->name=="map"){
			//load the level. (fileName=???)
            loadLevelFromNode(imageManager,renderer,obj.subNodes[i],"?record?");
			//remove this node to prevent delete it.
			obj.subNodes[i]=NULL;
			//over
			loaded=true;
			break;
		}
	}

	if(!loaded){
		cerr<<"ERROR: Can't find subnode named 'map' from record file"<<endl;
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
void Game::handleEvents(ImageManager& imageManager, SDL_Renderer& renderer){
	//First of all let the player handle input.
	player.handleInput(&shadow);

	//Check for an SDL_QUIT event.
	if(event.type==SDL_QUIT){
		//We need to quit so enter STATE_EXIT.
		setNextState(STATE_EXIT);
	}

	//Check for the escape key.
	if(stateID != STATE_LEVEL_EDITOR && inputMgr.isKeyDownEvent(INPUTMGR_ESCAPE)){
		//Escape means we go one level up, to the level select state.
		setNextState(STATE_LEVEL_SELECT);
		//Save the progress.
		levels->saveLevelProgress();

		//And change the music back to the menu music.
		getMusicManager()->playMusic("menu");
	}

	//Check if 'R' is pressed.
	if(inputMgr.isKeyDownEvent(INPUTMGR_RESTART)){
		//Restart game only if we are not watching a replay.
		if (!player.isPlayFromRecord() || interlevel) {
			//Reset the game at next frame.
			isReset = true;

			//Also delete any gui (most likely the interlevel gui). Only in game mode.
			if (GUIObjectRoot && stateID != STATE_LEVEL_EDITOR){
				delete GUIObjectRoot;
				GUIObjectRoot = NULL;
			}

			//And set interlevel to false.
			interlevel = false;
		}
	}

	//Check for the next level buttons when in the interlevel popup.
	if (inputMgr.isKeyDownEvent(INPUTMGR_SPACE) || inputMgr.isKeyDownEvent(INPUTMGR_SELECT)){
		if(interlevel){
			//The interlevel popup is shown so we need to delete it.
			if(GUIObjectRoot){
				delete GUIObjectRoot;
				GUIObjectRoot=NULL;
			}

			//Now goto the next level.
            gotoNextLevel(imageManager,renderer);
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
void Game::logic(ImageManager& imageManager, SDL_Renderer& renderer){
	//Add one tick to the time.
	time++;

	//NOTE: This code reverts some changes in commit 5f03ae5.
	//This is part of old prepareFrame() code.
	//This is needed since otherwise the script function block:setLocation() and block:moveTo() are completely broken.
	//Later we should rewrite collision system completely which will remove this piece of code.
	//NOTE: In new collision system the effect of dx/dy/xVel/yVel should only be restricted in one frame.
	for (auto obj : levelObjects) {
		switch (obj->type) {
		default:
			obj->dx = obj->dy = obj->xVel = obj->yVel = 0;
			break;
		case TYPE_PUSHABLE:
			//NOTE: Currently the dx/dy/etc. of pushable blocks are still carry across frames, in order to make the collision system work correct.
			break;
		case TYPE_CONVEYOR_BELT: case TYPE_SHADOW_CONVEYOR_BELT:
			//NOTE: We let the conveyor belt itself to reset its xVel/yVel.
			obj->dx = obj->dy = 0;
			break;
		}
	}

	//NOTE2: The above code breaks pushable block with moving block in most cases,
	//more precisely, if the pushable block is processed before the moving block then things may be broken.
	//Therefore later we must process other blocks before moving pushable block.

	//Process delay execution scripts.
	getScriptExecutor()->processDelayExecution();

	//Process any event in the queue.
	for(unsigned int idx=0;idx<eventQueue.size();idx++){
		//Get the event from the queue.
		typeGameObjectEvent &e=eventQueue[idx];

		//Check if the it has an id attached to it.
		if(e.target){
			//NOTE: Should we check if the target still exists???
			e.target->onEvent(e.eventType);
		}else if(e.flags&1){
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

	//Remove levelObjects whose isDelete is true.
	{
		int j = 0;
		for (int i = 0; i < (int)levelObjects.size(); i++) {
			if (levelObjects[i] == NULL) {
				j++;
			} else if (levelObjects[i]->isDelete) {
				delete levelObjects[i];
				levelObjects[i] = NULL;
				j++;
			} else if (j > 0) {
				levelObjects[i - j] = levelObjects[i];
			}
		}
		if (j > 0) levelObjects.resize(levelObjects.size() - j);
	}

	//Check if we should save/load state.
	//NOTE: This happens after event handling so no eventQueue has to be saved/restored.
	if(saveStateNextTime){
		saveState();
	}else if(loadStateNextTime){
		loadState();
	}
	saveStateNextTime=false;
	loadStateNextTime=false;
	
	//Loop through the gameobjects to update them.
	for(unsigned int i=0;i<levelObjects.size();i++){
		//Send GameObjectEvent_OnEnterFrame event to the script
		levelObjects[i]->onEvent(GameObjectEvent_OnEnterFrame);
	}
	//Let the gameobject handle movement.
	{
		std::vector<Block*> pushableBlocks;

		//First we process blocks which are not pushable blocks.
		for (auto o : levelObjects) {
			if (o->type == TYPE_PUSHABLE) {
				pushableBlocks.push_back(o);
			} else {
				o->move();
			}
		}

		//Sort pushable blocks by their position, which is an ad-hoc workaround for
		//<https://forum.freegamedev.net/viewtopic.php?f=48&t=8047#p77692>.
		std::stable_sort(pushableBlocks.begin(), pushableBlocks.end(),
			[](const Block* obj1, const Block* obj2)->bool
		{
			SDL_Rect r1 = const_cast<Block*>(obj1)->getBox(), r2 = const_cast<Block*>(obj2)->getBox();
			if (r1.y > r2.y) return true;
			else if (r1.y < r2.y) return false;
			else return r1.x < r2.x;
		});

		//Now we process pushable blocks.
		for (auto o : pushableBlocks) {
			o->move();
		}
	}
	//Also update the scenery.
	for (auto it = sceneryLayers.begin(); it != sceneryLayers.end(); ++it){
		it->second->updateAnimation();
	}

	//Let the player store his move, if recording.
	player.shadowSetState();
	//Let the player give his recording to the shadow, if configured.
	player.shadowGiveState(&shadow);

	//NOTE: to fix bugs regarding player/shadow swap, we should first process collision of player/shadow then move them

	const SDL_Rect playerLastPosition = player.getBox();
	const SDL_Rect shadowLastPosition = shadow.getBox();

	//NOTE: The following is ad-hoc code to fix shadow on blocked player on conveyor belt bug
	if (shadow.holdingOther) {
		//We need to process shadow collision first if shadow is holding player.

		//Let the shadow decide his move, if he's playing a recording.
		shadow.moveLogic();

		//Check collision for shadow.
		shadow.collision(levelObjects, NULL);

		//Get the new position of it.
		const SDL_Rect r = shadow.getBox();

		//Check collision for player. Transfer the velocity of shadow to it only if the shadow moves its position.
		player.collision(levelObjects, (r.x != shadowLastPosition.x || r.y != shadowLastPosition.y) ? &shadow : NULL);
	} else {
		//Otherwise we process player first.

		//Check collision for player.
		player.collision(levelObjects, NULL);

		//Get the new position of it.
		const SDL_Rect r = player.getBox();

		//Now let the shadow decide his move, if he's playing a recording.
		shadow.moveLogic();

		//Check collision for shadow. Transfer the velocity of player to it only if the player moves its position.
		shadow.collision(levelObjects, (r.x != playerLastPosition.x || r.y != playerLastPosition.y) ? &player : NULL);
	}

	//Let the player move.
	player.move(levelObjects, playerLastPosition.x, playerLastPosition.y);
	//Let the shadow move.
	shadow.move(levelObjects, shadowLastPosition.x, shadowLastPosition.y);

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
            recordingEnded(imageManager,renderer);
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
					if(bestTime>=0 && (targetTime<0 || bestTime<=targetTime))
						oldMedal++;
					if(bestRecordings>=0 && (targetRecordings<0 || bestRecordings<=targetRecordings))
						oldMedal++;
				}else{
					bestTime=time;
					bestRecordings=recordings;
				}

				if(bestTime<0 || bestTime>time) bestTime=time;
				if(bestRecordings<0 || bestRecordings>recordings) bestRecordings=recordings;

				if(targetTime<0 || bestTime<=targetTime)
					newMedal++;
				if(targetRecordings<0 || bestRecordings<=targetRecordings)
					newMedal++;

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

			//Check the achievement "Complete a level with checkpoint, but without saving"
			if (objLastCheckPoint.get() == NULL) {
				for (auto obj : levelObjects) {
					if (obj->type == TYPE_CHECKPOINT) {
						statsMgr.newAchievement("withoutsave");
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
					cerr<<"ERROR: Couldn't get auto-save record file path"<<endl;
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
					cerr<<"ERROR: Couldn't get auto-save record file path"<<endl;
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
            replayPlay(imageManager,renderer);

			//Update achievements
			if(levels->levelpackName=="tutorial") statsMgr.updateTutorialAchievements();
			statsMgr.updateLevelAchievements();

			//NOTE: We set isReset false to prevent the user from getting a best time of 0.00s and 0 recordings.
			isReset = false;
		}
	}
	won=false;

	//Check if we should reset.
	if (isReset) {
		//NOTE: we don't need to reset save ??? it looks like that there are no bugs
		reset(false, false);
	}
	isReset=false;
}

/////////////////RENDER//////////////////
void Game::render(ImageManager&,SDL_Renderer &renderer){
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
            bg->draw(renderer);

			//And if it's the loaded background then also update the animation.
			//FIXME: Updating the animation in the render method?
			if(bg==background)
				bg->updateAnimation();
		}else{
			//There's no background so fill the screen with white.
            SDL_SetRenderDrawColor(&renderer, 255,255,255,255);
            SDL_RenderClear(&renderer);
		}
	}

	//Now draw the blackground layers.
	auto it = sceneryLayers.begin();
	for (; it != sceneryLayers.end(); ++it){
		if (it->first >= "f") break; // now we meet a foreground layer
		it->second->show(renderer);
	}

	//Now we draw the levelObjects.
	{
		//NEW: always render the pushable blocks in front of other blocks
		std::vector<Block*> pushableBlocks;

		for (auto o : levelObjects) {
			if (o->type == TYPE_PUSHABLE) {
				pushableBlocks.push_back(o);
			} else {
				o->show(renderer);
			}
		}

		for (auto o : pushableBlocks) {
			o->show(renderer);
		}
	}

	//Followed by the player and the shadow.
	//NOTE: We draw the shadow first, because he needs to be behind the player.
    shadow.show(renderer);
    player.show(renderer);

	//Now draw the foreground layers.
	for (; it != sceneryLayers.end(); ++it){
		it->second->show(renderer);
	}

	//Show the levelName if it isn't the level editor.
	if(stateID!=STATE_LEVEL_EDITOR && bmTips[0]!=NULL && !interlevel){
        withTexture(*bmTips[0], [&](SDL_Rect r){
            drawGUIBox(-2,SCREEN_HEIGHT-r.h-4,r.w+8,r.h+6,renderer,0xFFFFFFFF);
            applyTexture(2,SCREEN_HEIGHT-r.h,*bmTips[0],renderer,NULL);
        });
	}

	//Check if there's a tooltip.
	//NOTE: gameTipIndex 0 is used for the levelName, 1 for shadow death, 2 for restart text, 3 for restart+checkpoint.
	if(gameTipIndex>3 && gameTipIndex<TYPE_MAX){
		//Check if there's a tooltip for the type.
		if(bmTips[gameTipIndex]==NULL){
			//There isn't thus make it.
			string s;
			string keyCode = InputManagerKeyCode::describeTwo(inputMgr.getKeyCode(INPUTMGR_ACTION, false), inputMgr.getKeyCode(INPUTMGR_ACTION, true));
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
                bmTips[gameTipIndex]=textureFromText(renderer, *fontText, s.c_str(), objThemes.getTextColor(true));
			}
		}

		//We already have a gameTip for this type so draw it.
		if(bmTips[gameTipIndex]!=NULL){
            withTexture(*bmTips[gameTipIndex], [&](SDL_Rect r){
                drawGUIBox(-2,-2,r.w+8,r.h+6,renderer,0xFFFFFFFF);
                applyTexture(2,2,*bmTips[gameTipIndex],renderer);
            });
		}
	}
	//Set the gameTip to 0.
	gameTipIndex=0;
    // Limit the scope of bm, as it's a borrowed pointer.
    {
    //Pointer to the sdl texture that will contain a message, if any.
    SDL_Texture* bm=NULL;

	//Check if the player is dead, meaning we draw a message.
	if(player.dead){
		//Get user configured restart key
		string keyCodeRestart = InputManagerKeyCode::describeTwo(inputMgr.getKeyCode(INPUTMGR_RESTART, false), inputMgr.getKeyCode(INPUTMGR_RESTART, true));
		//The player is dead, check if there's a state that can be loaded.
		if(player.canLoadState()){
			//Now check if the tip is already made, if not make it.
			if(bmTips[3]==NULL){
				//Get user defined key for loading checkpoint
				string keyCodeLoad = InputManagerKeyCode::describeTwo(inputMgr.getKeyCode(INPUTMGR_LOAD, false), inputMgr.getKeyCode(INPUTMGR_LOAD, true));
				//Draw string
                bmTips[3]=textureFromText(renderer, *fontText,//TTF_RenderUTF8_Blended(fontText,
					/// TRANSLATORS: Please do not remove %s from your translation:
					///  - first %s means currently configured key to restart game
					///  - Second %s means configured key to load from last save
					tfm::format(_("Press %s to restart current level or press %s to load the game."),
						keyCodeRestart,keyCodeLoad).c_str(),
					objThemes.getTextColor(true));
			}
            bm=bmTips[3].get();
		}else{
			//Now check if the tip is already made, if not make it.
			if(bmTips[2]==NULL){
                bmTips[2]=textureFromText(renderer, *fontText,
					/// TRANSLATORS: Please do not remove %s from your translation:
					///  - %s will be replaced with currently configured key to restart game
					tfm::format(_("Press %s to restart current level."),keyCodeRestart).c_str(),
					objThemes.getTextColor(true));
			}
            bm=bmTips[2].get();
		}
	}

	//Check if the shadow has died (and there's no other notification).
	//NOTE: We use the shadow's jumptime as countdown, this variable isn't used when the shadow is dead.
	if(shadow.dead && bm==NULL && shadow.jumpTime>0){
		//Now check if the tip is already made, if not make it.
		if(bmTips[1]==NULL){
            bmTips[1]=textureFromText(renderer, *fontText,
				_("Your shadow has died."),
				objThemes.getTextColor(true));
		}
        bm=bmTips[1].get();

		//NOTE: Logic in the render loop, we substract the shadow's jumptime by one.
		shadow.jumpTime--;
		
		//return view to player and keep it there
		cameraMode=CAMERA_PLAYER;
	}

	//Draw the tip.
	if(bm!=NULL){
        const SDL_Rect textureSize = rectFromTexture(*bm);
        int x=(SCREEN_WIDTH-textureSize.w)/2;
		int y=32;
        drawGUIBox(x-8,y-8,textureSize.w+16,textureSize.h+14,renderer,0xFFFFFFFF);
        applyTexture(x,y,*bm,renderer);
	}
    }

	//Show the number of collectables the user has collected if there are collectables in the level.
	//We hide this when interlevel.
	if ((currentCollectables || totalCollectables) && !interlevel && time>0){
		if (collectablesTexture.needsUpdate(currentCollectables ^ (totalCollectables << 16))) {
            collectablesTexture.update(currentCollectables ^ (totalCollectables << 16),
                                       textureFromText(renderer,
                                                       *fontText,
													   tfm::format("%d/%d", currentCollectables, totalCollectables).c_str(),
                                                       objThemes.getTextColor(true)));
        }
        SDL_Rect bmSize = rectFromTexture(*collectablesTexture.get());
		
		//Draw background
        drawGUIBox(SCREEN_WIDTH-bmSize.w-34,SCREEN_HEIGHT-bmSize.h-4,bmSize.w+34+2,bmSize.h+4+2,renderer,0xFFFFFFFF);
		
		//Draw the collectable icon
        collectable.draw(renderer,SCREEN_WIDTH-50+12,SCREEN_HEIGHT-50+10);

		//Draw text
        applyTexture(SCREEN_WIDTH-50-bmSize.w+22,SCREEN_HEIGHT-bmSize.h,collectablesTexture.getTexture(),renderer);
	}

	//show time and records used in level editor or during replay.
	if((stateID==STATE_LEVEL_EDITOR || (!interlevel && player.isPlayFromRecord())) && time>0){
        const SDL_Color fg=objThemes.getTextColor(true),bg={255,255,255,255};
        const int alpha = 160;
        if (recordingsTexture.needsUpdate(recordings)) {
            recordingsTexture.update(recordings,
                                     textureFromTextShaded(
                                         renderer,
                                         *fontText,
                                         tfm::format(ngettext("%d recording","%d recordings",recordings).c_str(),recordings).c_str(),
                                         fg,
                                         bg
                                     ));
            SDL_SetTextureAlphaMod(recordingsTexture.get(),alpha);
        }

        int y=SCREEN_HEIGHT - textureHeight(*recordingsTexture.get());
		if (stateID != STATE_LEVEL_EDITOR && bmTips[0] != NULL && !interlevel) {
			y -= textureHeight(bmTips[0]) + 4;
		}

		applyTexture(0,y,*recordingsTexture.get(), renderer);

        if(timeTexture.needsUpdate(time)) {
            const size_t len = 32;
            timeTexture.update(time,
                               textureFromTextShaded(
                                   renderer,
                                   *fontText,
								   tfm::format("%-.2fs", time / 40.0).c_str(),
                                   fg,
                                   bg
                               ));
        }

		y -= textureHeight(*timeTexture.get());

        applyTexture(0,y,*timeTexture.get(), renderer);
	}

	//Draw the current action in the upper right corner.
	if(player.record){
		const SDL_Rect r = { 0, 0, 50, 50 };
		applyTexture(SCREEN_WIDTH - 50, 0, *action, renderer, &r);
	} else if (shadow.state != 0){
		const SDL_Rect r={50,0,50,50};
        applyTexture(SCREEN_WIDTH-50,0,*action,renderer,&r);
	}

	//if the game is play from record then draw something indicates it
	if(player.isPlayFromRecord()){
		//Dim the screen if interlevel is true.
        if( interlevel){
            dimScreen(renderer,191);
		}else if((time & 0x10)==0x10){
			// FIXME: replace this ugly ad-hoc animation by a better one
			const SDL_Rect r={50,0,50,50};
            applyTexture(0,0,*action,renderer,&r);
            //applyTexture(0,SCREEN_HEIGHT-50,*action,renderer,&r);
            //applyTexture(SCREEN_WIDTH-50,SCREEN_HEIGHT-50,*action,renderer,&r);
		}
	}else if(auto blockId = player.objNotificationBlock.get()){
		//If the player is in front of a notification block show the message.
		//And it isn't a replay.
        //Check if we need to update the notification message texture.
        int maxWidth = 0;
        int y = 20;
        //We check against blockId rather than the full message, as blockId is most likely shorter.
        if(notificationTexture.needsUpdate(blockId)) {
            const std::string &untranslated_message=blockId->message;
            std::string message=_CC(levels->getDictionaryManager(),untranslated_message);

			//Expand the variables.
			std::map<std::string, std::string> cachedVariables;
			for (;;) {
				size_t lps = message.find("{{{");
				if (lps == std::string::npos) break;
				size_t lpe = message.find("}}}", lps);
				if (lpe == std::string::npos) break;

				std::string varName = message.substr(lps + 3, lpe - lps - 3), varValue;
				auto it = cachedVariables.find(varName);

				if (it != cachedVariables.end()) {
					varValue = it->second;
				} else {
					bool isUnknown = true;

					if (varName.size() >= 4 && varName.substr(0, 4) == "key_") {
						//Probably a key name.
						InputManagerKeys key = InputManager::getKeyFromName(varName);
						if (key != INPUTMGR_MAX) {
							varValue = InputManagerKeyCode::describeTwo(inputMgr.getKeyCode(key, false), inputMgr.getKeyCode(key, true));
							isUnknown = false;
						}
					}

					if (isUnknown) {
						//Unknown variable
						cerr << "Warning: Unknown variable '{{{" << varName << "}}}' in notification block message!" << endl;
					}

					cachedVariables[varName] = varValue;
				}

				//Substitute.
				message.replace(message.begin() + lps, message.begin() + (lpe + 3), varValue);
			}

			std::vector<std::string> string_data;

			//Trim the message.
			{
				size_t lps = message.find_first_not_of("\n\r \t");
				if (lps == string::npos) {
					message.clear(); // it's completely empty
				} else {
					message = message.substr(lps, message.find_last_not_of("\n\r \t") - lps + 1);
				}
			}

			//Split the message into lines.
			for (int lps = 0;;) {
				// determine the end of line
				int lpe = lps;
				for (; message[lpe] != '\n' && message[lpe] != '\r' && message[lpe] != '\0'; lpe++);

				string_data.push_back(message.substr(lps, lpe - lps));

				// break if the string ends
				if (message[lpe] == '\0') break;

				// skip "\r\n" for Windows line ending
				if (message[lpe] == '\r' && message[lpe + 1] == '\n') lpe++;

				// point to the start of next line
				lps = lpe + 1;
			}

            vector<SurfacePtr> lines;

			//Create the image for each lines
			for (int i = 0; i < (int)string_data.size(); i++) {
				//Integer used to center the sentence horizontally.
				int x = 0;
				TTF_SizeUTF8(fontText, string_data[i].c_str(), &x, NULL);

				//Find out largest width
				if (x>maxWidth)
					maxWidth = x;

				lines.emplace_back(TTF_RenderUTF8_Blended(fontText, string_data[i].c_str(), objThemes.getTextColor(true)));

				//Increase y with 25, about the height of the text.
				y += 25;
            }

            maxWidth+=SCREEN_WIDTH*0.15;

            SurfacePtr surf = createSurface(maxWidth, y);
			int y1 = y;
            for(SurfacePtr &s : lines) {
                if(s) {
                    applySurface((surf->w-s->w)/2,surf->h - y1,s.get(),surf.get(),NULL);
                }
				y1 -= 25;
			}
            notificationTexture.update(blockId, textureUniqueFromSurface(renderer,std::move(surf)));
        } else {
            auto texSize = rectFromTexture(*notificationTexture.get());
            maxWidth=texSize.w;
            y=texSize.h;
        }

        drawGUIBox((SCREEN_WIDTH-maxWidth)/2,SCREEN_HEIGHT-y-25,maxWidth,y+20,renderer,0xFFFFFFBF);
        applyTexture((SCREEN_WIDTH-maxWidth)/2,SCREEN_HEIGHT-y,notificationTexture.getTexture(),renderer);
	}
}

void Game::resize(ImageManager&, SDL_Renderer& /*renderer*/){
	//Check if the interlevel popup is shown.
	if(interlevel && GUIObjectRoot){
		GUIObjectRoot->left=(SCREEN_WIDTH-GUIObjectRoot->width)/2;
	}
}

void Game::replayPlay(ImageManager& imageManager,SDL_Renderer& renderer){
	//Set interlevel true.
	interlevel=true;
	
	//Make a copy of the playerButtons.
	vector<int> recordCopy=player.recordButton;
	
	//Reset the game.
	//NOTE: We don't reset the saves. I'll see that if it will introduce bugs.
	reset(false, false);

	//Make the cursor visible when the interlevel popup is up.
	SDL_ShowCursor(SDL_ENABLE);

	//Set the copy of playerButtons back.
	player.recordButton=recordCopy;

	//Now play the recording.
	player.playRecord();
	
	//Create the gui if it isn't already done.
	if(!GUIObjectRoot){
		//Create a new GUIObjectRoot the size of the screen.
        GUIObjectRoot=new GUIObject(imageManager,renderer,0,0,SCREEN_WIDTH,SCREEN_HEIGHT);
		//Make child widgets change color properly according to theme.
		GUIObjectRoot->inDialog=true;

		//Create a GUIFrame for the upper frame.
        GUIFrame* upperFrame=new GUIFrame(imageManager,renderer,0,4,0,74);
		GUIObjectRoot->addChild(upperFrame);

		//Render the You've finished: text and add it to a GUIImage.
        //NOTE: The texture is managed by the GUIImage so no need to free it ourselfs.
        auto bm = SharedTexture(textureFromText(renderer, *fontGUI,_("You've finished:"),objThemes.getTextColor(true)));
        const SDL_Rect textureSize = rectFromTexture(*bm);

        GUIImage* title=new GUIImage(imageManager,renderer,0,4-GUI_FONT_RAISE,textureSize.w,textureSize.h,bm);
		upperFrame->addChild(title);

		//Create the sub title.
		string s;
		if (levels->getLevelCount()>0){
			/// TRANSLATORS: Please do not remove %s or %d from your translation:
			///  - %d means the level number in a levelpack
			///  - %s means the name of current level
			s=tfm::format(_("Level %d %s"),levels->getCurrentLevel()+1,_CC(levels->getDictionaryManager(),levelName));
		}
        GUIObject* obj=new GUILabel(imageManager,renderer,0,44,0,28,s.c_str(),0,true,true,GUIGravityCenter);
		upperFrame->addChild(obj);
		obj->render(renderer,0,0,false);

		//Determine the width the upper frame should have.
		int width;
        if(textureSize.w>obj->width)
            width=textureSize.w+32;
		else
			width=obj->width+32;
		//Set the left of the title.
		title->left=(width-title->width)/2;
		//Set the width of the level label to the width of the frame for centering.
		obj->width=width;
		//Now set the position and width of the frame.
		upperFrame->width=width;
		upperFrame->left=(SCREEN_WIDTH-width)/2;

		//Now create a GUIFrame for the lower frame.
        GUIFrame* lowerFrame=new GUIFrame(imageManager,renderer,0,SCREEN_HEIGHT-140,570,135);
		GUIObjectRoot->addChild(lowerFrame);

		//The different values.
		int bestTime=levels->getLevel()->time;
		int targetTime=levels->getLevel()->targetTime;
		int bestRecordings=levels->getLevel()->recordings;
		int targetRecordings=levels->getLevel()->targetRecordings;

		int medal=1;
		if(bestTime>=0 && (targetTime<0 || bestTime<=targetTime))
			medal++;
		if(bestRecordings>=0 && (targetRecordings<0 || bestRecordings<=targetRecordings))
			medal++;

		int maxWidth=0;
		int x=20;
		
		//Is there a target time for this level?
		int timeY=0;
		bool isTargetTime=true;
		if(targetTime<0){
			isTargetTime=false;
			timeY=12;
		}

		//Create the labels with the time and best time.
		/// TRANSLATORS: Please do not remove %-.2f from your translation:
		///  - %-.2f means time in seconds
		///  - s is shortened form of a second. Try to keep it so.
        obj=new GUILabel(imageManager,renderer,x,10+timeY,-1,36,tfm::format(_("Time: %-.2fs"),time/40.0).c_str());
		lowerFrame->addChild(obj);
		
        obj->render(renderer,0,0,false);
		maxWidth=obj->width;

		/// TRANSLATORS: Please do not remove %-.2f from your translation:
		///  - %-.2f means time in seconds
		///  - s is shortened form of a second. Try to keep it so.
        obj=new GUILabel(imageManager,renderer,x,34+timeY,-1,36,tfm::format(_("Best time: %-.2fs"),bestTime/40.0).c_str());
		lowerFrame->addChild(obj);
		
        obj->render(renderer,0,0,false);
		if(obj->width>maxWidth)
			maxWidth=obj->width;

		/// TRANSLATORS: Please do not remove %-.2f from your translation:
		///  - %-.2f means time in seconds
		///  - s is shortened form of a second. Try to keep it so.
		if(isTargetTime){
            obj=new GUILabel(imageManager,renderer,x,58,-1,36,tfm::format(_("Target time: %-.2fs"),targetTime/40.0).c_str());
			lowerFrame->addChild(obj);
			
            obj->render(renderer,0,0,false);
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
        obj=new GUILabel(imageManager,renderer,x,10+recsY,-1,36,tfm::format(_("Recordings: %d"),recordings).c_str());
		lowerFrame->addChild(obj);
		
        obj->render(renderer,0,0,false);
		maxWidth=obj->width;

		/// TRANSLATORS: Please do not remove %d from your translation:
		///  - %d means the number of recordings user has made
        obj=new GUILabel(imageManager,renderer,x,34+recsY,-1,36,tfm::format(_("Best recordings: %d"),bestRecordings).c_str());
		lowerFrame->addChild(obj);
		
        obj->render(renderer,0,0,false);
		if(obj->width>maxWidth)
			maxWidth=obj->width;

		/// TRANSLATORS: Please do not remove %d from your translation:
		///  - %d means the number of recordings user has made
		if(isTargetRecs){
            obj=new GUILabel(imageManager,renderer,x,58,-1,36,tfm::format(_("Target recordings: %d"),targetRecordings).c_str());
			lowerFrame->addChild(obj);
			
            obj->render(renderer,0,0,false);
			if(obj->width>maxWidth)
				maxWidth=obj->width;
		}
		
		x+=maxWidth;

		//The medal that is earned.
		/// TRANSLATORS: Please do not remove %s from your translation:
		///  - %s will be replaced with name of a prize medal (gold, silver or bronze)
		string s1=tfm::format(_("You earned the %s medal"),(medal>1)?(medal==3)?_("GOLD"):_("SILVER"):_("BRONZE"));
        obj=new GUILabel(imageManager,renderer,50,92,-1,36,s1.c_str(),0,true,true,GUIGravityCenter);
		lowerFrame->addChild(obj);
		
        obj->render(renderer,0,0,false);
		if(obj->left+obj->width>x){
			x=obj->left+obj->width+30;
		}else{
			obj->left=20+(x-20-obj->width)/2;
		}

		//Create the rectangle for the earned medal.
		SDL_Rect r;
		r.x=(medal-1)*30;
		r.y=0;
		r.w=30;
		r.h=30;
		
		//Create the medal on the left side.
        obj=new GUIImage(imageManager,renderer,16,92,30,30,medals,r);
		lowerFrame->addChild(obj);
		//And the medal on the right side.
        obj=new GUIImage(imageManager,renderer,x-24,92,30,30,medals,r);
		lowerFrame->addChild(obj);

		//Create the three buttons, Menu, Restart, Next.
		/// TRANSLATORS: used as return to the level selector menu
        GUIObject* b1=new GUIButton(imageManager,renderer,x,10,-1,36,_("Menu"),0,true,true,GUIGravityCenter);
		b1->name="cmdMenu";
		b1->eventCallback=this;
		lowerFrame->addChild(b1);
        b1->render(renderer,0,0,true);

		/// TRANSLATORS: used as restart level
        GUIObject* b2=new GUIButton(imageManager,renderer,x,50,-1,36,_("Restart"),0,true,true,GUIGravityCenter);
		b2->name="cmdRestart";
		b2->eventCallback=this;
		lowerFrame->addChild(b2);
        b2->render(renderer,0,0,true);

		/// TRANSLATORS: used as next level
        GUIObject* b3=new GUIButton(imageManager,renderer,x,90,-1,36,_("Next"),0,true,true,GUIGravityCenter);
		b3->name="cmdNext";
		b3->eventCallback=this;
		lowerFrame->addChild(b3);
        b3->render(renderer,0,0,true);
		
		maxWidth=b1->width;
		if(b2->width>maxWidth)
			maxWidth=b2->width;
		if(b3->width>maxWidth)
			maxWidth=b3->width;
		
		b1->left=b2->left=b3->left=x+maxWidth/2;
		
		x+=maxWidth;
		lowerFrame->width=x;
		lowerFrame->left=(SCREEN_WIDTH-lowerFrame->width)/2;
	}
}

void Game::recordingEnded(ImageManager& imageManager, SDL_Renderer& renderer){
	//Check if it's a normal replay, if so just stop.
	if(!interlevel){
		//Show the cursor so that the user can press the ok button.
		SDL_ShowCursor(SDL_ENABLE);
		//Now show the message box.
        msgBox(imageManager,renderer,_("Game replay is done."),MsgBoxOKOnly,_("Game Replay"));
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

		//Save the PRNG and seed.
		prngSaved = prng;
		prngSeedSaved = prngSeed;

		//Save the level size.
		levelRectSaved = levelRect;

		//Save the camera mode and target.
		cameraModeSaved=cameraMode;
		cameraTargetSaved=cameraTarget;

		//Save the current collectables
		currentCollectablesSaved = currentCollectables;
		totalCollectablesSaved = totalCollectables;

		//Save scripts.
		copyCompiledScripts(getScriptExecutor()->getLuaState(), compiledScripts, savedCompiledScripts);

		//Save other state, for example moving blocks.
		copyLevelObjects(levelObjects, levelObjectsSave, false);

		//Also save states of scenery layers.
		for (auto it = sceneryLayers.begin(); it != sceneryLayers.end(); ++it) {
			it->second->saveAnimation();
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
			case 100:
				statsMgr.newAchievement("save100");
				break;
			}
		}

		//Save the state for script executor.
		getScriptExecutor()->saveState();

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

		//Load the PRNG and seed.
		prng = prngSaved;
		prngSeed = prngSeedSaved;

		//Load the level size.
		levelRect = levelRectSaved;

		//Load the camera mode and target.
		cameraMode=cameraModeSaved;
		cameraTarget=cameraTargetSaved;

		//Load the current collactbles
		currentCollectables = currentCollectablesSaved;
		totalCollectables = totalCollectablesSaved;

		//Load scripts.
		copyCompiledScripts(getScriptExecutor()->getLuaState(), savedCompiledScripts, compiledScripts);

		//Load other state, for example moving blocks.
		copyLevelObjects(levelObjectsSave, levelObjects, true);

		//Also load states of scenery layers.
		for (auto it = sceneryLayers.begin(); it != sceneryLayers.end(); ++it) {
			it->second->loadAnimation();
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
			case 100:
				statsMgr.newAchievement("load100");
				break;
			}
		}

		//Load the state for script executor.
		getScriptExecutor()->loadState();

		//Execute the onLoad event, if any.
		executeScript(LevelEvent_OnLoad);

		//Return true.
		return true;
	}

	//We can't load the state so return false.
	return false;
}

static std::string createNewSeed() {
	static int createSeedTime = 0;

	struct Buffer {
		time_t systemTime;
		Uint32 sdlTicks;
		int x;
		int y;
		int createSeedTime;
	} buffer;

	buffer.systemTime = time(NULL);
	buffer.sdlTicks = SDL_GetTicks();
	SDL_GetMouseState(&buffer.x, &buffer.y);
	buffer.createSeedTime = ++createSeedTime;

	return Md5::toString(Md5::calc(&buffer, sizeof(buffer), NULL));
}

void Game::reset(bool save,bool noScript){
	//Some sanity check, i.e. if we switch from no-script mode to script mode, we should always reset the save
	assert(noScript || getScriptExecutor() || save);

	//We need to reset the game so we also reset the player and the shadow.
	player.reset(save);
	shadow.reset(save);

	saveStateNextTime=false;
	loadStateNextTime=false;

	//Reset the stats if interlevel isn't true.
	if(!interlevel){
		time=0;
		recordings=0;
	}

	recentSwap=-10000;
	if(save) recentSwapSaved=-10000;

	//Reset the pseudo-random number generator by creating a new seed, unless we are playing from record.
	if (levelFile == "?record?" || interlevel) {
		if (prngSeed.empty()) {
			cout << "WARNING: The record file doesn't provide a random seed! Will create a new random seed!"
				"This may breaks the behavior pseudo-random number generator in script!" << endl;
			prngSeed = createNewSeed();
		} else {
#ifdef _DEBUG
			cout << "Use existing PRNG seed: " << prngSeed << endl;
#endif
		}
	} else {
		prngSeed = createNewSeed();
#ifdef _DEBUG
		cout << "Create new PRNG seed: " << prngSeed << endl;
#endif
	}
	prng.seed(std::seed_seq(prngSeed.begin(), prngSeed.end()));
	if (save) {
		prngSaved = prng;
		prngSeedSaved = prngSeed;
	}

	//Reset the level size.
	levelRect = levelRectInitial;
	if (save) levelRectSaved = levelRectInitial;

	//Reset the camera.
	cameraMode=CAMERA_PLAYER;
	if(save) cameraModeSaved=CAMERA_PLAYER;
	cameraTarget = SDL_Rect{ 0, 0, 0, 0 };
	if (save) cameraTargetSaved = SDL_Rect{ 0, 0, 0, 0 };

	//Reset the number of collectables
	currentCollectables = currentCollectablesInitial;
	totalCollectables = totalCollectablesInitial;
	if (save) {
		currentCollectablesSaved = currentCollectablesInitial;
		totalCollectablesSaved = totalCollectablesInitial;
	}

	//Clear the event queue, since all the events are from before the reset.
	eventQueue.clear();

	//Reset states of scenery layers.
	for (auto it = sceneryLayers.begin(); it != sceneryLayers.end(); ++it) {
		it->second->resetAnimation(save);
	}

	//Also reset the background animation, if any.
	if(background)
		background->resetAnimation(save);

	//Reset the cached notification block
	notificationTexture.update(NULL, NULL);

	//Reset the script environment if necessary.
	if (noScript) {
		//Destroys the script environment completely.
		scriptExecutor->destroy();

		//Clear the level script.
		compiledScripts.clear();
		savedCompiledScripts.clear();
		initialCompiledScripts.clear();

		//Clear the block script.
		for (auto block : levelObjects){
			block->compiledScripts.clear();
		}
		for (auto block : levelObjectsSave){
			block->compiledScripts.clear();
		}
		for (auto block : levelObjectsInitial){
			block->compiledScripts.clear();
		}
	} else if (save) {
		//Create a new script environment.
		getScriptExecutor()->reset(true);

		//Recompile the level script.
		compiledScripts.clear();
		savedCompiledScripts.clear();
		initialCompiledScripts.clear();
		for (auto it = scripts.begin(); it != scripts.end(); ++it){
			int index = getScriptExecutor()->compileScript(it->second);
			compiledScripts[it->first] = index;
			lua_rawgeti(getScriptExecutor()->getLuaState(), LUA_REGISTRYINDEX, index);
			savedCompiledScripts[it->first] = luaL_ref(getScriptExecutor()->getLuaState(), LUA_REGISTRYINDEX);
			lua_rawgeti(getScriptExecutor()->getLuaState(), LUA_REGISTRYINDEX, index);
			initialCompiledScripts[it->first] = luaL_ref(getScriptExecutor()->getLuaState(), LUA_REGISTRYINDEX);
		}

		//Recompile the block script.
		for (auto block : levelObjects){
			block->compiledScripts.clear();
		}
		for (auto block : levelObjectsSave){
			block->compiledScripts.clear();
		}
		for (auto block : levelObjectsInitial) {
			block->compiledScripts.clear();
			for (auto it = block->scripts.begin(); it != block->scripts.end(); ++it){
				int index = getScriptExecutor()->compileScript(it->second);
				block->compiledScripts[it->first] = index;
			}
		}
	} else {
		assert(getScriptExecutor());

		//Do a soft reset.
		getScriptExecutor()->reset(false);

		//Restore the level script to initial state.
		copyCompiledScripts(getScriptExecutor()->getLuaState(), initialCompiledScripts, compiledScripts);

		//NOTE: We don't need to restore the block script since it will be restored automatically when the block array is copied.
	}

	//We reset levelObjects here since we need to wait the compiledScripts being initialized.
	copyLevelObjects(levelObjectsInitial, levelObjects, true);
	if (save) {
		copyLevelObjects(levelObjectsInitial, levelObjectsSave, false);
	}

	//Also reset the last checkpoint so set it to NULL.
	if (save)
		objLastCheckPoint = NULL;

	//Call the level's onCreate event.
	executeScript(LevelEvent_OnCreate);

	//Send GameObjectEvent_OnCreate event to the script
	for (auto block : levelObjects) {
		block->onEvent(GameObjectEvent_OnCreate);
	}

	//Close exit(s) if there are any collectables
	if (totalCollectables>0){
		for (auto block : levelObjects){
			if (block->type == TYPE_EXIT){
				block->onEvent(GameObjectEvent_OnSwitchOff);
			}
		}
	}

	//Hide the cursor (if not the leveleditor).
	if(stateID!=STATE_LEVEL_EDITOR)
		SDL_ShowCursor(SDL_DISABLE);
}

void Game::executeScript(int eventType){
	map<int,int>::iterator it;
	
	//Check if there's a script for the given event.
	it=compiledScripts.find(eventType);
	if(it!=compiledScripts.end()){
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

void Game::gotoNextLevel(ImageManager& imageManager, SDL_Renderer& renderer){
	//Goto the next level.
	levels->nextLevel();

	//Check if the level exists.
	if(levels->getCurrentLevel()<levels->getLevelCount()){
		setNextState(STATE_GAME);
	}else{
		if(!levels->congratulationText.empty()){
            msgBox(imageManager,renderer,_CC(levels->getDictionaryManager(),levels->congratulationText),MsgBoxOKOnly,_("Congratulations"));
		}else{
            msgBox(imageManager,renderer,_("You have finished the levelpack!"),MsgBoxOKOnly,_("Congratulations"));
		}
		//Now go back to the levelselect screen.
		setNextState(STATE_LEVEL_SELECT);
		//And set the music back to menu.
		getMusicManager()->playMusic("menu");
	}
}

void Game::GUIEventCallback_OnEvent(ImageManager& imageManager,SDL_Renderer& renderer, string name,GUIObject* obj,int eventType){
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

		interlevel=false;

		//And reset the game.
		//NOTE: We don't need to clear the save game because in level replay the game won't be saved (??)
		//TODO: it seems work (??); I'll see if it introduce bugs
		reset(false, false);
	}else if(name=="cmdNext"){
		//No matter what, clear the gui.
		if(GUIObjectRoot){
			delete GUIObjectRoot;
			GUIObjectRoot=NULL;
		}

		//And goto the next level.
        gotoNextLevel(imageManager,renderer);
	}
}

void Game::invalidateNotificationTexture(Block *block) {
	if (block == NULL || block == notificationTexture.getId()) {
		notificationTexture.update(NULL, NULL);
	}
}
