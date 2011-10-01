/****************************************************************************
** Copyright (C) 2011 Luka Horvat <redreaper132 at gmail.com>
** Copyright (C) 2011 Edward Lii <edward_iii at myway.com>
** Copyright (C) 2011 O. Bahri Gordebak <gordebak at gmail.com>
**
**
** This file may be used under the terms of the GNU General Public
** License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
**  You should have received a copy of the GNU General Public License
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/
#ifndef GUIOBJECT_H
#define GUIOBJECT_H

#include "Globals.h"
#include "Functions.h"
#include <string>
#include <vector>
#include <list>

const int GUIObjectNone=0;
const int GUIObjectLabel=1;
const int GUIObjectButton=2;
const int GUIObjectCheckBox=3;
//const int GUIObjectOptionButton=4;
const int GUIObjectTextBox=5;
const int GUIObjectFrame=6;

const int GUIEventClick=0;
const int GUIEventChange=1;

class GUIObject;

class GUIEventCallback{
public:
	virtual void GUIEventCallback_OnEvent(std::string Name,GUIObject* obj,int nEventType)=0;
};

class GUIObject{
public:
	int Left,Top,Width,Height;
	int Type;
	int Value;
	std::string Name,Caption;
	bool Enabled,Visible;
	std::vector<GUIObject*> ChildControls;
	GUIEventCallback *EventCallback;
protected:
	int State;
	SDL_Surface *bmGUI;
public:
	GUIObject(int Left=0,int Top=0,int Width=0,int Height=0,int Type=0,
		const char* Caption=NULL,int Value=0,
		bool Enabled=true,bool Visible=true):
		Left(Left),Top(Top),Width(Width),Height(Height),
		Type(Type),Value(Value),
		Enabled(Enabled),Visible(Visible),
		EventCallback(NULL),State(0)
	{
		if(Caption) GUIObject::Caption=Caption;
		bmGUI=load_image(get_data_path()+"gfx/gui.png");
	}
	virtual ~GUIObject();
	virtual bool handle_events(int x=0,int y=0,bool enabled=true,bool visible=true,bool processed=false);
	virtual void render(int x=0,int y=0);
};

void GUIObjectHandleEvents();

struct GUIEvent{
	GUIEventCallback *EventCallback;
	std::string Name;
	GUIObject* obj;
	int nEventType;
};

extern std::list<GUIEvent> GUIEventQueue;

#endif
