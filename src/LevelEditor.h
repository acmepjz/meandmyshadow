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
    //This is at this position as there is a space
    //here on the toolbar.
    NoTooltip,
    LevelSettings,
    SaveLevel,
    BackToMenu,
    Configure,
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

//The LevelEditor state, it's based on the Game state.
class LevelEditor: public Game{
	friend class LevelEditorSelectionPopup;
	friend class LevelEditorActionsPopup;
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

	//Vector containing pointers to the selected GameObjects.
	vector<GameObject*> selection;
	//The selection square.
    SharedTexture selectionMark;

	//A circle at the location of moving positions in configure mode.
    SharedTexture movingMark;

    //Texture showing the movement speed.
    CachedTexture<int> movementSpeedTexture;

	//GUI image.
    SharedTexture bmGUI;
    //Texture containing the text "Toolbox"
    TexturePtr toolboxText;

	//The current type of block to place in Add mode.
	int currentType;

    std::array<TexturePtr,TYPE_MAX> typeTextTextures;
    std::array<TexturePtr,static_cast<size_t>(ToolTips::TooltipMax)> tooltipTextures;

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

	//Boolean if the selection is dragged.
	bool selectionDrag;
	//Pointer to the gameobject that's the center of the drag.
	GameObject* dragCenter;

	//Integer containing a unique id.
	//Everytime a new id is needed it will increase by one.
	unsigned int currentId;

	//Vector containing the trigger GameObjects.
	map<Block*,vector<GameObject*> > triggers;
	//Boolean used in configure mode when linking triggers with their targets.
	bool linking;
	//Pointer to the trigger that's is being linked.
	Block* linkingTrigger;

	//Vector containing the moving GameObjects.
	map<Block*,vector<MovingPosition> > movingBlocks;
	//Integer containing the speed the block is moving for newly added blocks.
	//The movingSpeed is capped at 100.
	int movingSpeed;
	//Boolean used in configure mode when configuring moving blocks.
	bool moving;
	//Pointer to the moving block that's is being configured.
	Block* movingBlock;
	
	//Value used for placing the Movespeed label
	int movingSpeedWidth;

	//The clipboard.
	vector<map<string,string> > clipboard;

	//String containing the levelTheme.
	std::string levelTheme;

	//Integer containing the button of which a tool tip should be shown.
	int tooltip;

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

protected:
	//Inherits the function loadLevelFromNode from Game class.
    virtual void loadLevelFromNode(ImageManager& imageManager, SDL_Renderer& renderer, TreeStorageNode* obj, const std::string& fileName) override;

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
	//Event that is invoked when enter is pressed above an object.
	//obj: Pointer to the GameObject entered above.
	void onEnterObject(GameObject* obj);

	//Method used to add a GameObject to the level.
	//obj: Pointer to the gameobject to add.
	void addObject(GameObject* obj);
	//Method used to move a GameObject from the level.
	//obj: Pointer to the gameobject to move.
	//x: The new x location of the GameObject.
	//y: The new y location of the GameObject.
	void moveObject(GameObject* obj,int x,int y);
	//Method used to remove a GameObject from the level.
	//obj: Pointer to the gameobject to remove.
	void removeObject(GameObject* obj);

	//Call this function to start test play.
	void enterPlayMode();

    inline SharedTexture& getGuiTexture() {
        return bmGUI;
    }
};
#endif
