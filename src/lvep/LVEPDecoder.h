#pragma once

#include "FFMpegStream.h"

// LOVE
#include "filesystem/File.h"
#include "sound/Decoder.h"

// FFMPEG
extern "C"
{
#include "libswresample/swresample.h"
}
#include "../libav.h"

class LVEPDecoder : public love::sound::Decoder
{
public:
	LVEPDecoder(love::filesystem::FileData *data, int bufferSize);
	virtual ~LVEPDecoder();

	love::sound::Decoder *clone();
	int decode();
	int getSize() const;
	void *getBuffer() const;
	bool seek(float s);
	bool rewind();
	bool isSeekable();
	bool isFinished();
	int getChannelCount() const;
	int getBitDepth() const;
	int getSampleRate() const;
	double getDuration();

private:
	ls2x::libav::libavFunctions *f;
	FFMpegStream stream;
	AVFrame *frame;
	SwrContext *recodeContext;
	love::StrongRef<love::filesystem::File> file;
};
