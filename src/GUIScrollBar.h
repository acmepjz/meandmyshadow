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

const int ScrollBarHorizontal = 0;
const int ScrollBarVertical = 1;

class GUIScrollBar:public GUIObject{
public:
	int Min,Max,SmallChange,LargeChange;
	int Orientation;
private:
	float fThumbStart,fThumbEnd,fValuePerPixel,fStartDragPos;
	int nCriticalValue,nTimer;
	bool bChanged;
	void CalcPos();
	void pRenderScrollBarButton(int nIndex,int x1,int y1,int x2,int y2,int SrcLeft,int SrcTop);
public:
	GUIScrollBar(int Left=0,int Top=0,int Width=0,int Height=0,int Orientation=0,
		int Value=0,int Min=0,int Max=100,int SmallChange=10,int LargeChange=50,
		bool Enabled=true,bool Visible=true):
		GUIObject(Left,Top,Width,Height,0,NULL,Value,Enabled,Visible),
		Min(Min),Max(Max),SmallChange(SmallChange),LargeChange(LargeChange),Orientation(Orientation),
		nCriticalValue(0),nTimer(0),bChanged(false)
	{
		CalcPos();
	}
	virtual bool handle_events(int x=0,int y=0,bool enabled=true,bool visible=true,bool processed=false);
	virtual void render(int x=0,int y=0);
};

#endif