// Video encoding
// Part of Live Simulator: 2 Extensions
// See copyright notice in LS2X main.cpp

#ifdef LS2X_USE_LIBAV
#ifndef _LS2X_LIBAV_
#define _LS2X_LIBAV_

#include <map>
#include <string>

#include "dynwrap/dynwrap.h"
extern "C"
{
// libav
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libavutil/frame.h"
#include "libavutil/imgutils.h"
#include "libavutil/opt.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
}

// Need FFmpeg 3.2 or later
static_assert(LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57, 64, 0), "FFmpeg header is too old (avcodec)");
static_assert(LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(57, 56, 0), "FFmpeg header is too old (avformat)");
static_assert(LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(55, 34, 0), "FFmpeg header is too old (avutil)");
static_assert(LIBSWSCALE_VERSION_INT >= AV_VERSION_INT(4, 2, 0), "FFmpeg header is too old (swscale)");
static_assert(LIBSWRESAMPLE_VERSION_INT >= AV_VERSION_INT(2, 3, 0), "FFmpeg header is too old (swresample)");

namespace ls2x
{
namespace libav
{

struct libavFunctions
{
	// handle
	ls2x::so *lavc, *lavf, *lavu, *sws, *swr;

#define LOAD_AVFUNC(avtype, var, name) decltype(name) *var
#include "libavfunc.h"
#undef LOAD_AVFUNC
};

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
libavFunctions *getFunctionPointer();

const std::map<std::string, void*> &getFunctions();

}
}

#endif
#endif
