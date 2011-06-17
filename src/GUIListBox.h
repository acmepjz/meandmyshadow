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
#ifndef GUILISTBOX_H
#define GUILISTBOX_H

#include "GUIObject.h"
#include "GUIScrollBar.h"

class GUIListBox:public GUIObject{
public:
	std::vector<std::string> Item;
private:
	GUIScrollBar *m_oScrollBar;
public:
	GUIListBox(int Left=0,int Top=0,int Width=0,int Height=0,bool Enabled=true,bool Visible=true);
	virtual bool handle_events(int x=0,int y=0,bool enabled=true,bool visible=true,bool processed=false);
	virtual void render(int x=0,int y=0);
};

class GUISingleLineListBox:public GUIObject{
public:
	std::vector<std::string> Item;
public:
	GUISingleLineListBox(int Left=0,int Top=0,int Width=0,int Height=0,bool Enabled=true,bool Visible=true);
	virtual bool handle_events(int x=0,int y=0,bool enabled=true,bool visible=true,bool processed=false);
	virtual void render(int x=0,int y=0);
};

#endif