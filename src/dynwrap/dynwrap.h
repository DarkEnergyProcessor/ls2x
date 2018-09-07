// Dynamic Loader Wrapper
// Part of Live Simulator: 2 Extensions
// See copyright notice in LS2X main.cpp

#ifndef _LS2X_DYNWRAP_
#define _LS2X_DYNWRAP_

namespace ls2x
{

// shared object
class so
{
public:
	static so *newSharedObject(const char *path);
	void *getSymbol(const char *sym);
	~so();
protected:
	so(void *h);
	void *handle;
private:
	so(const so&) = delete;
};

}

#endif