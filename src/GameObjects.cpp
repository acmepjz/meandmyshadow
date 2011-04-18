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
#include "GameObjects.h"
#include "Functions.h"
#include "Globals.h"
#include "Player.h"

GameObject::GameObject(Game *objParent):m_objParent(objParent),surface(NULL)
{

}

GameObject::~GameObject()
{

}

SDL_Rect GameObject::get_box(int nBoxType)
{
	return box;
}

void GameObject::save_state(){
}

void GameObject::load_state(){
}

void GameObject::reset(){
}

void GameObject::play_animation(int flags){
}

void GameObject::OnEvent(int nEventType){
}

int GameObject::QueryProperties(int nPropertyType,Player* obj){
	return 0;
}

void GameObject::GetEditorData(std::vector<std::pair<std::string,std::string> >& obj){
}

void GameObject::SetEditorData(std::map<std::string,std::string>& obj){
}

void GameObject::move(){
}
