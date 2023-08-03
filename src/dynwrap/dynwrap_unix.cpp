// Dynamic Loader Wrapper
// Part of Live Simulator: 2 Extensions
// See copyright notice in LS2X main.cpp

#include "dynwrap.h"

#ifndef _WIN32

#include <dlfcn.h>

namespace ls2x
{

so::so(void *h)
: handle(h) {}

// delete with C++ delete operator
so *so::newSharedObject(const char *path)
{
	auto winhand = dlopen(path, RTLD_NOW);
	if (winhand == nullptr)
		return nullptr;
	return new so(winhand);
}

void *so::getSymbol(const char *sym)
{
	return dlsym(handle, sym);
}

so::~so()
{
	dlclose(handle);
}

}

#endif // !_WIN32
