// Audio mixing system
// Part of Live Simulator: 2 Extensions
// See copyright notice in LS2X main.cpp

#ifndef _LS2X_AUDIOMIX_
#define _LS2X_AUDIOMIX_

#include <cstdlib>
#include <string>
#include <map>

namespace ls2x
{

namespace audiomix
{

void resample(const short *src, short *dst, size_t smpSrc, size_t smpDst, int channelCount);
bool startSession(int sampleRate, size_t smpLen);
// Must be in 16-bit depth and same sample rate
bool mixSample(const short *data, size_t smpLen, int channelCount);
// With size of smpLen
const short *getSamplePointer();
// Free all memory for current session
void endSession();
const std::map<std::string, void*> &getFunctions();

}
}

#endif