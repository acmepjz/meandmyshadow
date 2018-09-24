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
#include "Commands.h"
#include "CommandManager.h"
#include "GameState.h"
#include "Functions.h"
#include "GameObjects.h"
#include "ThemeManager.h"
#include "LevelPack.h"
#include "LevelEditor.h"
#include "TreeStorageNode.h"
#include "POASerializer.h"
#include "GUIListBox.h"
#include "GUITextArea.h"
#include "GUIWindow.h"
#include "GUISpinBox.h"
#include "MusicManager.h"
#include "InputManager.h"
#include "StatisticsManager.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include <string.h>
#include <stdio.h>
#include <sstream>

#include <SDL_ttf.h>

#include "libs/tinyformat/tinyformat.h"

using namespace std;

static int levelTime,levelRecordings;

//Array containing translateble block names
static const char* blockNames[TYPE_MAX]={
	__("Block"),__("Player Start"),__("Shadow Start"),
	__("Exit"),__("Shadow Block"),__("Spikes"),
	__("Checkpoint"),__("Swap"),__("Fragile"),
	__("Moving Block"),__("Moving Shadow Block"),__("Moving Spikes"),
	__("Teleporter"),__("Button"),__("Switch"),
	__("Conveyor Belt"),__("Shadow Conveyor Belt"),__("Notification Block"),__("Collectable"),__("Pushable")
};

