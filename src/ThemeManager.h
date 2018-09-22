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

#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include "Globals.h"
#include <string>
#include <vector>
#include <utility>
#include <iostream>
using namespace std;

class ImageManager;
class TreeStorageNode;

//Structure containing offset data for one frame.
struct ThemeOffsetPoint{
	//The location (x,y) and size (w,h).
	int x,y,w,h;
	//The frame to which this offset applies.
	int frameCount;
	//The number of frames this offset is shown.
	int frameDisplayTime;
};

//We already need the classes so declare them here.
class ThemeOffsetData;
class ThemePicture;
class ThemeObject;
class ThemeBlockState;
class ThemeBlock;

class Block;
class ThemeBlockInstance;

//Instance class of a ThemeObject, this is used by the other Instance classes.
class ThemeObjectInstance{
	friend class ThemeBlockInstance;
	friend class ThemeBlock;
private:
	//Pointer to the picture.
	ThemePicture* picture;
	//Pointer to the parent the object an instance os is.
	ThemeObject* parent;
	
	//Integer containing the current animation frame.
	int animation;
public:
	//Constructor.
	ThemeObjectInstance();

	//We use the system default copy constructor and operator=.

	//Method used to draw the ThemeObject.
	//dest: The destination surface to draw the ThemeObject on.
	//x: The x location of the area to draw in.
	//y: The y location of the area to draw in.
	//w: The width of the area to draw in.
	//h: The height of the area to draw in.
	//clipRect: Rectangle used to clip.
    void draw(SDL_Renderer& renderer,int x,int y,int w=0,int h=0,const SDL_Rect* clipRect=NULL);
	
	//Method that will update the animation.
	void updateAnimation();

	//NOTE: save/load/reset code is removed in favor of copy constructor based approach.
};

//Instance class of a ThemeBlockState, this is used by the ThemeBlockInstance.
class ThemeBlockStateInstance{
	friend class ThemeBlockInstance;
	friend class ThemeBlock;
private:
	//Pointer to the parent the state an instance of is.
	ThemeBlockState *parent;
	//Vector containing the ThemeObjectInstances.
	vector<ThemeObjectInstance> objects;
	
	//Integer containing the current animation frame.
	int animation;
public:
	//Constructor.
	ThemeBlockStateInstance();

	//We use the system default copy constructor and operator=.

	//Method used to draw the ThemeBlockState.
	//dest: The destination surface to draw the ThemeBlockState on.
	//x: The x location of the area to draw in.
	//y: The y location of the area to draw in.
	//w: The width of the area to draw in.
	//h: The height of the area to draw in.
	//clipRect: Rectangle used to clip.
	void draw(SDL_Renderer& renderer, int x, int y, int w = 0, int h = 0, const SDL_Rect *clipRect = NULL);
	
	//Method that will update the animation.
	void updateAnimation();

	//NOTE: save/load/reset code is removed in favor of copy constructor based approach.
};

//Instance of a ThemeBlock, this is used by blocks in the game to prevent changing the theme in game.
//It also allows animation to run independently.
class ThemeBlockInstance{
	friend class Block;
	friend class ThemeBlock;
private:
	//Index to the current state. -1 means invalid.
	//NOTE: We use the index instead of the pointer because it will make copy constructor works easier.
	int currentState;
	//The name of the current state.
	string currentStateName;
	
	//Map containing the index of blockStates.
	map<string,int> blockStates;
	//Map containing the index of blockTransitionStates.
	map<pair<string,string>,int> transitions;

	//The array which contains actual ThemeBlockStateInstance.
	vector<ThemeBlockStateInstance> states;
public:
	//Constructor.
	ThemeBlockInstance();

	//We use the system default copy constructor and operator=.

	//Clear the instance.
	void clear();

	//Method used to draw the ThemeBlock.
    //renderer: The destination renderer to draw the ThemeBlock on.
	//x: The x location of the area to draw in.
	//y: The y location of the area to draw in.
	//w: The width of the area to draw in.
	//h: The height of the area to draw in.
	//clipRect: Rectangle used to clip.
	//Returns: True if it succeeds.
	bool draw(SDL_Renderer& renderer, int x, int y, int w = 0, int h = 0, const SDL_Rect *clipRect = NULL);

