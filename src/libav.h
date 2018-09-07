// Video encoding
// Part of Live Simulator: 2 Extensions
// See copyright notice in LS2X main.cpp

#ifdef LS2X_USE_LIBAV
#ifndef _LS2X_LIBAV_
#define _LS2X_LIBAV_

#include <map>
#include <string>

namespace ls2x
{
namespace libav
{

// meant to be allocated via av_malloc
struct songMetadata
{
	// utf8 null-terminated string
	size_t keySize;
	char *key;
	size_t valueSize;
	char *value;
};
// this should be allocated by caller
struct songInformation
{
	// whole pointer must be freed using av_free
	size_t sampleRate;
	size_t sampleCount;
	short *samples;
	size_t metadataCount;
	songMetadata *metadata;
	size_t coverArtWidth, coverArtHeight;
	char *coverArt;
};

bool isSupported();
bool startSession(const char *output, int width, int height, int framerate);
bool supply(const void *videoData);
void endSession();
bool loadAudioFile(const char *input, songInformation &info);
//bool loadAudioPointer(const char *data, songInformation &info);

const std::map<std::string, void*> &getFunctions();

}
}

#endif
#endif