static const std::array<const char*, static_cast<size_t>(ToolTips::TooltipMax)> tooltipNames = {
	__("Select"), __("Add"), __("Delete"), __("Play"), "", "", __("Level settings"), __("Save level"), __("Back to menu"), __("Configure")
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

static std::string describeSceneryName(const std::string& name) {
	//Check if the input is a valid block name.
	if (name.size() > 8 && name.substr(name.size() - 8) == "_Scenery") {
		auto it = Game::blockNameMap.find(name.substr(0, name.size() - 8));
		if (it != Game::blockNameMap.end()){
			return tfm::format(_("%s (Scenery)"), _(blockNames[it->second]));
		}
	}

	return name;
}

/////////////////LevelEditorActionsPopup/////////////////

class LevelEditorActionsPopup:private GUIEventCallback{
private:
	//The parent object.
	LevelEditor* parent;

	//The position and size of window.
	SDL_Rect rect;

	//Array containing the actions in this popup.
	GUIListBox* actions;

	//GUI image.
	SDL_Surface* bmGUI;

	//Pointer to the object the actions apply to.
	GameObject* target;

	//The behaviour names.
	vector<string> behaviour;
	//The fragile block states.
	vector<string> states;

	//The separator texture.
	//NOTE: It's created only when it's used. So don't access it directly!
	SharedTexture separatorTexture;

	//Get or create a new separator texture.
	SharedTexture getOrCreateSeparatorTexture(SDL_Renderer& renderer) {
		if (!separatorTexture) {
			//NOTE: we use an arbitrart width since it will be updated soon.
			createSeparatorTexture(renderer, 16);
		}

		return separatorTexture;
	}

	void createSeparatorTexture(SDL_Renderer& renderer, int width) {
		//create surface
		SurfacePtr surface = createSurface(width, 5);

		//draw horizontal separator
		const SDL_Rect r0 = { 4, 1, width - 8, 1 };
		const SDL_Rect r1 = { 4, 3, width - 8, 1 };
		const SDL_Rect r2 = { 4, 2, width - 8, 1 };
		Uint32 c0 = SDL_MapRGB(surface->format, 192, 192, 192);
		Uint32 c2 = SDL_MapRGB(surface->format, 64, 64, 64);
		SDL_FillRect(surface.get(), &r0, c0);
		SDL_FillRect(surface.get(), &r1, c0);
		SDL_FillRect(surface.get(), &r2, c2);

		//over
		separatorTexture = textureFromSurface(renderer, std::move(surface));
	}

	void updateSeparators(SDL_Renderer& renderer) {
		createSeparatorTexture(renderer, rect.w);

		for (unsigned int i = 0; i < actions->item.size(); i++) {
			if (actions->item[i] == "-") {
				actions->updateItem(renderer, i, "-", separatorTexture);
			}
		}
	}

public:
	SDL_Rect getRect(){
		return rect;
	}
	void dismiss(){
		//Remove the actionsPopup from the parent.
		if(parent!=NULL && parent->actionsPopup==this){
			parent->actionsPopup=NULL;
		}

		//And delete ourself.
		delete this;
	}
    SharedTexture createItem(SDL_Renderer& renderer,const char* caption,int icon,bool grayed=false){
		//FIXME: Add some sort of caching?
        //We draw using surfaces and convert to a texture in the end for now.
		SDL_Color color = objThemes.getTextColor(true);
		if (grayed) {
			color.r = 128 + color.r / 2;
			color.g = 128 + color.g / 2;
			color.b = 128 + color.b / 2;
		}
        SurfacePtr tip(TTF_RenderUTF8_Blended(fontText,caption,color));
		SDL_SetSurfaceBlendMode(tip.get(), SDL_BLENDMODE_NONE);

		//Create the surface, we add 16px to the width for an icon,
		//plus 8px for the border to make it looks better.
		SurfacePtr item = createSurface(tip->w + 24 + (icon >= 0x100 ? 24 : 0), 24);

		//Draw the text on the item surface.
		applySurface(24 + (icon >= 0x100 ? 24 : 0), 0, tip.get(), item.get(), NULL);

		//Temporarily set the blend mode of bmGUI to none, which simply copies RGBA channels to destination.
		SDL_BlendMode oldBlendMode = SDL_BLENDMODE_BLEND;
		SDL_GetSurfaceBlendMode(bmGUI, &oldBlendMode);
		SDL_SetSurfaceBlendMode(bmGUI, SDL_BLENDMODE_NONE);

		//Check if we should draw an icon.
		if(icon>0){
			//Draw the check (or not).
			SDL_Rect r={0,0,16,16};
			r.x=((icon-1)%8)*16;
			r.y=(((icon-1)/8)%8)*16;
            applySurface(4,3,bmGUI,item.get(),&r);
		}

		//Check if we should draw a secondary icon.
		if (icon >= 0x100) {
			SDL_Rect r = { 0, 0, 16, 16 };
			r.x = ((icon / 0x100 - 1) % 8) * 16;
			r.y = (((icon / 0x100 - 1) / 8) % 8) * 16;
			applySurface(28, 3, bmGUI, item.get(), &r);
		}

		//Reset the blend mode of bmGUI.
		SDL_SetSurfaceBlendMode(bmGUI, oldBlendMode);

		//Check if we should update the width., 8px extra on the width is for four pixels spacing on either side.
        if(item->w+8>rect.w) {
            rect.w=item->w+8;
        }

        return textureFromSurface(renderer, std::move(item));
	}
	void updateListBoxSize() {
		//Update the size of the GUIListBox.
		actions->width = rect.w;
		actions->height = rect.h;

		int x = rect.x, y = rect.y;

		if (x>SCREEN_WIDTH - rect.w) x = SCREEN_WIDTH - rect.w;
		else if (x<0) x = 0;
		if (y>SCREEN_HEIGHT - rect.h) y = SCREEN_HEIGHT - rect.h;
		else if (y<0) y = 0;
		rect.x = x;
		rect.y = y;
	}
    void updateItem(SDL_Renderer& renderer,int index,const char* action,const char* caption,int icon=0,bool grayed=false){
        auto item=createItem(renderer,caption,icon,grayed);
        actions->updateItem(renderer, index,action,item,!grayed);

		//Update the size of the GUIListBox.
		updateListBoxSize();
	}
    void addItem(SDL_Renderer& renderer,const char* action,const char* caption,int icon=0,bool grayed=false){
        auto item=createItem(renderer,caption,icon,grayed);
        actions->addItem(renderer,action,item,!grayed);

		//Update the height.
		rect.h += 24;

		//Update the size of the GUIListBox.
		updateListBoxSize();
	}
	void addSeparator(SDL_Renderer& renderer) {
		actions->addItem(renderer, "-", getOrCreateSeparatorTexture(renderer), false);

		//Update the height.
		rect.h += 5;
	}
    LevelEditorActionsPopup(ImageManager& imageManager,SDL_Renderer& renderer,LevelEditor* parent, GameObject* target, int x=0, int y=0){
		this->parent=parent;
		this->target=target;
		rect.x = x;
		rect.y = y;
		//NOTE: The size gets set in the addItem method, height is already four to prevent a scrollbar.
		rect.w=0;
		rect.h=4;

		//Load the gui images.
        bmGUI=imageManager.loadImage(getDataPath()+"gfx/gui.png");

		//Create the behaviour vector.
		behaviour.push_back(_("On"));
		behaviour.push_back(_("Off"));
		behaviour.push_back(_("Toggle"));

		//Create the states list.
		states.push_back(_("Complete"));
		states.push_back(_("One step"));
		states.push_back(_("Two steps"));
		states.push_back(_("Gone"));
		//TODO: The width should be based on the longest option.

		//Create default actions.
		//NOTE: Width and height are determined later on when the options are rendered.
        actions=new GUIListBox(imageManager,renderer,0,0,0,0);
		actions->eventCallback=this;

		//Check if it's a block or not.
		if(target!=NULL)
            addBlockItems(renderer);
		else
            addLevelItems(renderer);
	}

	static std::string getRepeatModeName(int mode) {
		switch (mode) {
		case Scenery::NEGATIVE_INFINITY:
			return _("Negative infinity");
		case Scenery::ZERO:
			return _("Zero");
		case Scenery::LEVEL_SIZE:
			return _("Level size");
		case Scenery::POSITIVE_INFINITY:
			return _("Positive infinity");
		default:
			return _("Default");
		}
	}

    void addBlockItems(SDL_Renderer& renderer){
		//Check if the block is selected or not.
		std::vector<GameObject*>::iterator it;
		it=find(parent->selection.begin(),parent->selection.end(),target);
		if(it!=parent->selection.end())
            addItem(renderer,"Deselect",_("Deselect"));
		else
            addItem(renderer,"Select",_("Select"));
        addItem(renderer,"Delete",_("Delete"),8);

		addSeparator(renderer);

		Scenery *scenery = dynamic_cast<Scenery*>(target);
		if (scenery) {
			// it is scenery block
			addItem(renderer, "RepeatMode0", tfm::format(_("Horizontal repeat start: %s"),
				getRepeatModeName(scenery->repeatMode & 0xFF)).c_str(), 8 * 2 + 3);
			addItem(renderer, "RepeatMode1", tfm::format(_("Horizontal repeat end: %s"),
				getRepeatModeName((scenery->repeatMode >> 8) & 0xFF)).c_str(), 8 * 2 + 4);
			addItem(renderer, "RepeatMode2", tfm::format(_("Vertical repeat start: %s"),
				getRepeatModeName((scenery->repeatMode >> 16) & 0xFF)).c_str(), 8 * 3 + 3);
			addItem(renderer, "RepeatMode3", tfm::format(_("Vertical repeat end: %s"),
				getRepeatModeName((scenery->repeatMode >> 24) & 0xFF)).c_str(), 8 * 3 + 4);

			if (scenery->sceneryName_.empty()) {
				addSeparator(renderer);
				addItem(renderer, "CustomScenery", _("Custom scenery"), 8 + 4);
			}

			return;
		}

		addItem(renderer, "Visible", _("Visible"), (target->getEditorProperty("visible") == "1") ? 2 : 1);

		//Get the type of the target.
		int type = target->type;

		//Determine what to do depending on the type.
		if(isLinkable[type]){
			//Check if it's a moving block type or trigger.
			if(type==TYPE_BUTTON || type==TYPE_SWITCH || type==TYPE_PORTAL){
				addItem(renderer, "Link", _("Link"), 8 * 2 + 8);
				addItem(renderer, "Remove Links", _("Remove Links"), 8 * 2 + 7);

				//Check if it's a portal, which contains a automatic option, and triggers a behaviour one.
				if(type==TYPE_PORTAL){
                    addItem(renderer,"Automatic",_("Automatic"),(target->getEditorProperty("automatic")=="1")?2:1);
				}else{
					//Get the current behaviour.
					int currentBehaviour=2;
					if(target->getEditorProperty("behaviour")=="on"){
						currentBehaviour=0;
					}else if(target->getEditorProperty("behaviour")=="off"){
						currentBehaviour=1;
					}

					addItem(renderer, "Behaviour", tfm::format(_("Behavior: %s"), behaviour[currentBehaviour]).c_str());
				}
			}else{
				addItem(renderer, "Path", _("Path"), 8 + 5);
				addItem(renderer, "Remove Path", _("Remove Path"), 8 * 4 + 2);

                addItem(renderer,"Activated",_("Activated"),(target->getEditorProperty("activated")=="1")?2:1);
                addItem(renderer,"Looping",_("Looping"),(target->getEditorProperty("loop")=="1")?2:1);
			}
		}
		//Check for a conveyor belt.
		if(type==TYPE_CONVEYOR_BELT || type==TYPE_SHADOW_CONVEYOR_BELT){
            addItem(renderer,"Activated",_("Activated"),(target->getEditorProperty("activated")=="1")?2:1);
            addItem(renderer,"Speed",_("Speed"));
		}
		//Check if it's a fragile block.
		if(type==TYPE_FRAGILE){
			//Get the current state.
			int currentState=atoi(target->getEditorProperty("state").c_str());
			addItem(renderer, "State", tfm::format(_("State: %s"), states[currentState]).c_str());
		}
		//Check if it's a notification block.
		if(type==TYPE_NOTIFICATION_BLOCK)
            addItem(renderer,"Message",_("Message"));
		//Add the custom appearance menu item.
		addItem(renderer, "Appearance", _("Appearance"), 8 + 4);

		addSeparator(renderer);

		//Finally add scripting to the bottom.
        addItem(renderer,"Scripting",_("Scripting"),8*2+1);
	}

    void addLevelItems(SDL_Renderer& renderer){
		// add the layers
		{
			// background layers.
			auto it = parent->sceneryLayers.begin();
			for (; it != parent->sceneryLayers.end(); ++it){
				if (it->first >= "f") break; // now we meet a foreground layer
				int icon = parent->layerVisibility[it->first] ? (8 * 3 + 1) : (8 * 3 + 2);
				icon |= (parent->selectedLayer == it->first ? 3 : 36) << 8;
				std::string s = "_layer:" + it->first;
				addItem(renderer, s.c_str(), tfm::format(_("Background layer: %s"), it->first).c_str(), icon);
			}

			// the Blocks layer.
			{
				int icon = parent->layerVisibility[std::string()] ? (8 * 3 + 1) : (8 * 3 + 2);
				icon |= (parent->selectedLayer.empty() ? 3 : 36) << 8;
				addItem(renderer, "_layer:", _("Blocks layer"), icon);
			}

			// foreground layers.
			for (; it != parent->sceneryLayers.end(); ++it){
				int icon = parent->layerVisibility[it->first] ? (8 * 3 + 1) : (8 * 3 + 2);
				icon |= (parent->selectedLayer == it->first ? 3 : 36) << 8;
				std::string s = "_layer:" + it->first;
				addItem(renderer, s.c_str(), tfm::format(_("Foreground layer: %s"), it->first).c_str(), icon);
			}
		}

		addSeparator(renderer);

		addItem(renderer, "AddLayer", _("Add new layer"), 8 * 3 + 6);
		addItem(renderer, "DeleteLayer", _("Delete selected layer"), 8 * 3 + 7, parent->selectedLayer.empty());
		addItem(renderer, "LayerSettings", _("Configure selected layer"), 8 * 3 + 8, parent->selectedLayer.empty());
		addItem(renderer, "MoveToLayer", _("Move selected object to layer"), 0, parent->selectedLayer.empty() || parent->selection.empty());

		addSeparator(renderer);

        addItem(renderer,"LevelSettings",_("Settings"),8*2);
        addItem(renderer,"LevelScripting",_("Scripting"),8*2+1);
	}

    virtual ~LevelEditorActionsPopup(){
        //bmGui is freed by imageManager.
		if(actions)
			delete actions;
	}

    void render(SDL_Renderer& renderer){
		//Check if we need to resize the separator.
		//NOTE: if separatorTexture is NULL then it means that we didn't use the separator at all, so we don't need to update it.
		if (separatorTexture && textureWidth(*separatorTexture) < rect.w) {
			updateSeparators(renderer);
		}

		//Draw the actions.
        actions->render(renderer,rect.x,rect.y);
	}
    void handleEvents(SDL_Renderer& renderer){
		//Check if a mouse is pressed outside the popup.
		int x,y;
		SDL_GetMouseState(&x,&y);
		SDL_Rect mouse={x,y,0,0};
		if(event.type==SDL_MOUSEBUTTONDOWN && !pointOnRect(mouse,rect)){
			dismiss();
			return;
		}
		//Let the listbox handle its events.
        actions->handleEvents(renderer,rect.x,rect.y);
	}
	static void addLayerNameNote(ImageManager& imageManager, SDL_Renderer& renderer, GUIWindow *root, int yy = 148) {
		std::string s = _("NOTE: the layers are sorted by name alphabetically.\nThe layer is background layer if its name is < 'f'\nby dictionary order, otherwise it's foreground layer.");
		for (int lps = 0;;) {
			size_t lpe = s.find_first_of('\n', lps);

			GUIObject *obj = new GUILabel(imageManager, renderer, 40, yy, 520, 36,
				lpe == string::npos ? (s.c_str() + lps) : s.substr(lps, lpe - lps).c_str());
			root->addChild(obj);

			if (lpe == string::npos) break;
			lps = lpe + 1;
			yy += 24;
		}
	}
    void GUIEventCallback_OnEvent(ImageManager& imageManager, SDL_Renderer& renderer, std::string name,GUIObject* obj,int eventType){
		//Skip next mouse up event since we're clicking a list box and possibly showing a new window.
		GUISkipNextMouseUpEvent = true;

		//NOTE: There should only be one GUIObject, so we know what event is fired.
		//Get the selected entry.
		std::string action=actions->item[actions->value];
		if(action=="Select"){
			//Add the target to the selection.
			parent->selection.push_back(target);
			dismiss();
			return;
		}else if(action=="Deselect"){
			//Check if the block is in the selection.
			std::vector<GameObject*>::iterator it;
			it=find(parent->selection.begin(),parent->selection.end(),target);
			if(it!=parent->selection.end()){
				//Remove the object from the selection.
				parent->selection.erase(it);
			}

			dismiss();
			return;
		}else if(action=="Delete"){
			parent->commandManager->doCommand(new AddRemoveGameObjectCommand(parent, target, false));
			dismiss();
			return;
		}else if(action=="Link"){
			parent->linking=true;
			parent->linkingTrigger=dynamic_cast<Block*>(target);
			parent->tool=LevelEditor::SELECT;
			dismiss();
			return;
		}else if(action=="Remove Links"){
			//Remove all the links
			Block *block = dynamic_cast<Block*>(target);
			if (block) {
				RemoveLinkCommand* pCommand = new RemoveLinkCommand(parent, block);
				parent->commandManager->doCommand(pCommand);
			}

			dismiss();
			return;
		}else if(action=="Path"){
			parent->moving=true;
			parent->pauseMode = false;
			parent->pauseTime = 0;
			parent->movingBlock=dynamic_cast<Block*>(target);
			parent->tool=LevelEditor::SELECT;
			dismiss();
			return;
		}else if(action=="Remove Path"){
			//Remove all the paths
			Block *block = dynamic_cast<Block*>(target);
			if (block) {
				RemovePathCommand* pCommand = new RemovePathCommand(parent, block);
				parent->commandManager->doCommand(pCommand);
			}

			dismiss();
			return;
		}else if(action=="Message"){
			//Create the GUI.
            GUIWindow* root=new GUIWindow(imageManager,renderer,(SCREEN_WIDTH-600)/2,(SCREEN_HEIGHT-250)/2,600,250,true,true,_("Notification block"));
			root->minWidth = root->width; root->minHeight = root->height;
			root->name="notificationBlockWindow";
			root->eventCallback=parent;
			GUIObject* obj;

            obj=new GUILabel(imageManager,renderer,40,50,240,36,_("Enter message here:"));
			root->addChild(obj);
            GUITextArea* textarea=new GUITextArea(imageManager,renderer,50,90,500,100);
			textarea->gravityRight = textarea->gravityBottom = GUIGravityRight;
			//Set the name of the text area, which is used to identify the object later on.
			textarea->name="message";
			string tmp=target->getEditorProperty("message");
			//Change \n with the characters '\n'.
			while(tmp.find("\\n")!=string::npos){
				tmp=tmp.replace(tmp.find("\\n"),2,"\n");
			}
            textarea->setString(renderer, tmp);
			root->addChild(textarea);

            obj=new GUIButton(imageManager,renderer,root->width*0.3,250-44,-1,36,_("OK"),0,true,true,GUIGravityCenter);
			obj->gravityLeft = obj->gravityRight = GUIGravityCenter;
			obj->gravityTop = obj->gravityBottom = GUIGravityRight;
			obj->name="cfgNotificationBlockOK";
			obj->eventCallback=root;
			root->addChild(obj);
            obj=new GUIButton(imageManager,renderer,root->width*0.7,250-44,-1,36,_("Cancel"),0,true,true,GUIGravityCenter);
			obj->gravityLeft = obj->gravityRight = GUIGravityCenter;
			obj->gravityTop = obj->gravityBottom = GUIGravityRight;
			obj->name="cfgCancel";
			obj->eventCallback=root;
			root->addChild(obj);

			//Add the window to the GUIObjectRoot and the objectWindows map.
			GUIObjectRoot->addChild(root);
			parent->objectWindows[root]=target;

			//And dismiss this popup.
			dismiss();
			return;
		}else if(action=="Activated"){
			//Get the previous state.
			bool enabled=(target->getEditorProperty("activated")=="1");

			//Switch the state.
			enabled=!enabled;

			parent->commandManager->doCommand(new SetEditorPropertyCommand(parent, imageManager, renderer,
				target, "activated", enabled ? "1" : "0", _("Activated")));

            updateItem(renderer,actions->value,"Activated",_("Activated"),enabled?2:1);
			actions->value=-1;
			return;
		} else if (action == "Visible"){
			//Get the previous state.
			bool visible = (target->getEditorProperty("visible") == "1");

			//Switch the state.
			visible = !visible;

			parent->commandManager->doCommand(new SetEditorPropertyCommand(parent, imageManager, renderer,
				target, "visible", visible ? "1" : "0", _("Visible")));

			updateItem(renderer, actions->value, "Visible", _("Visible"), visible ? 2 : 1);
			actions->value = -1;
			return;
		} else if (action == "Looping"){
			//Get the previous state.
			bool loop=(target->getEditorProperty("loop")=="1");

			//Switch the state.
			loop=!loop;
			parent->commandManager->doCommand(new SetEditorPropertyCommand(parent, imageManager, renderer,
				target, "loop", loop ? "1" : "0", _("Looping")));

            updateItem(renderer,actions->value,"Looping",_("Looping"),loop?2:1);
			actions->value=-1;
			return;
		}else if(action=="Automatic"){
			//Get the previous state.
			bool automatic=(target->getEditorProperty("automatic")=="1");

			//Switch the state.
			automatic=!automatic;
			parent->commandManager->doCommand(new SetEditorPropertyCommand(parent, imageManager, renderer,
				target, "automatic", automatic ? "1" : "0", _("Automatic")));

            updateItem(renderer,actions->value,"Automatic",_("Automatic"),automatic?2:1);
			actions->value=-1;
			return;
		}else if(action=="Behaviour"){
			//Get the current behaviour.
			int currentBehaviour=2;
			string behave=target->getEditorProperty("behaviour");
			if(behave=="on"){
				currentBehaviour=0;
			}else if(behave=="off"){
				currentBehaviour=1;
			}

			//Increase the behaviour.
			currentBehaviour++;
			if(currentBehaviour>2)
				currentBehaviour=0;

			//Update the data of the block.
			parent->commandManager->doCommand(new SetEditorPropertyCommand(parent, imageManager, renderer,
				target, "behaviour", behaviour[currentBehaviour], _("Behavior")));

			//And update the item.
			updateItem(renderer, actions->value, "Behaviour", tfm::format(_("Behavior: %s"), behaviour[currentBehaviour]).c_str());
			actions->value=-1;
			return;
		}else if(action=="State"){
			//Get the current state.
			int currentState=atoi(target->getEditorProperty("state").c_str());

			//Increase the state.
			currentState++;
			if(currentState>3)
				currentState=0;

			//Update the data of the block.
			char s[64];
			sprintf(s,"%d",currentState);
			parent->commandManager->doCommand(new SetEditorPropertyCommand(parent, imageManager, renderer,
				target, "state", s, _("State")));

			//And update the item.
			updateItem(renderer, actions->value, "State", tfm::format(_("State: %s"), states[currentState]).c_str());
			actions->value=-1;
			return;
		}else if(action=="Speed"){
			//Create the GUI.
            GUIWindow* root=new GUIWindow(imageManager,renderer,(SCREEN_WIDTH-600)/2,(SCREEN_HEIGHT-250)/2,600,250,true,true,_("Conveyor belt speed"));
			root->minWidth = root->width; root->minHeight = root->height;
			root->name="conveyorBlockWindow";
			root->eventCallback=parent;
			GUIObject* obj;

            obj=new GUILabel(imageManager,renderer,40,100,240,36,_("Enter speed here:"));
			root->addChild(obj);
            GUISpinBox* obj2=new GUISpinBox(imageManager,renderer,240,100,320,36);
			obj2->gravityRight = GUIGravityRight;
			//Set the name of the text area, which is used to identify the object later on.
			obj2->name="speed";
			obj2->caption=target->getEditorProperty("speed10");
			obj2->format = "%1.0f";
			obj2->update();
			root->addChild(obj2);

			obj = new GUILabel(imageManager, renderer, 40, 160, 520, 36, _("NOTE: 1 Speed = 0.08 block/s"));
			root->addChild(obj);

            obj=new GUIButton(imageManager,renderer,root->width*0.3,250-44,-1,36,_("OK"),0,true,true,GUIGravityCenter);
			obj->gravityLeft = obj->gravityRight = GUIGravityCenter;
			obj->gravityTop = obj->gravityBottom = GUIGravityRight;
			obj->name="cfgConveyorBlockOK";
			obj->eventCallback=root;
			root->addChild(obj);
            obj=new GUIButton(imageManager,renderer,root->width*0.7,250-44,-1,36,_("Cancel"),0,true,true,GUIGravityCenter);
			obj->gravityLeft = obj->gravityRight = GUIGravityCenter;
			obj->gravityTop = obj->gravityBottom = GUIGravityRight;
			obj->name="cfgCancel";
			obj->eventCallback=root;
			root->addChild(obj);

			//Add the window to the GUIObjectRoot and the objectWindows map.
			GUIObjectRoot->addChild(root);
			parent->objectWindows[root]=target;

			//And dismiss this popup.
			dismiss();
			return;
		}else if(action=="Scripting"){
			//Create the GUI.
            GUIWindow* root=new GUIWindow(imageManager,renderer,(SCREEN_WIDTH-600)/2,(SCREEN_HEIGHT-500)/2,600,500,true,true,_("Scripting"));
			root->minWidth = root->width; root->minHeight = root->height;
			root->name="scriptingWindow";
			root->eventCallback=parent;
			GUIObject* obj;

            obj=new GUILabel(imageManager,renderer,50,60,240,36,_("Id:"));
			root->addChild(obj);

            obj=new GUITextBox(imageManager,renderer,100,60,240,36,dynamic_cast<Block*>(target)->id.c_str());
			obj->gravityRight = GUIGravityRight;
			obj->name="id";
			root->addChild(obj);

            GUISingleLineListBox* list=new GUISingleLineListBox(imageManager,renderer,50,100,500,36);
			list->gravityRight = GUIGravityRight;
			std::map<std::string,int>::iterator it;
			for(it=Game::gameObjectEventNameMap.begin();it!=Game::gameObjectEventNameMap.end();++it)
				list->addItem(it->first);
			list->name="cfgScriptingEventType";
			list->value=0;
			list->eventCallback=root;
			root->addChild(list);

			//Add a text area for each event type.
			Block* block=dynamic_cast<Block*>(target);
			for(unsigned int i=0;i<list->item.size();i++){
                GUITextArea* text=new GUITextArea(imageManager,renderer,50,140,500,300);
				text->gravityRight = text->gravityBottom = GUIGravityRight;
				text->name=list->item[i].first;
				text->setFont(fontMono);
				//Only set the first one visible and enabled.
				text->visible=(i==0);
				text->enabled=(i==0);

				map<int,string>::iterator it=block->scripts.find(Game::gameObjectEventNameMap[list->item[i].first]);
				if(it!=block->scripts.end())
                    text->setString(renderer, it->second);

				root->addChild(text);
			}


            obj=new GUIButton(imageManager,renderer,root->width*0.3,500-44,-1,36,_("OK"),0,true,true,GUIGravityCenter);
			obj->gravityLeft = obj->gravityRight = GUIGravityCenter;
			obj->gravityTop = obj->gravityBottom = GUIGravityRight;
			obj->name="cfgScriptingOK";
			obj->eventCallback=root;
			root->addChild(obj);
            obj=new GUIButton(imageManager,renderer,root->width*0.7,500-44,-1,36,_("Cancel"),0,true,true,GUIGravityCenter);
			obj->gravityLeft = obj->gravityRight = GUIGravityCenter;
			obj->gravityTop = obj->gravityBottom = GUIGravityRight;
			obj->name="cfgCancel";
			obj->eventCallback=root;
			root->addChild(obj);

			//Add the window to the GUIObjectRoot and the objectWindows map.
			GUIObjectRoot->addChild(root);
			parent->objectWindows[root]=target;

			//And dismiss this popup.
			dismiss();
			return;
		}else if(action=="LevelSettings"){
			//Open the levelSettings window.
            parent->levelSettings(imageManager,renderer);

			//And dismiss this popup.
			dismiss();
			return;
		}else if(action=="LevelScripting"){
			//Create the GUI.
            GUIWindow* root=new GUIWindow(imageManager,renderer,(SCREEN_WIDTH-600)/2,(SCREEN_HEIGHT-500)/2,600,500,true,true,_("Level Scripting"));
			root->minWidth = root->width; root->minHeight = root->height;
			root->name="levelScriptingWindow";
			root->eventCallback=parent;
			GUIObject* obj;

            GUISingleLineListBox* list=new GUISingleLineListBox(imageManager,renderer,50,60,500,36);
			list->gravityRight = GUIGravityRight;
			std::map<std::string,int>::iterator it;
			for(it=Game::levelEventNameMap.begin();it!=Game::levelEventNameMap.end();++it)
				list->addItem(it->first);
			list->name="cfgLevelScriptingEventType";
			list->value=0;
			list->eventCallback=root;
			root->addChild(list);

			//Add a text area for each event type.
			for(unsigned int i=0;i<list->item.size();i++){
                GUITextArea* text=new GUITextArea(imageManager,renderer,50,100,500,340);
				text->gravityRight = text->gravityBottom = GUIGravityRight;
				text->name=list->item[i].first;
				text->setFont(fontMono);
				//Only set the first one visible and enabled.
				text->visible=(i==0);
				text->enabled=(i==0);

				map<int,string>::iterator it=parent->scripts.find(Game::levelEventNameMap[list->item[i].first]);
				if(it!=parent->scripts.end())
                    text->setString(renderer, it->second);

				root->addChild(text);
			}


            obj=new GUIButton(imageManager,renderer,root->width*0.3,500-44,-1,36,_("OK"),0,true,true,GUIGravityCenter);
			obj->gravityLeft = obj->gravityRight = GUIGravityCenter;
			obj->gravityTop = obj->gravityBottom = GUIGravityRight;
			obj->name="cfgLevelScriptingOK";
			obj->eventCallback=root;
			root->addChild(obj);
            obj=new GUIButton(imageManager,renderer,root->width*0.7,500-44,-1,36,_("Cancel"),0,true,true,GUIGravityCenter);
			obj->gravityLeft = obj->gravityRight = GUIGravityCenter;
			obj->gravityTop = obj->gravityBottom = GUIGravityRight;
			obj->name="cfgCancel";
			obj->eventCallback=root;
			root->addChild(obj);

			//Add the window to the GUIObjectRoot and the objectWindows map.
			GUIObjectRoot->addChild(root);
			parent->objectWindows[root]=target;

			//And dismiss this popup.
			dismiss();
			return;
		} else if (action.size() >= 7 && action.substr(0, 7) == "_layer:") {
			std::string s0 = action.substr(7);
			auto it = parent->layerVisibility.find(s0);
			if (it != parent->layerVisibility.end()) {
				int x;
				SDL_GetMouseState(&x, NULL);
				if (x < rect.x + 24) {
					// toggle the visibility
					it->second = !it->second;

					if (parent->selectedLayer == it->first) {
						// deselect all blocks if the visibility of current layer is changed
						parent->deselectAll();
					}
				} else if (parent->selectedLayer != it->first) {
					// deselect all blocks if the selected layer is changed
					parent->deselectAll();

					// uncheck the previously selected layer
					std::string oldSelected = "_layer:" + parent->selectedLayer;
					for (unsigned int idx = 0; idx < actions->item.size(); idx++) {
						if (actions->item[idx] == oldSelected) {
							int icon = parent->layerVisibility[parent->selectedLayer] ? (8 * 3 + 1) : (8 * 3 + 2);
							icon |= 36 << 8;
							updateItem(renderer, idx, oldSelected.c_str(),
								parent->selectedLayer.empty() ? _("Blocks layer") :
								tfm::format((parent->selectedLayer < "f") ? _("Background layer: %s") : _("Foreground layer: %s"), parent->selectedLayer).c_str(),
								icon);
							break;
						}
					}

					// change the selected layer
					parent->selectedLayer = it->first;
				} else {
					actions->value = -1;
					return;
				}

				int icon = it->second ? (8 * 3 + 1) : (8 * 3 + 2);
				icon |= (parent->selectedLayer == it->first ? 3 : 36) << 8;
				std::string s = "_layer:" + it->first;
				updateItem(renderer, actions->value, s.c_str(),
					it->first.empty() ? _("Blocks layer") :
					tfm::format((it->first < "f") ? _("Background layer: %s") : _("Foreground layer: %s"), it->first).c_str(),
					icon);

				// update some other menu items according to selection/visibility changes
				for (unsigned int i = 0; i < actions->item.size(); i++) {
					if (actions->item[i] == "DeleteLayer") {
						updateItem(renderer, i, "DeleteLayer", _("Delete selected layer"), 8 * 3 + 7, parent->selectedLayer.empty());
					} else if (actions->item[i] == "LayerSettings") {
						updateItem(renderer, i, "LayerSettings", _("Configure selected layer"), 8 * 3 + 8, parent->selectedLayer.empty());
					} else if (actions->item[i] == "MoveToLayer") {
						updateItem(renderer, i, "MoveToLayer", _("Move selected object to layer"), 0, parent->selectedLayer.empty() || parent->selection.empty());
					}
				}
			}
			actions->value = -1;
			return;
		} else if (action == "AddLayer") {
			//Create the add layer GUI.
			GUIWindow* root = new GUIWindow(imageManager, renderer, (SCREEN_WIDTH - 600) / 2, (SCREEN_HEIGHT - 400) / 2, 600, 400, true, true, _("Add layer"));
			root->minWidth = root->width; root->minHeight = root->height;
			root->name = "addLayerWindow";
			root->eventCallback = parent;
			GUIObject* obj;

			obj = new GUILabel(imageManager, renderer, 40, 64, 520, 36, _("Enter the layer name:"));
			root->addChild(obj);
			GUITextBox* obj2 = new GUITextBox(imageManager, renderer, 40, 100, 520, 36);
			obj2->gravityRight = GUIGravityRight;
			//Set the name of the text area, which is used to identify the object later on.
			obj2->name = "layerName";
			root->addChild(obj2);

			addLayerNameNote(imageManager, renderer, root);

			obj = new GUIButton(imageManager, renderer, root->width*0.3, 400 - 44, -1, 36, _("OK"), 0, true, true, GUIGravityCenter);
			obj->gravityLeft = obj->gravityRight = GUIGravityCenter;
			obj->gravityTop = obj->gravityBottom = GUIGravityRight;
			obj->name = "cfgAddLayerOK";
			obj->eventCallback = root;
			root->addChild(obj);
			obj = new GUIButton(imageManager, renderer, root->width*0.7, 400 - 44, -1, 36, _("Cancel"), 0, true, true, GUIGravityCenter);
			obj->gravityLeft = obj->gravityRight = GUIGravityCenter;
			obj->gravityTop = obj->gravityBottom = GUIGravityRight;
			obj->name = "cfgCancel";
			obj->eventCallback = root;
			root->addChild(obj);

			//Add the window to the GUIObjectRoot and the objectWindows map.
			GUIObjectRoot->addChild(root);
			parent->objectWindows[root] = target;

			//And dismiss this popup.
			dismiss();
			return;
		} else if (action == "DeleteLayer") {
			// delete selected layer
			if (parent->selectedLayer.empty()) {
				// can't delete Blocks layer
				actions->value = -1;
				return;
			}

			if (parent->sceneryLayers.find(parent->selectedLayer) == parent->sceneryLayers.end()) {
				// can't find the layer with given name
				actions->value = -1;
				return;
			}

			if (msgBox(imageManager, renderer,
				tfm::format(_("Are you sure you want to delete layer '%s'?"), parent->selectedLayer).c_str(),
				MsgBoxYesNo, _("Delete layer")) == MsgBoxYes)
			{
				// do the actual operation
				parent->commandManager->doCommand(new AddRemoveLayerCommand(parent, parent->selectedLayer, false));
			}

			dismiss();
			return;
		} else if (action == "LayerSettings") {
			// rename selected layer
			if (parent->selectedLayer.empty()) {
				// can't rename Blocks layer
				actions->value = -1;
				return;
			}

			auto it = parent->sceneryLayers.find(parent->selectedLayer);
			if (it == parent->sceneryLayers.end()) {
				// can't find the layer with given name
				actions->value = -1;
				return;
			}

			//Create the rename layer GUI.
			GUIWindow* root = new GUIWindow(imageManager, renderer, (SCREEN_WIDTH - 600) / 2, (SCREEN_HEIGHT - 500) / 2, 600, 500, true, true, _("Layer settings"));
			root->minWidth = root->width; root->minHeight = root->height;
			root->name = "layerSettingsWindow";
			root->eventCallback = parent;
			GUIObject* obj;

			obj = new GUILabel(imageManager, renderer, 40, 64, 520, 36, _("Layer name:"));
			root->addChild(obj);
			GUITextBox* textBox = new GUITextBox(imageManager, renderer, 40, 100, 520, 36, it->first.c_str());
			textBox->gravityRight = GUIGravityRight;
			//Set the name of the text area, which is used to identify the object later on.
			textBox->name = "layerName";
			root->addChild(textBox);

			// A stupid code to save the old name
			obj = new GUIObject(imageManager, renderer, 0, 0, 0, 0, it->first.c_str(), 0, false, false);
			obj->name = "oldName";
			root->addChild(obj);

			addLayerNameNote(imageManager, renderer, root);

			obj = new GUILabel(imageManager, renderer, 40, 284, 520, 36, _("Layer moving speed (1 speed = 0.8 block/s):"));
			root->addChild(obj);
			obj = new GUILabel(imageManager, renderer, 40, 320, 40, 36, "X");
			root->addChild(obj);
			obj = new GUILabel(imageManager, renderer, 320, 320, 40, 36, "Y");
			obj->gravityLeft = obj->gravityRight = GUIGravityCenter;
			root->addChild(obj);

			GUISpinBox *spinBox = new GUISpinBox(imageManager, renderer, 80, 320, 200, 36);
			spinBox->gravityRight = GUIGravityCenter;
			spinBox->name = "speedX";
			spinBox->caption = tfm::format("%g", it->second->speedX);
			spinBox->format = "%g";
			root->addChild(spinBox);
			spinBox = new GUISpinBox(imageManager, renderer, 360, 320, 200, 36);
			spinBox->gravityLeft = GUIGravityCenter; spinBox->gravityRight = GUIGravityRight;
			spinBox->name = "speedY";
			spinBox->caption = tfm::format("%g", it->second->speedY);
			spinBox->format = "%g";
			root->addChild(spinBox);

			obj = new GUILabel(imageManager, renderer, 40, 364, 520, 36, _("Speed of following camera:"));
			root->addChild(obj);
			obj = new GUILabel(imageManager, renderer, 40, 400, 40, 36, "X");
			root->addChild(obj);
			obj = new GUILabel(imageManager, renderer, 320, 400, 40, 36, "Y");
			obj->gravityLeft = obj->gravityRight = GUIGravityCenter;
			root->addChild(obj);

			spinBox = new GUISpinBox(imageManager, renderer, 80, 400, 200, 36);
			spinBox->gravityRight = GUIGravityCenter;
			spinBox->name = "cameraX";
			spinBox->caption = tfm::format("%g", it->second->cameraX);
			spinBox->format = "%g";
			spinBox->change = 0.1f;
			root->addChild(spinBox);
			spinBox = new GUISpinBox(imageManager, renderer, 360, 400, 200, 36);
			spinBox->gravityLeft = GUIGravityCenter; spinBox->gravityRight = GUIGravityRight;
			spinBox->name = "cameraY";
			spinBox->caption = tfm::format("%g", it->second->cameraY);
			spinBox->format = "%g";
			spinBox->change = 0.1f;
			root->addChild(spinBox);

			obj = new GUIButton(imageManager, renderer, root->width*0.3, 500 - 44, -1, 36, _("OK"), 0, true, true, GUIGravityCenter);
			obj->gravityLeft = obj->gravityRight = GUIGravityCenter;
			obj->gravityTop = obj->gravityBottom = GUIGravityRight;
			obj->name = "cfgLayerSettingsOK";
			obj->eventCallback = root;
			root->addChild(obj);
			obj = new GUIButton(imageManager, renderer, root->width*0.7, 500 - 44, -1, 36, _("Cancel"), 0, true, true, GUIGravityCenter);
			obj->gravityLeft = obj->gravityRight = GUIGravityCenter;
			obj->gravityTop = obj->gravityBottom = GUIGravityRight;
			obj->name = "cfgCancel";
			obj->eventCallback = root;
			root->addChild(obj);

			//Add the window to the GUIObjectRoot and the objectWindows map.
			GUIObjectRoot->addChild(root);
			parent->objectWindows[root] = target;

			//And dismiss this popup.
			dismiss();
			return;
		} else if (action == "MoveToLayer") {
			// move the selected object to another layer
			if (parent->selection.empty() || parent->selectedLayer.empty()) {
				// either nothing selected, or can't move objects in Blocks layer
				actions->value = -1;
				return;
			}

			//Create the rename layer GUI.
			GUIWindow* root = new GUIWindow(imageManager, renderer, (SCREEN_WIDTH - 600) / 2, (SCREEN_HEIGHT - 300) / 2, 600, 400, true, true, _("Move to layer"));
			root->minWidth = root->width; root->minHeight = root->height;
			root->name = "moveToLayerWindow";
			root->eventCallback = parent;
			GUIObject* obj;

			obj = new GUILabel(imageManager, renderer, 40, 64, 520, 36, _("Enter the layer name (create new layer if necessary):"));
			root->addChild(obj);
			GUITextBox* obj2 = new GUITextBox(imageManager, renderer, 40, 100, 520, 36, parent->selectedLayer.c_str());
			obj2->gravityRight = GUIGravityRight;
			//Set the name of the text area, which is used to identify the object later on.
			obj2->name = "layerName";
			root->addChild(obj2);

			// A stupid code to save the old name
			obj = new GUIObject(imageManager, renderer, 0, 0, 0, 0, parent->selectedLayer.c_str(), 0, false, false);
			obj->name = "oldName";
			root->addChild(obj);

			addLayerNameNote(imageManager, renderer, root);

			obj = new GUIButton(imageManager, renderer, root->width*0.3, 400 - 44, -1, 36, _("OK"), 0, true, true, GUIGravityCenter);
			obj->gravityLeft = obj->gravityRight = GUIGravityCenter;
			obj->gravityTop = obj->gravityBottom = GUIGravityRight;
			obj->name = "cfgMoveToLayerOK";
			obj->eventCallback = root;
			root->addChild(obj);
			obj = new GUIButton(imageManager, renderer, root->width*0.7, 400 - 44, -1, 36, _("Cancel"), 0, true, true, GUIGravityCenter);
			obj->gravityLeft = obj->gravityRight = GUIGravityCenter;
			obj->gravityTop = obj->gravityBottom = GUIGravityRight;
			obj->name = "cfgCancel";
			obj->eventCallback = root;
			root->addChild(obj);

			//Add the window to the GUIObjectRoot and the objectWindows map.
			GUIObjectRoot->addChild(root);
			parent->objectWindows[root] = target;

			//And dismiss this popup.
			dismiss();
			return;
		} else if (action.size() > 10 && action.substr(0, 10) == "RepeatMode") {
			Scenery *scenery = dynamic_cast<Scenery*>(target);
			if (scenery) {
				int index = atoi(action.c_str() + 10);
				assert(index >= 0 && index < 4);

				//Get the current repeat mode.
				unsigned int repeatMode = scenery->repeatMode;

				//Extract the value we want to modify.
				unsigned int i = (repeatMode >> (index * 8)) & 0xFF;
				repeatMode &= ~(0xFFu << (index * 8));

				//Increase the value.
				for (;;) {
					i++;
					if (i >= Scenery::REPEAT_MODE_MAX) i = 0;

					// skip invalid values (POSITIVE_INFINITY for start, NEGATIVE_INFINITY for end)
					if ((index & 1) == 0 && i == Scenery::POSITIVE_INFINITY) continue;
					if ((index & 1) == 1 && i == Scenery::NEGATIVE_INFINITY) continue;

					break;
				}

				//Update the repeat mode of the block.
				char s[64];
				sprintf(s, "%u", repeatMode | (i << (index * 8)));
				parent->commandManager->doCommand(new SetEditorPropertyCommand(parent, imageManager, renderer,
					target, "repeatMode", s, _("Repeat mode")));

				//And update the item.
				const char* ss[4] = {
					__("Horizontal repeat start: %s"),
					__("Horizontal repeat end: %s"),
					__("Vertical repeat start: %s"),
					__("Vertical repeat end: %s"),
				};
				const int icons[4] = {
					8 * 2 + 3, 8 * 2 + 4, 8 * 3 + 3, 8 * 3 + 4,
				};
				updateItem(renderer, actions->value, action.c_str(), tfm::format(_(ss[index]), getRepeatModeName(i)).c_str(), icons[index]);
			}
			actions->value = -1;
			return;
		} else if (action == "CustomScenery") {
			//Create the GUI.
			GUIWindow* root = new GUIWindow(imageManager, renderer, (SCREEN_WIDTH - 600) / 2, (SCREEN_HEIGHT - 400) / 2, 600, 400, true, true, _("Custom scenery"));
			root->minWidth = root->width; root->minHeight = root->height;
			root->name = "customSceneryWindow";
			root->eventCallback = parent;
			GUIObject* obj;

			obj = new GUILabel(imageManager, renderer, 50, 60, 240, 36, _("Custom scenery:"));
			root->addChild(obj);

			//Add a text area.
			Scenery* scenery = dynamic_cast<Scenery*>(target);
			GUITextArea* text = new GUITextArea(imageManager, renderer, 50, 100, 500, 240);
			text->gravityRight = text->gravityBottom = GUIGravityRight;
			text->name = "cfgCustomScenery";
			text->setFont(fontMono);
			//Only set the first one visible and enabled.
			text->visible = true;
			text->enabled = true;

			// FIXME: an ad-hoc code
			std::string s;
			for (char c : scenery->customScenery_) {
				if (c == '\t')
					s.append("  ");
				else
					s.push_back(c);
			}
			text->setString(renderer, s);
			root->addChild(text);

			obj = new GUIButton(imageManager, renderer, root->width*0.3, 400 - 44, -1, 36, _("OK"), 0, true, true, GUIGravityCenter);
			obj->gravityLeft = obj->gravityRight = GUIGravityCenter;
			obj->gravityTop = obj->gravityBottom = GUIGravityRight;
			obj->name = "cfgCustomSceneryOK";
			obj->eventCallback = root;
			root->addChild(obj);
			obj = new GUIButton(imageManager, renderer, root->width*0.7, 400 - 44, -1, 36, _("Cancel"), 0, true, true, GUIGravityCenter);
			obj->gravityLeft = obj->gravityRight = GUIGravityCenter;
			obj->gravityTop = obj->gravityBottom = GUIGravityRight;
			obj->name = "cfgCancel";
			obj->eventCallback = root;
			root->addChild(obj);

			//Add the window to the GUIObjectRoot and the objectWindows map.
			GUIObjectRoot->addChild(root);
			parent->objectWindows[root] = target;

			//And dismiss this popup.
			dismiss();
			return;
		} else if (action == "Appearance"){
			//Create the GUI.
			GUIWindow* root = new GUIWindow(imageManager, renderer, (SCREEN_WIDTH - 600) / 2, (SCREEN_HEIGHT - 400) / 2, 600, 400, true, true, _("Appearance"));
			root->minWidth = root->width; root->minHeight = root->height;
			root->name = "appearanceWindow";
			root->eventCallback = parent;
			GUIObject* obj;

			Block* block = dynamic_cast<Block*>(target);

			//Create a list box showing all available scenery blocks.
			GUIListBox *list = new GUIListBox(imageManager, renderer, 50, 60, 440, 280);
			list->gravityRight = list->gravityBottom = GUIGravityRight;
			list->name = "lstAppearance";
			list->eventCallback = root;
			root->addChild(list);

			//TODO: Show the image along with the text in the list box.
			//Currently I don't like to do that because this requires a lot of video memory since there are a lot of available scenery blocks.
			list->addItem(renderer, _("(Use the default appearance for this block)"));
			if (block->customAppearanceName.empty()) list->value = 0;
			for (const std::string& s : parent->sceneryBlockNames) {
				list->addItem(renderer, describeSceneryName(s));
				if (block->customAppearanceName == s) list->value = list->item.size() - 1;
			}
			if (list->value < 0) {
				std::cerr << "WARNING: The custom appearance '" << block->customAppearanceName << "' is unrecognized, added it to the list box anyway" << std::endl;
				list->addItem(renderer, block->customAppearanceName);
				list->value = list->item.size() - 1;
			}

			//Ask the list box to update scrollbar and we scroll the scrollbar to the correct position (FIXME: ad-hoc code)
			list->render(renderer, 0, 0, false);
			list->scrollScrollbar(list->value);

			//Create an image widget showing the appearance of selected scenery block.
			GUIImage *image = new GUIImage(imageManager, renderer, 500, 60, 50, 50);
			image->gravityLeft = image->gravityRight = GUIGravityRight;
			image->name = "imgAppearance";
			root->addChild(image);

			//Add a Change event to the list box which will be processed next frame, which is used to update the image widget.
			GUIEventQueue.push_back(GUIEvent{ list->eventCallback, list->name, list, GUIEventChange });

			obj = new GUIButton(imageManager, renderer, root->width*0.3, 400 - 44, -1, 36, _("OK"), 0, true, true, GUIGravityCenter);
			obj->gravityLeft = obj->gravityRight = GUIGravityCenter;
			obj->gravityTop = obj->gravityBottom = GUIGravityRight;
			obj->name = "cfgAppearanceOK";
			obj->eventCallback = root;
			root->addChild(obj);
			obj = new GUIButton(imageManager, renderer, root->width*0.7, 400 - 44, -1, 36, _("Cancel"), 0, true, true, GUIGravityCenter);
			obj->gravityLeft = obj->gravityRight = GUIGravityCenter;
			obj->gravityTop = obj->gravityBottom = GUIGravityRight;
			obj->name = "cfgCancel";
			obj->eventCallback = root;
			root->addChild(obj);

			//Add the window to the GUIObjectRoot and the objectWindows map.
			GUIObjectRoot->addChild(root);
			parent->objectWindows[root] = target;

			//And dismiss this popup.
			dismiss();
			return;
		}
	}
};


/////////////////LevelEditorSelectionPopup/////////////////

class LevelEditorSelectionPopup{
private:
    //The parent object.
	LevelEditor* parent;

    //The position of window.
	SDL_Rect rect;

    //GUI image.
    SharedTexture bmGUI;

	//The selection
	std::vector<GameObject*> selection;

    //The scrollbar.
	GUIScrollBar* scrollBar;

    //Highlighted object.
	GameObject* highlightedObj;

	//Highlighted button index. 0=none 1=select/deselect 2=delete 3=configure
	int highlightedBtn;
public:
	int startRow,showedRow;

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
    void updateScrollBar(ImageManager& imageManager, SDL_Renderer& renderer){
		int m=selection.size()-showedRow;
		if(m>0){
			if(startRow<0) startRow=0;
			else if(startRow>m) startRow=m;

			if(scrollBar==NULL){
                scrollBar=new GUIScrollBar(imageManager,renderer,0,0,16,rect.h-16,ScrollBarVertical,startRow,0,m,1,showedRow);
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
    void updateSelection(ImageManager& imageManager, SDL_Renderer& renderer){
		if(parent!=NULL){
			std::vector<Block*>& v=parent->levelObjects;

			for(int i=selection.size()-1;i>=0;i--){
				if(find(v.begin(),v.end(),selection[i])==v.end()){
					selection.erase(selection.begin()+i);
				}
			}

            updateScrollBar(imageManager,renderer);
		}
	}
	void dismiss(){
		if(parent!=NULL && parent->selectionPopup==this){
			parent->selectionPopup=NULL;
		}
		delete this;
	}
    LevelEditorSelectionPopup(LevelEditor* parent, ImageManager& imageManager, SDL_Renderer& renderer, std::vector<GameObject*>& selection, int x=0, int y=0){
		this->parent=parent;
		this->selection=selection;

		dirty=false;
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

        updateScrollBar(imageManager,renderer);

		//Load the gui images.
        bmGUI=imageManager.loadTexture(getDataPath()+"gfx/gui.png",renderer);
	}
    virtual ~LevelEditorSelectionPopup(){
		if(scrollBar)
			delete scrollBar;
	}
	void move(int x,int y){
		if(x>SCREEN_WIDTH-rect.w) x=SCREEN_WIDTH-rect.w;
		else if(x<0) x=0;
		if(y>SCREEN_HEIGHT-rect.h) y=SCREEN_HEIGHT-rect.h;
		else if(y<0) y=0;
		rect.x=x;
		rect.y=y;
	}
    void render(ImageManager& imageManager, SDL_Renderer& renderer){
		//Check dirty
		if(dirty){
            updateSelection(imageManager,renderer);
			if(selection.empty()){
				dismiss();
				return;
			}
			dirty=false;
		}

		//background
        drawGUIBox(rect.x,rect.y,rect.w,rect.h,renderer,0xFFFFFFFFU);

		//get mouse position
		int x,y;
		SDL_GetMouseState(&x,&y);
		SDL_Rect mouse={x,y,0,0};

		//the tool tip of item
		SDL_Rect tooltipRect;
        //string tooltip;

		if(scrollBar && scrollBar->visible){
			startRow=scrollBar->value;
		}

		highlightedObj=NULL;
		highlightedBtn=0;

        ToolTips toolTip = ToolTips::TooltipMax;

		int maxWidth = 0;

		//draw avaliable item
		for(int i=0;i<showedRow;i++){
			int j=startRow+i;
			if(j>=(int)selection.size()) break;

			SDL_Rect r={rect.x+8,rect.y+i*64+8,rect.w-16,64};
			if(scrollBar && scrollBar->visible) r.w-=24;

			//check highlight
			if(pointOnRect(mouse,r)){
				highlightedObj=selection[j];
                //0xCCCCCC
                SDL_SetRenderDrawColor(&renderer,0xCC,0xCC,0xCC,0xFF);
                SDL_RenderFillRect(&renderer,&r);
			}

			const int type = selection[j]->type;
			Scenery *scenery = dynamic_cast<Scenery*>(selection[j]);
			if (scenery) {
				if (scenery->themeBlock == &(scenery->internalThemeBlock)) {
					// custom scenery, draw an ad-hoc stupid icon
					if (parent) {
						const SDL_Rect srcRect = { 48, 16, 16, 16 };
						const SDL_Rect dstRect = { r.x + 7, r.y + 7, 16, 16 };
						SDL_RenderCopy(&renderer, parent->bmGUI.get(), &srcRect, &dstRect);
					}
				} else {
					scenery->themeBlock->editorPicture.draw(renderer, r.x + 7, r.y + 7);
				}
			} else {
				//draw tile picture
				ThemeBlock* obj = objThemes.getBlock(type);
				if (obj){
					//obj->editorPicture.draw(screen,r.x+7,r.y+7);
					obj->editorPicture.draw(renderer, r.x + 7, r.y + 7);
				}
			}

			if(parent!=NULL){
                //draw name
				TexturePtr& tex = scenery ? (parent->getCachedTextTexture(renderer, scenery->sceneryName_.empty()
					/// TRANSLATORS: Block name
					? std::string(_("Custom scenery block")) : describeSceneryName(scenery->sceneryName_)))
					: parent->typeTextTextures.at(type);
				if (tex) {
					const int w = textureWidth(tex) + 160;
					if (w > maxWidth) maxWidth = w;
					applyTexture(r.x + 64, r.y + (64 - textureHeight(tex)) / 2, tex, renderer);
				}

				//draw selected
				{
					std::vector<GameObject*> &v=parent->selection;
					bool isSelected=find(v.begin(),v.end(),selection[j])!=v.end();

					SDL_Rect r1={isSelected?16:0,0,16,16};
					SDL_Rect r2={r.x+r.w-72,r.y+20,24,24};
					if(pointOnRect(mouse,r2)){
                        drawGUIBox(r2.x,r2.y,r2.w,r2.h,renderer,0x999999FFU);
						tooltipRect=r2;
                        //tooltip=_("Select");
						highlightedBtn=1;
                        toolTip=ToolTips::Select;
					}
					r2.x+=4;
					r2.y+=4;
                    r2.w=r1.w;
                    r2.h=r1.h;
                    SDL_RenderCopy(&renderer, bmGUI.get(),&r1,&r2);
				}

				//draw delete
				{
					SDL_Rect r1={112,0,16,16};
					SDL_Rect r2={r.x+r.w-48,r.y+20,24,24};
					if(pointOnRect(mouse,r2)){
                        drawGUIBox(r2.x,r2.y,r2.w,r2.h,renderer,0x999999FFU);
						tooltipRect=r2;
                        //tooltip=_("Delete");
						highlightedBtn=2;
                        toolTip=ToolTips::Delete;
					}
					r2.x+=4;
					r2.y+=4;
                    r2.w=r1.w;
                    r2.h=r1.h;
                    SDL_RenderCopy(&renderer, bmGUI.get(),&r1,&r2);
				}

				//draw configure
				{
					SDL_Rect r1={112,16,16,16};
					SDL_Rect r2={r.x+r.w-24,r.y+20,24,24};
					if(pointOnRect(mouse,r2)){
                        drawGUIBox(r2.x,r2.y,r2.w,r2.h,renderer,0x999999FFU);
						tooltipRect=r2;
                        //tooltip=_("Configure");
                        toolTip=ToolTips::Configure;
						highlightedBtn=3;
					}
					r2.x+=4;
					r2.y+=4;
                    r2.w=r1.w;
                    r2.h=r1.h;
                    SDL_RenderCopy(&renderer, bmGUI.get(),&r1,&r2);
				}
			}
		}

		//draw scrollbar
		if(scrollBar && scrollBar->visible){
            scrollBar->render(renderer,rect.x+rect.w-24,rect.y+8);
		}

		//draw tooltip
        if(parent && int(toolTip) < parent->tooltipTextures.size()){
			//Tool specific text.
            TexturePtr& tip=parent->tooltipTextures.at(size_t(toolTip));

			//Draw only if there's a tooltip available
            if(tip){
                const auto tipSize = rectFromTexture(tip);
				tooltipRect.y-=4;
				tooltipRect.h+=8;
                if(tooltipRect.y+tooltipRect.h+tipSize.h>SCREEN_HEIGHT-20)
                    tooltipRect.y-=tipSize.h;
				else
					tooltipRect.y+=tooltipRect.h;

                if(tooltipRect.x+tipSize.w>SCREEN_WIDTH-20)
                    tooltipRect.x=SCREEN_WIDTH-20-tipSize.w;

				//Draw borders around text
				Uint32 color=0xFFFFFF00|230;
                drawGUIBox(tooltipRect.x-2,tooltipRect.y-2,tipSize.w+4,tipSize.h+4,renderer,color);

				//Draw tooltip's text
                applyTexture(tooltipRect.x,tooltipRect.y,tip,renderer);
			}
		}

		//Resize the selection popup if necessary
		if (maxWidth > rect.w) {
			rect.w = maxWidth;
			move(rect.x, rect.y);
		}
	}
    void handleEvents(ImageManager& imageManager,SDL_Renderer& renderer){
		//Check dirty
		if(dirty){
            updateSelection(imageManager,renderer);
			if(selection.empty()){
				dismiss();
				return;
			}
			dirty=false;
		}

		//Check scrollbar event
		if(scrollBar && scrollBar->visible){
            if(scrollBar->handleEvents(renderer,rect.x+rect.w-24,rect.y+8)) return;
		}

		if(event.type==SDL_MOUSEBUTTONDOWN){
			if(event.button.button==SDL_BUTTON_LEFT){
				SDL_Rect mouse={event.button.x,event.button.y,0,0};

				//Check if close it
				if(!pointOnRect(mouse,rect)){
					dismiss();
					return;
				}

				//Check if item is clicked
				if(highlightedObj!=NULL && highlightedBtn>0 && parent!=NULL){
					//std::vector<Block*>& v=parent->levelObjects;

					if(/*find(v.begin(),v.end(),highlightedObj)!=v.end()*/true/*???*/){
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
							parent->commandManager->doCommand(new AddRemoveGameObjectCommand(parent, highlightedObj, false));
							break;
						case 3:
							if(parent->actionsPopup)
								delete parent->actionsPopup;
                            parent->actionsPopup=new LevelEditorActionsPopup(imageManager,renderer,parent,highlightedObj,mouse.x,mouse.y);
							break;
						}
					}
				}
			}
		}
		else if(event.type == SDL_MOUSEWHEEL) {
			//check mousewheel
			if(event.wheel.y < 0){
				startRow-=2;
                updateScrollBar(imageManager,renderer);
				return;
			} else {
				startRow+=2;
                updateScrollBar(imageManager,renderer);
				return;
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
LevelEditor::LevelEditor(SDL_Renderer& renderer, ImageManager& imageManager):Game(renderer, imageManager){
	//This will set some default settings.
	reset();

	//Create the GUI root.
    GUIObjectRoot=new GUIObject(imageManager,renderer,0,0,SCREEN_WIDTH,SCREEN_HEIGHT);

	//Load the toolbar.
    toolbar=imageManager.loadTexture(getDataPath()+"gfx/menu/toolbar.png",renderer);
    toolbarRect={(SCREEN_WIDTH-460)/2,SCREEN_HEIGHT-50,460,50};

	selectionPopup=NULL;
	actionsPopup=NULL;

	movingSpeedWidth=-1;

	//Load the selectionMark.
    selectionMark=imageManager.loadTexture(getDataPath()+"gfx/menu/selection.png",renderer);

	//Load the movingMark.
    movingMark=imageManager.loadTexture(getDataPath()+"gfx/menu/moving.png",renderer);

	//Load the gui images.
    bmGUI=imageManager.loadTexture(getDataPath()+"gfx/gui.png",renderer);
    toolboxText=textureFromText(renderer,*fontText,_("Toolbox"),objThemes.getTextColor(true));

    for(size_t i = 0;i < typeTextTextures.size();++i) {
        typeTextTextures[i] =
                    textureFromText(renderer,
                                    *fontText,
                                    _(blockNames[i]),
                                    objThemes.getTextColor(true));
    }

    for(size_t i = 0;i < tooltipTextures.size();++i) {
		if (tooltipNames[i][0]) {
            tooltipTextures[i] =
                    textureFromText(renderer,
                                    *fontText,
                                    _(tooltipNames[i]),
                                    objThemes.getTextColor(true));
        }
    }

	//Count the level editing time.
	statsMgr.startLevelEdit();

	//Create the command manager with a maximum of 100 commands.
	commandManager = new CommandManager(100);
}

// FIXME: I have to write this function here since we need to access the static blockNames[]
std::string MoveGameObjectCommand::describe() {
	if (objects.size() == 1) {
		const bool isResize = oldPosition[0].w != newPosition[0].w || oldPosition[0].h != newPosition[0].h;
		Scenery *scenery = dynamic_cast<Scenery*>(objects[0]);
		return tfm::format(isResize ? _("Resize %s") : _("Move %s"), scenery ?
			/// TRANSLATORS: Context: Resize/Move ...
			(scenery->sceneryName_.empty() ? _("Custom scenery block")
				: describeSceneryName(scenery->sceneryName_).c_str())
				: _(blockNames[objects[0]->type]));
	} else {
		const size_t number_of_objects = objects.size();
		/// TRANSLATORS: Context: Undo/Redo ...
		return tfm::format(ngettext("Move %d object", "Move %d objects", number_of_objects).c_str(), number_of_objects);
	}
}

// FIXME: I have to write this function here since we need to access the static blockNames[]
std::string AddRemoveGameObjectCommand::describe() {
	if (objects.size() == 1) {
		Scenery *scenery = dynamic_cast<Scenery*>(objects[0]);
		return tfm::format(isAdd ? _("Add %s") : _("Remove %s"), scenery ? (scenery->sceneryName_.empty() ?
			/// TRANSLATORS: Context: Add/Remove ...
			_("Custom scenery block")
			: describeSceneryName(scenery->sceneryName_).c_str())
			: _(blockNames[objects[0]->type]));
	} else {
		const size_t number_of_objects = objects.size();
		/// TRANSLATORS: Context: Undo/Redo ...
		return tfm::format(isAdd ? ngettext("Add %d object", "Add %d objects", number_of_objects).c_str() :
			/// TRANSLATORS: Context: Undo/Redo ...
			ngettext("Remove %d object", "Remove %d objects", number_of_objects).c_str(), number_of_objects);
	}
}

// FIXME: I have to write this function here since we need to access the static blockNames[]
std::string AddRemovePathCommand::describe() {
	return tfm::format(isAdd ?
		/// TRANSLATORS: Context: Undo/Redo ...
		_("Add path to %s") :
		/// TRANSLATORS: Context: Undo/Redo ...
		_("Remove a path point from %s"), _(blockNames[target->type]));
}

// FIXME: I have to write this function here since we need to access the static blockNames[]
std::string RemovePathCommand::describe() {
	/// TRANSLATORS: Context: Undo/Redo ...
	return tfm::format(_("Remove all paths from %s"), _(blockNames[target->type]));
}

// FIXME: I have to write this function here since we need to access the static blockNames[]
std::string AddLinkCommand::describe() {
	/// TRANSLATORS: Context: Undo/Redo ...
	return tfm::format(_("Add link from %s to %s"), _(blockNames[target->type]), _(blockNames[clickedObj->type]));
}

// FIXME: I have to write this function here since we need to access the static blockNames[]
std::string RemoveLinkCommand::describe() {
	/// TRANSLATORS: Context: Undo/Redo ...
	return tfm::format(_("Remove all links from %s"), _(blockNames[target->type]));
}

// FIXME: I have to write this function here since we need to access the static blockNames[]
std::string SetEditorPropertyCommand::describe() {
	Scenery *scenery = dynamic_cast<Scenery*>(target);
	/// TRANSLATORS: Context: Undo/Redo ...
	std::string s = _("Modify the %2 property of %1");
	size_t lp = s.find("%1");
	if (lp != string::npos) {
		std::string s1 = scenery ?
			(scenery->sceneryName_.empty() ?
				/// TRANSLATORS: Context: Undo/Redo ...
				_("Custom scenery block")
				: describeSceneryName(scenery->sceneryName_).c_str())
			: _(blockNames[target->type]);
		s = s.substr(0, lp) + s1 + s.substr(lp + 2);
	}
	lp = s.find("%2");
	if (lp != string::npos) {
		s = s.substr(0, lp) + desc + s.substr(lp + 2);
	}
	return s;
}

// FIXME: I have to write this function here since we need to access the static variable levelTime and levelRecordings
SetLevelPropertyCommand::SetLevelPropertyCommand(LevelEditor* levelEditor, const LevelProperty& levelProperty)
	: editor(levelEditor), newProperty(levelProperty)
{
	oldProperty.levelName = editor->levelName;
	oldProperty.levelTheme = editor->levelTheme;
	oldProperty.levelMusic = editor->levelMusic;
	oldProperty.levelTime = levelTime;
	oldProperty.levelRecordings = levelRecordings;
}

// FIXME: I have to write this function here since we need to access the static variable levelTime and levelRecordings
void SetLevelPropertyCommand::setLevelProperty(const LevelProperty& levelProperty) {
	bool musicChanged = editor->levelMusic != levelProperty.levelMusic;

	editor->levelName = levelProperty.levelName;
	editor->levelTheme = levelProperty.levelTheme;
	editor->levelMusic = levelProperty.levelMusic;
	levelTime = levelProperty.levelTime;
	levelRecordings = levelProperty.levelRecordings;

	if (musicChanged) {
#ifdef _DEBUG
		printf("DEBUG: Level music is changed dynamically in level editor\n");
#endif
		editor->editorData["music"] = editor->levelMusic;
		editor->reloadMusic();
	}
}

// FIXME: I have to write this function here since we need to access the static blockNames[]
std::string SetScriptCommand::describe() {
	if (target) {
		/// TRANSLATORS: Context: Undo/Redo ...
		return tfm::format(_("Edit the script of %s"), _(blockNames[target->type]));
	} else {
		/// TRANSLATORS: Context: Undo/Redo ...
		return _("Edit the script of level");
	}
}

LevelEditor::~LevelEditor(){
	//Delete the command manager.
	delete commandManager;

	// NOTE: We don't need to delete levelObjects, etc. since they are deleted in Game::~Game().

	// Clear selection
	selection.clear();

	//Delete the GUI.
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}

	//Delete the popup
	if(selectionPopup){
		delete selectionPopup;
		selectionPopup=NULL;
	}

	//Delete the popup
	if(actionsPopup){
		delete actionsPopup;
		actionsPopup=NULL;
	}

	//Reset the camera.
	camera.x=0;
	camera.y=0;

	//Count the level editing time.
	statsMgr.endLevelEdit();
}

TexturePtr& LevelEditor::getCachedTextTexture(SDL_Renderer& renderer, const std::string& text) {
	auto it = cachedTextTextures.find(text);
	if (it != cachedTextTextures.end()) return it->second;
	return (cachedTextTextures[text] = textureFromText(renderer,
		*fontText,
		text.c_str(),
		objThemes.getTextColor(true)));
}

void LevelEditor::reset(){
	//Set some default values.
	playMode=false;
	tool=ADD;
	currentType=0;
	toolboxVisible=false;
	toolboxRect.x=-1;
	toolboxRect.y=-1;
	toolboxRect.w=0;
	toolboxRect.h=0;
	toolboxIndex=0;
	pressedShift=false;
	pressedLeftMouse=false;
	dragging=false;
	selectionDrag=-1;
	dragCenter=NULL;
	cameraXvel=0;
	cameraYvel=0;
	linking=false;
	linkingTrigger=NULL;
	currentId=0;
	movingBlock=NULL;
	moving=false;
	movingSpeed=10;
	pauseTime = 0;
	tooltip=-1;

	//Set the player and shadow to their starting position.
	player.setLocation(player.fx,player.fy);
	shadow.setLocation(shadow.fx,shadow.fy);

	selection.clear();
	clipboard.clear();
	triggers.clear();
	movingBlocks.clear();

	//Delete any gui.
	if(GUIObjectRoot){
		delete GUIObjectRoot;
		GUIObjectRoot=NULL;
	}
	//Clear the GUIWindow object map.
	objectWindows.clear();
}

void LevelEditor::loadLevelFromNode(ImageManager& imageManager, SDL_Renderer& renderer,TreeStorageNode* obj, const std::string& fileName){
	//call the method of base class.
    Game::loadLevelFromNode(imageManager,renderer,obj,fileName);

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

	levelTheme = editorData["theme"];
	levelMusic = editorData["music"];

	//NOTE: We set the camera here since we know the dimensions of the level.
	if(LEVEL_WIDTH<SCREEN_WIDTH)
		camera.x=-(SCREEN_WIDTH-LEVEL_WIDTH)/2;
	else
		camera.x=0;
	if(LEVEL_HEIGHT<SCREEN_HEIGHT)
		camera.y=-(SCREEN_HEIGHT-LEVEL_HEIGHT)/2;
	else
		camera.y=0;

	//The level is loaded, so call postLoad.
	postLoad();
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

	//The level theme.
	if(!levelTheme.empty())
		node.attributes["theme"].push_back(levelTheme);

	//The level music.
	if (!levelMusic.empty())
		node.attributes["music"].push_back(levelMusic);

	//target time and recordings.
	{
		char c[32];
		if(levelTime>=0){
			sprintf(c,"%d",levelTime);
			node.attributes["time"].push_back(c);

			//Update the target time the levelpack.
			levels->getLevel()->targetTime=levelTime;
		}
		if(levelRecordings>=0){
			sprintf(c,"%d",levelRecordings);
			node.attributes["recordings"].push_back(c);

			//Update the target recordings the levelpack.
			levels->getLevel()->targetRecordings=levelRecordings;
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
			obj1->value.push_back(blockName[objectType]);

			//Get the box for the location of the gameObject.
			SDL_Rect box=levelObjects[o]->getBox(BoxType_Base);
			//Put the location and size in the storageNode.
			sprintf(s,"%d",box.x);
			obj1->value.push_back(s);
			sprintf(s,"%d",box.y);
			obj1->value.push_back(s);
			sprintf(s,"%d",box.w);
			obj1->value.push_back(s);
			sprintf(s,"%d",box.h);
			obj1->value.push_back(s);

			//Loop through the editor data and save it also.
			vector<pair<string,string> > obj;
			levelObjects[o]->getEditorData(obj);
			for (const auto& o : obj) {
				//Skip the data whose key or value is empty.
				if (o.first.empty()) continue;
				if (o.second.empty()) continue;

				//Skip SOME data whose value is the default value. Currently only some boolean values are skipped.
				//WARNING: When the default values are changed, these codes MUST be modified accordingly!!!

				//NOTE: Currently we skip the "visible" property since it is used for every block and usually it's the default value.
				if (o.first == "visible" && o.second == "1") continue;

#if 0
				//NOTE: The following codes are more aggressive!!!
				//if (o.first == "activated" && o.second == "1") continue; //moving blocks and conveyor belt // Don't use this because there was a "disabled" property
				if (o.first == "loop" && o.second == "1") continue; //moving blocks and conveyor belt
				if (o.first == "automatic" && o.second == "0") continue; //portal
#endif

				//Save the data.
				obj1->attributes[o.first].push_back(o.second);
			}

			//Loop through the scripts and add them to the storage node of the game object.
			map<int,string>::iterator it;
			Block* object=(dynamic_cast<Block*>(levelObjects[o]));
			for(it=object->scripts.begin();it!=object->scripts.end();++it){
				//Make sure the script isn't an empty string.
				if(it->second.empty())
					continue;

				TreeStorageNode* script=new TreeStorageNode;
				obj1->subNodes.push_back(script);

				script->name="script";
				script->value.push_back(gameObjectEventTypeMap[it->first]);

				script->attributes["script"].push_back(it->second);
			}
		}
	}

	//Loop through the level scripts and save them.
	for(auto it=scripts.begin();it!=scripts.end();++it){
		//Make sure the script isn't an empty string.
		if(it->second.empty())
			continue;

		TreeStorageNode* script=new TreeStorageNode;
		node.subNodes.push_back(script);

		script->name="script";
		script->value.push_back(levelEventTypeMap[it->first]);

		script->attributes["script"].push_back(it->second);
	}

	//Loop through the scenery layers and save them.
	for (auto it = sceneryLayers.begin(); it != sceneryLayers.end(); ++it) {
		TreeStorageNode* layer = new TreeStorageNode;
		node.subNodes.push_back(layer);

		layer->name = "scenerylayer";
		layer->value.push_back(it->first);

		it->second->saveToNode(layer);
	}

	//Create a POASerializer and write away the level node.
	POASerializer objSerializer;
	objSerializer.writeNode(&node,save,true,true);
}

void LevelEditor::deselectAll() {
	selection.clear();
	dragCenter = NULL;
	selectionDrag = -1;
	selectionDirty();
}

///////////////EVENT///////////////////
void LevelEditor::handleEvents(ImageManager& imageManager, SDL_Renderer& renderer){
	//Check if we need to quit, if so we enter the exit state.
	if(event.type==SDL_QUIT){
		setNextState(STATE_EXIT);
	}

	//If playing/testing we should the game handle the events.
	if(playMode){
        Game::handleEvents(imageManager,renderer);

		//Also check if we should exit the playMode.
		if(inputMgr.isKeyDownEvent(INPUTMGR_ESCAPE)){
			//Reset the game and disable playMode, disable script.
			Game::reset(true, true);
			playMode=false;
			GUIObjectRoot->visible=true;
			camera.x=cameraSave.x;
			camera.y=cameraSave.y;

			//NOTE: To prevent the mouse to still "be pressed" we set it to false.
			pressedLeftMouse=false;
		}
	}else{
		//Also check if we should exit the editor.
		if(inputMgr.isKeyDownEvent(INPUTMGR_ESCAPE)){
			std::string s;
			//Check if the file is changed
			if (commandManager->isChanged()) {
				s = _("The level has unsaved changes.");
				s.push_back('\n');
			}
			//Before we quit ask a make sure question.
            if(msgBox(imageManager,renderer,s+_("Are you sure you want to quit?"),MsgBoxYesNo,_("Quit prompt"))==MsgBoxYes){
				//We exit the level editor.
				setNextState(STATE_LEVEL_EDIT_SELECT);

				//Play the menu music again.
				getMusicManager()->playMusic("menu");

				//No need for handling other events, so return.
				return;
			}
		}

		//Check if we should redirect the event to the actions popup
		if(actionsPopup!=NULL){
            actionsPopup->handleEvents(renderer);
			return;
		}
		//Check if we should redirect the event to selection popup
		if(selectionPopup!=NULL){
			if(event.type==SDL_MOUSEBUTTONDOWN
				|| event.type==SDL_MOUSEBUTTONUP
				|| event.type==SDL_MOUSEMOTION)
			{
                selectionPopup->handleEvents(imageManager,renderer);
				return;
			}
		}
		//TODO: Don't handle any Events when GUIWindows process them.
		{
			//Get the current mouse location.
			int x,y;
			SDL_GetMouseState(&x,&y);
			//Create the rectangle.
			SDL_Rect mouse={x,y,0,0};
			for(unsigned int i=0;i<GUIObjectRoot->childControls.size();i++){
				SDL_Rect box={0,0,0,0};
				box.x=GUIObjectRoot->childControls[i]->left;
				box.y=GUIObjectRoot->childControls[i]->top;
				box.w=GUIObjectRoot->childControls[i]->width;
				box.h=GUIObjectRoot->childControls[i]->height;
				if(pointOnRect(mouse,box))
					return;
			}
		}

		//Check if toolbar is clicked.
		if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button==SDL_BUTTON_LEFT && tooltip>=0){
			int t=tooltip;

			if(t<NUMBER_TOOLS){
				tool=(Tools)t;

				//Stop linking or moving if the mode is not SELECT.
				if (tool != SELECT) {
					if (linking) {
						linking = false;
						linkingTrigger = NULL;
					}
					if (moving) {
						moving = false;
						movingBlock = NULL;
					}
				}
			}else{
				//The selected button isn't a tool.
				//Now check which button it is.
				if (t == (int)ToolTips::Play){
					enterPlayMode();
				}
				if (t == (int)ToolTips::LevelSettings){
					//Open up level settings dialog
                    levelSettings(imageManager,renderer);
				}
				if (t == (int)ToolTips::BackToMenu){
					//If the file is changed we show a confirmation dialog
					if (commandManager->isChanged()) {
						std::string s = _("The level has unsaved changes.");
						s.push_back('\n');
						if (msgBox(imageManager, renderer, s + _("Are you sure you want to quit?"), MsgBoxYesNo, _("Quit prompt")) != MsgBoxYes) return;
					}

					//Go back to the level selection screen of Level Editor
					setNextState(STATE_LEVEL_EDIT_SELECT);
					//Change the music back to menu music.
					getMusicManager()->playMusic("menu");
				}
				if (t == (int)ToolTips::SaveLevel){
					//Save current level
					saveCurrentLevel(imageManager, renderer);
				}
				if (t == (int)ToolTips::UndoNoTooltip) {
					commandManager->undo();
				}
				if (t == (int)ToolTips::RedoNoTooltip) {
					commandManager->redo();
				}
			}

			return;
		}

		//Check if tool box is clicked.
		if(event.type==SDL_MOUSEBUTTONDOWN && event.button.button==SDL_BUTTON_LEFT && toolboxRect.w>0){
			if(toolboxVisible){
				if(event.button.y<64){
					//Check if we need to hide it
					if(event.button.x>=SCREEN_WIDTH-24 && event.button.x<SCREEN_WIDTH && event.button.y<20){
						toolboxVisible=false;
						return;
					}

					const int m = (SCREEN_WIDTH - 48) / 64;

					//Check if a block is clicked.
					if(event.button.x>=24 && event.button.x<SCREEN_WIDTH-24){
						int i=(event.button.x-24)/64;
						if(i<m && i+toolboxIndex<getEditorOrderMax()){
							currentType=i+toolboxIndex;
						}
					}

					//Check if move left button is clicked
					if (event.button.x >= 0 && event.button.x < 24 && event.button.y >= 20 && event.button.y < 44) {
						toolboxIndex -= m;
						if (toolboxIndex < 0) toolboxIndex = 0;
					}

					//Check if move right button is clicked
					if (event.button.x >= SCREEN_WIDTH - 24 && event.button.x < SCREEN_WIDTH && event.button.y >= 20 && event.button.y < 44) {
						toolboxIndex += m;
						if (toolboxIndex > getEditorOrderMax() - m) toolboxIndex = getEditorOrderMax() - m;
						if (toolboxIndex < 0) toolboxIndex = 0;
					}

					return;
				}
			}else if(event.button.x>=toolboxRect.x && event.button.x<toolboxRect.x+toolboxRect.w
				&& event.button.y>=toolboxRect.y && event.button.y<toolboxRect.y+toolboxRect.h)
			{
				toolboxVisible=true;
				return;
			}
		}

		//Check if shift is pressed.
		pressedShift=inputMgr.isKeyDown(INPUTMGR_SHIFT);

		//Check if delete is pressed.
		if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_DELETE){
			if (!selection.empty()){
				AddRemoveGameObjectCommand *command = new AddRemoveGameObjectCommand(this, selection, false);

				//clear the selection vector first.
				deselectAll();

				//perform the actual deletion.
				commandManager->doCommand(command);
			}
		}

		//Check for copy (Ctrl+c) or cut (Ctrl+x).
		if(event.type==SDL_KEYDOWN && (event.key.keysym.sym==SDLK_c || event.key.keysym.sym==SDLK_x) && (event.key.keysym.mod & KMOD_CTRL)){
			//Check if the selection isn't empty.
			if(!selection.empty()){
				//Clear the current clipboard.
				clipboard.clear();

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
					SDL_Rect r = selection[o]->getBox();
					sprintf(s, "%d", r.x - x);
					objMap["x"]=s;
					sprintf(s, "%d", r.y - y);
					objMap["y"]=s;
					sprintf(s, "%d", selection[o]->getBox().w);
					objMap["w"] = s;
					sprintf(s, "%d", selection[o]->getBox().h);
					objMap["h"] = s;

					Scenery *scenery = dynamic_cast<Scenery*>(selection[o]);
					if (scenery) {
						objMap["sceneryName"] = scenery->sceneryName_;
						objMap["customScenery"] = scenery->customScenery_;
					} else {
						sprintf(s, "%d", selection[o]->type);
						objMap["type"] = s;
					}

					//Overwrite the id to prevent triggers, portals, buttons, movingblocks, etc. from malfunctioning.
					//We give an empty string as id, which is invalid and thus suitable.
					objMap["id"]="";
					//Do the same for destination if the type is portal.
					if(selection[o]->type==TYPE_PORTAL){
						objMap["destination"]="";
					}

					//And add the map to the clipboard vector.
					clipboard.push_back(objMap);
				}

				//Cutting means deleting the game object.
				if (event.key.keysym.sym == SDLK_x && !selection.empty()){
					AddRemoveGameObjectCommand *command = new AddRemoveGameObjectCommand(this, selection, false);

					//clear the selection vector first.
					deselectAll();

					//perform the actual deletion.
					commandManager->doCommand(command);
				}
			}
		}

		//Check for paste (Ctrl+v).
		if(event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_v && (event.key.keysym.mod & KMOD_CTRL)){
			//First make sure that the clipboard isn't empty.
			if(!clipboard.empty()){
				//Clear the current selection.
				deselectAll();

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

				std::vector<GameObject*> newObjects;

				//Loop through the clipboard.
				for(unsigned int o=0;o<clipboard.size();o++){
					GameObject *obj = NULL;

					if (clipboard[o].find("sceneryName") == clipboard[o].end()) {
						// a normal block
						if (!selectedLayer.empty()) continue;
						obj = new Block(this, 0, 0, 50, 50, atoi(clipboard[o]["type"].c_str()));
					} else {
						// a scenery block
						if (selectedLayer.empty()) continue;
						Scenery *scenery = new Scenery(this, 0, 0, 50, 50, clipboard[o]["sceneryName"]);
						if (clipboard[o]["sceneryName"].empty()) {
							scenery->customScenery_ = clipboard[o]["customScenery"];
							scenery->updateCustomScenery(imageManager, renderer);
						}
						obj = scenery;
					}

					obj->setBaseLocation(atoi(clipboard[o]["x"].c_str())+x,atoi(clipboard[o]["y"].c_str())+y);
					obj->setBaseSize(atoi(clipboard[o]["w"].c_str()), atoi(clipboard[o]["h"].c_str()));
					obj->setEditorData(clipboard[o]);

					//add the object.
					newObjects.push_back(obj);

					//Also add the block to the selection.
					selection.push_back(obj);
				}

				// Do the actual object insertion
				if (!newObjects.empty()) {
					commandManager->doCommand(new AddRemoveGameObjectCommand(this, newObjects, true));
				}
			}
		}

		//Check for the arrow keys, used for moving the camera when playMode=false.
		if (inputMgr.isKeyDown(INPUTMGR_RIGHT)) {
			if (cameraXvel < 5) cameraXvel = 5;
		} else if (inputMgr.isKeyDown(INPUTMGR_LEFT)) {
			if (cameraXvel > -5) cameraXvel = -5;
		} else {
			cameraXvel = 0;
		}
		if(inputMgr.isKeyDown(INPUTMGR_DOWN)){
			if (cameraYvel < 5) cameraYvel = 5;
		} else if (inputMgr.isKeyDown(INPUTMGR_UP)){
			if (cameraYvel > -5) cameraYvel = -5;
		} else {
			cameraYvel = 0;
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
		if(pressedLeftMouse && event.type==SDL_MOUSEMOTION) {
			//Check if this is the start of the dragging.
			if(!dragging){
				//The mouse is moved enough so let's set dragging true.
				dragging=true;
				// NOTE: We start drag from previous mouse position to prevent resize area hit test bug
				onDragStart(event.motion.x - event.motion.xrel + camera.x, event.motion.y - event.motion.yrel + camera.y);
				onDrag(event.motion.xrel, event.motion.yrel);
			} else {
				//Dragging was already true meaning we call onDrag() instead of onDragStart().
				onDrag(event.motion.xrel,event.motion.yrel);
			}
		}

		//Update cursor.
		if(dragging){
			if (tool == REMOVE) {
				currentCursor = CURSOR_REMOVE;
			} else {
				switch (selectionDrag) {
				case 0: case 8:
					currentCursor = CURSOR_SIZE_FDIAG;
					break;
				case 1: case 7:
					currentCursor = CURSOR_SIZE_VER;
					break;
				case 2: case 6:
					currentCursor = CURSOR_SIZE_BDIAG;
					break;
				case 3: case 5:
					currentCursor = CURSOR_SIZE_HOR;
					break;
				case 4:
					currentCursor = CURSOR_DRAG;
					break;
				default:
					currentCursor = CURSOR_POINTER;
					break;
				}
			}
		}

		//Get the current mouse location.
		int x,y;
		SDL_GetMouseState(&x,&y);
		//Create the rectangle.
		SDL_Rect mouse={x,y,0,0};

		//Check if we scroll up, meaning the currentType++;
		if((event.type==SDL_MOUSEWHEEL && event.wheel.y > 0) || inputMgr.isKeyDownEvent(INPUTMGR_NEXT)){
			switch(tool){
			case ADD:
				//Check if mouse is in tool box.
				if(toolboxVisible && toolboxRect.w>0){
					int x,y;
					SDL_GetMouseState(&x,&y);
					if(y<64){
						const int m = getEditorOrderMax() - (SCREEN_WIDTH - 48) / 64;
						toolboxIndex -= 2;
						if (toolboxIndex>m) toolboxIndex = m;
						if (toolboxIndex<0) toolboxIndex = 0;
						break;
					}
				}
				//Only change the current type when using the add tool.
				currentType++;
				if (currentType >= getEditorOrderMax()){
					currentType=0;
				}
				break;
			case SELECT:
				//When configuring moving blocks.
				if(moving){
					if (pauseMode) {
						//Here we have to use Ctrl because Shift means snap
						pauseTime += (SDL_GetModState() & KMOD_CTRL) ? 10 : 1;
					} else {
						//Here we have to use Ctrl because Shift means snap
						movingSpeed += (SDL_GetModState() & KMOD_CTRL) ? 10 : 1;
						//The movingspeed is capped at 125 (10 block/s).
						if (movingSpeed > 125){
							movingSpeed = 125;
						}
					}
					break;
				}
				//Fall through.
			default:
				//When in other mode, just scrolling the map
				if (pressedShift) camera.x = clamp(camera.x - 200, -1000 - SCREEN_WIDTH, LEVEL_WIDTH + 1000);
				else camera.y = clamp(camera.y - 200, -1000 - SCREEN_HEIGHT, LEVEL_HEIGHT + 1000);
				break;
			}
		}
		//Check if we scroll down, meaning the currentType--;
		if((event.type==SDL_MOUSEWHEEL && event.wheel.y < 0) || inputMgr.isKeyDownEvent(INPUTMGR_PREVIOUS)){
			switch(tool){
			case ADD:
				//Check if mouse is in tool box.
				if(toolboxVisible && toolboxRect.w>0){
					int x,y;
					SDL_GetMouseState(&x,&y);
					if(y<64){
						const int m = getEditorOrderMax() - (SCREEN_WIDTH - 48) / 64;
						toolboxIndex+=2;
						if(toolboxIndex>m) toolboxIndex=m;
						if(toolboxIndex<0) toolboxIndex=0;
						break;
					}
				}
				//Only change the current type when using the add tool.
				currentType--;
				if(currentType<0){
					currentType = getEditorOrderMax() - 1;
				}
				break;
			case SELECT:
				//When configuring moving blocks.
				if(moving){
					if (pauseMode) {
						//Here we have to use Ctrl because Shift means snap
						pauseTime -= (SDL_GetModState() & KMOD_CTRL) ? 10 : 1;
						if (pauseTime < -1){
							pauseTime = -1;
						}
					} else {
						//Here we have to use Ctrl because Shift means snap
						movingSpeed -= (SDL_GetModState() & KMOD_CTRL) ? 10 : 1;
						if (movingSpeed <= 0){
							movingSpeed = 1;
						}
					}
					break;
				}
				//Fall through.
			default:
				//When in other mode, just scrolling the map
				if (pressedShift) camera.x = clamp(camera.x + 200, -1000 - SCREEN_WIDTH, LEVEL_WIDTH + 1000);
				else camera.y = clamp(camera.y + 200, -1000 - SCREEN_HEIGHT, LEVEL_HEIGHT + 1000);
				break;
			}
		}

		if (event.type == SDL_KEYDOWN) {
			bool unlink = false;

			//Check if we should enter playMode.
			if (event.key.keysym.sym == SDLK_F5){
				enterPlayMode();
			}
			//Check for tool shortcuts.
			if (event.key.keysym.sym == SDLK_F2){
				tool = SELECT;
			}
			if (event.key.keysym.sym == SDLK_F3){
				tool = ADD;
				unlink = true;
			}
			if (event.key.keysym.sym == SDLK_F4){
				tool = REMOVE;
				unlink = true;
			}

			//Stop linking or moving if the mode is not SELECT.
			if (unlink) {
				if (linking) {
					linking = false;
					linkingTrigger = NULL;
				}
				if (moving) {
					moving = false;
					movingBlock = NULL;
				}
			}
		}

		//Check for certain events.

		//First make sure the mouse isn't above the toolbar.
		if(!pointOnRect(mouse,toolbarRect) && !pointOnRect(mouse,toolboxRect)){
			mouse.x+=camera.x;
			mouse.y+=camera.y;

			//Boolean if there's a click event fired.
			bool clickEvent=false;
			//Check if a mouse button is pressed.
			if(event.type==SDL_MOUSEBUTTONDOWN){
				//Right click in path or link mode means return to normal mode.
				if (event.button.button == SDL_BUTTON_RIGHT && (linking || moving)) {
					//Stop linking.
					linking = false;
					linkingTrigger = NULL;

					//Stop moving.
					moving = false;
					movingBlock = NULL;

					//Stop processing further.
					return;
				}

				std::vector<GameObject*> clickObjects;

				//Loop through the objects to check collision.
				if (selectedLayer.empty()) {
					if (layerVisibility[selectedLayer]) {
						for (unsigned int o = 0; o<levelObjects.size(); o++){
							if (pointOnRect(mouse, levelObjects[o]->getBox()) == true){
								clickObjects.push_back(levelObjects[o]);
							}
						}
					}
				} else {
					auto it = sceneryLayers.find(selectedLayer);
					if (it != sceneryLayers.end() && layerVisibility[selectedLayer]) {
						for (auto o : it->second->objects){
							if (pointOnRect(mouse, o->getBox()) == true){
								clickObjects.push_back(o);
							}
						}
					}
				}

				//Check if there are multiple objects above eachother or just one.
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
                        onRightClickObject(imageManager,renderer,clickObjects[0],isSelected);
					}
                }else if(clickObjects.size()>=1){
					//There are more than one object under the mouse
                    //SDL2 port (never managed to trigger this without changing the parameters.
					std::vector<GameObject*>::iterator it;
					it=find(selection.begin(),selection.end(),clickObjects[0]);

					//Set event true since there's a click event.
					clickEvent=true;

					//Check if the clicked object is in the selection or not.
					bool isSelected=(it!=selection.end());

					//Only show the selection popup when right clicking.
					if(event.button.button==SDL_BUTTON_LEFT){
						onClickObject(clickObjects[0],isSelected);
					}else if(event.button.button==SDL_BUTTON_RIGHT){
						//Remove the selection popup if there's one.
						if(selectionPopup!=NULL)
							delete selectionPopup;

						//Get the mouse location.
						int x,y;
						SDL_GetMouseState(&x,&y);
                        selectionPopup=new LevelEditorSelectionPopup(this,imageManager,renderer,clickObjects,x,y);
					}
				}
			}

			//If event is false then we clicked on void.
			if(!clickEvent){
				if(event.type==SDL_MOUSEBUTTONDOWN){
					if(event.button.button==SDL_BUTTON_LEFT){
						//Left mouse button on void.
						onClickVoid(mouse.x,mouse.y);
					}else if(event.button.button==SDL_BUTTON_RIGHT){
                        onRightClickVoid(imageManager,renderer,mouse.x,mouse.y);
					}
				}
			}
		}

		//Check for backspace when moving to remove a movingposition.
		if(moving && event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_BACKSPACE){
			if(movingBlocks[movingBlock].size()>0){
				commandManager->doCommand(new AddRemovePathCommand(this, movingBlock, MovingPosition(0, 0, 0), false));
			}
		}

		//Check for the tab key, level settings.
		if(inputMgr.isKeyDownEvent(INPUTMGR_TAB)){
			//Show the levelSettings.
            levelSettings(imageManager,renderer);
		}

		//Check if we should save the level (Ctrl+s).
		if(event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_s && (event.key.keysym.mod & KMOD_CTRL)){
			saveCurrentLevel(imageManager, renderer);
		}

		//Undo ctrl+z
		if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_z && (event.key.keysym.mod & KMOD_CTRL)){
			undo();
		}

		//Redo ctrl+y
		if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_y && (event.key.keysym.mod & KMOD_CTRL)){
			redo();
		}
	}
}

