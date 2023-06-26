// Video encoding
// Part of Live Simulator: 2 Extensions
// See copyright notice in LS2X main.cpp

#ifdef LS2X_USE_LIBAV
#include "libav.h"

#include <queue>

namespace ls2x
{
namespace libav
{

libavFunctions avF;
bool encodingSupported = false;
bool libavSatisfied = false;
const char *usedEncoder = nullptr;

ls2x::so *loadLibAVDLL(const char *name, int version)
{
	char temp[128];
	ls2x::so *ret = nullptr;
#define TEST_PATTERN(pattern) \
	sprintf(temp, pattern, name, version); \
	if ((ret = ls2x::so::newSharedObject(temp))) return ret;

	TEST_PATTERN("%s-%d");
	TEST_PATTERN("%s.%d");
	TEST_PATTERN("lib%s-%d.so");
	TEST_PATTERN("lib%s.%d.so");
	TEST_PATTERN("lib%s.so.%d");
	TEST_PATTERN("lib%s-%d.dylib");
	TEST_PATTERN("lib%s.%d.dylib");
	TEST_PATTERN("lib%s.dylib.%d");
#undef TEST_PATTERN
	return ret;
}

bool initAVDLL()
{
	// linked libav means the libav function always exist
	// and is available in linker.
#ifndef LS2X_LIBAV_LINKED
	// av functions
	avF.lavu = loadLibAVDLL("avutil", LIBAVUTIL_VERSION_MAJOR);
	avF.swr = loadLibAVDLL("swresample", LIBSWRESAMPLE_VERSION_MAJOR);
	avF.lavc = loadLibAVDLL("avcodec", LIBAVCODEC_VERSION_MAJOR);
	avF.lavf = loadLibAVDLL("avformat", LIBAVFORMAT_VERSION_MAJOR);
	avF.sws = loadLibAVDLL("swscale", LIBSWSCALE_VERSION_MAJOR);
	if (!(avF.lavu && avF.swr && avF.lavc && avF.lavf && avF.sws))
	{
		if (avF.sws) {delete avF.sws; avF.sws = nullptr;}
		if (avF.lavf) {delete avF.lavf; avF.lavf = nullptr;}
		if (avF.lavc) {delete avF.lavc; avF.lavc = nullptr;}
		if (avF.swr) {delete avF.swr; avF.swr = nullptr;}
		if (avF.lavu) {delete avF.lavu; avF.lavu = nullptr;}
		return false;
	}
#endif
	return true;
}

#define STR(x) #x

#ifdef LS2X_LIBAV_LINKED
#define LOAD_AVFUNC(_, var, name) \
	avF.var = &##name;
#else
#define LOAD_AVFUNC(avtype, var, name) \
	avF.var = (decltype(name)*) avF.avtype->getSymbol(STR(name)); \
	if (avF.var == nullptr) return false;
#endif

bool initLibAVFunctions()
{
#define LS2X_NOLOAD_VERSIONING
#include "libavfunc.h"
#undef LS2X_NOLOAD_VERSIONING
	// done
	return true;
}

bool freeLibAV()
{
	if (avF.sws) {delete avF.sws; avF.sws = nullptr;}
	if (avF.lavf) {delete avF.lavf; avF.lavf = nullptr;}
	if (avF.lavc) {delete avF.lavc; avF.lavc = nullptr;}
	if (avF.swr) {delete avF.swr; avF.swr = nullptr;}
	if (avF.lavu) {delete avF.lavu; avF.lavu = nullptr;}
	return false;
}

static void printAVError(int err)
{
	char temp[128];
	avF.strerror(err, temp, 127);
	fprintf(stderr, "%s\n", temp);
}

template<typename AV>
static void fixChannelLayout(AV *ctx)
{
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(60, 0, 0)
#	if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(58, 2, 0)
	if (ctx->ch_layout.order == AV_CHANNEL_ORDER_UNSPEC)
		avF.getDefaultChannelLayout(&ctx->ch_layout, ctx->ch_layout.nb_channels);
# 	else
#		error "Using libavcodec 60 requires libavutil 58.2"
#	endif
#else
	if (ctx->channel_layout == 0)
		ctx->channel_layout = (uint64_t) avF.getDefaultChannelLayout(ctx->channels);
#endif
}

static bool initializeSwresample(SwrContext **swr, AVCodecContext *ctx)
{
	fixChannelLayout(ctx);

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(60, 0, 0)
	// FFmpeg 6
#	if LIBSWRESAMPLE_VERSION_INT >= AV_VERSION_INT(4, 10, 0)
	// use swr_alloc_set_opts2
	static AVChannelLayout stereo = {AV_CHANNEL_ORDER_UNSPEC};
	if (stereo.order == AV_CHANNEL_ORDER_UNSPEC)
		avF.channelLayoutFromMask(&stereo, AV_CH_LAYOUT_STEREO);

	return avF.swrAllocSetOpts(swr,
		&stereo,
		AV_SAMPLE_FMT_S16,
		ctx->sample_rate,
		&ctx->ch_layout,
		ctx->sample_fmt,
		ctx->sample_rate,
		0, nullptr
	) == 0;
#	else
#		error "Using libavcodec 60.0 requires libswresample 4.10"
#	endif // LIBSWRESAMPLE_VERSION_INT >= AV_VERSION_INT(4, 10, 0)
#else // LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(60, 0, 100)
	// use swr_alloc_set_opts
	SwrContext *newSwr = avF.swrAllocSetOpts(*swr,
		AV_CH_LAYOUT_STEREO,
		AV_SAMPLE_FMT_S16,
		ctx->sample_rate,
		ctx->channel_layout,
		ctx->sample_fmt,
		ctx->sample_rate,
		0, nullptr
	);
	if (newSwr != nullptr)
		*swr = newSwr;
	return newSwr != nullptr;
#endif // LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(60, 0, 100)
}

static void setFrameChannelLayout(AVFrame *frame, uint64_t mask)
{
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(60, 0, 0)
#	if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(58, 2, 0)
	avF.channelLayoutFromMask(&frame->ch_layout, mask);
# 	else
#		error "Using libavcodec 60 requires libavutil 58.2"
#	endif
#else
	frame->channel_layout = mask;
#endif
}

// isSupported also initialize the libav
bool isSupported()
{
	static bool initialized = false;
	if (initialized) return libavSatisfied;

	initialized = true;
	avF.lavc = avF.lavf = avF.lavu = nullptr;
	if (!initAVDLL())
		return false;

	// register very basic functions
	LOAD_AVFUNC(lavc, codecVersion, avcodec_version);
	LOAD_AVFUNC(lavf, formatVersion, avformat_version);
	LOAD_AVFUNC(lavu, utilVersion, avutil_version);

	// check version
	if (
		avF.codecVersion() < LIBAVCODEC_VERSION_INT ||
		avF.formatVersion()  < LIBAVFORMAT_VERSION_INT ||
		avF.utilVersion() < LIBAVUTIL_VERSION_INT
	)
		return (libavSatisfied = freeLibAV());

	if (!initLibAVFunctions())
		return (libavSatisfied = freeLibAV());

	// initialize
#if LIBAVFORMAT_VERSION_MAJOR < 58
	avF.registerAll();
#endif
#if LIBAVCODEC_VERSION_MAJOR < 58
	avF.codecRegisterAll();
#endif
	libavSatisfied = true;

	// make sure there's matroska muxer
	{
		AVFormatContext *x = nullptr;
		if (avF.formatAllocOutputContext(&x, nullptr, "matroska", nullptr) < 0)
		{
			// no muxer
			encodingSupported = false;
			return libavSatisfied;
		}
		avF.formatFreeContext(x);
	}

	if (
		// Check libx264rgb. Will use fallbacks
		avF.codecFindEncoderByName(usedEncoder = "libx264rgb") == nullptr &&
		// No x264rgb. try normal x264.
		avF.codecFindEncoderByName(usedEncoder = "libx264") == nullptr &&
		// Okay. Try libvpx-vp9
		avF.codecFindEncoderByName(usedEncoder = "libvpx-vp9") == nullptr &&
		// Huh? PNG, must be supported everywhre, right?
		avF.codecFindEncoderByName(usedEncoder = "png") == nullptr &&
		// Fine, mpeg4
		avF.codecFindEncoderByName(usedEncoder = "mpeg4") == nullptr
	)
	{
		// FFmpeg is compiled with --disable-encoders???
		encodingSupported = false;
		return libavSatisfied;
	}

	encodingSupported = true;
	return true;
}

AVFormatContext *g_FormatContext = nullptr;
AVCodecContext *g_VCodecContext = nullptr;
AVFrame *g_VFrame = nullptr;
SwsContext *g_SwsContext = nullptr;
AVPacket *g_Packet = nullptr;

int g_Width = 0, g_Height = 0, g_Framerate = 0;
size_t g_FrameCounter = 0;

void clean()
{
	avF.swsFreeContext(g_SwsContext); g_SwsContext = nullptr;
	if (g_Packet)
		avF.packetFree(&g_Packet);
	if (g_VFrame)
		avF.frameFree(&g_VFrame);
	if (g_VCodecContext)
		avF.codecFreeContext(&g_VCodecContext);
	if (g_FormatContext)
	{
		if (g_FormatContext->pb)
			avF.ioClose(&g_FormatContext->pb);
		avF.formatFreeContext(g_FormatContext);
		g_FormatContext = nullptr;
	}
}

bool startSession(const char *output, int width, int height, int framerate)
{
	if (!encodingSupported) return false;

	// open output context
	if (avF.formatAllocOutputContext(&g_FormatContext, nullptr, nullptr, output) < 0)
		return false;
	// open file
	if ((g_FormatContext->oformat->flags & AVFMT_NOFILE) == 0)
		if (avF.ioOpen(&g_FormatContext->pb, output, AVIO_FLAG_WRITE) < 0)
		{
			clean();
			return false;
		}

	// new video stream
	const AVCodec *vCodec = avF.codecFindEncoderByName(usedEncoder);
	AVStream *vStream = avF.formatNewStream(g_FormatContext, vCodec);
	vStream->id = g_FormatContext->nb_streams - 1;
	vStream->time_base = {1, framerate};
	vStream->avg_frame_rate = {framerate, 1};
	vStream->codecpar->width = width;
	vStream->codecpar->height = height;

	// new codec context
	g_VCodecContext = avF.codecAllocContext(vCodec);
	if (g_VCodecContext == nullptr)
	{
		clean();
		return false;
	}

	// Set codec context values
	g_VCodecContext->width = width;
	g_VCodecContext->height = height;
	g_VCodecContext->time_base = {1, framerate};
	g_VCodecContext->framerate = {framerate, 1};
	g_VCodecContext->pix_fmt = AV_PIX_FMT_YUV420P;

	// x264-specific: use qp=0 and preset=slow
	if (strstr(usedEncoder, "libx264"))
	{
		avF.optSet(g_VCodecContext->priv_data, "crf", "0", 0);
		avF.optSet(g_VCodecContext->priv_data, "preset", "slow", 0);
		
		if (strcmp(usedEncoder, "libx264rgb") == 0)
			g_VCodecContext->pix_fmt = AV_PIX_FMT_BGR0;
		else if (strcmp(usedEncoder, "libx264") == 0)
			g_VCodecContext->pix_fmt = AV_PIX_FMT_YUV444P;
	}
	// libvpx-vp9-specific: use lossless=1
	else if (strcmp(usedEncoder, "libvpx-vp9") == 0)
	{
		g_VCodecContext->pix_fmt = AV_PIX_FMT_GBRP;
		avF.optSet(g_VCodecContext->priv_data, "lossless", "1", 0);
	}
	// png: pixel format = rgb24
	else if (strcmp(usedEncoder, "png") == 0)
		g_VCodecContext->pix_fmt = AV_PIX_FMT_RGB24;

	if (g_FormatContext->oformat->flags & AVFMT_GLOBALHEADER)
		g_VCodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	// Open codec
	if (avF.codecOpen(g_VCodecContext, vCodec, nullptr) < 0)
	{
		clean();
		return false;
	}

	avF.codecParametersFromContext(vStream->codecpar, g_VCodecContext);

	// new video frame
	g_VFrame = avF.frameAlloc();
	g_VFrame->format = g_VCodecContext->pix_fmt;
	g_VFrame->width = width;
	g_VFrame->height = height;
	if (avF.frameGetBuffer(g_VFrame, 32) < 0)
	{
		clean();
		return false;
	}
	g_SwsContext = avF.swsGetContext(
		width, height, AV_PIX_FMT_RGBA, // src (supplier)
		width, height, g_VCodecContext->pix_fmt, // dest (encode)
		SWS_BILINEAR, // flags
		nullptr, nullptr, nullptr // other
	);
	if (g_SwsContext == nullptr)
	{
		clean();
		return false;
	}

	// new packet
	g_Packet = avF.packetAlloc();
	if (g_Packet == nullptr) return false;

	// write header
	int ret = avF.formatWriteHeader(g_FormatContext, nullptr);
	if (ret < 0)
	{
		printAVError(ret);
		clean();
		return false;
	}
	// debug
	avF.dumpFormat(g_FormatContext, 0, output, true);
	g_Width = width; g_Height = height; g_Framerate = framerate;
	g_FrameCounter = 0;
	return true;
}

bool supply(const void *videoData)
{
	// video scaling
	if (avF.frameMakeWritable(g_VFrame) < 0)
		return false;
	const uint8_t *vData[] = {(const uint8_t*)videoData};
	int vLinesize[] = {g_Width * 4};
	avF.swsScale(g_SwsContext, vData, vLinesize, 0, g_Height, g_VFrame->data, g_VFrame->linesize);
	g_VFrame->pts = g_VCodecContext->frame_number;

	// Video encode
	int ret = avF.codecSendFrame(g_VCodecContext, g_VFrame);
	if (ret < 0)
		return false;
	while (ret >= 0)
	{
		ret = avF.codecReceivePacket(g_VCodecContext, g_Packet);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			break;
		else if (ret < 0)
			return false;

		avF.packetRescaleTS(g_Packet, g_VCodecContext->time_base, g_FormatContext->streams[0]->time_base);
		if (avF.interleavedWriteFrame(g_FormatContext, g_Packet) < 0)
			return false;

		avF.packetUnref(g_Packet);
	}

	return true;
}

void endSession()
{
	if (g_FormatContext == nullptr) return;

	// Flush
	int ret = avF.codecSendFrame(g_VCodecContext, nullptr);
	while (ret >= 0)
	{
		ret = avF.codecReceivePacket(g_VCodecContext, g_Packet);
		if (ret >= 0)
		{
			avF.packetRescaleTS(g_Packet, g_VCodecContext->time_base, g_FormatContext->streams[0]->time_base);
			avF.interleavedWriteFrame(g_FormatContext, g_Packet);
		}
		avF.packetUnref(g_Packet);
	}

	// write trailer
	avF.interleavedWriteFrame(g_FormatContext, nullptr);
	avF.writeTrailer(g_FormatContext);
	clean();
}

void deleteQueue(std::queue<AVFrame*> *&queue)
{
	size_t s = queue->size();
	while (s-- > 0)
	{
		AVFrame *temp = queue->front();
		avF.frameFree(&temp);
		queue->pop();
	}
	delete queue; queue = nullptr;
}

std::queue<AVFrame*>* loadAudioMainRoutine(AVFormatContext *fmtContext, songInformation &info, size_t &audioLen)
{
	std::queue<AVFrame*>* ret = nullptr;
	songInformation *sInfo = nullptr;
	songMetadata *sMeta = nullptr;
	const AVCodec *aCodec = nullptr, *vCodec = nullptr;
	AVCodecContext *aCodecCtx = nullptr, *vCodecCtx = nullptr;
	SwrContext *swr = nullptr;
	SwsContext *sws = nullptr;
	AVPacket *packet = nullptr;
	AVFrame *tempFrame = nullptr;
	int64_t delay = 0;
	int rval = 0;

	avF.dumpFormat(fmtContext, 0, "None", 0);

	// assume it's already open
	if (avF.formatFindStreamInfo(fmtContext, nullptr) < 0)
		return nullptr;

	// use "video" stream for cover art
	AVStream *aStream = nullptr, *vStream = nullptr;
	int aStreamIndex = -1, vStreamIndex = -1;
	for (unsigned int i = 0; i < fmtContext->nb_streams && (aStream == nullptr || vStream == nullptr); i++)
	{
		AVMediaType x = fmtContext->streams[i]->codecpar->codec_type;

		if (x == AVMEDIA_TYPE_AUDIO && aStream == nullptr)
			aStream = fmtContext->streams[aStreamIndex = i];
		else if (x == AVMEDIA_TYPE_VIDEO && vStream == nullptr)
			vStream = fmtContext->streams[vStreamIndex = i];
	}
	if (aStream == nullptr)
		return nullptr;

	// read metadata
	sInfo = (songInformation*) avF.malloc(sizeof(songInformation));
	size_t metadataCount = avF.dictCount(fmtContext->metadata);
	if (metadataCount > 0)
	{
		AVDictionaryEntry *entry = nullptr;
		int i = 0;
		sMeta = (songMetadata*) avF.malloc(sizeof(songMetadata) * metadataCount);
		entry = avF.dictGet(fmtContext->metadata, "", entry, AV_DICT_IGNORE_SUFFIX);
		while (entry != nullptr)
		{
			char *k = avF.strdup(entry->key);
			char *v = avF.strdup(entry->value);
			sMeta[i++] = {strlen(k), k, strlen(v), v};
			entry = avF.dictGet(fmtContext->metadata, "", entry, AV_DICT_IGNORE_SUFFIX);
		}
	}

	// Set sample rate
	sInfo->sampleRate = aStream->codecpar->sample_rate;

	// get codec
	aCodec = avF.codecFindDecoder(aStream->codecpar->codec_id);
	vCodec = nullptr;
	if (aCodec == nullptr)
		goto cleanup;
	if (vStream)
		// if no decoder for the cover art is present, that's okay
		vCodec = avF.codecFindDecoder(vStream->codecpar->codec_id);

	// initialize decoder
	aCodecCtx = avF.codecAllocContext(aCodec);
	if (aCodecCtx == nullptr)
		goto cleanup;
	if (vCodec)
		if ((vCodecCtx = avF.codecAllocContext(vCodec)) == nullptr)
			goto cleanup;

	// initialize parameters
	avF.codecParametersToContext(aCodecCtx, aStream->codecpar);

	if (vCodecCtx)
		avF.codecParametersToContext(vCodecCtx, vStream->codecpar);

	// open codec
	if (avF.codecOpen(aCodecCtx, aCodec, nullptr) < 0 || (vCodec && avF.codecOpen(vCodecCtx, vCodec, nullptr) < 0))
		goto cleanup;

	// init swr
	if (!initializeSwresample(&swr, aCodecCtx))
		goto cleanup;
	if (avF.swrInit(swr) < 0)
		goto cleanup;
	if (vCodecCtx)
	{
		sws = avF.swsGetContext(
			vStream->codecpar->width, vStream->codecpar->height,
			(AVPixelFormat) vStream->codecpar->format,
			vStream->codecpar->width, vStream->codecpar->height,
			AV_PIX_FMT_RGBA,
			SWS_BILINEAR,
			nullptr, nullptr, nullptr
		);
		if (sws == nullptr)
			goto cleanup;
	}

	// start decoding
	packet = avF.packetAlloc();
	tempFrame = avF.frameAlloc();
	ret = new std::queue<AVFrame*>;
	audioLen = 0;
	rval = avF.readPacket(fmtContext, packet);

	while (rval >= 0)
	{
		bool isV = vStream && packet->stream_index == vStreamIndex;
		bool isA = packet->stream_index == aStreamIndex;
		bool processPacket = isV || isA;
		if (processPacket)
		{
			AVCodecContext *ctx = isV ? vCodecCtx : aCodecCtx;

			int r = avF.codecSendPacket(ctx, packet);
			if (r < 0)
			{
				avF.packetUnref(packet);
				deleteQueue(ret);
				goto cleanup;
			}
			while (r >= 0)
			{
				r = avF.codecReceiveFrame(ctx, tempFrame);

				if (r == AVERROR(EAGAIN) || r == AVERROR_EOF)
					break;
				else if (r < 0)
				{
					printAVError(r);
					avF.packetUnref(packet);
					deleteQueue(ret);
					goto cleanup;
				}
				
				if (packet->stream_index == aStreamIndex)
				{
					// audio decode
					AVFrame *frame = avF.frameAlloc();
					frame->format = AV_SAMPLE_FMT_S16;
					frame->nb_samples = tempFrame->nb_samples;
					frame->sample_rate = tempFrame->sample_rate;
					setFrameChannelLayout(frame, AV_CH_LAYOUT_STEREO);
					fixChannelLayout(tempFrame);

					// swr_convert_frame will call av_frame_get_buffer
					if (avF.frameGetBuffer(frame, 0) < 0)
					{
						avF.frameFree(&frame);
						avF.packetUnref(packet);
						deleteQueue(ret);
						goto cleanup;
					}
					int convertResult = avF.swrConvertFrame(swr, frame, tempFrame);
					if (convertResult < 0)
					{
						printAVError(convertResult);
						avF.frameFree(&frame);
						avF.packetUnref(packet);
						deleteQueue(ret);
						goto cleanup;
					}

					ret->push(frame);
					audioLen += frame->nb_samples;
				}
				else
				{
					// video decode
					uint8_t *imageData[4];
					int lineSizes[4];

					if (avF.imageAlloc(imageData, lineSizes, tempFrame->width, tempFrame->height, AV_PIX_FMT_RGBA, 1) < 0)
					{
						avF.packetUnref(packet);
						deleteQueue(ret);
						goto cleanup;
					}

					if (avF.swsScale(sws, tempFrame->data, tempFrame->linesize, 0, tempFrame->height, imageData, lineSizes) < 0)
					{
						avF.packetUnref(packet);
						deleteQueue(ret);
						goto cleanup;
					}

					// set image data
					sInfo->coverArtWidth = tempFrame->width;
					sInfo->coverArtHeight = tempFrame->height;
					sInfo->coverArt = (char *) imageData[0];

					// done with video decoding
					avF.swsFreeContext(sws); sws = nullptr;
					avF.codecFreeContext(&vCodecCtx);
					vStream = nullptr;

					break;
				}
			}
		}

		avF.packetUnref(packet);
		rval = avF.readPacket(fmtContext, packet);
	}
	if (rval != AVERROR_EOF)
	{
		deleteQueue(ret);
		goto cleanup;
	}
	// flush decode
	{
		int r = avF.codecSendPacket(aCodecCtx, nullptr);
		if (r >= 0)
		{
			r = avF.codecReceiveFrame(aCodecCtx, tempFrame);
			while (r >= 0)
			{
				// audio decode
				AVFrame *frame = avF.frameAlloc();
				frame->format = AV_SAMPLE_FMT_S16;
				frame->nb_samples = tempFrame->nb_samples;
				setFrameChannelLayout(frame, AV_CH_LAYOUT_STEREO);

				// swr_convert_frame will call av_frame_get_buffer
				if (avF.swrConvertFrame(swr, frame, tempFrame) < 0)
				{
					avF.frameFree(&frame);
					deleteQueue(ret);
					goto cleanup;
				}
				ret->push(frame);
				audioLen += frame->nb_samples;
			}
		}
	}

	// flush swr
	delay = avF.swrGetDelay(swr, aStream->codecpar->sample_rate);
	if (delay > 0)
	{
		AVFrame *frame = avF.frameAlloc();
		frame->format = AV_SAMPLE_FMT_S16;
		frame->nb_samples = (int) delay;
		setFrameChannelLayout(frame, AV_CH_LAYOUT_STEREO);

		if (avF.frameGetBuffer(frame, 0) < 0)
		{
			avF.frameFree(&frame);
			deleteQueue(ret);
			goto cleanup;
		}
		avF.swrConvert(swr, frame->data, (int) delay, nullptr, 0);
		ret->push(frame);
		audioLen += frame->nb_samples;
	}
	// set information
	memcpy(&info, sInfo, sizeof(songInformation));
	info.metadata = sMeta;
	info.metadataCount = metadataCount;
	
cleanup:
	if (tempFrame)
		avF.frameFree(&tempFrame);
	if (packet)
		avF.packetFree(&packet);
	if (swr)
		avF.swrFree(&swr);
	if (sws)
		avF.swsFreeContext(sws);
	if (aCodecCtx)
		avF.codecFreeContext(&aCodecCtx);
	if (vCodecCtx)
		avF.codecFreeContext(&vCodecCtx);
	if (ret == nullptr && metadataCount > 0)
	{
		for (int i = 0; i < metadataCount; i++)
		{
			avF.free(sMeta[i].key);
			avF.free(sMeta[i].value);
		}
		avF.free(sMeta); sMeta = nullptr;
	}
	avF.free(sInfo); sInfo = nullptr;

	return ret;
}

short *loadAudioConcat(std::queue<AVFrame*> *&queue, size_t smpSize)
{
	short *smp = (short*) avF.malloc(smpSize * sizeof(short) * 2);
	if (smp == nullptr)
		return nullptr;

	short *curPos = smp;
	size_t s = queue->size();
	while (s-- > 0)
	{
		AVFrame *temp = queue->front();
		queue->pop();
		memcpy(curPos, temp->data[0], temp->nb_samples * sizeof(short) * 2);
		curPos += temp->nb_samples * 2;
		avF.frameFree(&temp);
	}

	delete queue; queue = nullptr;
	return smp;
}

bool loadAudioFile(const char *input, songInformation &info)
{
	bool ret = false;
	if (!libavSatisfied) return false;

	AVFormatContext *fmtContext = nullptr;
	if (avF.formatOpenInput(&fmtContext, input, nullptr, nullptr) < 0)
		return false;

	size_t smpLen;
	short *sample = nullptr;
	std::queue<AVFrame*> *queue = loadAudioMainRoutine(fmtContext, info, smpLen);
	if (queue == nullptr)
		goto cleanup;

	sample = loadAudioConcat(queue, smpLen);
	if (sample == nullptr)
		goto cleanup;

	info.sampleCount = smpLen;
	info.samples = sample;
	ret = true;

cleanup:
	if (queue)
		deleteQueue(queue);
	if (fmtContext)
		avF.formatCloseInput(&fmtContext);
	return ret;
}

bool hasEncodingSupported()
{
	return encodingSupported;
}

libavFunctions *getFunctionPointer()
{
	return libavSatisfied ? &avF : nullptr;
}

const std::map<std::string, void*> &getFunctions()
{
	static std::map<std::string, void*> funcs = {
		{std::string("encodingSupported"), (void*) hasEncodingSupported},
		{std::string("startEncodingSession"), (void*) startSession},
		{std::string("supplyEncoder"), (void*) supply},
		{std::string("endEncodingSession"), (void*) endSession},
		{std::string("loadAudioFile"), (void*) loadAudioFile},
		{std::string("av_free"), (void*) avF.free},
	};
	return funcs;
}

}
}
#endif
