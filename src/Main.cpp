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
#include <SDL/SDL.h>
#include "Functions.h"
#include "Timer.h"
#include "Objects.h"
#include "Globals.h"
#include "Title_Menu.h"
#include "GUIObject.h"
#include <cstdlib>
#include <ctime>

#if 0

//test only

#include "POASerializer.h"
#include "TreeStorageNode.h"
using namespace std;

class clsTest:public ITreeStorageBuilder{
private:
	int nIndent;
public:
	clsTest():nIndent(0){}
	virtual void SetName(std::string& sName){
		for(int x=0;x<nIndent;x++) cout<<"\t";
		cout<<"NodeName="<<sName<<endl;
	}
	virtual void SetValue(std::vector<std::string>& sValue){
		for(int x=0;x<nIndent;x++) cout<<"\t";
		cout<<"NodeValue=";
		for(unsigned int i=0;i<sValue.size();i++){
			if(i>0) cout<<",";
			cout<<sValue[i];
		}
		cout<<endl;
	}
	virtual ITreeStorageBuilder* NewNode(){
		for(int x=0;x<nIndent;x++) cout<<"\t";
		cout<<"NewNode"<<endl;
		nIndent++;
		return this;
	}
	virtual void EndNode(){
		nIndent--;
		for(int x=0;x<nIndent;x++) cout<<"\t";
		cout<<"EndNode"<<endl;
	}
	virtual void NewAttribute(std::string& sName,std::vector<std::string>& sValue){
		for(int x=0;x<nIndent;x++) cout<<"\t";
		cout<<"AttributeName="<<sName<<endl;
		for(int x=0;x<nIndent;x++) cout<<"\t";
		cout<<"AttributeValue=";
		for(unsigned int i=0;i<sValue.size();i++){
			if(i>0) cout<<",";
			cout<<sValue[i];
		}
		cout<<endl;
	}
};

int main(int argc,char** argv){
	clsTest obj1;
	POASerializer objS;
	bool b=objS.LoadNodeFromFile("test1.txt",&obj1,true);
	cout<<(b?"OK":"Error")<<endl;
	TreeStorageNode *obj2=new TreeStorageNode;
	b=objS.LoadNodeFromFile("test1.txt",obj2,true);
	cout<<(b?"OK":"Error")<<endl;
	objS.WriteNode(obj2,cout,true,true);
	delete obj2;
	return 0;
}

#else

int main ( int argc, char * args[] )
{
	if ( init() == false )
	{
		return 1;
	}

	if ( load_files() == false )
	{
		return 1;
	}

	//IGRA/////
	stateID = STATE_MENU;
	currentState = new Menu();

	//////LEVEL EDITOR////////
	/*stateID = STATE_LEVEL_EDITOR;
	currentState = new LevelEditor();*/

	delta.start();

	srand(time(NULL));

	Mix_PlayMusic(music, -1);

	s_temp = SDL_CreateRGBSurface(SDL_HWSURFACE|SDL_SRCALPHA,
		screen->w,screen->h,screen->format->BitsPerPixel,
		screen->format->Rmask,screen->format->Gmask,screen->format->Bmask,0);
	int nFadeIn=0;

	while ( stateID != STATE_EXIT)
	{
		FPS.start();

		while(SDL_PollEvent(&event)){
			currentState->handle_events();
			GUIObjectHandleEvents();
		}

		currentState->logic();

		delta.start();

		set_camera();

		currentState->render();
		if(GUIObjectRoot) GUIObjectRoot->render();
		if(nFadeIn>0&&nFadeIn<255){
			SDL_BlitSurface(screen,NULL,s_temp,NULL);
			SDL_FillRect(screen,NULL,0);
			SDL_SetAlpha(s_temp, SDL_SRCALPHA, nFadeIn);
			SDL_BlitSurface(s_temp,NULL,screen,NULL);
			nFadeIn+=17;
		}
		SDL_Flip(screen);

		if(nextState!=STATE_NULL) nFadeIn=17;
		change_state();

		int t=FPS.get_ticks();
		t=( 1000 / g_FPS ) - t;
		if ( t>0 )
		{
			SDL_Delay( t );
		}

	}

	SDL_FreeSurface(s_temp);

	o_mylevels.save_levels();

	clean();
	return 0;
}

#endif