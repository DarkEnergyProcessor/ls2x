// STL
#include <iostream>
#include <fstream>

// Lua
#include <lua.hpp>

// LOVE
#include "common/runtime.h"
#include "filesystem/wrap_Filesystem.h"

// libav
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}
#include "../libav.h"

// LVEP
#include "LVEPVideoStream.h"
#include "LVEPDecoder.h"

int w_newVideoStream(lua_State *L)
{
	love::filesystem::File *file = love::filesystem::luax_getfile(L, 1);

	love::video::VideoStream *stream = nullptr;
	love::luax_catchexcept(L, [&]() {
		// Can't check if open for reading
		if (!file->isOpen() && !file->open(love::filesystem::File::MODE_READ))
			luaL_error(L, "File is not open and cannot be opened");

		stream = new LVEPVideoStream(file);
	});

	luax_pushtype(L, stream);
	stream->release();
	return 1;
}

int w_newDecoder(lua_State *L)
{
	love::filesystem::FileData *data = love::filesystem::luax_getfiledata(L, 1);
	int bufferSize = (int) luaL_optinteger(L, 2, love::sound::Decoder::DEFAULT_BUFFER_SIZE);

	love::sound::Decoder *t = nullptr;
	love::luax_catchexcept(L,
		[&]() { t = new LVEPDecoder(data, bufferSize); },
		[&](bool) { data->release(); }
	);

	if (t == nullptr)
		return luaL_error(L, "Extension \"%s\" not supported.", data->getExtension().c_str());

	luax_pushtype(L, t);
	t->release();
	return 1;
}

extern "C" int luaopen_lvep(lua_State *L)
{
	auto f = ls2x::libav::getFunctionPointer();
	f->registerAll();
	f->codecRegisterAll();

	lua_newtable(L);
	lua_pushcfunction(L, w_newVideoStream);
	lua_setfield(L, -2, "newVideoStream");
	lua_pushcfunction(L, w_newDecoder);
	lua_setfield(L, -2, "newDecoder");
	return 1;
}
