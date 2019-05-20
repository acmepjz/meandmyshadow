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
#include "Settings.h"
#include "GameObjects.h"
#include "ThemeManager.h"
#include "Game.h"
#include "WordWrapper.h"
#include "LevelEditor.h"
#include "GUIOverlay.h"
#include "TreeStorageNode.h"
#include "POASerializer.h"
#include "InputManager.h"
#include "MusicManager.h"
#include "Render.h"
#include "StatisticsManager.h"
#include "ScriptExecutor.h"
#include "ScriptDelayExecution.h"
#include "MD5.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <algorithm>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <SDL_ttf_fontfallback.h>

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

set<string> Game::survivalistLevels;
string Game::survivalistLevel2;

bool Game::expertSurvivalistIsOngoing = false;

//Some static variables used for interlevel GUI only.
static int theOldMedal, theOldTime, theOldRecordings;

#define UTF8_RIGHT_ARROW "\xe2\x86\x92"

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

class GUIZoomAnimatedImage : public GUIImage {
public:
	GUIZoomAnimatedImage(ImageManager& imageManager, SDL_Renderer& renderer, int left = 0, int top = 0, int width = 0, int height = 0,
		SharedTexture image = nullptr, int animationInterval = 0, int animationSize = 0, SDL_Rect clip = SDL_Rect{ 0, 0, 0, 0 },
		bool enabled = true, bool visible = true)
		: GUIImage(imageManager, renderer, left, top, width, height, image, clip, enabled, visible)
		, animationInterval(animationInterval), animationSize(animationSize)
		, animationTime(0)
	{
	}

	void render(SDL_Renderer& renderer, int x, int y, bool draw) override {
		//There's no need drawing the widget when it's invisible.
		//Also make sure the image isn't null.
		if (!visible || !image || !draw)
			return;

		//Get the absolute x and y location.
		x += left;
		y += top;

		//Create a clip rectangle.
		SDL_Rect r = clip;

		//The width and height are capped by the GUIImage itself.
		if (r.w > width || r.w == 0) {
			r.w = width;
		}
		if (r.h > height || r.h == 0) {
			r.h = height;
		}

		if (r.w <= 0 || r.h <= 0) return;

		//Calculate the animation.
		animationTime++;
		if (animationTime >= animationInterval) animationTime = 0;

		float t = animationInterval > 0 ? float(animationTime) / float(animationInterval) * 2.0f : 0.0f;
		if (t > 1.0f) t = 2.0f - t;
		t *= t*(3.0f - 2.0f*t);
		t *= float(animationSize) / float(std::max(r.w, r.h));

		const int dx = int(float(r.w)*t + 0.5f), dy = int(float(r.h)*t + 0.5f);

		const SDL_Rect dstRect = { x - dx, y - dy, r.w + dx * 2, r.h + dy * 2 };
		SDL_RenderCopy(&renderer, image.get(), &r, &dstRect);
	}

public:
	int animationInterval, animationSize;

protected:
	int animationTime;
};