	//Method that will draw a specific state.
	//s: The name of the state to draw.
	//dest: The destination surface to draw the ThemeBlock on.
	//x: The x location of the area to draw in.
	//y: The y location of the area to draw in.
	//w: The width of the area to draw in.
	//h: The height of the area to draw in.
	//clipRect: Rectangle used to clip.
	//Returns: True if it succeeds.
	bool drawState(const string& s, SDL_Renderer& renderer, int x, int y, int w = 0, int h = 0, SDL_Rect *clipRect = NULL);
	
	//Method that will change the current state.
	//s: The name of the state to change to.
	//reset: Boolean if the animation should reset.
	//onlyIfStateChanged: Boolean if the animation should be played only when the new state is not equal to the current state.
	//Returns: True if it succeeds (exists).
	bool changeState(const string& s, bool reset = true, bool onlyIfStateChanged = false);
	
	//Method that will update the animation.
	void updateAnimation();

	//NOTE: save/load/reset code is removed in favor of copy constructor based approach.
};

//Class containing the offset data.
class ThemeOffsetData{
public:
	//Vector containing the offsetDatas.
	vector<ThemeOffsetPoint> offsetData;
	//The length of the "animation" in frames.
	int length;
public:
	//Constructor.
	ThemeOffsetData();
	
	//Method used to destroy the offsetData.
	void destroy();
	
	//Method that will load the offsetData from a node.
	//objNode: Pointer to the TreeStorageNode to read the data from.
	//Returns: True if it succeeds without errors.
	bool loadFromNode(TreeStorageNode* objNode);
};

enum Alignment{
	//Horizontal alignments
	LEFT,
	CENTRE,
	RIGHT,

	//Vertical alignments
	TOP,
	MIDDLE,
	BOTTOM,

	//NOTE: Repeat can be used for both horizontal and vertical alignments.
	REPEAT,

	//NOTE: Stretch can be used for both horizontal and vertical alignments.
	STRETCH,
};

//Class containing the positioning and repeat data.
class ThemePositioningData{
public:
	//Horizontal and vertical alignment data.
	Alignment horizontalAlign,verticalAlign;
public:
	//Constructor.
	ThemePositioningData();

	//Method used to destroy the positioningData.
	void destroy();

	//Method that will load the positioningData from a node.
	//objNode: Pointer to the TreeStorageNode to read the data from.
	//Returns: True if it succeeds without errors.
	bool loadFromNode(TreeStorageNode* objNode);
};

//This is the lowest level of the theme system.
//It's a picture with offset data.
class ThemePicture{
public:
    //Pointer to actual texture. Handled by ImageManager.
    SharedTexture texture;
	//Offset data for the picture.
	ThemeOffsetData offset;
    //int x;
    //int y;
public:
	//Constructor.
	ThemePicture();
	
	//Method used to destroy the picture.
	void destroy();

    bool loadFromNode(TreeStorageNode* objNode, string themePath, ImageManager& imageManager, SDL_Renderer& renderer);
	
	//Method that will draw the ThemePicture.
	//dest: The destination surface.
	//x: The x location on the dest to draw the picture.
	//y: The y location on the dest to draw the picture.
	//animation: The frame of the animation to draw.
	//clipRect: Rectangle to clip the picture.
    void draw(SDL_Renderer& renderer,int x,int y,int animation=0, SDL_Rect* clipRect=NULL);
};

//The ThemeObject class is used to contain a basic theme element.
//Contains the picture, animation information, etc...
class ThemeObject{
public:
	//Integer containing the length of the animation.
	int animationLength;
	//Integer containing the frame from where the animation is going to loop.
	int animationLoopPoint;
	
	//Boolean if the animation is invisible at run time (Game state).
	bool invisibleAtRunTime;
	//Boolean if the animation is invisible at design time (Level editor).
	bool invisibleAtDesignTime;
	
	//Picture of the ThemeObject.
	ThemePicture picture;
	//Picture of the ThemeObject shown when in the level editor.
	ThemePicture editorPicture;
	
