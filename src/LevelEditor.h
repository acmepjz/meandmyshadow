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

#ifndef LEVELEDITOR_H
#define LEVELEDITOR_H

#include "CachedTexture.h"
#include "GameState.h"
#include "GameObjects.h"
#include "Player.h"
#include "Game.h"
#include "GUIObject.h"
#include <vector>
#include <map>
#include <string>

enum class ToolTips {
    Select = 0,
    Add,
    Delete,
    Play,
    UndoNoTooltip, // dynamically generated
	RedoNoTooltip, // dynamically generated
	LevelSettings,
    SaveLevel,
    BackToMenu,
	Select_UsedInSelectionPopup,
	Delete_UsedInSelectionPopup,
	Configure_UsedInSelectionPopup,
    TooltipMax,
};

//Class that represents a moving position for moving blocks.
class MovingPosition{
public:
	//Integer containing the relative time used to store in the level.
	int time;
	//The x location.
	int x;
	//The x location.
	int y;

	//Constructor.
	//x: The x position relative to the moving block's position.
	//y: The y position relative to the moving block's position.
	//time: The time it takes from the previous position to here.
	MovingPosition(int x,int y,int time);
	//Destructor.
	~MovingPosition();

	//This will update the moving position.
	//x: The x position relative to the moving block's position.
	//y: The y position relative to the moving block's position.
	void updatePosition(int x,int y);
};

//Internal selection popup class.
class LevelEditorSelectionPopup;

//Internal actions popup class.
class LevelEditorActionsPopup;

class CommandManager;

class AddRemoveGameObjectCommand;
class MoveGameObjectCommand;
class AddLinkCommand;
class RemoveLinkCommand;
class AddRemovePathCommand;
class RemovePathCommand;
class SetLevelPropertyCommand;
class SetScriptCommand;
class AddRemoveLayerCommand;
class SetLayerPropertyCommand;
class MoveToLayerCommand;
class SetEditorPropertyCommand;
class ResizeLevelCommand;

class HelpManager;

//The LevelEditor state, it's based on the Game state.
class LevelEditor: public Game{
	friend class Game;
	friend class LevelEditorSelectionPopup;
	friend class LevelEditorActionsPopup;

	friend class AddRemoveGameObjectCommand;
	friend class MoveGameObjectCommand;
	friend class AddLinkCommand;
	friend class RemoveLinkCommand;
	friend class AddRemovePathCommand;
	friend class RemovePathCommand;
	friend class SetLevelPropertyCommand;
	friend class SetScriptCommand;
	friend class AddRemoveLayerCommand;
	friend class SetLayerPropertyCommand;
	friend class MoveToLayerCommand;
	friend class SetEditorPropertyCommand;
	friend class ResizeLevelCommand;
private:
	//Boolean if the user isplaying/testing the level.
	bool playMode;

	//Enumaration containing the tools.
	//SELECT: The select tool, for selecting/dragging blocks.
	//ADD: For adding blocks.
	//REMOVE: For removing blocks.
	enum Tools{
		SELECT,
		ADD,
		REMOVE,
		NUMBER_TOOLS
	};
	//The tool the user has selected.
	Tools tool;
	//The toolbar surface.
    SharedTexture toolbar;
	//Rectangle the size and location of the toolbar on screen.
	SDL_Rect toolbarRect;

	//The selection popup (if any)
	LevelEditorSelectionPopup* selectionPopup;
	//The actions popup (if any)
	LevelEditorActionsPopup* actionsPopup;

	//Map used to get the GameObject that belongs to a certain GUIWindow.
	map<GUIObject*,GameObject*> objectWindows;

	//Map which store the visibility of each scenery layers, "" (empty) means the Block layer
	map<string, bool> layerVisibility;

	//The selected layer, "" (empty) means the Block layer
	string selectedLayer;

	//Vector containing pointers to the selected GameObjects.
	vector<GameObject*> selection;
	//The selection square.
    SharedTexture selectionMark;

	//A circle at the location of moving positions in configure mode.
    SharedTexture movingMark;

    //Texture showing the movement speed.
    CachedTexture<int> movementSpeedTexture;

	//Texture showing the pause time
	CachedTexture<int> pauseTimeTexture;

	//GUI image.
    SharedTexture bmGUI;
    //Texture containing the text "Toolbox"
    TexturePtr toolboxText;

	//Keeps track of commands for undo and redo.
	CommandManager* commandManager;

	//The current type of block to place in Add mode.
	int currentType;

    std::array<TexturePtr,TYPE_MAX> typeTextTextures;
    std::array<TexturePtr,static_cast<size_t>(ToolTips::TooltipMax)> tooltipTextures;

	CachedTexture<std::string> undoTooltipTexture;
	CachedTexture<std::string> redoTooltipTexture;

	std::map<std::string, TexturePtr> cachedTextTextures;

