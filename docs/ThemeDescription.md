Me and My Shadow Theme File Description
=======================================

(draft)

The theme file contains:

~~~
name=<theme name> //The theme name
textColor=<r>,<g>,<b> //Optional theme text color. Should be dark or light depending on the theme background is light or dark.
textColorDialog=<r>,<g>,<b> //Optional theme text color in dialog. Should be dark since the dialog background is light.
~~~

and the following subnode.

1 block subnode
---------------

~~~
block(<block name>){...} //subnode specifies the block's appearance
~~~

block name:

"Block", "PlayerStart", "ShadowStart",
"Exit", "ShadowBlock", "Spikes",
"Checkpoint", "Swap", "Fragile",
"MovingBlock", "MovingShadowBlock", "MovingSpikes",
"Teleporter", "Button", "Switch",
"ConveyorBelt", "ShadowConveyorBelt", "NotificationBlock", "Collectable", "Pushable".

For the newest version of this list, see `Game::blockName` in `Game.cpp`.

In this subnode:

### 1.1 state/characterState/blockState/transitionState subnode

NOTE: blockState and characterState are for backwards compatibility, use state instead.

~~~
blockState(<state name>){...} //subnode specifies the appearance of each state of the block
state(<state name>){...}
characterState(<state name>){...}
transitionState(<current state>,<new state>){...}
~~~

the state name:

* for all blocks
  - "base": Always draw the base before other states (intended to be used in level editor).
  - "default": The default state.
* for "Checkpoint","Swap","Switch"
  - "activated": The activated state.
* for "Collectable"
  - "inactive": The collected state.
* for "Exit"
  - "closed": The closed state.
* for "Button"
  - "button": The button (which is a separated, movable part of the button).
* for "Fragile"
  - "fragile1": The stepped once state.
  - "fragile2": The stepped twice state.
  - "fragile3": The broken state.

transition state:

If we found a transition state which matches the current state and the next state,
then we switch to this transition state instead of the new state.

NOTE: the transition state should have the oneTimeAnimation attribute and set the correct next state.

optional attributes:

~~~
oneTimeAnimation=<length>,<next state> //if this state is one-time animation only
~~~

#### 1.1.1 object subnode

~~~
object{...} //subnode specifies (multiple) objects to display in each state
~~~

optional attributes:

~~~
animation=<length>,<loop point> //if object has looped animation
oneTimeAnimation=<length>,<end point>
invisibleAtRunTime=1 //if this object is invisible when playing game
invisibleAtDesignTime=1 //if this object is invisible when editing the map
~~~

optional nodes specifies object to display:

##### 1.1.1.1 picture subnode

~~~
picture(<file name>,<x>,<y>,<w>,<h>)
~~~

The (x,y,w,h) defines the source rectangle.

NOTE: picture and pictureAnimation are mutually exclusive. (?)

##### 1.1.1.2 optionalPicture subnode

~~~
optionalPicture(<file name>,<x>,<y>,<w>,<h>,<probability>)
~~~

If this subnode is set, the picture will be randomly used according to the given probability.

##### 1.1.1.3 editorPicture subnode

~~~
editorPicture(<file name>,<x>,<y>,<w>,<h>)
~~~

If this subnode is set, the picture will be used instead in the level editor.

##### 1.1.1.4 positioning subnode

~~~
positioning(<xalign>,<yalign>)
~~~

* xalign: 'left', 'centre' (same as 'center'), 'right' or 'repeat' or 'stretch'
* yalign: 'top', 'middle', 'bottom' or 'repeat' or 'stretch'

##### 1.1.1.5 offset subnode

~~~
offset(<x>,<y>[,<w>[,<h>]])
~~~

Shift the left,top,right,bottom of the destination rectangle by x,y,-w,-h.

NOTE: w,h are only used when the corresponding positioning modes are 'repeat' or 'stretch'.

##### 1.1.1.6 pictureAnimation subnode