	//Vector containing optionalPicture for the ThemeObject.
	vector<pair<double,ThemePicture*> > optionalPicture;
	
	//ThemeOffsetData for the ThemeObject.
	ThemeOffsetData offset;
	//ThemePositionData for the ThemeObject.
	ThemePositioningData positioning;
public:
	//Constructor.
	ThemeObject();

	//Destructor.
	~ThemeObject();
	
	//Method that will destroy the ThemeObject.
	void destroy();
	
	//Method that will load a ThemeObject from a node.
	//objNode: The TreeStorageNode to read the object from.
	//themePath: Path to the theme.
	//Returns: True if it succeeds.
    bool loadFromNode(TreeStorageNode* objNode,string themePath, ImageManager& imageManager, SDL_Renderer& renderer);
};

//Class containing a single state of a themed block.
class ThemeBlockState{
public:
	//The length in frames of the oneTimeAnimation.
	int oneTimeAnimationLength;
	//String containing the name of the next state.
	string nextState;
	//Vector containing the themeObjects that make up this state.
	vector<ThemeObject*> themeObjects;
public:
	//Constructor.
	ThemeBlockState();

	//Destructor.
	~ThemeBlockState();
	
	//Method that will destroy the ThemeBlockState.
	void destroy();
	
	//Method that will load a ThemeBlockState from a node.
	//objNode: The TreeStorageNode to read the state from.
	//themePath: Path to the theme.
	//Returns: True if it succeeds.
    bool loadFromNode(TreeStorageNode* objNode,string themePath, ImageManager& imageManager, SDL_Renderer& renderer);
};

//Class containing the needed things for a themed block.
class ThemeBlock{
public:
	//Picture that is shown only in the level editor.
	ThemePicture editorPicture;
	
	//Map containing ThemeBlockStates for the different states of a block.
	map<string,ThemeBlockState*> blockStates;
	//Map containing the transition states between blocks states.
	map<pair<string,string>,ThemeBlockState*> transitions;
public:
	//Constructor.
	ThemeBlock();

	//Destructor.
	~ThemeBlock();
	
	//Method that will destroy the ThemeBlock.
	void destroy();
	
	//Method that will load a ThemeBlock from a node.
	//objNode: The TreeStorageNode to load the ThemeBlock from.
	//themePath: The path to the theme.
	//Returns: True if it succeeds.
    bool loadFromNode(TreeStorageNode* objNode,string themePath, ImageManager& imageManager, SDL_Renderer& renderer);
	
	//Method that will create a ThemeBlockInstance.
	//obj: Pointer that will be filled with the instance.
	//initialState: The name of the initial state.
	void createInstance(ThemeBlockInstance* obj, const std::string& initialState = "default");
private:
	//Method that will create a ThemeBlockStateInstance.
	//obj: Pointer that will be filled with the instance.
	void createStateInstance(ThemeBlockStateInstance* obj);
};

//ThemeBackgroundPicture is a class containing the picture for the background.
class ThemeBackgroundPicture{
private:
	//Rectangle that should be taken from the picture.
	//NOTE The size is pixels of the image.
	SDL_Rect cachedSrcSize;
	//Rectangle with the size it will have on the destination (screen).
	//NOTE The size is in pixels or in precentages (if scaleToScreen is true).
	SDL_Rect cachedDestSize;
	
    //Pointer to the SDL_Texture containing the picture. (Creation/destruction handled by ImageManager)
    SharedTexture texture;
	//Rectangle that should be taken from the picture.
	//NOTE The size is pixels of the image.
	SDL_Rect srcSize;
	//Rectangle with the size it will have on the destination (screen).
	//NOTE The size is in pixels even though the loaded value from the theme description file can be in precentages (if scaleToScreen is true).
	SDL_Rect destSize;
	
	//Boolean if the background picture should be scaled to screen.
	bool scale;
	
	//Boolean if the image should be repeated over the x-axis.
	bool repeatX;
	//Boolean if the image should be repeated over the y-axis.
	bool repeatY;
	