	TexturePtr& getCachedTextTexture(SDL_Renderer& renderer, const std::string& text);
	TexturePtr& getSmallCachedTextTexture(SDL_Renderer& renderer, const std::string& text);

	//Boolean if the tool box is displayed.
	bool toolboxVisible;
	//The rect of tool box tip.
	SDL_Rect toolboxRect;
	//The first item in tool box.
	int toolboxIndex;

	//Boolean if the shift button is pressed.
	bool pressedShift;
	//Boolean if the left mouse button is pressed.
	bool pressedLeftMouse;
	//Boolean if the mouse is dragged. (Left button pressed and moved)
	bool dragging;

	//The camera x velocity.
	int cameraXvel;
	int cameraYvel;
	//SDL_Rect used to store the camera's location when entering playMode.
	SDL_Rect cameraSave;

	//Integer indicating if the selection is dragged and the drag mode.
	// -1: not dragged
	// 4: dragged
	// 0,1,2,3,5,6,7,8: resizing the dragCenter
	// 012
	// 3x5
	// 678
	int selectionDrag;

	//Pointer to the gameobject that's the center of the drag.
	GameObject* dragCenter;

	// The drag start position which is used when dragging blocks
	SDL_Point dragSrartPosition;

	//Integer containing a unique id.
	//Everytime a new id is needed it will increase by one.
	unsigned int currentId;

	typedef map<Block*, vector<GameObject*> > Triggers;

	//Vector containing the trigger GameObjects.
	Triggers triggers;
	//Boolean used in configure mode when linking triggers with their targets.
	bool linking;
	//Pointer to the trigger that's is being linked.
	Block* linkingTrigger;

	//Vector containing the moving GameObjects.
	map<Block*,vector<MovingPosition> > movingBlocks;
	//Integer containing the speed the block is moving for newly added blocks. 1 movingSpeed = 0.1 pixel/frame = 0.08 block/s
	//The movingSpeed is capped at 125 (10 block/s).
	int movingSpeed;
	//The pause time for path edit if the current point is equal to the previous time. 1 pauseTime = 1 frame = 0.04s
	int pauseTime;
	//Boolean used in configure mode when configuring moving blocks.
	bool moving;
	//Another boolean used in configure mode when configuring moving blocks.
	bool pauseMode;
	//Pointer to the moving block that's is being configured.
	Block* movingBlock;
	
	//Value used for placing the Movespeed label
	int movingSpeedWidth;

	//The clipboard.
	vector<map<string,string> > clipboard;

	//String containing the levelTheme.
	std::string levelTheme;

	//String containing the levelMusic.
	std::string levelMusic;

	//Integer containing the button of which a tool tip should be shown.
	int tooltip;

	//The target time and recordings of the current editing level.
	int levelTime, levelRecordings;

	//The current time and recordings of the current editing level.
	int currentTime, currentRecordings;

	//The best time and recordings of the current editing level.
	int bestTime, bestRecordings;

	//The help manager for script API document.
	HelpManager *helpMgr;

	//GUI event handling is done here.
    void GUIEventCallback_OnEvent(ImageManager&, SDL_Renderer&, std::string name,GUIObject* obj,int eventType);

	//Method for deleting a GUIWindow.
	//NOTE: This function checks for the presence of the window in the GUIObjectRoot and objectWindows map.
	//window: Pointer to the GUIWindow.
	void destroyWindow(GUIObject* window);

	//Method that will let you configure the levelSettings.
    void levelSettings(ImageManager& imageManager,SDL_Renderer &renderer);

	//Method used to save the level.
	//fileName: Thge filename to write the level to.
	void saveLevel(string fileName);

	//Method used to convert a given x and y to snap to grid.
	//x: Pointer to the x location.
	//y: Pointer to the y location.
	void snapToGrid(int* x,int* y);

	//Method used to check if the cursor is near the border of screen and we should move the camera.
	//This method will check if the mouse is near a screen edge.
	//r: An array of SDL_Rect, does nothing if mouse inside these rectange(s).
	//count: Number of rectangles.
	//If so it will move the camera.
	void setCamera(const SDL_Rect* r,int count);

	//Just an internal function.
	static std::string describeSceneryName(const std::string& name);

	//Array containing translateble block names
	static const char* blockNames[TYPE_MAX];

public:
	//Array containing the ids of different block types in a wanted order
	//Maybe also useful to disable deprecated block types in the editor
	//PLEASE NOTE: Must be updated for new block types
	//Ordered for Edward Liis proposal:

	//Normal->Shadow->Spikes->Fragile
	//Normal moving->Shadow moving->Moving spikes
	//Conveyor belt->Shadow conveyor belt
	//Button->Switch->Portal->Swap->Checkpoint->Notification block
	//Player start->Shadow start->Exit
	//Collectable->Pushable

	static const int EDITOR_ORDER_MAX=20;
	static const int editorTileOrder[EDITOR_ORDER_MAX];

	//Get the localized block name
	static std::string getLocalizedBlockName(int type);

