// Audio mixing system
// Part of Live Simulator: 2 Extensions
// See copyright notice in LS2X main.cpp

#include "audiomix.h"

#include <cmath>

#include <new>

template <typename T> inline T CLAMP(T value,T low,T high)
{
	return (value < low) ? low : ((value > high) ? high : value);
}

template <typename T, typename U = float> inline T LERP(T v0, T v1, U t)
{
	return T((1 - t) * U(v0) + t * U(v1));
}

namespace ls2x
{
namespace audiomix
{

int g_SampleRate;
size_t g_BufferSize;
short *g_BufferData;

void resample(const short *src, short *dst, size_t smpSrc, size_t smpDst, int channelCount)
{
	double ratio = double(smpSrc) / double(smpDst);

	switch (channelCount)
	{
		case 1:
		{
			for (size_t i = 0; i < smpDst; i++)
			{
				double idx = double(i) * ratio;
				dst[i] = LERP(src[size_t(idx)], src[size_t(idx+.5)], idx-floor(idx));
			}
			break;
		}
		case 2:
		{
			for (size_t i = 0; i < smpDst; i++)
			{
				double idx = double(i) * ratio;
				dst[i*2] = LERP(src[size_t(idx)*2], src[size_t(idx+.5)*2+1], idx-floor(idx));
				dst[i*2+1] = LERP(src[size_t(idx)*2+1], src[size_t(idx+.5)*2+1], idx-floor(idx));
			}
			break;
		}
		default: return;
	}
}

bool startSession(int sampleRate, size_t smpLen)
{
	// false if existing session is open
	if (g_BufferData) return false;

	// new memory
	g_BufferData = new (std::nothrow) short[smpLen * 2]; // stereo
	if (g_BufferData == nullptr) return false;

	g_SampleRate = sampleRate;
	g_BufferSize = smpLen;
	return true;
}

bool mixSample(const short *data, size_t smpLen, int channelCount)
{
	// only mono or stereo atm
	size_t maxLen = smpLen > g_BufferSize ? g_BufferSize : smpLen;

	switch (channelCount)
	{
		case 1:
		{
			for (size_t i = 0; i < maxLen; i++)
			{
				int smp = int(data[i]);
				g_BufferData[i * 2] = CLAMP(g_BufferData[i * 2] + smp, -32767, 32767);
				g_BufferData[i * 2 + 1] = CLAMP(g_BufferData[i * 2 + 1] + smp, -32767, 32767);
			}
			break;
		}
		case 2:
		{
			for(size_t i = 0; i < maxLen; i++)
			{
				g_BufferData[i * 2] = CLAMP(int(g_BufferData[i * 2]) + int(data[i * 2]), -32767, 32767);
				g_BufferData[i * 2 + 1] = CLAMP(int(g_BufferData[i * 2 + 1]) + int(data[i * 2 + 1]), -32767, 32767);
			}
			break;
		}
		default: return false;
	}

	return true;
}

const short *getSamplePointer()
{
	return g_BufferData;
}

void endSession()
{
	g_BufferSize = 0;
	delete[] g_BufferData;
}

const std::map<std::string, void*> &getFunctions()
{
	static std::map<std::string, void*> func = {
		{"resample", &resample},
		{"startAudioMixSession", &startSession},
		{"mixSample", &mixSample},
		{"getAudioMixPointer", &getSamplePointer},
		{"endAudioMixSession", &endSession}
	};

	return func;
}

}
}