	//Float containing the speed the background picture moves along the x-axis.
	float speedX;
	//Float containing the speed the background picture moves along the y-axis.
	float speedY;
	
	//Float containing the horizontal speed the picture will have when moving the camera (horizontally).
	float cameraX;
	//Float containing the vertical speed the picture will have when moving the camera (vertically).
	float cameraY;
private:
	//Float with the current x position.
	float currentX;
	//Float with the current y position.
	float currentY;
	
	//Stored x location for when loading a state.
	float savedX;
	//Stored y location for when loading a state.
	float savedY;
public:
	//Constructor.
	ThemeBackgroundPicture();
	
	//Method that will update the animation.
	void updateAnimation();
	
	//Method that will reset the animation.
	//save: Boolean if the saved state should be deleted.
	void resetAnimation(bool save);

	//Method that will save the animation.
	void saveAnimation();

	//Method that will load the animation.
	void loadAnimation();
	
	//Method used to draw the ThemeBackgroundPicture.
    //dest: Pointer to the SDL_Renderer the picture should be drawn on.
    void draw(SDL_Renderer& dest);


	//Method used to load the ThemeBackgroundPicture from a node.
	//objNode: The TreeStorageNode to load the picture from.
	//themePath: The path to the theme.
    bool loadFromNode(TreeStorageNode* objNode,string themePath, ImageManager& imageManager, SDL_Renderer& renderer);
	
	//This method will scale the background picture (if needed and configured) to the current SCREEN_WIDTH and SCREEN_HEIGHT.
	void scaleToScreen();
};

//Class that forms the complete background of a theme.
//It is in fact nothing more than a vector containing multiple ThemeBackgroundPictures.
class ThemeBackground{
private:
	//Vector containing the ThemeBackgroundPictures.
	vector<ThemeBackgroundPicture> picture;
public:
	//Method that will update the animation of all the background pictures.
	void updateAnimation();
	
	//Method that will reset the animation of all the background pictures.
	//save: Boolean if the saved state should be deleted.
	void resetAnimation(bool save);
	
	//Method that will save the animation of all the background pictures.
	void saveAnimation();

	//Method that will load the animation of all the background pictures.
	void loadAnimation();
	
	//Method that will scale the background pictures (if set) to the current screen resolution.
	void scaleToScreen();
	
	//This method will draw all the background pictures.
    //dest: Pointer to the SDL_Renderer to draw them on.
	void draw(SDL_Renderer& renderer);
	
	//Method that will add a ThemeBackgroundPicture to the ThemeBackground.
	//objNode: The treeStorageNode to read from.
	//themePath: The path to the theme.
	//Returns: True if it succeeds.
	bool addPictureFromNode(TreeStorageNode* objNode, string themePath, ImageManager& imageManager, SDL_Renderer& renderer);
};

//The ThemeManager is actually a whole theme, filled with ThemeBlocks and ThemeBackground.
class ThemeManager{
private:
	//The ThemeBlock of the shadow.
	ThemeBlock* shadow;
	//The ThemeBlock of the player.
	ThemeBlock* player;
	
	//Array containing a ThemeBlock for every block type.
	ThemeBlock* objBlocks[TYPE_MAX];

	//Map containing all scenery blocks.
	std::map<std::string, ThemeBlock*> objScenery;
	
	//The ThemeBackground.
	ThemeBackground* objBackground;
	
	//ThemeBackground for menu.
	ThemeBackground* menuBackground;
	//Level selection background block.
	ThemeBlock* menuBlock;
	//Level selection background block for locked level.
	ThemeBlock* menuShadowBlock;

	//Boolean indicates if we have theme text colors.
	bool hasThemeTextColor, hasThemeTextColorDialog;
	
	//Theme text colors.
	SDL_Color themeTextColor, themeTextColorDialog;
public:
	//String containing the path to the string.
	string themePath;
	//String containing the theme name.
	string themeName;
public:
	//Constructor.
	ThemeManager();

	//Destructor.
	~ThemeManager();

	//Method used to destroy the ThemeManager.
	void destroy();
	