	//Array containing the names of available scenery blocks
	std::vector<std::string> sceneryBlockNames;

	// get the number of available blocks depending on the selected layer
	int getEditorOrderMax() const {
		if (selectedLayer.empty()) return EDITOR_ORDER_MAX;
		return sceneryBlockNames.size() + 1; // the added one is for custom scenery block
	}

protected:
	//Inherits the function loadLevelFromNode from Game class.
	virtual void loadLevelFromNode(ImageManager& imageManager, SDL_Renderer& renderer, TreeStorageNode* obj, const std::string& fileName, const std::string& scriptFileName) override;

public:
	//Constructor.
    LevelEditor(SDL_Renderer &renderer, ImageManager &imageManager);
	//Destructor.
    ~LevelEditor();

	//Method that will reset some default values.
	void reset();

	//Inherited from Game(State).
    void handleEvents(ImageManager& imageManager, SDL_Renderer& renderer) override;
    void logic(ImageManager& imageManager, SDL_Renderer& renderer) override;
    void render(ImageManager& imageManager, SDL_Renderer& renderer) override;
    void resize(ImageManager& imageManager, SDL_Renderer& renderer) override;

	//Method used to draw the currentType on the placement surface.
	//This will only be called when the tool is ADD.
    void showCurrentObject(SDL_Renderer &renderer);
	//Method used to draw the selection that's being dragged.
    void showSelectionDrag(SDL_Renderer &renderer);
	//Method used to draw configure tool specific things like moving positions, teleport lines.
    void showConfigure(SDL_Renderer &renderer);

	//Method that will render the HUD.
	//It will be rendered after the placement suface but before the toolbar.
    void renderHUD(SDL_Renderer &renderer);

	//Method called after loading a level.
	//It will fill the triggers vector.
	void postLoad();

	//Event that is invoked when there's a mouse click on an object.
	//obj: Pointer to the GameObject clicked on.
	//selected: Boolean if the GameObject that has been clicked on was selected.
	void onClickObject(GameObject* obj,bool selected);
	//Event that is invoked when there's a right mouse button click on an object.
	//obj: Pointer to the GameObject clicked on.
	//selected: Boolean if the GameObject that has been clicked on was selected.
    void onRightClickObject(ImageManager& imageManager, SDL_Renderer& renderer, GameObject* obj, bool);
	//Event that is invoked when there's a mouse click but not on any object.
	//x: The x location of the click on the game field (+= camera.x).
	//y: The y location of the click on the game field (+= camera.y).
	void onClickVoid(int x,int y);
	//Event that is invoked when there's a mouse right click but not on any object.
	//x: The x location of the click on the game field (+= camera.x).
	//y: The y location of the click on the game field (+= camera.y).
    void onRightClickVoid(ImageManager& imageManager, SDL_Renderer& renderer, int x,int y);
	//Event that is invoked when the dragging starts.
	//x: The x location the drag started. (mouse.x+camera.x)
	//y: The y location the drag started. (mouse.y+camera.y)
	void onDragStart(int x,int y);
	//Event that is invoked when the mouse is dragged.
	//dx: The relative x distance the mouse dragged.
	//dy: The relative y distance the mouse dragged.
	void onDrag(int dx,int dy);
	//Event that is invoked when the mouse stopped dragging.
	//x: The x location the drag stopped. (mouse.x+camera.x)
	//y: The y location the drag stopped. (mouse.y+camera.y)
	void onDrop(int x,int y);
	//Event that is invoked when the camera is moved.
	//dx: The relative x distance the camera moved.
	//dy: The relative y distance the camera moved.
	void onCameraMove(int dx,int dy);

	//internal function called by onClickObject() and onClickVoid().
	void addMovingPosition(int x,int y);

	//Set dirty of selection popup
	void selectionDirty();
	//Deselect all blocks
	void deselectAll();

	//Save current level and show a notification dialog
	void saveCurrentLevel(ImageManager& imageManager, SDL_Renderer& renderer);

	// An internal function to determine new position in drag mode.
	// Make sure selectionDrag=4 when calling this function.
	void determineNewPosition(int& x, int& y);
	// An internal function to determine new size in resize mode.
	// Make sure selectionDrag=0,1,2,3,5,6,7,8 when calling this function.
	void determineNewSize(int x, int y, SDL_Rect& r);

	//Call this function to start test play.
	void enterPlayMode(ImageManager& imageManager, SDL_Renderer& renderer);

	//Update the additional texture displayed in test play.
	void updateAdditionalTexture(ImageManager& imageManager, SDL_Renderer& renderer);

	//Update the record in play mode.
	void updateRecordInPlayMode(ImageManager& imageManager, SDL_Renderer& renderer);

	void undo();
	void redo();

	//Get the GUI texture.
    inline SharedTexture& getGuiTexture() {
        return bmGUI;
    }

	//Get the play mode.
	bool isPlayMode() const {
		return playMode;
	}
};
#endif