void LevelEditor::saveCurrentLevel(ImageManager& imageManager, SDL_Renderer& renderer) {
	saveLevel(levelFile);

	//Clear the dirty flag
	commandManager->resetChange();

	//And give feedback to the user.
	if (levelName.empty())
		msgBox(imageManager, renderer, tfm::format(_("Level \"%s\" saved"), fileNameFromPath(levelFile)), MsgBoxOKOnly, _("Saved"));
	else
		msgBox(imageManager, renderer, tfm::format(_("Level \"%s\" saved"), levelName), MsgBoxOKOnly, _("Saved"));
}

void LevelEditor::enterPlayMode(){
	//Check if we are already in play mode.
	if(playMode) return;

	//Stop linking or moving.
	if(linking){
		linking=false;
		linkingTrigger=NULL;
	}

	if(moving){
		moving=false;
		movingBlock=NULL;
	}

	//We need to reposition player and shadow here, since the related code is removed from object placement.
	for (auto obj : levelObjects) {
		//If the object is a player or shadow start then change the start position of the player or shadow.
		if (obj->type == TYPE_START_PLAYER){
			//Center the player horizontally.
			player.fx = obj->getBox().x + (obj->getBox().w - player.getBox().w) / 2;
			player.fy = obj->getBox().y;
		}
		if (obj->type == TYPE_START_SHADOW){
			//Center the shadow horizontally.
			shadow.fx = obj->getBox().x + (obj->getBox().w - shadow.getBox().w) / 2;
			shadow.fy = obj->getBox().y;
		}
	}

	//Change mode.
	playMode=true;
	GUIObjectRoot->visible=false;
	cameraSave.x=camera.x;
	cameraSave.y=camera.y;

	//Restart the game with scripting enabled.
	Game::reset(true, false);
}