	//Method that will load the theme from a file.
	//fileName: The file to load the theme from.
	//Returns: True if it succeeds.
    bool loadFile(const string& fileName, ImageManager& imageManager, SDL_Renderer& renderer);
	
	//Method that will scale the theme to the current SCREEN_WIDTH and SCREEN_HEIGHT.
	void scaleToScreen();
	
	//Get a pointer to the ThemeBlock of a given block type.
	//index: The type of block.
	//menu: Boolean if get spefial blocks for menu
	//Returns: Pointer to the ThemeBlock.
	ThemeBlock* getBlock(int index, bool menu);

	//Get a pointer to the ThemeBlock of a given scenery type.
	//name: The name of scenery block.
	ThemeBlock* getScenery(const std::string& name);

	// Add all names of available scenery blocks to a given set.
	void getSceneryBlockNames(std::set<std::string> &s);

	//Get a pointer to the ThemeBlock of the shadow or the player.
	//isShadow: Boolean if it's the shadow
	//Returns: Pointer to the ThemeBlock.
	ThemeBlock* getCharacter(bool isShadow);

	//Get a pointer to the ThemeBackground of the theme.
	//menu: Boolean if get menu background
	//Returns: Pointer to the ThemeBackground.
	ThemeBackground* getBackground(bool menu);

	//Get theme text color.
	//isDialog: Boolean if get theme text color for dialog.
	//color [out]: The color.
	//Returns: if the color is specified in the theme file.
	bool getTextColor(bool isDialog, SDL_Color& color);
};

//Class that combines multiple ThemeManager into one stack.
//If a file is not in a certain theme it will use one of a lower theme.
class ThemeStack{
private:
	//Vector containing the themes in the stack.
	vector<ThemeManager*> objThemes;

	//Boolean indicates if we have already cached the theme text colors.
	bool hasThemeTextColor, hasThemeTextColorDialog;

	//The cached theme text colors.
	SDL_Color themeTextColor, themeTextColorDialog;
public:
	//Constructor.
	ThemeStack();
	//Destructor.
	~ThemeStack();
	
	//Method that will destroy the ThemeStack.
	void destroy();
	
	//Method that will append a theme to the stack.
	//obj: The ThemeManager to add.
	void appendTheme(ThemeManager* obj);
	//Method that will remove the last theme added to the stack.
	void removeTheme();
	
	//Method that will append a theme that will be loaded from file.
	//fileName: The file to load the theme from.
	//Returns: Pointer to the newly added theme, NULL if failed.
    ThemeManager* appendThemeFromFile(const string& fileName, ImageManager& imageManager, SDL_Renderer& renderer);
	
	//Method that is used to let the themes scale.
	void scaleToScreen();
	
	//Get the number of themes in the stack.
	//Returns: The theme count.
	int themeCount(){
		return (int)objThemes.size();
	}
	
	//Operator overloading so that the themes can be accesed using the [] operator.
	//i: The index.
	ThemeManager* operator[](int i){
		return objThemes[i];
	}
	//Get a pointer to the ThemeBlock of a given block type.
	//index: The type of block.
	//Returns: Pointer to the ThemeBlock.
	ThemeBlock* getBlock(int index,bool menu=false);
	//Get a pointer to the ThemeBlock of a given scenery type.
	//name: The name of scenery block.
	ThemeBlock* getScenery(const std::string& name);
	// Add all names of available scenery blocks to a given set.
	void getSceneryBlockNames(std::set<std::string> &s);
	//Get a pointer to the ThemeBlock of the shadow or the player.
	//isShadow: Boolean if it's the shadow
	//Returns: Pointer to the ThemeBlock.
	ThemeBlock* getCharacter(bool isShadow);
	//Get a pointer to the ThemeBackground of the theme.
	//Returns: Pointer to the ThemeBackground.
	ThemeBackground* getBackground(bool menu);

	//Get theme text color.
	//isDialog: Boolean if get theme text color for dialog.
	//Returns: The color (default: black).
	SDL_Color getTextColor(bool isDialog);
};

//The ThemeStack that is be used by the GameState.
extern ThemeStack objThemes;

#endif