Game::Game(SDL_Renderer &renderer, ImageManager &imageManager):isReset(false)
	, scriptExecutor(new ScriptExecutor())
	,currentLevelNode(NULL)
	,customTheme(NULL)
	,background(NULL)
	, levelRect(SDL_Rect{ 0, 0, 0, 0 }), levelRectInitial(SDL_Rect{ 0, 0, 0, 0 })
	, arcade(false)
	,won(false)
	,interlevel(false)
	,time(0)
	,recordings(0)
	,cameraMode(CAMERA_PLAYER)
	,player(this),shadow(this),objLastCheckPoint(NULL)
	, currentCollectables(0), currentCollectablesInitial(0)
	, totalCollectables(0), totalCollectablesInitial(0)
{
	levelRectSaved = SDL_Rect{ 0, 0, 0, 0 };
	timeSaved = 0;
	recordingsSaved = 0;
	cameraModeSaved = (int)CAMERA_PLAYER;
	currentCollectablesSaved = 0;
	totalCollectablesSaved = 0;

	scriptSaveState = LUA_REFNIL;
	scriptExecutorSaved.savedDelayExecutionObjects = NULL;

	saveStateNextTime=false;
	saveStateByShadow = false;
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

GameSaveState::GameSaveState() {
	gameSaved.scriptSaveState = LUA_REFNIL;
	gameSaved.scriptExecutorSaved.savedDelayExecutionObjects = NULL;
}

GameSaveState::~GameSaveState() {
	//Simply call our destroy method.
	destroy();
}

void GameSaveState::destroy() {
	lua_State *state = NULL;

	if (auto game = dynamic_cast<Game*>(currentState)) {
		if (auto se = game->getScriptExecutor()) {
			state = se->getLuaState();
		}
	}

	//Delete script related stuff.
	if (state) {
		//Delete the compiledScripts.
		for (auto it = gameSaved.savedCompiledScripts.begin(); it != gameSaved.savedCompiledScripts.end(); ++it) {
			luaL_unref(state, LUA_REGISTRYINDEX, it->second);
		}

		//Delete the script save state.
		luaL_unref(state, LUA_REGISTRYINDEX, gameSaved.scriptSaveState);

		//Sanity check.
		assert(gameSaved.scriptExecutorSaved.savedDelayExecutionObjects == NULL || gameSaved.scriptExecutorSaved.savedDelayExecutionObjects->state == state);
	} else {
		if (gameSaved.scriptExecutorSaved.savedDelayExecutionObjects) {
			// Set the state to NULL since the state is already destroyed. This prevents the destructor to call luaL_unref.
			gameSaved.scriptExecutorSaved.savedDelayExecutionObjects->state = NULL;
		}
	}

	gameSaved.savedCompiledScripts.clear();
	gameSaved.scriptSaveState = LUA_REFNIL;

	delete gameSaved.scriptExecutorSaved.savedDelayExecutionObjects;
	gameSaved.scriptExecutorSaved.savedDelayExecutionObjects = NULL;

	for (auto o : gameSaved.levelObjectsSave) delete o;
	gameSaved.levelObjectsSave.clear();
}

void Game::destroy(){
	delete scriptExecutor;
	scriptExecutor = NULL;

	if (scriptExecutorSaved.savedDelayExecutionObjects) {
		// Set the state to NULL since the state is already destroyed. This prevents the destructor to call luaL_unref.
		scriptExecutorSaved.savedDelayExecutionObjects->state = NULL;
		delete scriptExecutorSaved.savedDelayExecutionObjects;
		scriptExecutorSaved.savedDelayExecutionObjects = NULL;
	}

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
	scriptFile.clear();
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

void Game::loadLevelFromNode(ImageManager& imageManager, SDL_Renderer& renderer, TreeStorageNode* obj, const string& fileName, const std::string& scriptFileName){
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

	//Get the arcade property.
	{
		string &s = editorData["arcade"];
		arcade = atoi(s.c_str()) != 0;
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
			std::string layerName = obj1->value[0];

			//Upgrade the layer naming convention.
			if (layerName >= "f") {
				//Foreground layer.
				if (layerName.size() < 3 || layerName[0] != 'f' || layerName[1] != 'g' || layerName[2] != '_') {
					layerName = "fg_" + layerName;
				}
			} else {
				//Background layer.
				if (layerName.size() < 3 || layerName[0] != 'b' || layerName[1] != 'g' || layerName[2] != '_') {
					layerName = "bg_" + layerName;
				}
			}

			//Check if the layer exists.
			if (sceneryLayers[layerName] == NULL) {
				sceneryLayers[layerName] = new SceneryLayer();
			}

			//Load contents from node.
			sceneryLayers[layerName]->loadFromNode(this, imageManager, renderer, obj1);
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
	scriptFile = scriptFileName;

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

        bmTipsLevelName=textureFromText(renderer, *fontText,s.c_str(),objThemes.getTextColor(true));
	}

	//Get the background
	background=objThemes.getBackground(false);

	//Now the loading is finished, we reset all objects to their initial states.
	//Before doing that we swap the levelObjects to levelObjectsInitial.
	std::swap(levelObjects, levelObjectsInitial);
	reset(true, stateID == STATE_LEVEL_EDITOR);
}

void Game::loadLevel(ImageManager& imageManager,SDL_Renderer& renderer,const std::string& fileName){
	//Create a TreeStorageNode that will hold the loaded data.
	TreeStorageNode *obj=new TreeStorageNode();
	{
		POASerializer objSerializer;

		//Parse the file.
		if(!objSerializer.loadNodeFromFile(fileName.c_str(),obj,true)){
			cerr<<"ERROR: Can't load level file "<<fileName<<endl;
			delete obj;
			return;
		}
	}

	//Now call another function.
	loadLevelFromNode(imageManager, renderer, obj, fileName, std::string());

	//Set variable for Survivalist achievement.
	survivalistLevel2 = fileName;

	//Set variable for Expert Survivalist achievement.
	if (levels->type == MAIN && levels->levelpackName == "default") {
		if (levels->getCurrentLevel() == 0) expertSurvivalistIsOngoing = true;
	} else {
		expertSurvivalistIsOngoing = false;
	}
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

	//put the external script file into the attributes.
	{
		std::string s = scriptFile;
		if (s.empty() && !levelFile.empty() && levelFile[0] != '?') {
			s = levelFile;
			size_t lp = s.find_last_of('.');
			if (lp != std::string::npos) {
				s = s.substr(0, lp);
			}
			s += ".lua";
		}

		//Now load the external script file.
		if (!s.empty()) {
			if (s[0] == '?') {
				s = s.substr(1);
			} else {
				FILE *f = fopen(s.c_str(), "r");
				if (f) {
					fseek(f, 0, SEEK_END);
					size_t m = ftell(f);
					fseek(f, 0, SEEK_SET);
					std::vector<char> tmp(m);
					fread(tmp.data(), 1, m, f);
					fclose(f);
					tmp.push_back(0);
					s = tmp.data();
				} else {
					s.clear();
				}
			}
		}

		if (!s.empty()) {
			obj.attributes["script"].push_back(s);
		}
	}

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

		//Set variable for Expert Survivalist achievement.
		expertSurvivalistIsOngoing = false;

		//Save the progress if we are not watching a replay.
		if (!player.isPlayFromRecord() || interlevel) {
			levels->saveLevelProgress();
		}

		//And change the music back to the menu music.
		getMusicManager()->playMusic("menu");
	}

	//Check if 'R' is pressed.
	if(inputMgr.isKeyDownEvent(INPUTMGR_RESTART)){
		//Restart game only if we are not watching a replay.
		if (!player.isPlayFromRecord() || interlevel) {
			//Reset the game at next frame.
			isReset = true;

			//Set variable for Expert Survivalist achievement.
			if (!interlevel) expertSurvivalistIsOngoing = false;

			//Also delete any gui (most likely the interlevel gui). Only in game mode.
			if (GUIObjectRoot && stateID != STATE_LEVEL_EDITOR){
				delete GUIObjectRoot;
				GUIObjectRoot = NULL;
			}

			//And set interlevel to false.
			interlevel = false;
		}
	}

	//Check for the next level button when in the interlevel popup.
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

	//Check for the save replay button when in the interlevel popup.
	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_s && (event.key.keysym.mod & KMOD_CTRL) != 0) {
		if (interlevel) {
			GUIEventCallback_OnEvent(imageManager, renderer, "cmdSaveReplay", NULL, GUIEventClick);
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
	//Reset the gameTip.
	gameTipText.clear();

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
	checkSaveLoadState();

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

		const float d0 = statsMgr.playerPushingDistance + statsMgr.shadowPushingDistance;

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

		const float d1 = statsMgr.playerPushingDistance + statsMgr.shadowPushingDistance;

		if (d0 <= 100.0f && d1 >= 100.0f) statsMgr.newAchievement("push100");
		if (d0 <= 1000.0f && d1 >= 1000.0f) statsMgr.newAchievement("push1k");
	}
	//Also update the scenery.
	for (auto it = sceneryLayers.begin(); it != sceneryLayers.end(); ++it){
		it->second->updateAnimation();
	}
	//Also update the animation of the background.
	{
		//Get a pointer to the background.
		ThemeBackground* bg = background;

		//Check if the background is null, but there are themes.
		if (bg == NULL && objThemes.themeCount() > 0){
			//Get the background from the first theme in the stack.
			bg = objThemes[0]->getBackground(false);
		}

		//Check if the background isn't null.
		//And if it's the loaded background then also update the animation.
		if (bg && bg == background) {
			bg->updateAnimation();
		}
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

	//Check if player and shadow are pushing the same pushable block with opposite direction.
	if (player.objCurrentPushing != NULL && player.objCurrentPushing == shadow.objCurrentPushing &&
		((player.objCurrentPushing_pushVel > 0 && shadow.objCurrentPushing_pushVel < 0) ||
		(player.objCurrentPushing_pushVel < 0 && shadow.objCurrentPushing_pushVel > 0)))
	{
		int sum = player.objCurrentPushing_pushVel + shadow.objCurrentPushing_pushVel;
		if ((sum > 0 && player.objCurrentPushing_pushVel > 0) || (sum < 0 && player.objCurrentPushing_pushVel < 0)) {
			player.objCurrentPushing_pushVel = sum;
			shadow.objCurrentPushing_pushVel = 0;
		} else {
			player.objCurrentPushing_pushVel = 0;
			shadow.objCurrentPushing_pushVel = sum;
		}

		statsMgr.newAchievement("duel");
	}

	//Let the player move.
	player.move(levelObjects, playerLastPosition.x, playerLastPosition.y);
	//Let the shadow move.
	shadow.move(levelObjects, shadowLastPosition.x, shadowLastPosition.y);

	//Check collision and stuff for the shadow and player.
	player.otherCheck(&shadow);

	bool doNotResetWon = false;

	//Check if we won.
	if(won){
		//Check if it's level editor test play
		if (stateID == STATE_LEVEL_EDITOR) {
			if (auto editor = dynamic_cast<LevelEditor*>(this)) {
				editor->updateRecordInPlayMode(imageManager, renderer);
			}
		}
		//Check if it's playing from record
		else if (player.isPlayFromRecord() && !interlevel) {
			//We don't reset the won to false, and let RecordPlayback class handle it.
			doNotResetWon = true;
		}else{
			//Local copy of interlevel property since the replayPlay() will change it later.
			const bool previousInterlevel = interlevel;

			//We only update the level statistics when the previous state is not interlevel mode.
			if (!previousInterlevel) {
				//the string to store auto-save record path.
				string bestTimeFilePath, bestRecordingFilePath;
				//and if we can't get test path.
				bool filePathError = false;

				//Fixed a bug that the MD5 of current level may be not calculated yet.
				levels->getLevelMD5();

				//Get current level
				LevelPack::Level *level = levels->getLevel();

				//Get previous medal
				const int oldMedal = level->getMedal();
				theOldMedal = oldMedal;

				//Check if MD5 is changed
				bool md5Changed = false;

				for (int i = 0; i < 16; i++) {
					if (level->md5Digest[i] != level->md5InLevelProgress[i]) {
						md5Changed = true;
						break;
					}
				}

				//Erase existing record if MD5 is changed
				if (md5Changed) {
					//print some debug message
#if _DEBUG
					cout << "MD5 is changed, old " << Md5::toString(level->md5InLevelProgress);
					cout << ", new " << Md5::toString(level->md5Digest) << endl;
#endif
					level->time = -1;
					level->recordings = -1;
					theOldMedal = 0;

					//Update the MD5 in record
					memcpy(level->md5InLevelProgress, level->md5Digest, sizeof(level->md5Digest));
				}

				theOldTime = level->time;
				theOldRecordings = level->recordings;

				//Get better time and recordings
				const int betterTime = level->getBetterTime(time);
				const int betterRecordings = level->getBetterRecordings(level->arcade ? currentCollectables : recordings);

				//Get new medal
				const int newMedal = level->getMedal(betterTime, betterRecordings);

				//Check if we need to update statistics
				if (newMedal > oldMedal || md5Changed) {
					//Get category.
					int category = (int)levels->type;
					if (category > CUSTOM) category = CUSTOM;

					//Update pack medal.
					if (levels->type != COLLECTION) {
						int packMedal = 3;
						for (int i = 0, j = levels->getCurrentLevel(), m = levels->getLevelCount(); i < m; i++) {
							if (i != j) {
								int medal = levels->getLevel(i)->getMedal();
								if (packMedal > medal) packMedal = medal;
							}
						}
						int oldPackMedal = std::min(packMedal, oldMedal), newPackMedal = std::min(packMedal, newMedal);

						//Erase statictics for old medal
						if (oldPackMedal > 0) {
							statsMgr.completedLevelpacks--;
							statsMgr.completedLevelpacksByCategory[category]--;
						}
						if (oldPackMedal == 2) {
							statsMgr.silverLevelpacks--;
							statsMgr.silverLevelpacksByCategory[category]--;
						}
						if (oldPackMedal == 3) {
							statsMgr.goldLevelpacks--;
							statsMgr.goldLevelpacksByCategory[category]--;
						}

						//Update statistics for new medal
						if (newPackMedal > 0) {
							statsMgr.completedLevelpacks++;
							statsMgr.completedLevelpacksByCategory[category]++;
						}
						if (newPackMedal == 2) {
							statsMgr.silverLevelpacks++;
							statsMgr.silverLevelpacksByCategory[category]++;
						}
						if (newPackMedal == 3) {
							statsMgr.goldLevelpacks++;
							statsMgr.goldLevelpacksByCategory[category]++;
						}

						//Check achievement "Complete any level pack besides tutorial"
						if (newPackMedal > 0 && levels->levelpackName != "tutorial") {
							statsMgr.newAchievement("complete_levelpack");
						}
					}

					//Erase statictics for old medal
					if (oldMedal > 0) {
						statsMgr.completedLevels--;
						statsMgr.completedLevelsByCategory[category]--;
					}
					if (oldMedal == 2) {
						statsMgr.silverLevels--;
						statsMgr.silverLevelsByCategory[category]--;
					}
					if (oldMedal == 3) {
						statsMgr.goldLevels--;
						statsMgr.goldLevelsByCategory[category]--;
					}

					//Update statistics for new medal
					if (newMedal > 0) {
						statsMgr.completedLevels++;
						statsMgr.completedLevelsByCategory[category]++;
					}
					if (newMedal == 2) {
						statsMgr.silverLevels++;
						statsMgr.silverLevelsByCategory[category]++;
					}
					if (newMedal == 3) {
						statsMgr.goldLevels++;
						statsMgr.goldLevelsByCategory[category]++;
					}
				}

				//Check the achievement "Complete a level with recordings less than the target recordings"
				if (!arcade && level->targetRecordings >= 0 && recordings < level->targetRecordings) {
					statsMgr.newAchievement("underpar");
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

				//Check the Survivalist achievement.
				if (!survivalistLevel2.empty()) {
					survivalistLevels.insert(survivalistLevel2);
					if (survivalistLevels.size() >= 10) {
						statsMgr.newAchievement("survivalist");
					}
				}

				//Set the current level won.
				level->won = true;
				if (level->time != betterTime) {
					level->time = betterTime;
					//save the best-time game record.
					if (bestTimeFilePath.empty()){
						getCurrentLevelAutoSaveRecordPath(bestTimeFilePath, bestRecordingFilePath, true);
					}
					if (bestTimeFilePath.empty()){
						cerr << "ERROR: Couldn't get auto-save record file path" << endl;
						filePathError = true;
					} else{
						saveRecord(bestTimeFilePath.c_str());
					}
				}
				if (level->recordings != betterRecordings) {
					level->recordings = betterRecordings;
					//save the best-recordings game record.
					if (bestRecordingFilePath.empty() && !filePathError){
						getCurrentLevelAutoSaveRecordPath(bestTimeFilePath, bestRecordingFilePath, true);
					}
					if (bestRecordingFilePath.empty()){
						cerr << "ERROR: Couldn't get auto-save record file path" << endl;
						filePathError = true;
					} else{
						saveRecord(bestRecordingFilePath.c_str());
					}
				}

				//Set the next level unlocked if it exists.
				if (levels->getCurrentLevel() + 1 < levels->getLevelCount()){
					levels->setLocked(levels->getCurrentLevel() + 1);
				}
				//And save the progress.
				levels->saveLevelProgress();
			}

			//Now go to the interlevel screen.
            replayPlay(imageManager,renderer);

			//Update achievements (only when the previous state is not interlevel mode)
			if (!previousInterlevel) {
				if (levels->levelpackName == "tutorial") statsMgr.updateTutorialAchievements();
				statsMgr.updateLevelAchievements();
			}

			//NOTE: We set isReset false to prevent the user from getting a best time of 0.00s and 0 recordings.
			isReset = false;
		}
	}
	if (!doNotResetWon) won = false;

	//Check if we should reset.
	if (isReset) {
		//NOTE: we don't need to reset save ??? it looks like that there are no bugs
		reset(false, false);
	}
	isReset=false;

	//Finally we update the animation of player and shadow (was previously in render() function).
	shadow.updateAnimation();
	player.updateAnimation();
}

void Game::checkSaveLoadState() {
	//Check if we should save/load state.
	//NOTE: This happens after event handling so no eventQueue has to be saved/restored.
	if (saveStateNextTime){
		saveState();
	} else if (loadStateNextTime){
		loadState();
	}
	saveStateNextTime = false;
	saveStateByShadow = false;
	loadStateNextTime = false;
}

/////////////////RENDER//////////////////
void Game::render(ImageManager&,SDL_Renderer &renderer){
	//Update the camera (was previously in logic() function).
	switch (cameraMode) {
	case CAMERA_PLAYER:
		player.setMyCamera();
		break;
	case CAMERA_SHADOW:
		shadow.setMyCamera();
		break;
	case CAMERA_CUSTOM:
		//NOTE: The target is (should be) screen size independent so calculate the real target x and y here. 
		int targetX = cameraTarget.x - (SCREEN_WIDTH / 2);
		int targetY = cameraTarget.y - (SCREEN_HEIGHT / 2);
		//Move the camera to the cameraTarget.
		if (camera.x > targetX) {
			camera.x -= (camera.x - targetX) >> 4;
			//Make sure we don't go too far.
			if (camera.x < targetX)
				camera.x = targetX;
		} else if (camera.x < targetX) {
			camera.x += (targetX - camera.x) >> 4;
			//Make sure we don't go too far.
			if (camera.x > targetX)
				camera.x = targetX;
		}
		if (camera.y > targetY) {
			camera.y -= (camera.y - targetY) >> 4;
			//Make sure we don't go too far.
			if (camera.y < targetY)
				camera.y = targetY;
		} else if (camera.y < targetY) {
			camera.y += (targetY - camera.y) >> 4;
			//Make sure we don't go too far.
			if (camera.y > targetY)
				camera.y = targetY;
		}
		break;
	}

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
	if(stateID!=STATE_LEVEL_EDITOR && bmTipsLevelName!=NULL && !interlevel){
		SDL_Rect r(rectFromTexture(bmTipsLevelName));
		drawGUIBox(-2, SCREEN_HEIGHT - r.h - 4, r.w + 8, r.h + 6, renderer, 0xFFFFFFFF);
		applyTexture(2, SCREEN_HEIGHT - r.h, *bmTipsLevelName, renderer, NULL);
	}

	//Check if there's a tooltip.
	if(!gameTipText.empty()){
		std::string message = gameTipText;

		if (gameTipTexture.needsUpdate(gameTipText)) {
			int maxWidth = 0, y = 0;
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

			if (!message.empty()) {
				message.push_back('\0');

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
					int w = 0, h = 0;
					TTF_SizeUTF8(fontText, string_data[i].c_str(), &w, &h);

					//Find out largest width
					if (w > maxWidth)
						maxWidth = w;

					lines.emplace_back(TTF_RenderUTF8_Blended(fontText, string_data[i].c_str(), objThemes.getTextColor(true)));

					//Increase y with the height of the text.
					y += h;
				}

				SurfacePtr surf = createSurface(maxWidth, y);
				y = 0;
				for (auto &s : lines) {
					if (s) {
						applySurface(0, y, s.get(), surf.get(), NULL);
						y += s->h;
					}
				}
				gameTipTexture.update(gameTipText, textureUniqueFromSurface(renderer, std::move(surf)));
			}
		}

		//We already have a gameTip for this type so draw it.
		if (!message.empty() && gameTipTexture.get()){
			SDL_Rect r(rectFromTexture(*gameTipTexture.get()));
			drawGUIBox(-2, -2, r.w + 8, r.h + 6, renderer, 0xFFFFFFFF);
			applyTexture(2, 2, *gameTipTexture.get(), renderer);
		}
	}

	// Limit the scope of bm, as it's a borrowed pointer.
    {
    //Pointer to the sdl texture that will contain a message, if any.
    SDL_Texture* bm=NULL;

	//Check if the player is dead and the game is normal mode, meaning we draw a message.
	if(player.dead && !arcade){
		//Get user configured restart key
		string keyCodeRestart = InputManagerKeyCode::describeTwo(inputMgr.getKeyCode(INPUTMGR_RESTART, false), inputMgr.getKeyCode(INPUTMGR_RESTART, true));
		//The player is dead, check if there's a state that can be loaded.
		if(player.canLoadState()){
			//Now check if the tip is already made, if not make it.
			if(bmTipsRestartCheckpoint==NULL){
				//Get user defined key for loading checkpoint
				string keyCodeLoad = InputManagerKeyCode::describeTwo(inputMgr.getKeyCode(INPUTMGR_LOAD, false), inputMgr.getKeyCode(INPUTMGR_LOAD, true));
				//Draw string
				bmTipsRestartCheckpoint = textureFromText(renderer, *fontText,//TTF_RenderUTF8_Blended(fontText,
					/// TRANSLATORS: Please do not remove %s from your translation:
					///  - first %s means currently configured key to restart game
					///  - Second %s means configured key to load from last save
					tfm::format(_("Press %s to restart current level or press %s to load the game."),
						keyCodeRestart,keyCodeLoad).c_str(),
					objThemes.getTextColor(true));
			}
			bm = bmTipsRestartCheckpoint.get();
		}else{
			//Now check if the tip is already made, if not make it.
			if (bmTipsRestart == NULL){
				bmTipsRestart = textureFromText(renderer, *fontText,
					/// TRANSLATORS: Please do not remove %s from your translation:
					///  - %s will be replaced with currently configured key to restart game
					tfm::format(_("Press %s to restart current level."),keyCodeRestart).c_str(),
					objThemes.getTextColor(true));
			}
			bm = bmTipsRestart.get();
		}
	}

	//Check if the shadow has died (and there's no other notification).
	//NOTE: We use the shadow's jumptime as countdown, this variable isn't used when the shadow is dead.
	if(shadow.dead && bm==NULL && shadow.jumpTime>0){
		//Now check if the tip is already made, if not make it.
		if(bmTipsShadowDeath==NULL){
			bmTipsShadowDeath = textureFromText(renderer, *fontText,
				_("Your shadow has died."),
				objThemes.getTextColor(true));
		}
		bm = bmTipsShadowDeath.get();

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

	{
		int y = SCREEN_HEIGHT;

		//Show the number of collectables the user has collected if there are collectables in the level.
		//We hide this when interlevel.
		if ((currentCollectables || totalCollectables) && !interlevel){
			if (arcade) {
				//Only show the current collectibles in arcade mode.
				if (collectablesTexture.needsUpdate(currentCollectables)) {
					collectablesTexture.update(currentCollectables,
						textureFromText(renderer,
						*fontText,
						tfm::format("%d", currentCollectables).c_str(),
						objThemes.getTextColor(true)));
				}
			} else {
				if (collectablesTexture.needsUpdate(currentCollectables ^ (totalCollectables << 16))) {
					collectablesTexture.update(currentCollectables ^ (totalCollectables << 16),
						textureFromText(renderer,
						*fontText,
						tfm::format("%d/%d", currentCollectables, totalCollectables).c_str(),
						objThemes.getTextColor(true)));
				}
			}
			SDL_Rect bmSize = rectFromTexture(*collectablesTexture.get());

			//Draw background
			drawGUIBox(SCREEN_WIDTH - bmSize.w - 34, SCREEN_HEIGHT - bmSize.h - 4, bmSize.w + 34 + 2, bmSize.h + 4 + 2, renderer, 0xFFFFFFFF);

			//Draw the collectable icon
			collectable.draw(renderer, SCREEN_WIDTH - 50 + 12, SCREEN_HEIGHT - 50 + 10);

			//Draw text
			applyTexture(SCREEN_WIDTH - 50 - bmSize.w + 22, SCREEN_HEIGHT - bmSize.h, collectablesTexture.getTexture(), renderer);

			y -= bmSize.h + 4;
		}

		//Show additional message in level editor.
		if (stateID == STATE_LEVEL_EDITOR && additionalTexture) {
			SDL_Rect r = rectFromTexture(*additionalTexture.get());
			r.x = SCREEN_WIDTH - r.w;
			r.y = y - r.h;
			SDL_SetRenderDrawColor(&renderer, 255, 255, 255, 255);
			SDL_RenderFillRect(&renderer, &r);
			applyTexture(r.x, r.y, additionalTexture, renderer);
		}
	}

	//show time and records used in level editor or during replay or in arcade mode.
	if ((stateID == STATE_LEVEL_EDITOR || (!interlevel && (player.isPlayFromRecord() || arcade)))){
        const SDL_Color fg=objThemes.getTextColor(true),bg={255,255,255,255};
        const int alpha = 160;

		//don't show number of records in arcade mode
        if (!arcade && recordingsTexture.needsUpdate(recordings)) {
            recordingsTexture.update(recordings,
                                     textureFromTextShaded(
                                         renderer,
                                         *fontMono,
                                         tfm::format(ngettext("%d recording","%d recordings",recordings).c_str(),recordings).c_str(),
                                         fg,
                                         bg
                                     ));
            SDL_SetTextureAlphaMod(recordingsTexture.get(),alpha);
        }

		int y = SCREEN_HEIGHT - (arcade ? 0 : textureHeight(*recordingsTexture.get()));
		if (stateID != STATE_LEVEL_EDITOR && bmTipsLevelName != NULL && !interlevel) {
			y -= textureHeight(bmTipsLevelName) + 4;
		}

		if (!arcade) applyTexture(0,y,*recordingsTexture.get(), renderer);

        if(timeTexture.needsUpdate(time)) {
            timeTexture.update(time,
                               textureFromTextShaded(
                                   renderer,
                                   *fontMono,
								   tfm::format(_("%-.2fs"), time / 40.0).c_str(),
                                   fg,
                                   bg
                               ));
			SDL_SetTextureAlphaMod(timeTexture.get(), alpha);
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
        if(interlevel){
            dimScreen(renderer,191);
		}
	}else if(auto blockId = player.objNotificationBlock.get()){
		//If the player is in front of a notification block and it isn't a replay, show the message.

		//Check if we need to update the notification message texture.
        //We check against blockId rather than the full message, as blockId is most likely shorter.
        if(notificationTexture.needsUpdate(blockId)) {
			std::string message = translateAndExpandMessage(blockId->message);
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

			WordWrapper wrapper;
			wrapper.font = fontText;
			wrapper.maxWidth = SCREEN_WIDTH - 40;
			wrapper.wordWrap = true;
			wrapper.hyphen = "-";

			//Split the message into lines.
			wrapper.addString(string_data, message);

			//Create the image.
			notificationTexture.update(blockId,
				textureFromMultilineText(renderer, *fontText, string_data, objThemes.getTextColor(true), GUIGravityCenter));
        }

		SDL_Rect texSize = rectFromTexture(*notificationTexture.get());
		SDL_Rect boxSize = texSize;
		const int verticalPadding = 15;
		const int verticalPadding2 = 5; // additional padding from the bottom of GUI box to the bottom of screen
		boxSize.w += int(SCREEN_WIDTH*0.15);
		boxSize.h += verticalPadding * 2;
		if (boxSize.w > SCREEN_WIDTH - 20) boxSize.w = SCREEN_WIDTH - 20;

		drawGUIBox((SCREEN_WIDTH - boxSize.w) / 2,
			SCREEN_HEIGHT - boxSize.h - verticalPadding2 - 3 /* ?? */,
			boxSize.w,
			boxSize.h + 3 /* ?? */,
			renderer, 0xFFFFFFBF);
		applyTexture((SCREEN_WIDTH - texSize.w) / 2,
			SCREEN_HEIGHT - texSize.h - verticalPadding - verticalPadding2,
			notificationTexture.getTexture(), renderer);
	}
}

std::string Game::translateAndExpandMessage(const std::string &untranslated_message) {
	std::string message = _CC(levels->getDictionaryManager(), untranslated_message);

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

	//Over.
	return message;
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

	//Backup of time and recordings, etc. before we reset the level.
	//We choose the same variable names so that we don't need to modify the existing code.
	const int time = this->time, recordings = this->recordings, currentCollectables = this->currentCollectables;
	
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

		int medal = levels->getLevel()->getMedal();
		assert(medal > 0);

		int maxWidth=0;
		int x=20;

		//Create the rectangle for the gold star.
		SDL_Rect r = { 60, 0, 30, 30 };

		//Is there a target time for this level?
		int timeY=0;
		bool isTargetTime=true;
		if(targetTime<0){
			isTargetTime=false;
			timeY=12;
		}

		/// TRANSLATORS: Please do not remove %-.2f from your translation:
		///  - %-.2f means time in seconds
		///  - s is shortened form of a second. Try to keep it so.
		const std::string timeFormat = _("%-.2fs");

		std::string s1;

		//Create the labels with the time and best time.

		s1 = _("Time:");
		s1 += " ";
		s1 += tfm::format(timeFormat.c_str(), time / 40.0);

		obj = new GUILabel(imageManager, renderer, x, 10 + timeY, -1, 36, s1.c_str());
		lowerFrame->addChild(obj);

        obj->render(renderer,0,0,false);
		if (!isTargetTime || time <= targetTime) {
			lowerFrame->addChild(new GUIImage(imageManager, renderer, x + obj->width, 10 + timeY, 30, 30, medals, r));
			obj->width += 30;
		}
		maxWidth = obj->width;

		const int tmpTime = (theOldTime >= 0 && theOldTime != bestTime) ? theOldTime : bestTime;
		s1 = _("Best time:");
		s1 += " ";
		s1 += tfm::format(timeFormat.c_str(), tmpTime / 40.0);

		obj = new GUILabel(imageManager, renderer, x, 34 + timeY, -1, 36, s1.c_str());
		lowerFrame->addChild(obj);
		obj->render(renderer, 0, 0, false);
		if (!isTargetTime || tmpTime <= targetTime) {
			lowerFrame->addChild(new GUIImage(imageManager, renderer, x + obj->width, 34 + timeY, 30, 30, medals, r));
			obj->width += 30;
		}

		if (theOldTime >= 0 && theOldTime != bestTime) {
			s1 = " " UTF8_RIGHT_ARROW " ";
			s1 += tfm::format(timeFormat.c_str(), bestTime / 40.0);

			auto obj2 = new GUILabel(imageManager, renderer, x + obj->width, 34 + timeY, -1, 36, s1.c_str());
			lowerFrame->addChild(obj2);
			obj2->render(renderer, 0, 0, false);
			obj->width += obj2->width;
			if (!isTargetTime || bestTime <= targetTime) {
				lowerFrame->addChild(new GUIImage(imageManager, renderer, x + obj->width, 34 + timeY, 30, 30, medals, r));
				obj->width += 30;
			}
		}
		if (obj->width > maxWidth)
			maxWidth = obj->width;

		if(isTargetTime){
			s1 = _("Target time:");
			s1 += " ";
			s1 += tfm::format(timeFormat.c_str(), targetTime / 40.0);

			obj = new GUILabel(imageManager, renderer, x, 58, -1, 36, s1.c_str());
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

		s1 = arcade ? _("Collectibles:") : _("Recordings:");
		s1 += tfm::format(" %d", arcade ? currentCollectables : recordings);

		obj = new GUILabel(imageManager, renderer, x, 10 + recsY, -1, 36, s1.c_str());
		lowerFrame->addChild(obj);
		
        obj->render(renderer,0,0,false);
		if (!isTargetRecs || (arcade ? currentCollectables >= targetRecordings : recordings <= targetRecordings)) {
			lowerFrame->addChild(new GUIImage(imageManager, renderer, x + obj->width, 10 + recsY, 30, 30, medals, r));
			obj->width += 30;
		}
		maxWidth = obj->width;

		const int tmpRecordings = (theOldRecordings >= 0 && theOldRecordings != bestRecordings) ? theOldRecordings : bestRecordings;
		s1 = arcade ? _("Best collectibles:") : _("Best recordings:");
		s1 += tfm::format(" %d", tmpRecordings);

		obj = new GUILabel(imageManager, renderer, x, 34 + recsY, -1, 36, s1.c_str());
		lowerFrame->addChild(obj);
		obj->render(renderer, 0, 0, false);
		if (!isTargetRecs || (arcade ? tmpRecordings >= targetRecordings : tmpRecordings <= targetRecordings)) {
			lowerFrame->addChild(new GUIImage(imageManager, renderer, x + obj->width, 34 + recsY, 30, 30, medals, r));
			obj->width += 30;
		}

		if (theOldRecordings >= 0 && theOldRecordings != bestRecordings) {
			s1 = tfm::format(" " UTF8_RIGHT_ARROW " %d", bestRecordings);

			auto obj2 = new GUILabel(imageManager, renderer, x + obj->width, 34 + recsY, -1, 36, s1.c_str());
			lowerFrame->addChild(obj2);
			obj2->render(renderer, 0, 0, false);
			obj->width += obj2->width;
			if (!isTargetRecs || (arcade ? bestRecordings >= targetRecordings : bestRecordings <= targetRecordings)) {
				lowerFrame->addChild(new GUIImage(imageManager, renderer, x + obj->width, 34 + recsY, 30, 30, medals, r));
				obj->width += 30;
			}
		}
		if (obj->width > maxWidth)
			maxWidth = obj->width;

		if(isTargetRecs){
			s1 = arcade ? _("Target collectibles:") : _("Target recordings:");
			s1 += tfm::format(" %d", targetRecordings);

			obj = new GUILabel(imageManager, renderer, x, 58, -1, 36, s1.c_str());
			lowerFrame->addChild(obj);
			
            obj->render(renderer,0,0,false);
			if(obj->width>maxWidth)
				maxWidth=obj->width;
		}
		
		x+=maxWidth;

		//The medal that is earned.
		std::string newMedalName = (medal > 1) ? (medal == 3) ? _("GOLD") : _("SILVER") : _("BRONZE");
		if (medal > theOldMedal) {
			/// TRANSLATORS: Please do not remove %s from your translation:
			///  - %s will be replaced with name of a prize medal (gold, silver or bronze)
			s1 = tfm::format(_("You earned the %s medal"), newMedalName);
		} else {
			/// TRANSLATORS: Please do not remove %s from your translation:
			///  - %s will be replaced with name of a prize medal (gold, silver or bronze)
			s1 = tfm::format(_("The best medal you earned is %s medal"), newMedalName);
		}

		obj=new GUILabel(imageManager,renderer,50,92,-1,36,s1.c_str(),0,true,true,GUIGravityCenter);
		lowerFrame->addChild(obj);
		
        obj->render(renderer,0,0,false);
		if(obj->left+obj->width+30>x){
			x=obj->left+obj->width+30;
		}else{
			obj->left=20+(x-20-obj->width)/2;
		}

		if (medal > theOldMedal && theOldMedal > 0) {
			std::string oldMedalName = (theOldMedal > 1) ? (theOldMedal == 3) ? _("GOLD") : _("SILVER") : _("BRONZE");

			/// TRANSLATORS: Please do not remove %s from your translation:
			///  - %s will be replaced with name of a prize medal (gold, silver or bronze)
			s1 = tfm::format(_("(previous best medal: %s)"), oldMedalName);

			obj = new GUILabel(imageManager, renderer, 50, 116, -1, 36, s1.c_str(), 0, true, true, GUIGravityCenter);
			lowerFrame->addChild(obj);

			obj->render(renderer, 0, 0, false);
			obj->left = 20 + (x - 20 - obj->width) / 2;
		}

		//Create the rectangle for the earned medal.
		r.x=(medal-1)*30;
		
		int animationSize = (medal > theOldMedal) ? 5 : 0;

		//Create the medal on the left side.
		obj = new GUIZoomAnimatedImage(imageManager, renderer, 16, 92, 30, 30, medals, 80, animationSize, r);
		lowerFrame->addChild(obj);
		//And the medal on the right side.
		obj = new GUIZoomAnimatedImage(imageManager, renderer, x - 24, 92, 30, 30, medals, 80, animationSize, r);
		lowerFrame->addChild(obj);

		//Create the three buttons, Menu, Restart, Next.
		int buttonY = 10;
		std::vector<GUIButton*> buttons;

		/// TRANSLATORS: used as return to the level selector menu
		buttons.push_back(new GUIButton(imageManager, renderer, x, buttonY, -1, 30, _("Menu"), 0, true, true, GUIGravityCenter));
		buttons.back()->name = "cmdMenu";
		buttonY += 30;

		buttons.push_back(new GUIButton(imageManager, renderer, x, buttonY, -1, 30, _("Save replay"), 0, true, true, GUIGravityCenter));
		buttons.back()->name = "cmdSaveReplay";
		buttonY += 30;

		/// TRANSLATORS: used as restart level
		buttons.push_back(new GUIButton(imageManager, renderer, x, buttonY, -1, 30, _("Restart"), 0, true, true, GUIGravityCenter));
		buttons.back()->name = "cmdRestart";
		buttonY += 30;

		if (player.canLoadState() && shadow.canLoadState()) {
			/// TRANSLATORS: used as load the saved level
			buttons.push_back(new GUIButton(imageManager, renderer, x, buttonY, -1, 30, _("Load"), 0, true, true, GUIGravityCenter));
			buttons.back()->name = "cmdLoad";
			buttonY += 30;
		}

		/// TRANSLATORS: used as next level
		buttons.push_back(new GUIButton(imageManager, renderer, x, buttonY, -1, 30, _("Next"), 0, true, true, GUIGravityCenter));
		buttons.back()->name = "cmdNext";
		buttonY += 30;

		maxWidth = 0;
		for (auto btn : buttons) {
			btn->smallFont = true;
			btn->eventCallback = this;
			lowerFrame->addChild(btn);
			btn->render(renderer, 0, 0, true);

			if (btn->width > maxWidth)
				maxWidth = btn->width;
		}

		for (auto btn : buttons) {
			btn->left = x + maxWidth / 2;
		}

		x+=maxWidth;
		lowerFrame->width=x;
		lowerFrame->left=(SCREEN_WIDTH-lowerFrame->width)/2;

		//Recalculate the height of lower frame
		lowerFrame->height = 0;
		for (auto ctl : lowerFrame->childControls) {
			if (ctl->top + ctl->height > lowerFrame->height) {
				lowerFrame->height = ctl->top + ctl->height;
			}
		}
		lowerFrame->height += 10;
		lowerFrame->top = SCREEN_HEIGHT - 5 - lowerFrame->height;
	}
}

bool Game::canSaveState(){
	return (player.canSaveState() && shadow.canSaveState());
}

void Game::saveStateInternal(GameSaveState* o) {
	//Let the player and the shadow save their state.
	player.saveStateInternal(&o->playerSaved);
	shadow.saveStateInternal(&o->shadowSaved);

	//Save the game state.
	saveGameOnlyStateInternal(&o->gameSaved);

	//TODO: Save scenery layers, background animation,
}

void Game::loadStateInternal(GameSaveState* o) {
	//Let the player and the shadow load their state.
	player.loadStateInternal(&o->playerSaved);
	shadow.loadStateInternal(&o->shadowSaved);

	//Load the game state.
	loadGameOnlyStateInternal(&o->gameSaved);

	//TODO: load scenery layers, background animation,
}

void Game::saveGameOnlyStateInternal(GameOnlySaveState* o) {
	if (o == NULL) o = static_cast<GameOnlySaveState*>(this);

	auto state = getScriptExecutor()->getLuaState();

	//Save the stats.
	o->timeSaved = time;
	o->recordingsSaved = recordings;
	o->recentSwapSaved = recentSwap;

	//Save the PRNG and seed.
	o->prngSaved = prng;
	o->prngSeedSaved = prngSeed;

	//Save the level size.
	o->levelRectSaved = levelRect;

	//Save the camera mode and target.
	o->cameraModeSaved = (int)cameraMode;
	o->cameraTargetSaved = cameraTarget;

	//Save the current collectables
	o->currentCollectablesSaved = currentCollectables;
	o->totalCollectablesSaved = totalCollectables;

	//Save scripts.
	copyCompiledScripts(state, compiledScripts, o->savedCompiledScripts);

	//Save other state, for example moving blocks.
	copyLevelObjects(levelObjects, o->levelObjectsSave, false);

	//Save the state for script executor.
	getScriptExecutor()->saveState(&o->scriptExecutorSaved);

	//Check if we have onSave event.
	if (compiledScripts.find(LevelEvent_OnSave) != compiledScripts.end()) {
		//Backup old SAVESTATE.
		lua_getglobal(state, "SAVESTATE");
		int oldIndex = luaL_ref(state, LUA_REGISTRYINDEX);

		//Prepare the SAVESTATE variable for onSave event.
		lua_pushnil(state);
		lua_setglobal(state, "SAVESTATE");

		//Execute the onSave event.
		executeScript(LevelEvent_OnSave);

		//Retrieve the SAVESTATE and save it.
		luaL_unref(state, LUA_REGISTRYINDEX, o->scriptSaveState); // remove old save state
		lua_getglobal(state, "SAVESTATE");
		o->scriptSaveState = luaL_ref(state, LUA_REGISTRYINDEX);

		//Restore old SAVESTATE.
		lua_rawgeti(state, LUA_REGISTRYINDEX, oldIndex);
		luaL_unref(state, LUA_REGISTRYINDEX, oldIndex);
		lua_setglobal(state, "SAVESTATE");
	}

	//TODO: Save scenery layers, background animation,
}

void Game::loadGameOnlyStateInternal(GameOnlySaveState* o) {
	if (o == NULL) o = static_cast<GameOnlySaveState*>(this);

	auto state = getScriptExecutor()->getLuaState();

	//Load the stats.
	time = o->timeSaved;
	recordings = o->recordingsSaved;
	recentSwap = o->recentSwapSaved;

	//Load the PRNG and seed.
	prng = o->prngSaved;
	prngSeed = o->prngSeedSaved;

	//Load the level size.
	levelRect = o->levelRectSaved;

	//Load the camera mode and target.
	cameraMode = (CameraMode)o->cameraModeSaved;
	cameraTarget = o->cameraTargetSaved;

	//Load the current collactbles
	currentCollectables = o->currentCollectablesSaved;
	totalCollectables = o->totalCollectablesSaved;

	//Load scripts.
	copyCompiledScripts(state, o->savedCompiledScripts, compiledScripts);

	//Load other state, for example moving blocks.
	copyLevelObjects(o->levelObjectsSave, levelObjects, true);

	//Load the state for script executor.
	getScriptExecutor()->loadState(&o->scriptExecutorSaved);

	//Check if we have onLoad event.
	if (compiledScripts.find(LevelEvent_OnLoad) != compiledScripts.end()) {
		//Backup old SAVESTATE.
		lua_getglobal(state, "SAVESTATE");
		int oldIndex = luaL_ref(state, LUA_REGISTRYINDEX);

		//Prepare the SAVESTATE variable for onLoad event.
		lua_rawgeti(state, LUA_REGISTRYINDEX, o->scriptSaveState);
		lua_setglobal(state, "SAVESTATE");

		//Execute the onLoad event.
		executeScript(LevelEvent_OnLoad);

		//Restore old SAVESTATE.
		lua_rawgeti(state, LUA_REGISTRYINDEX, oldIndex);
		luaL_unref(state, LUA_REGISTRYINDEX, oldIndex);
		lua_setglobal(state, "SAVESTATE");
	}

	//TODO: load scenery layers, background animation,
}

bool Game::saveState(){
	//Check if the player and shadow can save the current state.
	if(canSaveState()){
		//Let the player and the shadow save their state.
		player.saveState();
		shadow.saveState();

		//Save the game state.
		saveGameOnlyStateInternal();

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
			if (saveStateByShadow) statsMgr.shadowSaveTimes++;
			else statsMgr.playerSaveTimes++;
			
			//Update achievements
			switch (statsMgr.playerSaveTimes + statsMgr.shadowSaveTimes) {
			case 100:
				statsMgr.newAchievement("save100");
				break;
			}
		}

		//Return true.
		return true;
	}

	//We can't save the state so return false.
	return false;
}

bool Game::loadState(){
	//Check if there's a state that can be loaded.
	if(player.canLoadState() && shadow.canLoadState()){
		//Reset the stats for level editor.
		if (stateID == STATE_LEVEL_EDITOR) {
			if (auto editor = dynamic_cast<LevelEditor*>(this)) {
				editor->currentTime = editor->currentRecordings = -1;
				editor->updateAdditionalTexture(getImageManager(), getRenderer());
			}
		}

		//Let the player and the shadow load their state.
		player.loadState();
		shadow.loadState();

		//Load the game state.
		loadGameOnlyStateInternal();

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
	saveStateByShadow = false;
	loadStateNextTime=false;

	//Reset the stats.
	time=0;
	recordings=0;

	recentSwap=-10000;
	if(save) recentSwapSaved=-10000;

	//Reset the stats for level editor.
	if (stateID == STATE_LEVEL_EDITOR) {
		if (auto editor = dynamic_cast<LevelEditor*>(this)) {
			editor->currentTime = editor->currentRecordings = -1;
			editor->updateAdditionalTexture(getImageManager(), getRenderer());
		}
	}

	//Reset the pseudo-random number generator by creating a new seed, unless we are playing from record.
	if (levelFile == "?record?" || interlevel) {
		if (prngSeed.empty()) {
			cout << "WARNING: The record file doesn't provide a random seed! Will use a default random seed! "
				"This may breaks the behavior pseudo-random number generator in script!" << endl;
			prngSeed = std::string(32, '0');
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
	{
		std::seed_seq seq(prngSeed.begin(), prngSeed.end());
		prng.seed(seq);
	}
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

		//Reset some variables.
		scriptSaveState = LUA_REFNIL;
		if (scriptExecutorSaved.savedDelayExecutionObjects) {
			// Set the state to NULL since the state is already destroyed. This prevents the destructor to call luaL_unref.
			scriptExecutorSaved.savedDelayExecutionObjects->state = NULL;
			delete scriptExecutorSaved.savedDelayExecutionObjects;
			scriptExecutorSaved.savedDelayExecutionObjects = NULL;
		}

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

		//Reset some variables.
		scriptSaveState = LUA_REFNIL;
		if (scriptExecutorSaved.savedDelayExecutionObjects) {
			// Set the state to NULL since the state is already destroyed. This prevents the destructor to call luaL_unref.
			scriptExecutorSaved.savedDelayExecutionObjects->state = NULL;
			delete scriptExecutorSaved.savedDelayExecutionObjects;
			scriptExecutorSaved.savedDelayExecutionObjects = NULL;
		}

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

		//Check if <levelname>.lua is present.
		{
			std::string s = scriptFile;
			if (s.empty() && !levelFile.empty() && levelFile[0] != '?') {
				s = levelFile;
				size_t lp = s.find_last_of('.');
				if (lp != std::string::npos) {
					s = s.substr(0, lp);
				}
				s += ".lua";
			}

			int index = LUA_REFNIL;

			//Now compile the file.
			if (!s.empty()) {
				if (s[0] == '?') {
					index = getScriptExecutor()->compileScript(s.substr(1));
				} else {
					FILE *f = fopen(s.c_str(), "r");
					if (f) {
						fclose(f);
						index = getScriptExecutor()->compileFile(s);
					}
				}
			}

			if (index != LUA_REFNIL) {
				compiledScripts[LevelEvent_BeforeCreate] = index;
				lua_rawgeti(getScriptExecutor()->getLuaState(), LUA_REGISTRYINDEX, index);
				savedCompiledScripts[LevelEvent_BeforeCreate] = luaL_ref(getScriptExecutor()->getLuaState(), LUA_REGISTRYINDEX);
				lua_rawgeti(getScriptExecutor()->getLuaState(), LUA_REGISTRYINDEX, index);
				initialCompiledScripts[LevelEvent_BeforeCreate] = luaL_ref(getScriptExecutor()->getLuaState(), LUA_REGISTRYINDEX);
			}
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

	//Send GameObjectEvent_OnCreate event to the script
	for (auto block : levelObjects) {
		block->onEvent(GameObjectEvent_OnCreate);
	}

	//Call the level's onCreate event.
	executeScript(LevelEvent_BeforeCreate);
	executeScript(LevelEvent_OnCreate);

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
		//Check the Expert Survivalist achievement.
		if (expertSurvivalistIsOngoing) statsMgr.newAchievement("survivalist2");
		expertSurvivalistIsOngoing = false;
		//Show a congratulations dialog.
		if (!levels->congratulationText.empty()){
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

		//Set variable for Expert Survivalist achievement.
		expertSurvivalistIsOngoing = false;

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
	} else if (name == "cmdLoad") {
		if (player.canLoadState() && shadow.canLoadState()) {
			//Clear the gui.
			if (GUIObjectRoot){
				delete GUIObjectRoot;
				GUIObjectRoot = NULL;
			}

			interlevel = false;

			//And load the game.
			player.playRecord(-1);
			loadStateNextTime = true;
		}
	} else if (name == "cmdSaveReplay") {
		//Open a message popup.
		GUIObject *root = new GUIFrame(imageManager, renderer, (SCREEN_WIDTH - 700) / 2, (SCREEN_HEIGHT - 240) / 2, 700, 240, _("Save replay"));
		GUIObject *obj;

		obj = new GUILabel(imageManager, renderer, 40, 50, 240, 36, _("Replay file name:"));
		root->addChild(obj);

		string s = levels->getLevelAutoSaveRecordPrefix(-1);
		s += '-';
		s += Md5::toString(levels->getLevelMD5());
		s += "-";

		obj = new GUILabel(imageManager, renderer, 60, 90, 580, 36, s.c_str());
		root->addChild(obj);

		time_t rawtime;
		struct tm *timeinfo;
		char buffer[256];

		::time(&rawtime);
		timeinfo = localtime(&rawtime);

		strftime(buffer, sizeof(buffer), "%Y%m%d-%H%M%S", timeinfo);

		obj = new GUITextBox(imageManager, renderer, 60, 120, 460, 36, buffer);
		obj->name = "ReplayFileName";
		root->addChild(obj);

		obj = new GUILabel(imageManager, renderer, 520, 120, 180, 36, ".mnmsrec");
		root->addChild(obj);

		GUIButton *okButton = new GUIButton(imageManager, renderer, root->width*0.3, 240 - 44, -1, 36, _("OK"), 0, true, true, GUIGravityCenter);
		okButton->name = "SaveReplayOK";
		okButton->eventCallback = this;
		root->addChild(okButton);
		GUIButton *cancelButton = new GUIButton(imageManager, renderer, root->width*0.7, 240 - 44, -1, 36, _("Cancel"), 0, true, true, GUIGravityCenter);
		cancelButton->name = "SaveReplayCancel";
		cancelButton->eventCallback = this;
		root->addChild(cancelButton);

		//Create the gui overlay.
		//NOTE: We don't need to store a pointer since it will auto cleanup itself.
		new AddonOverlay(renderer, root, okButton, cancelButton, NULL, UpDownFocus | TabFocus);
	} else if (name == "SaveReplayOK") {
		if (GUIObjectRoot) {
			if (auto obj = GUIObjectRoot->getChild("ReplayFileName")) {
				string s = obj->caption;

				if (s.empty()) {
					msgBox(imageManager, renderer, _("No file name given for the replay file."), MsgBoxOKOnly, _("Missing file name"));
					return;
				}

				if (s == "best-time" || s == "best-recordings") {
					msgBox(imageManager, renderer, tfm::format(_("The '%s' is a reserved word."), s), MsgBoxOKOnly, _("Error"));
					return;
				}

				s = levels->getLevelAutoSaveRecordPrefix(-1) + "-" + Md5::toString(levels->getLevelMD5()) + "-" + s + ".mnmsrec";
				s = escapeFileName(s);

				string fileName = levels->getLevelpackAutoSaveRecordPath(true) + s;

				//First check if the file doesn't exist already.
				FILE* f;
				f = fopen(fileName.c_str(), "rb");

				//Check if it exists.
				if (f){
					//Close the file.
					fclose(f);

					//Notify the user.
					msgBox(imageManager, renderer, tfm::format(_("The file %s already exists."), s), MsgBoxOKOnly, _("Error"));
					return;
				}

				//Now we save the file.
				saveRecord(fileName.c_str());

				//Notify the user.
				msgBox(imageManager, renderer, tfm::format(_("The replay is saved to '%s'."), s), MsgBoxOKOnly, _("Save replay"));

				delete GUIObjectRoot;
				GUIObjectRoot = NULL;
			}
		}
	} else if (name == "SaveReplayCancel") {
		if (GUIObjectRoot) {
			delete GUIObjectRoot;
			GUIObjectRoot = NULL;
		}
	}
}

void Game::invalidateNotificationTexture(Block *block) {
	if (block == NULL || block == notificationTexture.getId()) {
		notificationTexture.update(NULL, NULL);
	}
}
