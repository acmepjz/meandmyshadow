/*
 * Copyright (C) 2012 Me and My Shadow
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

#ifndef SCRIPTUSERDATA_H
#define SCRIPTUSERDATA_H

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}
#include <string>

#ifdef _DEBUG
#include <assert.h>
#endif

#if defined(_DEBUG) && defined(DISABLED_DEBUG_STUFF)
//Some debug functions
void scriptUserClassDebugCreate(char sig1,char sig2,char sig3,char sig4,const void* p1,const void* p2);
void scriptUserClassDebugInvalidate(char sig1,char sig2,char sig3,char sig4,const void* p1,const void* p2);
void scriptUserClassDebugUnlink(char sig1,char sig2,char sig3,char sig4,const void* p1,const void* p2);
#endif

//A struct represents the Lua user data.
struct ScriptUserData{
	char sig1,sig2,sig3,sig4;
	void* data;
	ScriptUserData* prev;
	ScriptUserData* next;
};

//A helper class to bind C++ class to Lua user data.
template<char sig1,char sig2,char sig3,char sig4,class T>
class ScriptUserClass{
public:
	ScriptUserClass():scriptUserDataHead(NULL){
	}

	ScriptUserClass(const ScriptUserClass& other):scriptUserDataHead(NULL){
	}

	ScriptUserClass& operator=(const ScriptUserClass& other){
		//do nothing
	}

	//Create a Lua user data pointed to this object. (-0,+1,e)
	//state: Lua state.
	//metatableName: Metatable name.
	void createUserData(lua_State* state,const char* metatableName){
		//Convert this object to T.
		T* obj=dynamic_cast<T*>(this);
#ifdef _DEBUG
		//It should not be NULL unless there is a bug in code
		assert(obj!=NULL);
#endif

		//Create user data.
		ScriptUserData* ud=(ScriptUserData*)lua_newuserdata(state,sizeof(ScriptUserData));

		ud->sig1=sig1;
		ud->sig2=sig2;
		ud->sig3=sig3;
		ud->sig4=sig4;
		ud->data=obj;

		//Add it to the linked list.
		ud->next=scriptUserDataHead;
		ud->prev=NULL;
		if(scriptUserDataHead) scriptUserDataHead->prev=ud;
		scriptUserDataHead=ud;

		//Set matatable and we are done.
		luaL_getmetatable(state,metatableName);
		lua_setmetatable(state,-2);

#if defined(_DEBUG) && defined(DISABLED_DEBUG_STUFF)
		scriptUserClassDebugCreate(sig1,sig2,sig3,sig4,this,ud);
#endif
	}

	//Destroys all Lua user data associated to this object.
	void destroyUserData(){
		while(scriptUserDataHead){
#if defined(_DEBUG) && defined(DISABLED_DEBUG_STUFF)
			scriptUserClassDebugInvalidate(sig1,sig2,sig3,sig4,this,scriptUserDataHead);
#endif
			scriptUserDataHead->data=NULL;
			scriptUserDataHead=scriptUserDataHead->next;
		}
	}

	//Convert a Lua user data in Lua stack to object. (-0,+0,e)
	//state: Lua state.
	//idx: Index.
	//Returns: The object. NULL if this user data is invalid.
	//NOTE: This data should be a user data.
	static T* getObjectFromUserData(lua_State* state,int idx){
		ScriptUserData* ud=(ScriptUserData*)lua_touserdata(state,idx);

		if(ud && ud->sig1==sig1 && ud->sig2==sig2 && ud->sig3==sig3 && ud->sig4==sig4)
			return reinterpret_cast<T*>(ud->data);
		return NULL;
	}

	//Register __gc, __eq to given table. (-0,+0,e)
	//state: Lua state.
	//idx: Index.
	static void registerMetatableFunctions(lua_State *state,int idx){
		lua_pushstring(state,"__gc");
		lua_pushcfunction(state,&garbageCollectorFunction);
		lua_rawset(state,idx);
		lua_pushstring(state,"__eq");
		lua_pushcfunction(state,&checkEqualFunction);
		lua_rawset(state,idx);
	}

	virtual ~ScriptUserClass(){
		destroyUserData();
	}
private:
	ScriptUserData* scriptUserDataHead;

	//The garbage collector (__gc) function.
	static int garbageCollectorFunction(lua_State* state){
		//Check if it's a user data. It can be a table (the library itself)
		if(!lua_isuserdata(state,1)) return 0;

		ScriptUserData* ud=(ScriptUserData*)lua_touserdata(state,1);

		if(ud){
			if(ud->data){
#if defined(_DEBUG) && defined(DISABLED_DEBUG_STUFF)
				//It should be impossible unless there is a bug in code
				assert(ud->sig1==sig1 && ud->sig2==sig2 && ud->sig3==sig3 && ud->sig4==sig4);
#endif
				//Unlink it
				if(ud->next) ud->next->prev=ud->prev;
				if(ud->prev) ud->prev->next=ud->next;
				else{
					ScriptUserClass* owner=static_cast<ScriptUserClass*>(reinterpret_cast<T*>(ud->data));
					owner->scriptUserDataHead=ud->next;
				}
#if defined(_DEBUG) && defined(DISABLED_DEBUG_STUFF)
				scriptUserClassDebugUnlink(sig1,sig2,sig3,sig4,
					static_cast<ScriptUserClass*>(reinterpret_cast<T*>(ud->data)),ud);
#endif
			}

			ud->data=NULL;
			ud->next=NULL;
			ud->prev=NULL;
		}

		return 0;
	}

	//The 'operator==' (__eq) function.
	static int checkEqualFunction(lua_State* state){
		//Check if it's a user data. It can be a table (the library itself)
		if(!lua_isuserdata(state,1) || !lua_isuserdata(state,2)) return 0;

		ScriptUserData* ud1=(ScriptUserData*)lua_touserdata(state,1);
		ScriptUserData* ud2=(ScriptUserData*)lua_touserdata(state,2);

		if(ud1!=NULL && ud2!=NULL){
#ifdef _DEBUG
			//It should be impossible unless there is a bug in code
			assert(ud1->sig1==sig1 && ud1->sig2==sig2 && ud1->sig3==sig3 && ud1->sig4==sig4);
			assert(ud2->sig1==sig1 && ud2->sig2==sig2 && ud2->sig3==sig3 && ud2->sig4==sig4);
#endif
			lua_pushboolean(state,ud1->data==ud2->data);
			return 1;
		}

		return 0;
	}
};

#endif
