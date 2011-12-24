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
#ifndef GUISCROLLBAR_H
#define GUISCROLLBAR_H

#include "GUIObject.h"

const int ScrollBarHorizontal=0;
const int ScrollBarVertical=1;

class GUIScrollBar:public GUIObject{
public:
	int min,max,smallChange,largeChange;
	int orientation;
private:
	float thumbStart,thumbEnd,valuePerPixel,startDragPos;
	int criticalValue,timer;
	bool changed;
	void calcPos();
	void renderScrollBarButton(int index,int x1,int y1,int x2,int y2,int srcLeft,int srcTop);
public:
	GUIScrollBar(int left=0,int top=0,int width=0,int height=0,int orientation=0,
		int value=0,int min=0,int max=100,int smallChange=10,int largeChange=50,
		bool enabled=true,bool visible=true):
		GUIObject(left,top,width,height,0,NULL,value,enabled,visible),
		min(min),max(max),smallChange(smallChange),largeChange(largeChange),orientation(orientation),
		criticalValue(0),timer(0),changed(false)
	{
		calcPos();
	}
	virtual bool handleEvents(int x=0,int y=0,bool enabled=true,bool visible=true,bool processed=false);
	virtual void render(int x=0,int y=0);
};

#endif