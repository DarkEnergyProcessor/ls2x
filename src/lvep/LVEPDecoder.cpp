#include "LVEPDecoder.h"

extern "C"
{
#include "libavutil/opt.h"
}

LVEPDecoder::LVEPDecoder(love::filesystem::FileData *data, int bufferSize)
	: love::sound::Decoder(data, bufferSize)
	, stream(data, FFMpegStream::TYPE_AUDIO)
	, frame(nullptr)
	, f(ls2x::libav::getFunctionPointer())
{
	frame = f->frameAlloc();
	if (!stream.readFrame(frame))
	{
		f->frameFree(&frame);
		throw love::Exception("No first frame");
	}
	
#if LVEP_USE_FFMPEG6
	AVChannelLayout *channelLayout = &frame->ch_layout;

	f->swrAllocSetOpts(
		&recodeContext,
		channelLayout, AV_SAMPLE_FMT_S16, frame->sample_rate,
		channelLayout, (AVSampleFormat) frame->format, frame->sample_rate,
		0, nullptr
	);
#else
	int channelLayout = frame->channel_layout;
	if (!channelLayout)
		channelLayout = f->getDefaultChannelLayout(frame->channels);

	recodeContext = f->swrAllocSetOpts(
		nullptr,
		channelLayout, AV_SAMPLE_FMT_S16, frame->sample_rate,
		channelLayout, (AVSampleFormat) frame->format, frame->sample_rate,
		0, nullptr
	);
#endif
	f->swrInit(recodeContext);
}

LVEPDecoder::~LVEPDecoder()
{
	f->swrFree(&recodeContext);
	f->frameFree(&frame);
}

love::sound::Decoder *LVEPDecoder::clone()
{
	return new LVEPDecoder((love::filesystem::FileData *) data.get(), bufferSize);
}

int LVEPDecoder::getSize() const
{
	return bufferSize;
}

void *LVEPDecoder::getBuffer() const
{
	return (void*) buffer;
}

int LVEPDecoder::decode()
{
	if (!stream.readFrame(frame))
	{
		eof = true;
		return 0;
	}

	uint8_t *buffers[2] = {(uint8_t *) buffer, nullptr};
#if LVEP_USE_FFMPEG6
	int nchannels = frame->ch_layout.nb_channels;
#else
	int nchannels = frame->channels;
#endif

	int decoded = f->swrConvert(recodeContext,
				buffers, (bufferSize >> 1) / nchannels,
				(const uint8_t**) &frame->data[0], frame->nb_samples);

	if (decoded < 0)
		return 0;

	return decoded*nchannels*2;
}

bool LVEPDecoder::seek(double s)
{
	eof = false;
	return stream.seek(s);
}

bool LVEPDecoder::rewind()
{
	return seek(0);
}

bool LVEPDecoder::isSeekable()
{
	return true;
}

bool LVEPDecoder::isFinished()
{
	return eof;
}

int LVEPDecoder::getChannelCount() const
{
#if LVEP_USE_FFMPEG6
	return frame->ch_layout.nb_channels;
#else
	return frame->channels;
#endif
}

int LVEPDecoder::getBitDepth() const
{
	return 16;
}

int LVEPDecoder::getSampleRate() const
{
	return frame->sample_rate;
}

double LVEPDecoder::getDuration()
{
	return stream.getDuration();
}
