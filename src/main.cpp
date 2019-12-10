// Live Simulator: 2 Extensions
// Copyright(c) 2040 Dark Energy Processor
//
// This software is provided 'as-is',without any express or implied
// warranty.In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications,and to alter it and redistribute it
// freely,subject to the following restrictions:
// 
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software.If you use this software
//    in a product,an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such,and must not
//    be misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source
//    distribution.

#define LUA_LIB ls2xlib

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#ifdef LS2X_EMBEDDED_IN_LOVE
int luaopen_lvep(lua_State *L);
#endif
}

#include "audiomix.h"
#include "fft.h"
#include "libav.h"

#include <string>
#include <map>

union FunctionString
{
	const void *ptr;
	const char str[sizeof(void*)];
};

void registerFunc(lua_State *L, int idx, const std::map<std::string, void*> &func)
{
	static FunctionString fstr = {};

	for (auto &x: func)
	{
		fstr.ptr = x.second;
		lua_pushstring(L, x.first.c_str());
		lua_pushlstring(L, fstr.str, sizeof(void*));
		lua_rawset(L, idx);
	}
}

#ifdef LUA_BUILD_AS_DLL
extern "C" int LUALIB_API luaopen_ls2xlib(lua_State *L)
#else
extern "C" int luaopen_ls2xlib(lua_State *L)
#endif
{
	lua_newtable(L);
	int baseTable = lua_gettop(L);
	lua_pushstring(L, "_VERSION");
	lua_pushstring(L, "1.0");
	lua_rawset(L, -3);
	// rawptr
	lua_pushstring(L, "rawptr");
	lua_newtable(L);
	int ptrTable = lua_gettop(L);
	// capabilities
	lua_pushstring(L, "features");
	lua_newtable(L);
	{
		// audiomix is always supported
		lua_pushstring(L, "audiomix");
		lua_pushboolean(L, 1);
		lua_rawset(L, -3);
		registerFunc(L, ptrTable, ls2x::audiomix::getFunctions());
#ifdef LS2X_USE_KISSFFT
		// kissfft
		lua_pushstring(L, "fft");
		lua_pushboolean(L, 1);
		lua_rawset(L, -3);
		registerFunc(L, ptrTable, ls2x::fft::getFunctions());
#endif
#ifdef LS2X_USE_LIBAV
		// venc is bit more complex.
		if (ls2x::libav::isSupported())
		{
			lua_pushstring(L, "libav");
			lua_pushboolean(L, 1);
			lua_rawset(L, -3);
			registerFunc(L, ptrTable, ls2x::libav::getFunctions());
#ifdef LS2X_EMBEDDED_IN_LOVE
			// lvep
			lua_getglobal(L, "package");
			lua_getfield(L, -1, "preload");
			lua_pushcfunction(L, luaopen_lvep);
			lua_setfield(L, -2, "lvep");
			lua_pop(L, 2);
			lua_pushstring(L, "lvep");
			lua_pushboolean(L, 1);
			lua_rawset(L, -3);
#endif
		}
#endif
	}
	lua_rawset(L, baseTable);
	lua_rawset(L, baseTable);
	
	return 1;
}