~~~
pictureAnimation(<file name>){
    point(<x>,<y>,<w>,<h>[,<frame count>[,<display time of each frame>]])
    point(<x>,<y>,<w>,<h>[,<frame count>[,<display time of each frame>]])
    ...
}
~~~

NOTE: picture and pictureAnimation are mutually exclusive. (?)

NOTE: The default value of frame_count and the display_time_of_each_frame are 1.

The source rectangle (x,y,w,h) is animated by the following way:

- (x,y,w,h)=point[0] is displayed for time display_time[0]
- for i=1,2,3... the following sequences of pictures are displayed:
    - for r=1/frame_count[i],2/frame_count[i],...,1
      the (x,y,w,h)=point[i-1]*(1-r)+point[i]*r is displayed for time display_time[i]

##### 1.1.1.7 offsetAnimation subnode

~~~
offsetAnimation{
    point(<x>,<y>[,<frame count>[,<display time of each frame>[,<w>[,<h>]]]])
    point(<x>,<y>[,<frame count>[,<display time of each frame>[,<w>[,<h>]]]])
    ...
}
~~~

This is similar to pictureAnimation subnode, but this time it animates the offset.

Notice that the w,h are the last two arguments.

### 1.2 editorPicture subnode

Syntax:

~~~
editorPicture(<file name>,<x>,<y>,<w>,<h>) //specifies the picture shows in editor
~~~

This subnode is required (?) for block and scenery.

2 background subnode
--------------------

Specifies the background of level.
There can be multiple background subnodes.
Each subnode is a layer of background.

Syntax:

~~~
background(<file name>){
    srcSize=<x>,<y>,<w>,<h>  //Specifies the source size and offset of picture (optional, default value=image size)
    destSize=<x>,<y>,<w>,<h> //Specifies the destination size and offset of picture (optional, default value=size of game window WHEN THE THEME FILE IS LOADED (!!!))
    scaleToScreen=<0 or 1> //Specifies the destination should be scale to screen. (optional, default is 1)
    repeat=<repeat x>,<repeat y> //Repeat in x,y direction? (0 or 1) (optional, default value=1,1)
    speed=<speed x>,<speed y> //Specifies the moving speed (pixel/frame, a real number) (optional, default=0,0)
    cameraSpeed=<x>,<y> //The speed of following camera (a real number, typically in 0-1) (optional, default=0,0)
}
~~~

3 character subnode
-------------------

Specifies the appearance of player and shadow.

Syntax:

~~~
character(Player){
    ...
}
~~~

or

~~~
character(Shadow){
    ...
}
~~~

The other format is the same as the block subnode.

Here are the states for character which should be implemented.
Unless otherwise specified, all of them should be of size 23x40.

* "standleft", "standright", "walkleft", "walkright", "jumpleft", "jumpright",
  "fallleft", "fallright" -- various animations
* "dieleft", "dieright" -- death animation, should be `oneTimeAnimation` to "dead"
* "holding" -- used when the player is holding the shadow, vice versa
* "line" -- used when the player is recording moves for the shadow, should be of size 5x5
* "dead" -- mainly used in level editor, should be visible only in editor

4 menuBackground subnode (optional)
----------------

Specifies the background of main menu.
There can be multiple menuBackground subnodes.
The format is the same as the background subnode.

5 menu block (optional)
------------

Specifies the appearance of blocks used in level selection screen.

Syntax:

~~~
menu(Block){
    ...
}
~~~

or

~~~
menu(ShadowBlock){
    ...
}
~~~

The other format is the same as the block subnode.

NOTE: if you defined menu(Block) but not defined menu(ShadowBlock),
then all blocks used in level selection screen will be menu(Block)
regardless of locked or not.

6 scenery block
---------------

Defines new scenery block type.

Syntax:

~~~
scenery(<name>){
    ...
}
~~~

The other format is the same as the block subnode.