void LevelEditor::undo(){
	commandManager->undo();
}

void LevelEditor::redo(){
	commandManager->redo();
}

void LevelEditor::levelSettings(ImageManager& imageManager,SDL_Renderer& renderer){
	//It isn't so open a popup asking for a name.
    GUIWindow* root=new GUIWindow(imageManager,renderer,(SCREEN_WIDTH-600)/2,(SCREEN_HEIGHT-450)/2,600,450,true,true,_("Level settings"));
	root->minWidth = root->width; root->minHeight = root->height;
	root->name="lvlSettingsWindow";
	root->eventCallback=this;
	GUIObject* obj;

	//Create the two textboxes with a label.
    obj=new GUILabel(imageManager,renderer,40,60,240,36,_("Name:"));
	root->addChild(obj);
    obj=new GUITextBox(imageManager,renderer,140,60,410,36,levelName.c_str());
	obj->gravityRight = GUIGravityRight;
	obj->name="name";
	root->addChild(obj);

	obj = new GUILabel(imageManager, renderer, 40, 110, 240, 36, (std::string("* ") + _("Theme:")).c_str());
	root->addChild(obj);
    obj=new GUITextBox(imageManager,renderer,140,110,410,36,levelTheme.c_str());
	obj->gravityRight = GUIGravityRight;
	obj->name="theme";
	root->addChild(obj);

	obj = new GUILabel(imageManager, renderer, 40, 150, 510, 36, _("Examples: %DATA%/themes/classic"));
	root->addChild(obj);
	obj = new GUILabel(imageManager, renderer, 40, 174, 510, 36, _("or %USER%/themes/Orange"));
	root->addChild(obj);

	obj = new GUILabel(imageManager, renderer, 40, 210, 240, 36, _("Music:"));
	root->addChild(obj);
	obj = new GUITextBox(imageManager, renderer, 140, 210, 410, 36, levelMusic.c_str());
	obj->gravityRight = GUIGravityRight;
	obj->name = "music";
	root->addChild(obj);

	//target time and recordings.
	{
        obj=new GUILabel(imageManager,renderer,40,260,240,36,_("Target time (s):"));
		root->addChild(obj);
        GUISpinBox* obj2=new GUISpinBox(imageManager,renderer,290,260,260,36);
		obj2->gravityRight = GUIGravityRight;
		obj2->name="time";

		ostringstream ss;
		ss << levelTime/40.0f;
		obj2->caption=ss.str();

		obj2->limitMin=-1.0f;
		obj2->format = "%g";
		obj2->change=0.1f;
		obj2->update();
		root->addChild(obj2);

        obj=new GUILabel(imageManager,renderer,40,310,240,36,_("Target recordings:"));
		root->addChild(obj);
        obj2=new GUISpinBox(imageManager,renderer,290,310,260,36);
		obj2->gravityRight = GUIGravityRight;

		ostringstream ss2;
		ss2 << levelRecordings;
		obj2->caption=ss2.str();

		obj2->limitMin=-1.0f;
		obj2->format="%1.0f";
		obj2->name="recordings";
		obj2->update();
		root->addChild(obj2);
	}

	obj = new GUILabel(imageManager, renderer, 40, 350, 510, 36, (std::string("* ") + _("Restart level editor is required")).c_str());
	root->addChild(obj);

	//Ok and cancel buttons.
    obj=new GUIButton(imageManager,renderer,root->width*0.3,450-44,-1,36,_("OK"),0,true,true,GUIGravityCenter);
	obj->gravityLeft = obj->gravityRight = GUIGravityCenter;
	obj->gravityTop = obj->gravityBottom = GUIGravityRight;
	obj->name="lvlSettingsOK";
	obj->eventCallback=root;
	root->addChild(obj);
    obj=new GUIButton(imageManager,renderer,root->width*0.7,450-44,-1,36,_("Cancel"),0,true,true,GUIGravityCenter);
	obj->gravityLeft = obj->gravityRight = GUIGravityCenter;
	obj->gravityTop = obj->gravityBottom = GUIGravityRight;
	obj->name="lvlSettingsCancel";
	obj->eventCallback=root;
	root->addChild(obj);

	GUIObjectRoot->addChild(root);
}

