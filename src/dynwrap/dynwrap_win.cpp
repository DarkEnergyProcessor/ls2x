// Dynamic Loader Wrapper
// Part of Live Simulator: 2 Extensions
// See copyright notice in LS2X main.cpp

#include "dynwrap.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace ls2x
{

so::so(void *h)
: handle(h) {}

// delete with C++ delete operator
so *so::newSharedObject(const char *path)
{
	auto winhand = LoadLibraryA(path);
	if (winhand == nullptr)
		return nullptr;
	return new so(winhand);
}

void *so::getSymbol(const char *sym)
{
	return GetProcAddress(HMODULE(handle), sym);
}

so::~so()
{
	FreeLibrary(HMODULE(handle));
}

}
