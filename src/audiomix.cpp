// Audio mixing system
// Part of Live Simulator: 2 Extensions
// See copyright notice in LS2X main.cpp

#include "audiomix.h"

#include <cmath>
#include <cstring>

#include <new>

template <typename T> inline T CLAMP(T value, T low, T high)
{
	return (value < low) ? low : ((value > high) ? high : value);
}

template <typename T, typename U = float> inline T INTERPOLATE(T v0, T v1, U t)
{
	// return T((1 - t) * U(v0) + t * U(v1));
	return T((2*t*t*t-3*t*t+1)*U(v0)+(-2*t*t*t+3*t*t)*U(v1)+(t*t*t-t*t));
}

namespace ls2x
{
namespace audiomix
{

int g_SampleRate;
float g_MasterVolume;
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
				size_t j1 = size_t(idx), j2 = size_t(idx+0.5);
				dst[i] = INTERPOLATE(src[j1], src[j2], idx-floor(idx));
			}
			break;
		}
		case 2:
		{
			for (size_t i = 0; i < smpDst; i++)
			{
				double idx = double(i) * ratio;
				double t = idx - floor(idx);
				size_t j1 = size_t(idx), j2 = size_t(idx+0.5);
				dst[i*2] = INTERPOLATE(src[j1*2], src[j2*2], t);
				dst[i*2+1] = INTERPOLATE(src[j1*2+1], src[j2*2+1], t);
			}
			break;
		}
		default: return;
	}
}

bool startSession(float masterVolume, int sampleRate, size_t smpLen)
{
	// false if existing session is open
	if (g_BufferData) return false;

	// new memory
	g_BufferData = new (std::nothrow) short[smpLen * 2]; // stereo
	if (g_BufferData == nullptr) return false;

	g_SampleRate = sampleRate;
	g_BufferSize = smpLen;
	g_MasterVolume = masterVolume;
	memset(g_BufferData, 0, smpLen * 4);
	return true;
}

bool mixSample(const short *data, size_t smpLen, int channelCount, float volume)
{
	// only mono or stereo atm
	size_t maxLen = smpLen > g_BufferSize ? g_BufferSize : smpLen;
	float vol = volume * g_MasterVolume;

	switch (channelCount)
	{
		case 1:
		{
			for (size_t i = 0; i < maxLen; i++)
			{
				short smp = (short) CLAMP<float>(float(data[i]) * vol, -32767.0, 32767.0);
				g_BufferData[i * 2] = (short) CLAMP<int>(g_BufferData[i * 2] + smp, -32767, 32767);
				g_BufferData[i * 2 + 1] = (short) CLAMP<int>(g_BufferData[i * 2 + 1] + smp, -32767, 32767);
			}
			break;
		}
		case 2:
		{
			for(size_t i = 0; i < maxLen; i++)
			{
				short smp1 = (short) CLAMP<float>(float(data[i * 2]) * vol, -32767.0, 32767.0);
				short smp2 = (short) CLAMP<float>(float(data[i * 2 + 1]) * vol, -32767.0, 32767.0);
				g_BufferData[i * 2] = (short) CLAMP<int>(g_BufferData[i * 2] + smp1, -32767, 32767);
				g_BufferData[i * 2 + 1] = (short) CLAMP<int>(g_BufferData[i * 2 + 1] + smp2, -32767, 32767);
			}
			break;
		}
		default: return false;
	}

	return true;
}

void getSamplePointer(short *dest)
{
	memcpy(dest, g_BufferData, g_BufferSize * 2 * sizeof(short));
	memset(g_BufferData, 0, g_BufferSize * 2 * sizeof(short));
}

void endSession()
{
	g_BufferSize = 0;
	delete[] g_BufferData;
}

const std::map<std::string, void*> &getFunctions()
{
	static std::map<std::string, void*> func = {
		{std::string("resample"), (void*) &resample},
		{std::string("startAudioMixSession"), (void*) &startSession},
		{std::string("mixSample"), (void*) &mixSample},
		{std::string("getAudioMixPointer"), (void*) &getSamplePointer},
		{std::string("endAudioMixSession"), (void*) &endSession}
	};

	return func;
}

}
}