void LevelEditor::postLoad(){
	//We need to find the triggers.
	for(unsigned int o=0;o<levelObjects.size();o++){
		//Check for the highest id.
		unsigned int id=atoi(levelObjects[o]->getEditorProperty("id").c_str());
		if(id>=currentId)
			currentId=id+1;

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
				//Get the moving position.
				const vector<SDL_Rect> &movingPos = levelObjects[o]->movingPos;

				//Add the object to the movingBlocks vector.
				movingBlocks[levelObjects[o]].clear();

				for (int i = 0, m = movingPos.size(); i < m; i++) {
					MovingPosition position(movingPos[i].x, movingPos[i].y, movingPos[i].w);
					movingBlocks[levelObjects[o]].push_back(position);
				}

				break;
			}
			default:
				break;
		}
	}

	// Set the visibility of all layers to true
	layerVisibility.clear();
	for (auto it = sceneryLayers.begin(); it != sceneryLayers.end(); ++it) {
		layerVisibility[it->first] = true;
	}
	layerVisibility[std::string()] = true;

	// Also set the current layer to the Block layer
	selectedLayer.clear();

	// Get all available scenery blocks
	std::set<std::string> tmpset;
	objThemes.getSceneryBlockNames(tmpset);
	sceneryBlockNames.clear();
	sceneryBlockNames.insert(sceneryBlockNames.end(), tmpset.begin(), tmpset.end());
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

