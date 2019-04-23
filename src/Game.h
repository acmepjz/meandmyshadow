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

#ifndef GAME_H
#define GAME_H

#include <SDL.h>
#include <array>
#include <vector>
#include <map>
#include <string>
#include <random>

#include "CachedTexture.h"
#include "GameState.h"
#include "GUIObject.h"
#include "Block.h"
#include "Scenery.h"
#include "SceneryLayer.h"
#include "Player.h"
#include "Render.h"
#include "Shadow.h"

//This structure contains variables that make a GameObjectEvent.
struct typeGameObjectEvent{
	//The type of event.
	int eventType;
	//The type of object that should react to the event.
	int objectType;
	//Flags, 0x1 means use the id.
	int flags;
	//Blocks with this id should react to the event.
	std::string id;

	//Optional pointer to the block the event should be called on.
	GameObject* target;
};

class ThemeManager;
class ThemeBackground;
class TreeStorageNode;
class ScriptExecutor;

//The different level events.
enum LevelEventType{
	//Event called when the level is created or the game is reset. This happens after all the blocks are created and their onCreate is called.
	LevelEvent_OnCreate=1,
	//Event called when the game is saved.
	LevelEvent_OnSave,
	//Event called when the game is loaded.
	LevelEvent_OnLoad,

	//Same as LevelEvent_OnCreate, but this is for the script saved in <levelname>.lua
	LevelEvent_BeforeCreate = 101,
};

struct GameOnlySaveState {
	//The level rect.
	//NOTE: the x,y of these rects can only be changed by script.
	//If not changed by script, they are always 0,0.
	SDL_Rect levelRectSaved;

	//The pseudo-random number generator which is mainly used in script.
	std::mt19937 prngSaved;

	//The seed of the pseudo-random number generator, which will be saved to and load from replay.
	std::string prngSeedSaved;

	//Integer containing the stored value of time.
	int timeSaved;

	//Integer containing the stored value of recordings.
	int recordingsSaved;

	//Integer keeping track of currently obtained collectibles
	int currentCollectablesSaved;

	//Integer keeping track of total collectibles in the level
	int totalCollectablesSaved;

	//Time of recent swap, for achievements. (in game-ticks)
	int recentSwapSaved;

	//The saved cameraMode.
	int cameraModeSaved;

	//The saved cameraTarget.
	SDL_Rect cameraTargetSaved;

	//Compiled scripts. Use lua_rawgeti(L, LUA_REGISTRYINDEX, r) to get the function.
	std::map<int, int> savedCompiledScripts;

	//Vector containing all the levelObjects in the current game.
	std::vector<Block*> levelObjectsSave;
};

struct GameSaveState {
	//The game save state.
	GameOnlySaveState gameSaved;

	//The player and shadow save state.
	PlayerSaveState playerSaved, shadowSaved;
};

class Game : public GameState, public GUIEventCallback, protected GameOnlySaveState {
private:
	//Boolean if the game should reset. This happens when player press 'R' button.
	bool isReset;

	//The script executor.
	ScriptExecutor *scriptExecutor;

protected:
	//contains currently played level.
	TreeStorageNode* currentLevelNode;

	//"tooltips" for level names, etc.
	//It will be shown in the topleft corner of the screen.
	TexturePtr bmTipsLevelName, bmTipsShadowDeath, bmTipsRestart, bmTipsRestartCheckpoint;

    //Texture containing the action images (record, play, etc..)
    SharedTexture action;
    //Texture containing the medal image.
    SharedTexture medals;

	//ThemeBlockInstance containing the collectable image.
	ThemeBlockInstance collectable;

	//The name of the current level.
	std::string levelName;
	//The path + file of the current level.
	std::string levelFile;

	//The path + file of the external script file.
	//If it's empty then load the levelFile with extension replaced by ".lua".
	//If starts with "?" then load from string.
	//Only used when the scripts are recompiled in reset() function.
	std::string scriptFile;

	//Editor data containing information like name, size, etc...
	std::map<std::string,std::string> editorData;

	//Vector used to queue the gameObjectEvents.
	std::vector<typeGameObjectEvent> eventQueue;

	//The themeManager.
	ThemeManager* customTheme;
	//The themeBackground.
	ThemeBackground* background;

