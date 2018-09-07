// Fast Fourier Transformation
// Part of Live Simulator: 2 Extensions
// See copyright notice in LS2X main.cpp

#ifdef LS2X_USE_KISSFFT
#ifndef _LS2X_FFT_
#define _LS2X_FFT_

#include "kiss_fft.h"

extern "C"
{
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#include <map>
#include <string>

namespace ls2x
{
namespace fft
{

// stereo input > stereo separated fftr
void fftr(const short *input, kiss_fft_scalar *out_l, kiss_fft_scalar *out_r, size_t sampleSize);
// stereo input > mono merged fftr (or mono input > mono fftr)
void fftr(const short *input, kiss_fft_scalar *out, size_t sampleSize, bool stereo);
// very ugly
const char *scalarType();

const std::map<std::string, void*> &getFunctions();

}
}

#endif
#endif