void LevelEditor::setCamera(const SDL_Rect* r,int count){
	//SetCamera only works in the Level editor and when mouse is inside window.
	if(stateID==STATE_LEVEL_EDITOR&&(SDL_GetMouseFocus() == sdlWindow)){
		//Get the mouse coordinates.
		int x,y;
		SDL_GetMouseState(&x,&y);
		SDL_Rect mouse={x,y,0,0};

		//Don't continue here if mouse is inside one of the boxes given as parameter.
		for(int i=0;i<count;i++){
			if(pointOnRect(mouse,r[i]))
				return;
		}

		//FIXME: two ad-hoc camera speed variables similar to cameraXvel and cameraYvel
		static int cameraXvelB = 0, cameraYvelB = 0;

		//Check if the mouse is near the left edge of the screen.
		//Else check if the mouse is near the right edge.
		if (x < 50) {
			//We're near the left edge so move the camera.
			if (cameraXvelB > -5) cameraXvelB = -5;
			if (pressedShift) cameraXvelB--;
		} else if (x > SCREEN_WIDTH - 50) {
			//We're near the right edge so move the camera.
			if (cameraXvelB < 5) cameraXvelB = 5;
			if (pressedShift) cameraXvelB++;
		} else {
			cameraXvelB = 0;
		}

		//Check if the tool box is visible and we need to calc screen size correctly.
		int y0=50;
		if (toolboxVisible && toolboxRect.w > 0) y0 += toolbarRect.h;

		//Check if the mouse is near the top edge of the screen.
		//Else check if the mouse is near the bottom edge.
		if (y < y0) {
			//We're near the top edge so move the camera.
			if (cameraYvelB > -5) cameraYvelB = -5;
			if (pressedShift) cameraYvelB--;
		} else if (y > SCREEN_HEIGHT - 50) {
			//We're near the bottom edge so move the camera.
			if (cameraYvelB < 5) cameraYvelB = 5;
			if (pressedShift) cameraYvelB++;
		} else {
			cameraYvelB = 0;
		}

		camera.x = clamp(camera.x + cameraXvelB, -1000 - SCREEN_WIDTH, LEVEL_WIDTH + 1000);
		camera.y = clamp(camera.y + cameraYvelB, -1000 - SCREEN_HEIGHT, LEVEL_HEIGHT + 1000);
	}
}

void LevelEditor::onClickObject(GameObject* obj,bool selected){
	switch(tool){
		case SELECT:
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

				AddLinkCommand* pCommand = new AddLinkCommand(this, linkingTrigger, obj);
				commandManager->doCommand(pCommand);

				//We return to prevent configuring stuff like conveyor belts, etc...
				linking=false;
				linkingTrigger=NULL;
				return;
			}

			//If we're moving add a movingposition.
			if(moving){
				//Get the current mouse location.
				int x, y;
				SDL_GetMouseState(&x, &y);
				x += camera.x;
				y += camera.y;

				addMovingPosition(x, y);
				return;
			}
		}
		case ADD:
		{
			//Check if object is already selected.
			if(!selected){
				//First check if shift is pressed or not.
				if(!pressedShift){
					//Clear the selection.
					deselectAll();
				}

				//Add the object to the selection.
				selection.push_back(obj);
			}
			break;
		}
		case REMOVE:
		{
			//Remove the object.
			commandManager->doCommand(new AddRemoveGameObjectCommand(this, obj, false));
			break;
		}
		default:
			break;
	}
}

void LevelEditor::addMovingPosition(int x,int y) {
	//Apply snap to grid.
	if (!pressedShift){
		snapToGrid(&x, &y);
	} else{
		x -= 25;
		y -= 25;
	}

	x -= movingBlock->getBox().x;
	y -= movingBlock->getBox().y;

	//Calculate the length.
	//First get the delta x and y.
	int dx, dy;
	if (movingBlocks[movingBlock].empty()){
		dx = x;
		dy = y;
	} else{
		dx = x - movingBlocks[movingBlock].back().x;
		dy = y - movingBlocks[movingBlock].back().y;
	}

	AddRemovePathCommand* pCommand = NULL;

	if (dx == 0 && dy == 0) {
		// pause mode
		if (pauseTime != 0) pCommand = new AddRemovePathCommand(this, movingBlock, MovingPosition(x, y, std::max(pauseTime, 0)), true);
		pauseTime = 0;
	} else {
		// add new point mode
		const double length = sqrt(double(dx*dx + dy*dy));
		pCommand = new AddRemovePathCommand(this, movingBlock, MovingPosition(x, y, (int)(length*(10 / (double)movingSpeed))), true);
	}
	if (pCommand) commandManager->doCommand(pCommand);
}

void LevelEditor::onRightClickObject(ImageManager& imageManager,SDL_Renderer& renderer,GameObject* obj,bool){
	//Create an actions popup for the game object.
	if(actionsPopup==NULL){
		//Get the mouse location.
		int x,y;
		SDL_GetMouseState(&x,&y);
        actionsPopup=new LevelEditorActionsPopup(imageManager,renderer,this,obj,x,y);
		return;
	}
}

void LevelEditor::onClickVoid(int x,int y){
	switch(tool){
		case ADD:
		{
			//We need to clear the selection.
			deselectAll();

			//Now place an object.
			//Apply snap to grid.
			if(!pressedShift){
				snapToGrid(&x,&y);
			}else{
				x-=25;
				y-=25;
			}
			if (currentType >= 0 && currentType < getEditorOrderMax()) {
				GameObject *obj;
				if (selectedLayer.empty()) {
					obj = new Block(this, x, y, 50, 50, editorTileOrder[currentType]);
				} else {
					obj = new Scenery(this, x, y, 50, 50,
						currentType < (int)sceneryBlockNames.size() ? sceneryBlockNames[currentType] : std::string());
				}
				commandManager->doCommand(new AddRemoveGameObjectCommand(this, obj, true));
			}
			break;
		}
		case SELECT:
		{
			//We need to clear the selection.
			deselectAll();

			//If we're linking we should stop, user abort.
			if(linking){
				linking=false;
				linkingTrigger=NULL;
				//And return.
				return;
			}

			//If we're moving we should add a point.
			if(moving){
				addMovingPosition(x, y);

				//And return.
				return;
			}
			break;
		}
		default:
			break;
	}
}

void LevelEditor::onRightClickVoid(ImageManager& imageManager,SDL_Renderer& renderer,int,int){
	//Create an actions popup for the game object.
	if(actionsPopup==NULL){
		//Get the mouse location.
		int x,y;
		SDL_GetMouseState(&x,&y);
        actionsPopup=new LevelEditorActionsPopup(imageManager,renderer,this,NULL,x,y);
		return;
	}
}

