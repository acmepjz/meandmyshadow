/*
 * Copyright (C) 2012-2013 Me and My Shadow
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

#ifndef SCRIPTAPI_H
#define SCRIPTAPI_H

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

class ScriptExecutor;

//Method for loading the block library.
int luaopen_block(lua_State* state);
//Method for loading the player and shadow library.
int luaopen_player(lua_State* state);
//Method for loading the level library.
int luaopen_level(lua_State* state);
//Method for loading the camera library.
int luaopen_camera(lua_State* state);
//Method for loading the audio library.
int luaopen_audio(lua_State* state);
//Method for loading the delayExecution library.
int luaopen_delayExecution(lua_State* state);
//Method for loading the gettext library.
int luaopen_gettext(lua_State* state);
//Method for loading the prng library.
int luaopen_prng(lua_State* state);

#endif