	//Texture containing the label of the number of collectibles.
    CachedTexture<int> collectablesTexture;
	//Texture containing the label of the number of recordings (mainly used in level editor).
	CachedTexture<int> recordingsTexture;
	//Texture containing the label of the time (mainly used in level editor).
	CachedTexture<int> timeTexture;
	//Texture containing the notification of message block.
	CachedTexture<Block*> notificationTexture;
	//Texture containing the notification of interactive block.
	CachedTexture<std::string> gameTipTexture;
	//Texture containing the target time, etc. which is only used in level editor.
	SharedTexture additionalTexture;

	//Load a level from node.
	//After calling this function the ownership of
	//node will transfer to Game class. So don't delete
	//the node after calling this function!
	virtual void loadLevelFromNode(ImageManager& imageManager, SDL_Renderer& renderer, TreeStorageNode* obj, const std::string& fileName, const std::string& scriptFileName);

	//(internal function) Reload the music according to level music and level pack music.
	//This is mainly used in level editor.
	void reloadMusic();

public:
	//Array used to convert GameObject type->string.
	static const char* blockName[TYPE_MAX];
	//Map used to convert GameObject string->type.
	static std::map<std::string,int> blockNameMap;

	//Map used to convert GameObjectEventType type->string.
	static std::map<int,std::string> gameObjectEventTypeMap;
	//Map used to convert GameObjectEventType string->type.
	static std::map<std::string,int> gameObjectEventNameMap;

	//Map used to convert LevelEventType type->string.
	static std::map<int,std::string> levelEventTypeMap;
	//Map used to convert LevelEventType string->type.
	static std::map<std::string,int> levelEventNameMap;

	//The level rect.
	//NOTE: the x,y of these rects can only be changed by script.
	//If not changed by script, they are always 0,0.
	SDL_Rect levelRect, levelRectInitial;

	//The pseudo-random number generator which is mainly used in script.
	std::mt19937 prng;

	//The seed of the pseudo-random number generator, which will be saved to and load from replay.
	std::string prngSeed;

	//Boolean that is set to true if the level is arcade mode.
	bool arcade;

	//Boolean that is set to true when a game is won.
	bool won;

	//Boolean that is set to true when we should save game on next logic update.
	bool saveStateNextTime;

	//Boolean that is set to true when we should load game on next logic update.
	bool loadStateNextTime;

	//Boolean if the replaying currently done is for the interlevel screen.
	bool interlevel;

	//String containing the current tip shown in top left corner.
	std::string gameTipText;

	//Integer containing the number of ticks passed since the start of the level.
	int time;

	//Integer containing the number of recordings it took to finish.
	int recordings;

	//Integer keeping track of currently obtained collectibles
	int currentCollectables, currentCollectablesInitial;
	//Integer keeping track of total collectibles in the level
	int totalCollectables, totalCollectablesInitial;

	//Time of recent swap, for achievements. (in game-ticks)
	int recentSwap;

	//Store time of recent save/load for achievements (in millisecond)
	Uint32 recentLoad,recentSave;

	//Enumeration with the different camera modes.
	enum CameraMode{
		CAMERA_PLAYER,
		CAMERA_SHADOW,
		CAMERA_CUSTOM
	};
	//The current camera mode.
	CameraMode cameraMode;
	//Rectangle containing the target for the camera.
	SDL_Rect cameraTarget;

	//Level scripts.
	std::map<int,std::string> scripts;

	//Compiled scripts. Use lua_rawgeti(L, LUA_REGISTRYINDEX, r) to get the function.
	std::map<int, int> compiledScripts, initialCompiledScripts;

	//Vector containing all the levelObjects in the current game.
	std::vector<Block*> levelObjects, levelObjectsInitial;

	//The layers for the scenery.
	//
	// We utilize the fact that std::map is sorted, and we compare the layer name with "f",
	// If name<"f" then it's background layer, if name>="f" then it's foreground layer.
	//
	// However, we hide this complexity from users, so when creating/editing layers,
	// the user is asked to choose whether the layer is foreground.
	//
	// In fact the background layer is always named "bg_"+actual name of the layer
	// and the foreground layer is always named "fg_"+actual name of the layer
	std::map<std::string, SceneryLayer*> sceneryLayers;
	
