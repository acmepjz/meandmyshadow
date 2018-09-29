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
#include <memory>

#include <assert.h>

//NOTE: Enable this you'll see a lot of annoying script debug messages which will lag the game.
//#define DISABLED_DEBUG_STUFF

#if defined(DISABLED_DEBUG_STUFF)
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

	ScriptUserClass(const ScriptUserClass& other) = delete;

	ScriptUserClass& operator=(const ScriptUserClass& other) = delete;

	//Create a Lua user data pointed to this object. (-0,+1,e)
	//state: Lua state.
	//metatableName: Metatable name.
	void createUserData(lua_State* state,const char* metatableName){
		//Create user data.
		ScriptUserData* ud=(ScriptUserData*)lua_newuserdata(state,sizeof(ScriptUserData));

		//Add it to the linked list.
		linkUserData(ud);

		//Set matatable and we are done.
		luaL_getmetatable(state,metatableName);
		lua_setmetatable(state,-2);
	}

	//Destroys all Lua user data associated to this object.
	void destroyUserData(){
		while(scriptUserDataHead){
#if defined(DISABLED_DEBUG_STUFF)
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

		return getObjectFromUserData(ud);
	}

	//Convert a ScriptUserData to object.
	static T* getObjectFromUserData(ScriptUserData* ud) {
		if (ud && ud->sig1 == sig1 && ud->sig2 == sig2 && ud->sig3 == sig3 && ud->sig4 == sig4)
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

	class ObservePointer;
	friend class ObservePointer;

	//Reinventing the wheel of std::weak_ptr.
	class ObservePointer {
	private:
		ScriptUserData* ud;
	public:
		ObservePointer() : ud(NULL) {}
		ObservePointer(const ObservePointer& other) : ud(NULL) {
			(*this) = other;
		}
		ObservePointer(const T* obj) : ud(NULL) {
			(*this) = obj;
		}
		~ObservePointer() {
			ScriptUserClass::unlinkUserData(ud);
			if (ud) delete ud;
		}
		ObservePointer& operator=(const T* obj) {
			if (ScriptUserClass::getObjectFromUserData(ud) == obj) return *this;

			//Unlink old one
			ScriptUserClass::unlinkUserData(ud);

			//Sanity check
			assert(ud == NULL || ud->data == NULL);

			//Link new one
			if (obj) {
				if (ud == NULL) ud = new ScriptUserData;
				const_cast<T*>(obj)->linkUserData(ud);
			} else if (ud) {
				ud->data = NULL;
			}

			return *this;
		}
		ObservePointer& operator=(const ObservePointer& other) {
			if (this == &other) return *this;
			(*this) = const_cast<ObservePointer&>(other).get();
			return *this;
		}
		T* get() const {
			return ScriptUserClass::getObjectFromUserData(const_cast<ScriptUserData*>(ud));
		}
		void swap(ObservePointer& other) {
			std::swap(ud, other.ud);
		}
	};
private:
	ScriptUserData* scriptUserDataHead;

	//Fill the user data and add it to the linked list.
	void linkUserData(ScriptUserData* ud) {
		//Convert this object to T.
		//NOTE: we omit the runtime safety check, only leave the compile time check (by static_cast).
		T* obj = static_cast<T*>(this);

		ud->sig1 = sig1;
		ud->sig2 = sig2;
		ud->sig3 = sig3;
		ud->sig4 = sig4;
		ud->data = obj;

		//Add it to the linked list.
		ud->next = scriptUserDataHead;
		ud->prev = NULL;
		if (scriptUserDataHead) scriptUserDataHead->prev = ud;
		scriptUserDataHead = ud;

#if defined(DISABLED_DEBUG_STUFF)
		scriptUserClassDebugCreate(sig1, sig2, sig3, sig4, this, ud);
#endif
	}

	//Unlink the user data from the linked list.
	static void unlinkUserData(ScriptUserData* ud) {
		if (ud) {
			if (ud->data) {
				//It should be impossible unless there is a bug in code
				assert(ud->sig1 == sig1 && ud->sig2 == sig2 && ud->sig3 == sig3 && ud->sig4 == sig4);

				//Unlink it
				if (ud->next) ud->next->prev = ud->prev;
				if (ud->prev) ud->prev->next = ud->next;
				else {
					ScriptUserClass* owner = static_cast<ScriptUserClass*>(reinterpret_cast<T*>(ud->data));
					owner->scriptUserDataHead = ud->next;
				}
#if defined(DISABLED_DEBUG_STUFF)
				scriptUserClassDebugUnlink(sig1, sig2, sig3, sig4,
					static_cast<ScriptUserClass*>(reinterpret_cast<T*>(ud->data)), ud);
#endif
			}

			ud->data = NULL;
			ud->next = NULL;
			ud->prev = NULL;
		}
	}

	//The garbage collector (__gc) function.
	static int garbageCollectorFunction(lua_State* state){
		//Check if it's a user data. It can be a table (the library itself)
		if(!lua_isuserdata(state,1)) return 0;

		ScriptUserData* ud=(ScriptUserData*)lua_touserdata(state,1);

		unlinkUserData(ud);

		return 0;
	}

	//The 'operator==' (__eq) function.
	static int checkEqualFunction(lua_State* state){
		//Check if it's a user data. It can be a table (the library itself)
		if(!lua_isuserdata(state,1) || !lua_isuserdata(state,2)) return 0;

		ScriptUserData* ud1=(ScriptUserData*)lua_touserdata(state,1);
		ScriptUserData* ud2=(ScriptUserData*)lua_touserdata(state,2);

		if(ud1!=NULL && ud2!=NULL){
			//It should be impossible unless there is a bug in code
			assert(ud1->sig1==sig1 && ud1->sig2==sig2 && ud1->sig3==sig3 && ud1->sig4==sig4);
			assert(ud2->sig1==sig1 && ud2->sig2==sig2 && ud2->sig3==sig3 && ud2->sig4==sig4);

			lua_pushboolean(state,ud1->data==ud2->data);
			return 1;
		}

		return 0;
	}
};

//Another helper class to bind C++ class to Lua user data.
//This allows dynamic changes of the pointer which pointing to the actual C++ class.
//Typical use case is a class which can dynamically create/delete during game running and has save/load feature.
template<char sig1, char sig2, char sig3, char sig4, class T>
class ScriptProxyUserClass {
public:
	//The default constructor, which creates a new proxy object.
	ScriptProxyUserClass() : proxy(new Proxy()) {
	}

	//The copy constructor, which reuses proxy object from existing one.
	//NOTE: You must call this function in your copy constructor!!!
	ScriptProxyUserClass(const ScriptProxyUserClass& other) : proxy(other.proxy) {
	}

	ScriptProxyUserClass& operator=(const ScriptProxyUserClass& other) = delete;

	virtual ~ScriptProxyUserClass() {
		if (proxy->object == static_cast<T*>(this)) {
			proxy->object = NULL;
		}
	}

	//Set current object as active object, i.e. accessible from Lua.
	//Usually called when the object is created at the first time, or when the game is loaded.
	void setActive() {
		proxy->object = static_cast<T*>(this);
	}

	//Create a Lua user data pointed to this object. (-0,+1,e)
	//state: Lua state.
	//metatableName: Metatable name.
	void createUserData(lua_State* state, const char* metatableName) {
		assert(proxy->object == static_cast<T*>(this));
		proxy->createUserData(state, metatableName);
	}

	//Convert a Lua user data in Lua stack to object. (-0,+0,e)
	//state: Lua state.
	//idx: Index.
	//Returns: The object. NULL if this user data is invalid.
	//NOTE: This data should be a user data.
	static T* getObjectFromUserData(lua_State* state, int idx) {
		Proxy *p = Proxy::getObjectFromUserData(state, idx);
		if (p == NULL) return NULL;
		return p->object;
	}

	//Register __gc, __eq to given table. (-0,+0,e)
	//state: Lua state.
	//idx: Index.
	static void registerMetatableFunctions(lua_State *state, int idx) {
		Proxy::registerMetatableFunctions(state, idx);
	}

	class Proxy : public ScriptUserClass<sig1, sig2, sig3, sig4, Proxy> {
		friend class ScriptProxyUserClass;
	public:
		Proxy() : object(NULL) {}

		virtual ~Proxy() {}

		T *get() {
			return object;
		}

	public:
		T *object;
	};

	std::shared_ptr<Proxy> proxy;

	class ObservePointer;
	friend class ObservePointer;

	//Reinventing the wheel of std::weak_ptr.
	class ObservePointer {
	private:
		typename Proxy::ObservePointer proxy;
	public:
		ObservePointer() {}
		ObservePointer(const ObservePointer& other) : proxy(other.proxy) {}
		ObservePointer(const T* obj) : proxy(obj ? obj->proxy.get() : NULL) {}
		~ObservePointer() {}
		ObservePointer& operator=(const T* obj) {
			if (obj) {
				proxy = obj->proxy.get();
			} else {
				proxy = NULL;
			}

			return *this;
		}
		ObservePointer& operator=(const ObservePointer& other) {
			if (this == &other) return *this;
			proxy = other.proxy;
			return *this;
		}
		T* get() const {
			Proxy *p = proxy.get();
			if (p) return p->get();
			return NULL;
		}
		void swap(ObservePointer& other) {
			proxy.swap(other.proxy);
		}
	};
};

#endif
