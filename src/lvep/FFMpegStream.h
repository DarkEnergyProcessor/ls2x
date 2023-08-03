#pragma once

#include "LFSIOContext.h"

// STL
#include <string>

// LOVE
#include "common/Exception.h"
#include "common/Object.h"
#include "filesystem/File.h"

// FFMPEG
extern "C"
{
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
#include "libavutil/pixdesc.h"
}
#include "../libav.h"

#define LVEP_USE_FFMPEG6 (LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(60, 0, 0)) && (LIBSWRESAMPLE_VERSION_INT >= AV_VERSION_INT(4, 10, 0))

class FFMpegStream
{
public:
	enum StreamType
	{
		TYPE_VIDEO,
		TYPE_AUDIO,
	};

	FFMpegStream(love::filesystem::File *file, StreamType type);
	FFMpegStream(love::filesystem::FileData *fileData, StreamType type);
	~FFMpegStream();

	bool readFrame(AVFrame *frame);
	double translateTimestamp(int64_t ts) const;
	double getDuration() const;
	bool seek(double target);

private:
	ls2x::libav::libavFunctions *f;
	LFSIOContext ioContext;
	AVFormatContext *formatContext;
	AVCodecContext *codecContext;
	AVPacket *packet;

	int targetStream;
	StreamType type;

	love::StrongRef<love::filesystem::File> file;
	love::StrongRef<love::filesystem::FileData> fileData;
	
	void initialize();
	bool readPacket();
};
