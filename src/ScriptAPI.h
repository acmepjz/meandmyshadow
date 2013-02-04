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

int test(lua_State* state){
	cout<<"Hello world"<<endl;
	return 0;
}

///////////////////////////BLOCK SPECIFIC///////////////////////////
int getBlockById(lua_State* state){
	//Get the number of args, this MUST be one.
	int args=lua_gettop(state);
	if(args!=1){
		lua_pushstring(state,_("Incorrect number of arguments for getBlockById, expected 1."));
		lua_error(state);
	}
	//Make sure the given argument is an id (string).
	if(!lua_isstring(state,1)){
		lua_pushstring(state,_("Invalid type for argument 1 of getBlockById."));
		lua_error(state);
	}

	//Get the actual game object.
	string id=lua_tostring(state,1);
	//FIXME: We can't just assume that the currentState is the game state.
	std::vector<GameObject*> levelObjects=(dynamic_cast<Game*>(currentState))->levelObjects;
	GameObject* object=NULL;
	for(int i=0;i<levelObjects.size();i++){
		if(levelObjects[i]->getEditorProperty("id")==id){
			object=levelObjects[i];
			break;
		}
	}
	if(object==NULL){
		//Unable to find the requested object.
		//Return nothing, will result in a nil in the script. 
		return 0;
	}

	//Create the userdatum.
	GameObject** datum = (GameObject**)lua_newuserdata(state,sizeof(GameObject*));
	*datum=object;
	//And set the metatable for the userdatum.
	luaL_getmetatable(state,"block");
	lua_setmetatable(state,-2);

	//We return one object, the userdatum.
	return 1;
}

int getBlockLocation(lua_State* state){
	//Make sure there's only one argument and that argument is an userdatum.
	int args=lua_gettop(state);
	if(args!=1){
		lua_pushstring(state,_("Incorrect number of arguments for getBlockLocation, expected 1."));
		lua_error(state);
	}
	if(!lua_isuserdata(state,1)){
		lua_pushstring(state,_("Invalid type for argument 1 of getBlockLocation."));
		lua_error(state);
	}
	GameObject* object = *(GameObject**)lua_touserdata(state,1);
	
	//Get the object.
	lua_pushnumber(state,object->getBox().x);
	lua_pushnumber(state,object->getBox().y);
	return 2;
}

int setBlockLocation(lua_State* state){
	//Check the number of arguments.
	int args=lua_gettop(state);

	//Make sure the number of arguments is correct.
	if(args!=3){
		lua_pushstring(state,_("Incorrect number of arguments for setBlockLocation, expected 3."));
		lua_error(state);
	}
	//Check if the arguments are of the right type.
	if(!lua_isuserdata(state,1)){
		lua_pushstring(state,_("Invalid type for argument 1 of getBlockLocation."));
		lua_error(state);
	}
	if(!lua_isnumber(state,2)){
		lua_pushstring(state,_("Invalid type for argument 2 of getBlockLocation."));
		lua_error(state);
	}
	if(!lua_isnumber(state,3)){
		lua_pushstring(state,_("Invalid type for argument 3 of getBlockLocation."));
		lua_error(state);
	}

	//Now get the pointer to the object.
	//TODO: Make sure the object sill exists.
	GameObject* object = *(GameObject**)lua_touserdata(state,1);

	int x=lua_tonumber(state,2);
	int y=lua_tonumber(state,3);
	object->setPosition(x,y);
	
	return 0;
}

//Array with the methods for the block library.
static const struct luaL_Reg blocklib_m[]={
	{"getBlockById",getBlockById},
	{"getLocation",getBlockLocation},
	{"setLocation",setBlockLocation},
	{NULL,NULL}
};
int luaopen_block(lua_State* state){
	luaL_newlib(state,blocklib_m);
	
	//Create the metatable for the block userdata.
	luaL_newmetatable(state,"block");

	lua_pushstring(state,"__index");
	lua_pushvalue(state,-2);
	lua_settable(state,-3);

	//Register the functions and methods.
	luaL_setfuncs(state,blocklib_m,0);
	return 1;
}

//Register the libraries.
void registerFunctions(ScriptExecutor* executor){
	//
}