void LevelEditor::onDragStart(int x,int y){
	switch(tool){
		case SELECT:
		case ADD:
		{
			//We can drag the selection so check if the selection isn't empty.
			if(!selection.empty()){
				//The selection isn't empty so search the dragCenter.
				//Create a mouse rectangle.
				SDL_Rect mouse={x,y,0,0};

				// record the drag start position
				dragSrartPosition.x = x;
				dragSrartPosition.y = y;

				//Loop through the objects to check collision.
				for(unsigned int o=0; o<selection.size(); o++){
					SDL_Rect r = selection[o]->getBox();
					if(pointOnRect(mouse, r)){
						//We have collision so set the dragCenter.
						dragCenter=selection[o];

						// determine which part is dragged
						selectionDrag = 4;
						int midx = r.x + r.w / 2 - 2;
						int midy = r.y + r.h / 2 - 2;
						if (mouse.x >= r.x && mouse.x < r.x + 5) {
							if (mouse.y >= r.y && mouse.y < r.y + 5) {
								selectionDrag = 0;
							} else if (mouse.y >= midy && mouse.y < midy + 5) {
								selectionDrag = 3;
							} else if (mouse.y >= r.y + r.h - 5 && mouse.y < r.y + r.h) {
								selectionDrag = 6;
							}
						} else if (mouse.x >= midx && mouse.x < midx + 5) {
							if (mouse.y >= r.y && mouse.y < r.y + 5) {
								selectionDrag = 1;
							} else if (mouse.y >= r.y + r.h - 5 && mouse.y < r.y + r.h) {
								selectionDrag = 7;
							}
						} else if (mouse.x >= r.x + r.w - 5 && mouse.x < r.x + r.w) {
							if (mouse.y >= r.y && mouse.y < r.y + 5) {
								selectionDrag = 2;
							} else if (mouse.y >= midy && mouse.y < midy + 5) {
								selectionDrag = 5;
							} else if (mouse.y >= r.y + r.h - 5 && mouse.y < r.y + r.h) {
								selectionDrag = 8;
							}
						}

						break;
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

		currentCursor=CURSOR_REMOVE;

		std::vector<GameObject*> objects;

		//Loop through the objects to check collision.
		if (selectedLayer.empty()) {
			if (layerVisibility[selectedLayer]) {
				for (unsigned int o = 0; o<levelObjects.size(); o++){
					if (pointOnRect(mouse, levelObjects[o]->getBox()) == true){
						objects.push_back(levelObjects[o]);
					}
				}
			}
		} else {
			auto it = sceneryLayers.find(selectedLayer);
			if (it != sceneryLayers.end() && layerVisibility[selectedLayer]) {
				for (auto o : it->second->objects){
					if (pointOnRect(mouse, o->getBox()) == true){
						objects.push_back(o);
					}
				}
			}
		}

		// Do the actual object deletion.
		if (!objects.empty()) {
			commandManager->doCommand(new AddRemoveGameObjectCommand(this, objects, false));
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
		{
			//Check if the drag center isn't null.
			if(dragCenter==NULL)
				return;
			//The location of the dragCenter.
			SDL_Rect r=dragCenter->getBox();

			if (selectionDrag == 4) { // dragging
				//Apply snap to grid.
				determineNewPosition(x, y);

				commandManager->doCommand(new MoveGameObjectCommand(this, selection, x - r.x, y - r.y));
			} else if (selectionDrag >= 0) { // resizing
				determineNewSize(x, y, r);

				commandManager->doCommand(new MoveGameObjectCommand(this, dragCenter, r.x, r.y, r.w, r.h));
			}

			//Make sure the dragCenter is null and set selectionDrag false.
			dragCenter=NULL;
			selectionDrag=-1;
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

				std::vector<GameObject*> objects;

				//Loop through the objects to check collision.
				if (selectedLayer.empty()) {
					if (layerVisibility[selectedLayer]) {
						for (unsigned int o = 0; o<levelObjects.size(); o++){
							if (pointOnRect(mouse, levelObjects[o]->getBox()) == true){
								objects.push_back(levelObjects[o]);
							}
						}
					}
				} else {
					auto it = sceneryLayers.find(selectedLayer);
					if (it != sceneryLayers.end() && layerVisibility[selectedLayer]) {
						for (auto o : it->second->objects){
							if (pointOnRect(mouse, o->getBox()) == true){
								objects.push_back(o);
							}
						}
					}
				}

				// Do the actual object deletion.
				if (!objects.empty()) {
					commandManager->doCommand(new AddRemoveGameObjectCommand(this, objects, false));
				}
			}
			break;
		}
		default:
			break;
	}
}

void LevelEditor::selectionDirty() {
	if (selectionPopup != NULL) selectionPopup->dirty = true;
}

void LevelEditor::GUIEventCallback_OnEvent(ImageManager& imageManager, SDL_Renderer& renderer, std::string name,GUIObject* obj,int eventType){
	//Check if one of the windows is closed.
	if (eventType == GUIEventClick && name.size() >= 6 && name.substr(name.size() - 6) == "Window") {
		destroyWindow(obj);
		return;
	}
	//Resize code for each GUIWindow.
	if (name.size() >= 6 && name.substr(name.size() - 6) == "Window") {
		//Currently we don't need to process custom resize code since they are already processed in GUIWindow::resize().
		return;
	}

	//Check for GUI events.
	//Notification block configure events.
	if(name=="cfgNotificationBlockOK"){
		//Get the configuredObject.
		GameObject* configuredObject=objectWindows[obj];
		if(configuredObject){
			//Get the message textbox from the GUIWindow.
			GUITextArea* message=(GUITextArea*)obj->getChild("message");

			if(message){
				//Set the message of the notification block.
				commandManager->doCommand(new SetEditorPropertyCommand(this, imageManager, renderer,
					configuredObject, "message", message->getString(), _("Message")));
			}
		}
	}
	//Conveyor belt block configure events.
	else if(name=="cfgConveyorBlockOK"){
		//Get the configuredObject.
		GameObject* configuredObject=objectWindows[obj];
		if(configuredObject){
			//Get the speed textbox from the GUIWindow.
			GUISpinBox* speed=(GUISpinBox*)obj->getChild("speed");

			if(speed){
				//Set the speed of the conveyor belt.
				commandManager->doCommand(new SetEditorPropertyCommand(this, imageManager, renderer,
					configuredObject, "speed10", speed->caption, _("Speed")));
			}
		}
	}
	//LevelSetting events.
	else if(name=="lvlSettingsOK"){
		SetLevelPropertyCommand::LevelProperty prop;

		prop.levelTime = -1;
		prop.levelRecordings = -1;

		GUIObject* object=obj->getChild("name");
		if(object)
			prop.levelName=object->caption;
		object=obj->getChild("theme");
		if(object)
			prop.levelTheme=object->caption;
		object = obj->getChild("music");
		if (object)
			prop.levelMusic = object->caption;

		//target time and recordings.
		GUISpinBox* object2 = dynamic_cast<GUISpinBox*>(obj->getChild("time"));
		if(object2){
			float number = atof(object2->caption.c_str());
			prop.levelTime = int(floor(number*40.0f + 0.5f));
		}

		object2 = dynamic_cast<GUISpinBox*>(obj->getChild("recordings"));
		if(object2){
			prop.levelRecordings = atoi(object2->caption.c_str());
		}

		// Perform the level setting modification
		commandManager->doCommand(new SetLevelPropertyCommand(this, prop));
	}
	//Level scripting window events.
	else if(name=="cfgLevelScriptingEventType"){
		//Get the script textbox from the GUIWindow.
		GUISingleLineListBox* list=(GUISingleLineListBox*)obj->getChild("cfgLevelScriptingEventType");

		if(list){
			//Loop through the scripts.
			for(unsigned int i=0;i<list->item.size();i++){
				GUIObject* script=obj->getChild(list->item[i].first);
				if(script){
					script->visible=(script->name==list->item[list->value].first);
					script->enabled=(script->name==list->item[list->value].first);
				}
			}
		}
		return;
	}
	else if(name=="cfgLevelScriptingOK"){
		//Get the script textbox from the GUIWindow.
		GUISingleLineListBox* list=(GUISingleLineListBox*)obj->getChild("cfgLevelScriptingEventType");

		if(list){
			std::map<int, std::string> newScript;

			//Loop through the scripts.
			for(unsigned int i=0;i<list->item.size();i++){
				//Get the GUITextArea.
				GUITextArea* script=dynamic_cast<GUITextArea*>(obj->getChild(list->item[i].first));
				if(script){
					//Set the script for the target block.
					string str=script->getString();
					if(!str.empty())
						newScript[levelEventNameMap[script->name]]=str;
				}
			}

			// Check achievement
			if (!newScript.empty()) {
				statsMgr.newAchievement("helloworld");
			}

			// Do the actual changes
			commandManager->doCommand(new SetScriptCommand(this, NULL, newScript));
		}
	}
	//Scripting window events.
	else if (name == "cfgScriptingEventType"){
		//TODO: Save any unsaved scripts? (Or keep track of all scripts and save upon cfgScriptingOK?)
		//Get the configuredObject.
		Block* configuredObject=dynamic_cast<Block*>(objectWindows[obj]);
		if(configuredObject){
			//Get the script textbox from the GUIWindow.
			GUISingleLineListBox* list=dynamic_cast<GUISingleLineListBox*>(obj->getChild("cfgScriptingEventType"));

			if(list){
				//Loop through the scripts.
				for(unsigned int i=0;i<list->item.size();i++){
					GUIObject* script=obj->getChild(list->item[i].first);
					if(script){
						script->visible=script->enabled=(script->name==list->item[list->value].first);
					}
				}
			}
		}
		return;
	}
	else if(name=="cfgScriptingOK"){
		//Get the configuredObject.
		Block* block = dynamic_cast<Block*>(objectWindows[obj]);
		if (block){
			std::map<int, std::string> newScript;
			std::string newId;

			//Get the script textbox from the GUIWindow.
			GUISingleLineListBox* list=(GUISingleLineListBox*)obj->getChild("cfgScriptingEventType");
			GUIObject* id=obj->getChild("id");

			if (list){
				//Loop through the scripts.
				for (unsigned int i = 0; i < list->item.size(); i++){
					//Get the GUITextArea.
					GUITextArea* script = dynamic_cast<GUITextArea*>(obj->getChild(list->item[i].first));
					if (script){
						//Set the script for the target block.
						string str = script->getString();
						if (!str.empty())
							newScript[gameObjectEventNameMap[script->name]] = str;
					}
				}
			}
			newId = block->id;
			if (id){
				newId = id->caption;
			}

			// Check achievement
			if (!newScript.empty()) {
				statsMgr.newAchievement("helloworld");
			}

			// now do the actual changes
			commandManager->doCommand(new SetScriptCommand(this, block, newScript, newId));
		}
	}
	else if (name == "cfgAddLayerOK") {
		GUIObject* object = obj->getChild("layerName");
		if (!object) return;
		if (object->caption.empty()) {
			msgBox(imageManager, renderer, _("Please enter a layer name."), MsgBoxOKOnly, _("Error"));
			return;
		}
		if (sceneryLayers.find(object->caption) != sceneryLayers.end()) {
			msgBox(imageManager, renderer, tfm::format(_("The layer '%s' already exists."), object->caption), MsgBoxOKOnly, _("Error"));
			return;
		}

		// do the actual operation
		commandManager->doCommand(new AddRemoveLayerCommand(this, object->caption, true));
	}
	else if (name == "cfgLayerSettingsOK") {
		SetLayerPropertyCommand::LayerProperty prop;

		GUIObject* object = obj->getChild("layerName");
		if (!object) return;
		prop.name = object->caption;

		object = obj->getChild("speedX");
		if (!object) return;
		prop.speedX = atof(object->caption.c_str());

		object = obj->getChild("speedY");
		if (!object) return;
		prop.speedY = atof(object->caption.c_str());

		object = obj->getChild("cameraX");
		if (!object) return;
		prop.cameraX = atof(object->caption.c_str());

		object = obj->getChild("cameraY");
		if (!object) return;
		prop.cameraY = atof(object->caption.c_str());

		object = obj->getChild("oldName");
		if (!object) return;
		const std::string& oldName = object->caption;

		if (prop.name.empty()) {
			msgBox(imageManager, renderer, _("Please enter a layer name."), MsgBoxOKOnly, _("Error"));
			return;
		}
		if (prop.name != oldName && sceneryLayers.find(prop.name) != sceneryLayers.end()) {
			msgBox(imageManager, renderer, tfm::format(_("The layer '%s' already exists."), prop.name), MsgBoxOKOnly, _("Error"));
			return;
		}

		// do the actual operation
		commandManager->doCommand(new SetLayerPropertyCommand(this, oldName, prop));
	}
	else if (name == "cfgMoveToLayerOK") {
		GUIObject* object = obj->getChild("layerName");
		if (!object) return;
		const std::string& layerName = object->caption;

		object = obj->getChild("oldName");
		if (!object) return;
		const std::string& oldName = object->caption;

		if (layerName.empty()) {
			msgBox(imageManager, renderer, _("Please enter a layer name."), MsgBoxOKOnly, _("Error"));
			return;
		}
		if (oldName == layerName) {
			msgBox(imageManager, renderer, _("Source and destination layers are the same."), MsgBoxOKOnly, _("Error"));
			return;
		}

		// do the actual operation
		commandManager->doCommand(new MoveToLayerCommand(this, selection, oldName, layerName));
	}
	else if (name == "cfgCustomSceneryOK") {
		//Get the configuredObject.
		Scenery* configuredObject = dynamic_cast<Scenery*>(objectWindows[obj]);
		if (configuredObject){
			//Get the custom scenery from the GUIWindow.
			GUITextArea* txt = (GUITextArea*)obj->getChild("cfgCustomScenery");

			if (txt){
				//Set the custom scenery.
				commandManager->doCommand(new SetEditorPropertyCommand(this, imageManager, renderer,
					configuredObject, "customScenery", txt->getString(), _("Scenery")));
			}
		}
	}
	else if (name == "lstAppearance") {
		//Get the configuredObject.
		Block *block = dynamic_cast<Block*>(objectWindows[obj]);
		GUIListBox *list = dynamic_cast<GUIListBox*>(obj->getChild("lstAppearance"));
		GUIImage *image = dynamic_cast<GUIImage*>(obj->getChild("imgAppearance"));
		if (block && list && image) {
			//Reset the image first.
			image->setImage(NULL);
			image->setClipRect(SDL_Rect{ 0, 0, 0, 0 });

			//Get the appearance name.
			std::string appearanceName;
			if (list->value <= 0) {
				//Do nothing since the selected is the default appearance.
			} else if (list->value <= (int)sceneryBlockNames.size()) {
				//A custom appearance is selected.
				appearanceName = sceneryBlockNames[list->value - 1];
			} else {
				//The configured object has an invalid custom appearance name.
				appearanceName = block->customAppearanceName;
			}

			//Try to find the theme block.
			ThemeBlock *themeBlock = NULL;
			if (!appearanceName.empty()) {
				themeBlock = objThemes.getScenery(appearanceName);
			}
			if (themeBlock == NULL) {
				themeBlock = objThemes.getBlock(block->type);
			}
			if (themeBlock) {
				image->setImage(themeBlock->editorPicture.texture);
				const auto& offsetData = themeBlock->editorPicture.offset.offsetData;
				if (offsetData.size() > 0) {
					const auto& r = offsetData[0];
					image->setClipRect(SDL_Rect{ r.x, r.y, r.w, r.h });
				}
			}
		}
		return;
	}
	else if (name == "cfgAppearanceOK") {
		//Get the configuredObject.
		Block *block = dynamic_cast<Block*>(objectWindows[obj]);
		GUIListBox *list = dynamic_cast<GUIListBox*>(obj->getChild("lstAppearance"));
		if (block && list) {
			//Get the appearance name.
			std::string appearanceName;
			if (list->value <= 0) {
				//Do nothing since the selected is the default appearance.
			} else if (list->value <= (int)sceneryBlockNames.size()) {
				//A custom appearance is selected.
				appearanceName = sceneryBlockNames[list->value - 1];
			} else {
				//The configured object has an invalid custom appearance name.
				appearanceName = block->customAppearanceName;
			}

			//Update the block property if it's changed.
			if (appearanceName != block->customAppearanceName) {
				commandManager->doCommand(new SetEditorPropertyCommand(this, imageManager, renderer,
					block, "appearance", appearanceName, _("Appearance")));
			}
		}
	}

	//NOTE: We assume every event came from a window
	//and the event is either window closed event or OK/Cancel button click event, so we remove it.
	destroyWindow(obj);
}

void LevelEditor::destroyWindow(GUIObject* window){
	//Make sure the given pointer isn't null.
	if(!window)
		return;

	//Remove the window from the GUIObject root.
	if(GUIObjectRoot){
		vector<GUIObject*>::iterator it;
		it=find(GUIObjectRoot->childControls.begin(),GUIObjectRoot->childControls.end(),window);
		if(it!=GUIObjectRoot->childControls.end()){
			GUIObjectRoot->childControls.erase(it);
		}
	}

	//Also remove the window from the objectWindows map.
	map<GUIObject*,GameObject*>::iterator it;
	it=objectWindows.find(window);
	if(it!=objectWindows.end()){
		objectWindows.erase(it);
	}

	//And delete the GUIWindow.
	delete window;
}

////////////////LOGIC////////////////////
void LevelEditor::logic(ImageManager& imageManager, SDL_Renderer& renderer){
	if(playMode){
		//PlayMode so let the game do it's logic.
        Game::logic(imageManager,renderer);
	}else{
		//Update animation even under edit mode. (There are checks in Block::move() which don't do game logic in edit mode.)
		for (unsigned int i = 0; i<levelObjects.size(); i++){
			//Let the gameobject handle movement.
			levelObjects[i]->move();
		}

		//Also update the scenery.
		for (auto it = sceneryLayers.begin(); it != sceneryLayers.end(); ++it){
			it->second->updateAnimation();
		}

		//In case of a selection or actions popup prevent the camera from moving.
		if(selectionPopup || actionsPopup)
			return;

		//Move the camera.
		if (cameraXvel != 0 || cameraYvel != 0) {
			if (pressedShift) {
				if (cameraXvel > 0) cameraXvel++;
				else if (cameraXvel < 0) cameraXvel--;
				if (cameraYvel > 0) cameraYvel++;
				else if (cameraYvel < 0) cameraYvel--;
			}
			camera.x = clamp(camera.x + cameraXvel, -1000 - SCREEN_WIDTH, LEVEL_WIDTH + 1000);
			camera.y = clamp(camera.y + cameraYvel, -1000 - SCREEN_HEIGHT, LEVEL_HEIGHT + 1000);
			//Call the onCameraMove event.
			onCameraMove(cameraXvel, cameraYvel);
		}

		//Move the camera with the mouse.
		//Get the mouse location.
		int x,y;
		SDL_GetMouseState(&x,&y);
		SDL_Rect mouse={x,y,0,0};
		{
			//Check if the mouse isn't above a GUIObject (window).
			bool inside=false;
			for(unsigned int i=0;i<GUIObjectRoot->childControls.size();i++){
				SDL_Rect box={0,0,0,0};
				box.x=GUIObjectRoot->childControls[i]->left;
				box.y=GUIObjectRoot->childControls[i]->top;
				box.w=GUIObjectRoot->childControls[i]->width;
				box.h=GUIObjectRoot->childControls[i]->height;
				if(pointOnRect(mouse,box))
					inside=true;
			}

			if(!inside){
				SDL_Rect r[3]={toolbarRect,toolboxRect};
				int m=2;
				//TODO: Also call onCameraMove when moving using the mouse.
				setCamera(r,m);
			}
		}

		//It isn't playMode so the mouse should be checked.
		tooltip=-1;

		//We loop through the number of tools + the number of buttons.
		for (int t = 0; t <= (int)ToolTips::BackToMenu; t++){
			SDL_Rect toolRect={(SCREEN_WIDTH-460)/2+(t*40)+((t+1)*10),SCREEN_HEIGHT-45,40,40};

			//Check for collision.
			if(pointOnRect(mouse,toolRect)==true){
				//Set the tooltip tool.
				tooltip=t;
			}
		}
	}
}

/////////////////RENDER//////////////////////
void LevelEditor::render(ImageManager& imageManager,SDL_Renderer& renderer){
	//Let the game render the game when it is the play mode.
	if (playMode) {
		Game::render(imageManager, renderer);
	} else {
		// The following code are partially copied from Game::render()

		//First of all render the background.
		{
			//Get a pointer to the background.
			ThemeBackground* bg = background;

			//Check if the background is null, but there are themes.
			if (bg == NULL && objThemes.themeCount()>0){
				//Get the background from the first theme in the stack.
				bg = objThemes[0]->getBackground(false);
			}

			//Check if the background isn't null.
			if (bg){
				//It isn't so draw it.
				bg->draw(renderer);

				//And if it's the loaded background then also update the animation.
				//FIXME: Updating the animation in the render method?
				if (bg == background)
					bg->updateAnimation();
			} else{
				//There's no background so fill the screen with white.
				SDL_SetRenderDrawColor(&renderer, 255, 255, 255, 255);
				SDL_RenderClear(&renderer);
			}
		}

		//Now draw the background layers.
		auto it = sceneryLayers.begin();
		for (; it != sceneryLayers.end(); ++it){
			if (it->first >= "f") break; // now we meet a foreground layer
			if (layerVisibility[it->first]) {
				it->second->show(renderer);
			}
		}

		//Now we draw the levelObjects.
		if (layerVisibility[std::string()]) {
			for (unsigned int o = 0; o < levelObjects.size(); o++){
				levelObjects[o]->show(renderer);
			}
		}

		//We don't draw the player and the shadow at all.

		//Now draw the foreground layers.
		for (; it != sceneryLayers.end(); ++it){
			if (layerVisibility[it->first]) {
				it->second->show(renderer);
			}
		}
	}

	//Only render extra stuff like the toolbar, selection, etc.. when not in playMode.
	if(!playMode){
		//Get the current mouse location.
		int x, y;
		SDL_GetMouseState(&x, &y);
		//Create the rectangle.
		SDL_Rect mouse = { x + camera.x, y + camera.y, 0, 0 };

		//Render the selectionmarks.
		//TODO: Check if block is in sight.
		for(unsigned int o=0; o<selection.size(); o++){
			//Get the location to draw.
			SDL_Rect r=selection[o]->getBox();

			// Change the mouse cursor if necessary
			if (selectionDrag < 0 && tool != REMOVE) {
				int midx = r.x + r.w / 2 - 2;
				int midy = r.y + r.h / 2 - 2;
				if (mouse.x >= r.x && mouse.x < r.x + 5) {
					if (mouse.y >= r.y && mouse.y < r.y + 5) {
						currentCursor = CURSOR_SIZE_FDIAG;
					} else if (mouse.y >= midy && mouse.y < midy + 5) {
						currentCursor = CURSOR_SIZE_HOR;
					} else if (mouse.y >= r.y + r.h - 5 && mouse.y < r.y + r.h) {
						currentCursor = CURSOR_SIZE_BDIAG;
					}
				} else if (mouse.x >= midx && mouse.x < midx + 5) {
					if (mouse.y >= r.y && mouse.y < r.y + 5) {
						currentCursor = CURSOR_SIZE_VER;
					} else if (mouse.y >= r.y + r.h - 5 && mouse.y < r.y + r.h) {
						currentCursor = CURSOR_SIZE_VER;
					}
				} else if (mouse.x >= r.x + r.w - 5 && mouse.x < r.x + r.w) {
					if (mouse.y >= r.y && mouse.y < r.y + 5) {
						currentCursor = CURSOR_SIZE_BDIAG;
					} else if (mouse.y >= midy && mouse.y < midy + 5) {
						currentCursor = CURSOR_SIZE_HOR;
					} else if (mouse.y >= r.y + r.h - 5 && mouse.y < r.y + r.h) {
						currentCursor = CURSOR_SIZE_FDIAG;
					}
				}
			}

			bool mouseIn = pointOnRect(mouse, r);

			r.x-=camera.x;
			r.y-=camera.y;

            drawGUIBox(r.x,r.y,r.w,r.h,renderer,0xFFFFFF33);

			//Draw the selectionMarks.
            applyTexture(r.x,r.y,selectionMark,renderer);
            applyTexture(r.x+r.w-5,r.y,selectionMark,renderer);
            applyTexture(r.x,r.y+r.h-5,selectionMark,renderer);
            applyTexture(r.x+r.w-5,r.y+r.h-5,selectionMark,renderer);

			// draw additional selection marks
			if (mouseIn && selectionDrag < 0 && tool != REMOVE) {
				applyTexture(r.x + r.w / 2 - 2, r.y, selectionMark, renderer);
				applyTexture(r.x + r.w / 2 - 2, r.y + r.h - 5, selectionMark, renderer);
				applyTexture(r.x, r.y + r.h / 2 - 2, selectionMark, renderer);
				applyTexture(r.x + r.w - 5, r.y + r.h / 2 - 2, selectionMark, renderer);
			}
		}

        //Set the color for the borders.
		{
			SDL_Color c = objThemes.getTextColor(false);
			SDL_SetRenderDrawColor(&renderer, c.r, c.g, c.b, 115);
		}

        int leftWidth=0;
        int rightWidth=0;

		//Draw the dark areas marking the outside of the level.
        SDL_Rect r{0,0,0,0};
		if(camera.x<0){
			//Draw left side.
			r.x=0;
			r.y=0;
			r.w=0-camera.x;
            leftWidth=r.w;
			r.h=SCREEN_HEIGHT;
            SDL_RenderFillRect(&renderer, &r);
		}
        if(camera.y<0){
            //Draw the top.
            r.x=leftWidth;
            r.y=0;
            r.w=SCREEN_WIDTH;
            r.h=0-camera.y;
            SDL_RenderFillRect(&renderer, &r);
        } else {
            r.h=0;
        }
        if(camera.x>LEVEL_WIDTH-SCREEN_WIDTH){
            //Draw right side.
            r.x=LEVEL_WIDTH-camera.x;
            r.y=std::max(r.y+r.h,0);
            r.w=SCREEN_WIDTH-(LEVEL_WIDTH-camera.x);
            rightWidth=r.w;
            r.h=SCREEN_HEIGHT;
            SDL_RenderFillRect(&renderer, &r);
        }
		if(camera.y>LEVEL_HEIGHT-SCREEN_HEIGHT){
			//Draw the bottom.
            r.x=leftWidth;
			r.y=LEVEL_HEIGHT-camera.y;
            r.w=SCREEN_WIDTH-rightWidth-leftWidth;
			r.h=SCREEN_HEIGHT-(LEVEL_HEIGHT-camera.y);
            SDL_RenderFillRect(&renderer, &r);
        }

        //Check if we should draw on stuff.
        showConfigure(renderer);
		if (selectionDrag >= 0 && tool != REMOVE) {
			showSelectionDrag(renderer);
		}

		//Find a block where the mouse is hovering on.
		bool isMouseOnSomething = false;
		if (selectedLayer.empty()){
			if (layerVisibility[selectedLayer]) {
				// Current layer is Blocks layer
				for (unsigned int o = 0; o<levelObjects.size(); o++){
					SDL_Rect rect = levelObjects[o]->getBox();
					if (pointOnRect(mouse, rect) == true){
						isMouseOnSomething = true;
						if (tool == REMOVE){
							drawGUIBox(rect.x - camera.x, rect.y - camera.y, rect.w, rect.h, renderer, 0xFF000055);
							currentCursor = CURSOR_REMOVE;
						} else{
							drawGUIBox(rect.x - camera.x, rect.y - camera.y, rect.w, rect.h, renderer, 0xFFFFFF33);
						}
					}
				}
			}
		} else {
			auto it = sceneryLayers.find(selectedLayer);
			if (it != sceneryLayers.end() && layerVisibility[selectedLayer]) {
				// Current layer is scenery layer
				for (auto o : it->second->objects){
					SDL_Rect rect = o->getBox();
					if (pointOnRect(mouse, rect) == true){
						isMouseOnSomething = true;
						if (tool == REMOVE){
							drawGUIBox(rect.x - camera.x, rect.y - camera.y, rect.w, rect.h, renderer, 0xFF000055);
							currentCursor = CURSOR_REMOVE;
						} else{
							drawGUIBox(rect.x - camera.x, rect.y - camera.y, rect.w, rect.h, renderer, 0xFFFFFF33);
						}
					}
				}
			}
		}

		// show current object only when mouse is not hover on any blocks
		if (!isMouseOnSomething && tool == ADD && selectionDrag < 0) {
			showCurrentObject(renderer);
		}

		//Draw the level borders.
        drawRect(-camera.x,-camera.y,LEVEL_WIDTH,LEVEL_HEIGHT,renderer);

		//Render the hud layer.
        renderHUD(renderer);

		//Render selection popup (if any).
		if(selectionPopup!=NULL){
			if(linking || moving){
				//If we switch to linking mode then delete it
				//FIXME: Logic in the render method.
				delete selectionPopup;
				selectionPopup=NULL;
			}else{
                selectionPopup->render(imageManager,renderer);
			}
		}

		//Render actions popup (if any).
		if(actionsPopup!=NULL){
            actionsPopup->render(renderer);
		}
	}
}

void LevelEditor::renderHUD(SDL_Renderer& renderer){
	//If moving show the moving speed in the top right corner.
	if(moving){
		//Calculate width of text "Movespeed: 125" to keep the same position with every value
		if (movingSpeedWidth == -1){
			int w;
			TTF_SizeUTF8(fontText, tfm::format(_("Speed: %d = %0.2f block/s"), 125, 10.0f).c_str(), &w, NULL);
			movingSpeedWidth = w + 4;
		}

		SDL_Texture *tex = NULL;

		//Check which text should we use.
		if (pauseMode) {
			//Update the text if necessary.
			if (pauseTimeTexture.needsUpdate(pauseTime)) {
				if (pauseTime < 0) {
					pauseTimeTexture.update(pauseTime,
						textureFromText(renderer, *fontText,
						_("Stop at this point"),
						objThemes.getTextColor(true)));
				} else {
					pauseTimeTexture.update(pauseTime,
						textureFromText(renderer, *fontText,
						tfm::format(_("Pause: %d = %0.3fs"), pauseTime, float(pauseTime)*0.025f).c_str(),
						objThemes.getTextColor(true)));
				}
			}
			tex = pauseTimeTexture.get();
		} else {
			//Update the text if necessary.
			if (movementSpeedTexture.needsUpdate(movingSpeed)) {
				movementSpeedTexture.update(movingSpeed,
					textureFromText(renderer, *fontText,
					tfm::format(_("Speed: %d = %0.2f block/s"), movingSpeed, float(movingSpeed)*0.08f).c_str(),
					objThemes.getTextColor(true)));
			}
			tex = movementSpeedTexture.get();
		}

		//Draw the text in the box.
        drawGUIBox(SCREEN_WIDTH-movingSpeedWidth-2,-2,movingSpeedWidth+8,
                   textureHeight(*tex)+6,renderer,0xFFFFFFFF);
        applyTexture(SCREEN_WIDTH-movingSpeedWidth,2,*tex,renderer,NULL);
	}

	//On top of all render the toolbar.
    drawGUIBox(toolbarRect.x,toolbarRect.y,9*50+10,52,renderer,0xEDEDEDFF);
	//Draw the first four options.
    SDL_Rect srcRect={0,0,200,50};
    SDL_Rect dstRect={toolbarRect.x+5, toolbarRect.y, srcRect.w, srcRect.h};
    SDL_RenderCopy(&renderer, toolbar.get(), &srcRect, &dstRect);

	//Draw the undo/redo button.
	SDL_SetTextureAlphaMod(toolbar.get(), commandManager->canUndo() ? 255 : 128);
	srcRect.x = 200;
	srcRect.w = 50;
	dstRect.x = toolbarRect.x + 205;
	dstRect.w = srcRect.w;
	SDL_RenderCopy(&renderer, toolbar.get(), &srcRect, &dstRect);
	SDL_SetTextureAlphaMod(toolbar.get(), commandManager->canRedo() ? 255 : 128);
	srcRect.x = 250;
	srcRect.w = 50;
	dstRect.x = toolbarRect.x + 255;
	dstRect.w = srcRect.w;
	SDL_RenderCopy(&renderer, toolbar.get(), &srcRect, &dstRect);
	SDL_SetTextureAlphaMod(toolbar.get(), 255);

	//And the last three.
    srcRect.x=300;
    srcRect.w=150;
    dstRect.x=toolbarRect.x+305;
    dstRect.w=srcRect.w;
    SDL_RenderCopy(&renderer, toolbar.get(), &srcRect, &dstRect);

	//Now render a tooltip.
    if(tooltip>=0 && static_cast<std::size_t>(tooltip)<tooltipTextures.size()) {
        SDL_Texture *tex = tooltipTextures.at(tooltip).get();

		if (tooltip == (int)ToolTips::UndoNoTooltip) {
			std::string s = commandManager->describeUndo();
			if (undoTooltipTexture.needsUpdate(s)) {
				undoTooltipTexture.update(s, textureFromText(renderer, *fontText, s.c_str(), objThemes.getTextColor(true)));
			}
			tex = undoTooltipTexture.get();
		} else if (tooltip == (int)ToolTips::RedoNoTooltip) {
			std::string s = commandManager->describeRedo();
			if (redoTooltipTexture.needsUpdate(s)) {
				redoTooltipTexture.update(s, textureFromText(renderer, *fontText, s.c_str(), objThemes.getTextColor(true)));
			}
			tex = redoTooltipTexture.get();
		}

		if(tex) {
            const SDL_Rect texSize = rectFromTexture(*tex);
            SDL_Rect r={(SCREEN_WIDTH-440)/2+(tooltip*40)+(tooltip*10),SCREEN_HEIGHT-45,40,40};
            r.y=SCREEN_HEIGHT-50-texSize.h;
            if(r.x+texSize.w>SCREEN_WIDTH-50)
                r.x=SCREEN_WIDTH-50-texSize.w;

            //Draw borders around text
            Uint32 color=0xFFFFFF00|230;
            drawGUIBox(r.x-2,r.y-2,texSize.w+4,texSize.h+4,renderer,color);
            applyTexture(r.x, r.y, *tex, renderer);
        }
	}

	// for toolbox button animation (0-31)
	static int tick = 8;

	const int mmm = getEditorOrderMax();
	if (currentType >= mmm)currentType = mmm - 1;
	if (currentType < 0) currentType = 0;

	//Render the tool box.
	if(!playMode && !moving && tool==ADD && selectionPopup==NULL && actionsPopup==NULL && objectWindows.empty()){
		// get mouse position
		int x, y;
		SDL_GetMouseState(&x, &y);

		if (toolboxVisible){
			toolboxRect.x=0;
			toolboxRect.y=0;
			toolboxRect.w=SCREEN_WIDTH;
			toolboxRect.h=64;

            drawGUIBox(-2,-2,SCREEN_WIDTH+4,66,renderer,0xFFFFFF00|230);

			bool isMouseOnSomething = false;

			//Draw the hide icon.
            SDL_Rect r={SCREEN_WIDTH-20,2,16,16};
            SDL_Rect r2={80,0,r.w,r.h};
			if (x >= SCREEN_WIDTH - 24 && x < SCREEN_WIDTH && y < 20) {
				isMouseOnSomething = true;
				tick = (tick + 1) & 31;
				r.y -= (tick < 16) ? (tick / 4 - 2) : (6 - tick / 4);
			}
			SDL_RenderCopy(&renderer, bmGUI.get(), &r2, &r);

			//Calculate the maximal number of blocks can be displayed.
			const int m=(SCREEN_WIDTH-48)/64;
			if(toolboxIndex>=mmm-m){
				toolboxIndex = mmm - m;
			}else{
				//Draw an icon.
				r.x=SCREEN_WIDTH-20;
				r.y=24;
				r2.x=96;
				r2.y=16;
				if (x >= SCREEN_WIDTH - 24 && x < SCREEN_WIDTH && y >= 20 && y < 44) {
					isMouseOnSomething = true;
					tick = (tick + 1) & 31;
					r.x += (tick < 16) ? (tick / 4 - 2) : (6 - tick / 4);
				}
                SDL_RenderCopy(&renderer, bmGUI.get(),&r2,&r);
			}
			if(toolboxIndex<=0){
				toolboxIndex=0;
			}else{
				//Draw an icon.
				r.x=4;
				r.y=24;
				r2.x=80;
				r2.y=16;
				if (x >= 0 && x < 24 && y >= 20 && y < 44) {
					isMouseOnSomething = true;
					tick = (tick + 1) & 31;
					r.x -= (tick < 16) ? (tick / 4 - 2) : (6 - tick / 4);
				}
				SDL_RenderCopy(&renderer, bmGUI.get(), &r2, &r);
			}

			// reset animation timer if there is no animation
			if (!isMouseOnSomething) {
				tick = 8;
			}

			//Draw available blocks.
			for(int i=0;i<m;i++){
				if (i + toolboxIndex >= mmm) break;

				//Draw a rectangle around the current tool.
				if(i+toolboxIndex==currentType){
                    drawGUIBox(i*64+24,3,64,58,renderer,0xDDDDDDFF);
				}

				if (selectedLayer.empty()) {
					// show normal blocks
					ThemeBlock* obj = objThemes.getBlock(editorTileOrder[i + toolboxIndex]);
					if (obj){
						obj->editorPicture.draw(renderer, i * 64 + 24 + 7, 7);
					}
				} else {
					// show scenery blocks
					if (i + toolboxIndex < (int)sceneryBlockNames.size()) {
						ThemeBlock* obj = objThemes.getScenery(sceneryBlockNames[i + toolboxIndex]);
						if (obj){
							obj->editorPicture.draw(renderer, i * 64 + 24 + 7, 7);
						}
					} else {
						// it's custom scenery block
						// just draw a stupid icon
						const SDL_Rect r = { 48, 16, 16, 16 };
						const SDL_Rect dstRect = { i * 64 + 24 + 7, 7, 16, 16 };
						SDL_RenderCopy(&renderer, bmGUI.get(), &r, &dstRect);
					}
				}
			}

			//Draw a tool tip.
			if(y<64 && x>=24 && x<24+m*64){
				int i=(x-24)/64;
				if (i + toolboxIndex < getEditorOrderMax()){
					TexturePtr& tip = (!selectedLayer.empty())
						? getCachedTextTexture(renderer, (i + toolboxIndex < (int)sceneryBlockNames.size())
						? describeSceneryName(sceneryBlockNames[i + toolboxIndex]).c_str() : _("Custom scenery block"))
						: typeTextTextures.at(editorTileOrder[i + toolboxIndex]);

					const SDL_Rect tipSize = rectFromTexture(*tip);

					SDL_Rect r = { 24 + i * 64, 64, 40, 40 };
					if (r.x + tipSize.w>SCREEN_WIDTH - 50)
						r.x = SCREEN_WIDTH - 50 - tipSize.w;

					//Draw borders around text
					Uint32 color = 0xFFFFFF00 | 230;
					drawGUIBox(r.x - 2, r.y - 2, tipSize.w + 4, tipSize.h + 4, renderer, color);

					//Draw tooltip's text
					applyTexture(r.x, r.y, tip, renderer);
				}
			}
		}else{
            const SDL_Rect tbtSize = rectFromTexture(*toolboxText);

            toolboxRect.x=SCREEN_WIDTH-tbtSize.w-28;
			toolboxRect.y=0;
            toolboxRect.w=tbtSize.w+28;
            toolboxRect.h=tbtSize.h+4;

            SDL_Rect r={SCREEN_WIDTH-tbtSize.w-24,2,16,16};
            drawGUIBox(r.x-4,-2,tbtSize.w+32,tbtSize.h+6,renderer,0xFFFFFFFF);

            //Draw "Toolbox" text.
            applyTexture(r.x, r.y, toolboxText, renderer);

            const SDL_Rect r2={96,0,16,16};
			r.x=SCREEN_WIDTH-20;
            r.w=r2.w;
            r.h=r2.h;

			// check if mouse is hovering on
			if (x >= toolboxRect.x && x < toolboxRect.x + toolboxRect.w && y >= toolboxRect.y && y < toolboxRect.y + toolboxRect.h) {
				tick = (tick + 1) & 31;
				r.y += (tick < 16) ? (tick / 4 - 2) : (6 - tick / 4);
			} else {
				tick = 8;
			}

			//Draw arrow.
            SDL_RenderCopy(&renderer, bmGUI.get(),&r2,&r);
		}
	}else{
		toolboxRect.x=-1;
		toolboxRect.y=-1;
		toolboxRect.w=0;
		toolboxRect.h=0;
	}

	//Draw a rectangle around the current tool.
	Uint32 color=0xFFFFFF00;
    drawGUIBox((SCREEN_WIDTH-440)/2+(tool*40)+(tool*10),SCREEN_HEIGHT-46,42,42,renderer,color);
}

void LevelEditor::showCurrentObject(SDL_Renderer& renderer){
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
	if(currentType>=0 && currentType<getEditorOrderMax()){
		if (selectedLayer.empty()) {
			// show normal blocks
			ThemeBlock* obj = objThemes.getBlock(editorTileOrder[currentType]);
			if (obj){
				obj->editorPicture.draw(renderer, x - camera.x, y - camera.y);
			}
		} else {
			// show scenery blocks
			if (currentType < (int)sceneryBlockNames.size()) {
				ThemeBlock* obj = objThemes.getScenery(sceneryBlockNames[currentType]);
				if (obj){
					obj->editorPicture.draw(renderer, x - camera.x, y - camera.y);
				}
			} else {
				// it's custom scenery block
				// just draw a stupid icon
				const SDL_Rect r = { 48, 16, 16, 16 };
				const SDL_Rect dstRect = { x - camera.x, y - camera.y, 16, 16 };
				SDL_RenderCopy(&renderer, bmGUI.get(), &r, &dstRect);
			}
		}
	}
}

void LevelEditor::determineNewPosition(int& x, int& y) {
	if (dragCenter) {
		SDL_Rect r = dragCenter->getBox();
		x -= dragSrartPosition.x - r.x;
		y -= dragSrartPosition.y - r.y;
	} else {
		x -= 25;
		y -= 25;
	}

	// Check if we should snap the block to grid or not.
	if (!pressedShift) {
		x = int(floor(x/50.0f + 0.5f)) * 50;
		y = int(floor(y/50.0f + 0.5f)) * 50;
	}
}

void LevelEditor::determineNewSize(int x, int y, SDL_Rect& r) {
	switch (selectionDrag % 3) {
	case 0:
		if (x > r.x + r.w - 15) x = r.x + r.w - 15;
		if (!pressedShift) {
			x = int(floor(x/50.0f + 0.5f)) * 50;
			while (x > r.x + r.w - 15) x -= 50;
		}
		r.w += r.x - x;
		r.x = x;
		break;
	case 2:
		if (x < r.x + 15) x = r.x + 15;
		if (!pressedShift) {
			x = int(floor(x/50.0f + 0.5f)) * 50;
			while (x < r.x + 15) x += 50;
		}
		r.w = x - r.x;
		break;
	}
	switch (selectionDrag / 3) {
	case 0:
		if (y > r.y + r.h - 15) y = r.y + r.h - 15;
		if (!pressedShift) {
			y = int(floor(y/50.0f + 0.5f)) * 50;
			while (y > r.y + r.h - 15) y -= 50;
		}
		r.h += r.y - y;
		r.y = y;
		break;
	case 2:
		if (y < r.y + 15) y = r.y + 15;
		if (!pressedShift) {
			y = int(floor(y/50.0f + 0.5f)) * 50;
			while (y < r.y + 15) y += 50;
		}
		r.h = y - r.y;
		break;
	}
}

void LevelEditor::showSelectionDrag(SDL_Renderer& renderer){
	//Check if the drag center isn't null.
	if (dragCenter == NULL) return;

	//Get the current mouse location.
	int x,y;
	SDL_GetMouseState(&x,&y);

	//Create the rectangle.
	x+=camera.x;
	y+=camera.y;

	//The location of the dragCenter.
	SDL_Rect r = dragCenter->getBox();

	if (selectionDrag == 4) { // dragging
		// Check if we should snap the block to grid or not.
		determineNewPosition(x, y);

		//Loop through the selection.
		//TODO: Check if block is in sight.
		for (unsigned int o = 0; o < selection.size(); o++){
			// FIXME: ad-hoc code which moves blocks temporarily, draw, and moves them back
			const SDL_Rect r1 = selection[o]->getBox();

			selection[o]->setBaseLocation((r1.x - r.x) + x, (r1.y - r.y) + y);
			selection[o]->show(renderer);
			selection[o]->setBaseLocation(r1.x, r1.y);
		}
	} else if (selectionDrag >= 0) { // resizing
		// Check if we should snap the block to grid or not.
		determineNewSize(x, y, r);

		drawGUIBox(r.x - camera.x, r.y - camera.y, r.w, r.h, renderer, 0xFFFFFF33);
	}
}

void LevelEditor::showConfigure(SDL_Renderer& renderer){
	//arrow animation value. go through 0-65535 and loops.
	static unsigned short arrowAnimation=0;
	arrowAnimation++;

	// skip if the Blocks layer is invisinble
	if (!layerVisibility[std::string()]) return;

	//Use theme color for arrows.
	Uint32 color;
	{
		SDL_Color c = objThemes.getTextColor(false);
		color = (Uint32(c.r) << 24) | (Uint32(c.g) << 16) | (Uint32(c.b) << 8) | 0xff;
	}

	//Draw the trigger lines.
	{
		map<Block*,vector<GameObject*> >::iterator it;
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
                    drawLineWithArrow(r.x-camera.x+25,r.y-camera.y+25,r1.x-camera.x+25,r1.y-camera.y+25,renderer,color,32,arrowAnimation%32);

					//Also draw two selection marks.
                    applyTexture(r.x-camera.x+25-2,r.y-camera.y+25-2,selectionMark,renderer);
                    applyTexture(r1.x-camera.x+25-2,r1.y-camera.y+25-2,selectionMark,renderer);
				}
			}
		}

		//Draw a line to the mouse from the linkingTrigger when linking.
		if(linking){
			//Get the current mouse location.
			int x,y;
			SDL_GetMouseState(&x,&y);

			//Draw the line from the center of the trigger to mouse.
            drawLineWithArrow(linkingTrigger->getBox().x-camera.x+25,linkingTrigger->getBox().y-camera.y+25,x,y,renderer,color,32,arrowAnimation%32);
		}
	}

	//This saves the stacked pause marks on each position.
	std::map<std::pair<int, int>, int> stackedMarks;

	//Draw the moving positions.
	map<Block*,vector<MovingPosition> >::iterator it;
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
                        drawLineWithArrow(r.x,r.y,x,y,renderer,color,32,offset);
					}else{
						//time==0 ???? so don't draw arrow at all
                        drawLine(r.x,r.y,x,y,renderer);
					}
				} else {
					// distance==0 which means pause mode
					// FIXME: it's ugly
					SDL_Rect r1 = { 0, 0, 16, 16 }, r2 = { x - 25, y - 25 + 15 * (stackedMarks[std::pair<int, int>(x, y)]++), 16, 16 };
					if (it->second[o].time) {
						char s[64];
						sprintf(s, "%gs", float(it->second[o].time) * 0.025f);

						r1.x = 0; r1.y = 80;
						SDL_RenderCopy(&renderer, bmGUI.get(), &r1, &r2);
						r2.x += 16;

						for (int i = 0; s[i]; i++) {
							if (s[i] >= '0' && s[i] <= '9') {
								r1.x = 16 + (s[i] - '0') * 8;
								r1.w = 8;
							} else if (s[i] == '.') {
								r1.x = 96;
								r1.w = 8;
							} else if (s[i] == 's') {
								r1.x = 104;
								r1.w = 8;
							} else {
								// show some garbage
								r1.x = 0;
								r1.w = 1;
							}
							r2.w = 8;
							SDL_RenderCopy(&renderer, bmGUI.get(), &r1, &r2);
							r2.x += 8;
						}
					} else {
						r1.x = 32; r1.y = 64;
						SDL_RenderCopy(&renderer, bmGUI.get(), &r1, &r2);
					}
				}

				//And draw a marker at the end.
                applyTexture(x-13,y-13,movingMark,renderer);

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

		//Check if the current point is the same as the previous point
		if (posX == x && posY == y) {
			pauseMode = true;
		} else {
			pauseMode = false;

			//Calculate offset to contain the moving speed.
			int offset = int(double(arrowAnimation)*movingSpeed / 10.0) % 32;

			//Draw the line.
			drawLineWithArrow(posX + 25, posY + 25, x + 25, y + 25, renderer, color, 32, offset);
		}

		//Draw a marker.
        applyTexture(x+12,y+12,movingMark,renderer);
	}

}

void LevelEditor::resize(ImageManager &imageManager, SDL_Renderer &renderer){
	//Call the resize method of the Game.
    Game::resize(imageManager, renderer);

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
	TYPE_COLLECTABLE,
	TYPE_PUSHABLE
};
