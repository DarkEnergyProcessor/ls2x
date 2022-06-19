#include "FFMpegStream.h"

inline void printError(const char *text, int err)
{
	char temp[96] = {0};
	ls2x::libav::getFunctionPointer()->strerror(err, temp, 95);
	fprintf(stderr, "%s: %s\n", text, temp);
}

FFMpegStream::FFMpegStream(love::filesystem::File *file, StreamType type)
	: ioContext(file)
	, formatContext(nullptr)
	, codecContext(nullptr)
	, targetStream(-1)
	, type(type)
	, file(file)
	, f(ls2x::libav::getFunctionPointer())
{
	packet = f->packetAlloc();
	initialize();
}

FFMpegStream::FFMpegStream(love::filesystem::FileData *fileData, StreamType type)
	: ioContext(fileData)
	, formatContext(nullptr)
	, codecContext(nullptr)
	, targetStream(-1)
	, type(type)
	, fileData(fileData)
	, f(ls2x::libav::getFunctionPointer())
{
	packet = f->packetAlloc();
	initialize();
}

void FFMpegStream::initialize()
{
	std::string filename = "";
	if (file.operator love::filesystem::File *()) filename = file->getFilename();
	else if (fileData.operator love::filesystem::FileData *()) filename = fileData->getFilename();

	try
	{
		formatContext = f->formatAllocContext();
		formatContext->pb = ioContext;
		formatContext->flags |= AVFMT_FLAG_CUSTOM_IO;

		if (f->formatOpenInput(&formatContext, filename.c_str(), nullptr, nullptr) < 0)
			throw love::Exception("Could not open input stream");

		if (f->formatFindStreamInfo(formatContext, nullptr) < 0)
			throw love::Exception("Could not find stream information");

		AVMediaType targetType = type == TYPE_VIDEO ? AVMEDIA_TYPE_VIDEO : AVMEDIA_TYPE_AUDIO;
		for (unsigned int i = 0; i < formatContext->nb_streams; i++)
			if (formatContext->streams[i]->codecpar->codec_type == targetType)
			{
				targetStream = i;
				break;
			}

		if (targetStream == -1)
			throw love::Exception("File does not contain target stream type");

		for (unsigned int i = 0; i < formatContext->nb_streams; i++)
			if (i != targetStream)
				formatContext->streams[i]->discard = AVDISCARD_ALL;

		AVCodecParameters *param = formatContext->streams[targetStream]->codecpar;
		const AVCodec *codec = f->codecFindDecoder(param->codec_id);
		codecContext = f->codecAllocContext(codec);
		f->codecParametersToContext(codecContext, param);

		if (f->codecOpen(codecContext, codec, nullptr) < 0)
			throw love::Exception("Could not open target stream");
	}
	catch (love::Exception &e)
	{
		f->packetFree(&packet);
		f->codecFreeContext(&codecContext);
		f->formatCloseInput(&formatContext);

		throw e;
	}
}

FFMpegStream::~FFMpegStream()
{
	f->packetFree(&packet);
	f->codecFreeContext(&codecContext);
	f->formatCloseInput(&formatContext);
}

bool FFMpegStream::readPacket()
{
	if (packet->buf)
		f->packetUnref(packet);

	do
	{
		int r = f->readPacket(formatContext, packet);
		if (r < 0)
		{
			printError("f->readPacket", r);
			return false;
		}
	}
	while (packet->stream_index != targetStream);

	return true;
}

bool FFMpegStream::readFrame(AVFrame *frame)
{
	if (frame->buf[0])
		f->frameUnref(frame);

	while (true)
	{
		if (!readPacket())
			return false;

		int r = f->codecSendPacket(codecContext, packet);
		if (r < 0)
		{
			printError("f->codecSendPacket", r);
			return false;
		}
		while (r >= 0)
		{
			r = f->codecReceiveFrame(codecContext, frame);
			if (r == AVERROR(EAGAIN) || r == AVERROR_EOF)
				break;
			else if (r < 0)
			{
				printError("f->codecReceiveFrame", r);
				return false;
			}

			// got packet
			return true;
		}
	}
}

double FFMpegStream::translateTimestamp(int64_t ts) const
{
	AVRational &base = formatContext->streams[targetStream]->time_base;
	return (ts*base.num)/double(base.den);
}

double FFMpegStream::getDuration() const
{
	return translateTimestamp(formatContext->streams[targetStream]->duration);
}

bool FFMpegStream::seek(double target)
{
	AVRational &base = formatContext->streams[targetStream]->time_base;
	int64_t ts = target*base.den/double(base.num);
	f->flushBuffers(codecContext);
	return f->seekFrame(formatContext, targetStream, ts, AVSEEK_FLAG_BACKWARD) >= 0;
}