	//The player...
	Player player;
	//... and his shadow.
	Shadow shadow;

	//warning: weak reference only, may point to invalid location
	Block::ObservePointer objLastCheckPoint;

	//Constructor.
    Game(SDL_Renderer& renderer, ImageManager& imageManager);
	//If this is not empty then when next Game class is created
	//it will play this record file.
	static std::string recordFile;
	//Destructor.
	//It will call destroy();
	~Game();

	//Method used to clean up the GameState.
	void destroy();

	//Inherited from GameState.
    void handleEvents(ImageManager&imageManager, SDL_Renderer&renderer) override;
    void logic(ImageManager& imageManager, SDL_Renderer& renderer) override;
    void render(ImageManager&,SDL_Renderer& renderer) override;
    void resize(ImageManager& imageManager, SDL_Renderer& renderer) override;

	//Check if we should save/load state.
	//This function is called by logic(). The RecordPlayback override it to provide seeking support.
	virtual void checkSaveLoadState();

	//This method will load a level.
	//fileName: The fileName of the level.
    virtual void loadLevel(ImageManager& imageManager, SDL_Renderer& renderer, std::string fileName);

	//Method used to broadcast a GameObjectEvent.
	//eventType: The type of event.
	//objectType: The type of object that should react to the event.
	//id: The id of the blocks that should react.
	//target: Pointer to the object 
	void broadcastObjectEvent(int eventType,int objectType=-1,const char* id=NULL,GameObject* target=NULL);

	//Compile all scripts and run onCreate script.
	//NOTE: Call this function only after script state reset, or there will be some memory leaks.
	void compileScript();

	//Method that will check if a script for a given levelEvent is present.
	//If that's the case the script will be executed.
	//eventType: The level event type to execute.
	void inline executeScript(int eventType);


	//Returns if the player and shadow can save the current state.
	bool canSaveState();

	//Method used to store the current state.
	//This is used for checkpoints.
	//Returns: True if it succeeds without problems.
	bool saveState();
	//Method used to load the stored state.
	//This is used for checkpoints.
	//Returns: True if it succeeds without problems.
	bool loadState();
	//Method that will reset the GameState to it's initial state.
	//save: Boolean if the saved state should also be deleted. This also means recreate the Lua context.
	//noScript: Boolean if we should not compile the script at all. This is used by level editor when exiting test play.
	void reset(bool save,bool noScript);

	void saveStateInternal(GameSaveState* o);
	void loadStateInternal(GameSaveState* o);
	void saveGameOnlyStateInternal(GameOnlySaveState* o);
	void loadGameOnlyStateInternal(GameOnlySaveState* o);

	//Save current game record to the file.
	//fileName: The filename of the destination file.
	void saveRecord(const char* fileName);

	//Method called by the player (or shadow) when he finished.
    void replayPlay(ImageManager& imageManager, SDL_Renderer &renderer);

	//get current level's auto-save record path,
	//using current level's MD5, file name and other information.
	void getCurrentLevelAutoSaveRecordPath(std::string &bestTimeFilePath,std::string &bestRecordingFilePath,bool createPath);

	//Method that will prepare the gamestate for the next level and start it.
	//If it's the last level it will show the congratulations text and return to the level select screen.
    void gotoNextLevel(ImageManager& imageManager, SDL_Renderer& renderer);

	//Get the name of the current level.
	const std::string& getLevelName(){
		return levelName;
	}

	//GUI event handling is done here.
    void GUIEventCallback_OnEvent(ImageManager&imageManager, SDL_Renderer&renderer, std::string name, GUIObject* obj, int eventType) override;

	//Get the script executor.
	ScriptExecutor* getScriptExecutor() {
		return scriptExecutor;
	}

	//Invalidates the notification texture.
	//block: The block which is updated. The cached texture won't be invalidated if it's not for this block.
	//NULL means invalidates the texture no matter which block is updated.
	void invalidateNotificationTexture(Block *block = NULL);

	//Translate and expand the message of a notification block.
	std::string translateAndExpandMessage(const std::string &untranslated_message);
};

#endif
