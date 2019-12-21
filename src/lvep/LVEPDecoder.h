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

	love::sound::Decoder *clone() override;
	int decode() override;
	int getSize() const override;
	void *getBuffer() const override;
	bool seek(double s) override;
	bool rewind() override;
	bool isSeekable() override;
	bool isFinished() override;
	int getChannelCount() const override;
	int getBitDepth() const override;
	int getSampleRate() const override;
	double getDuration() override;

private:
	ls2x::libav::libavFunctions *f;
	FFMpegStream stream;
	AVFrame *frame;
	SwrContext *recodeContext;
};
