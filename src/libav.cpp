// Video encoding
// Part of Live Simulator: 2 Extensions
// See copyright notice in LS2X main.cpp

#ifdef LS2X_USE_LIBAV
#include "libav.h"
#include "dynwrap/dynwrap.h"

extern "C"
{
// libav
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libavutil/opt.h"
#include "libavutil/frame.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
}

#include <queue>

namespace ls2x
{
namespace libav
{

bool supportCheck = false;
bool libavSatisfied = false;
const char *usedEncoder = nullptr;

struct libavFunctions
{
	// handle
	ls2x::so *lavc, *lavf, *lavu, *sws, *swr;
	// versioning
	decltype(avcodec_version) *codecVersion; //lavc
	decltype(avformat_version) *formatVersion; // lavf
	decltype(avutil_version) *utilVersion; // lavu
	// registration
	decltype(av_register_all) *registerAll; // lavf
	decltype(avcodec_register_all) *codecRegisterAll; // lavc
	// format
	decltype(avformat_open_input) *formatOpenInput; // lavf
	decltype(avformat_alloc_output_context2) *formatAllocOutputContext; // lavf
	decltype(avformat_find_stream_info) *formatFindStreamInfo; // lavf
	decltype(av_read_frame) *readPacket; // lavf
	decltype(avio_open) *ioOpen; // lavf
	decltype(avio_closep) *ioClose; // lavf
	decltype(av_interleaved_write_frame) *interleavedWriteFrame; // lavf
	decltype(avformat_init_output) *formatInitOutput; // lavf
	decltype(av_write_trailer) *writeTrailer; // lavf
	decltype(avformat_write_header) *formatWriteHeader; // lavf
	decltype(avformat_free_context) *formatFreeContext; // lavf
	decltype(avformat_new_stream) *formatNewStream; // lavf
	decltype(av_dump_format) *dumpFormat; // lavf
	decltype(avformat_close_input) *formatCloseInput; // lavf
	// codecs
	decltype(avcodec_alloc_context3) *codecAllocContext; // lavc
	decltype(avcodec_find_encoder_by_name) *codecFindEncoderByName; // lavc
	decltype(avcodec_find_decoder) *codecFindDecoder; // lavc
	decltype(avcodec_open2) *codecOpen; // lavc
	decltype(avcodec_send_packet) *codecSendPacket; // lavc
	decltype(avcodec_receive_packet) *codecReceivePacket; // lavc
	decltype(avcodec_send_frame) *codecSendFrame; // lavc
	decltype(avcodec_receive_frame) *codecReceiveFrame; // lavc
	decltype(av_packet_free) *packetFree; // lavc
	decltype(avcodec_free_context) *codecFreeContext; // lavc
	decltype(avcodec_parameters_from_context) *codecParametersFromContext; // lavc
	decltype(av_packet_alloc) *packetAlloc; // lavc
	decltype(av_packet_rescale_ts) *packetRescaleTS; // lavc
	decltype(av_packet_unref) *packetUnref; // lavc
	// util
	decltype(av_frame_alloc) *frameAlloc; // lavu
	decltype(av_frame_get_buffer) *frameGetBuffer; // lavu
	decltype(av_frame_make_writable) *frameMakeWritable; // lavu
	decltype(av_frame_free) *frameFree; // lavu
	decltype(av_opt_set) *optSet; // lavu
	decltype(av_dict_count) *dictCount; // lavu
	decltype(av_dict_get) *dictGet; // lavu
	decltype(av_strdup) *strdup; // lavu
	decltype(av_mallocz) *malloc; // lavu
	decltype(av_free) *free; // lavu
	decltype(av_strerror) *strerror; // lavu
	decltype(av_log_set_level) *logSetLevel; // lavu
	// sws
	decltype(sws_getContext) *swsGetContext; // sws
	decltype(sws_scale) *swsScale; // sws
	decltype(sws_freeContext) *swsFreeContext; // sws
	// swr
	decltype(swr_alloc_set_opts) *swrAllocSetOpts; // swr
	decltype(swr_init) *swrInit; // swr
	decltype(swr_convert_frame) *swrConvertFrame; // swr
	decltype(swr_get_delay) *swrGetDelay; // swr
	decltype(swr_convert) *swrConvert; // swr
	decltype(swr_free) *swrFree; // swr
};
libavFunctions avF;

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
	return true;
}

bool initLibAVFunctions()
{
#define STR(x) #x
#define LOAD(avtype, var, name) \
	avF.##var = (decltype(##name)*) avF.##avtype->getSymbol(STR(name)); \
	if (avF.##var == nullptr) return false;

	// registration
	LOAD(lavf, registerAll, av_register_all);
	LOAD(lavc, codecRegisterAll, avcodec_register_all);
	// lavf
	LOAD(lavf, formatOpenInput, avformat_open_input);
	LOAD(lavf, formatAllocOutputContext, avformat_alloc_output_context2);
	LOAD(lavf, formatFindStreamInfo, avformat_find_stream_info);
	LOAD(lavf, readPacket, av_read_frame);
	LOAD(lavf, ioOpen, avio_open);
	LOAD(lavf, ioClose, avio_closep);
	LOAD(lavf, interleavedWriteFrame, av_interleaved_write_frame);
	LOAD(lavf, formatInitOutput, avformat_init_output);
	LOAD(lavf, writeTrailer, av_write_trailer);
	LOAD(lavf, formatWriteHeader, avformat_write_header);
	LOAD(lavf, formatFreeContext, avformat_free_context);
	LOAD(lavf, formatNewStream, avformat_new_stream);
	LOAD(lavf, dumpFormat, av_dump_format);
	LOAD(lavf, formatCloseInput, avformat_close_input);
	// lavc
	LOAD(lavc, codecAllocContext, avcodec_alloc_context3);
	LOAD(lavc, codecFindEncoderByName, avcodec_find_encoder_by_name);
	LOAD(lavc, codecFindDecoder, avcodec_find_decoder);
	LOAD(lavc, codecOpen, avcodec_open2);
	LOAD(lavc, codecSendPacket, avcodec_send_packet);
	LOAD(lavc, codecReceivePacket, avcodec_receive_packet);
	LOAD(lavc, codecSendFrame, avcodec_send_frame);
	LOAD(lavc, codecReceiveFrame, avcodec_receive_frame);
	LOAD(lavc, packetFree, av_packet_free);
	LOAD(lavc, codecFreeContext, avcodec_free_context);
	LOAD(lavc, codecParametersFromContext, avcodec_parameters_from_context);
	LOAD(lavc, packetAlloc, av_packet_alloc);
	LOAD(lavc, packetRescaleTS, av_packet_rescale_ts);
	LOAD(lavc, packetUnref, av_packet_unref);
	// lavu
	LOAD(lavu, frameAlloc, av_frame_alloc);
	LOAD(lavu, frameGetBuffer, av_frame_get_buffer);
	LOAD(lavu, frameMakeWritable, av_frame_make_writable);
	LOAD(lavu, frameFree, av_frame_free);
	LOAD(lavu, optSet, av_opt_set);
	LOAD(lavu, dictCount, av_dict_count);
	LOAD(lavu, dictGet, av_dict_get);
	LOAD(lavu, strdup, av_strdup);
	LOAD(lavu, malloc, av_mallocz);
	LOAD(lavu, free, av_free);
	LOAD(lavu, strerror, av_strerror);
	LOAD(lavu, logSetLevel, av_log_set_level);
	// sws
	LOAD(sws, swsGetContext, sws_getContext);
	LOAD(sws, swsScale, sws_scale);
	LOAD(sws, swsFreeContext, sws_freeContext);
	// swr
	LOAD(swr, swrAllocSetOpts, swr_alloc_set_opts);
	LOAD(swr, swrInit, swr_init);
	LOAD(swr, swrConvertFrame, swr_convert_frame);
	LOAD(swr, swrGetDelay, swr_get_delay);
	LOAD(swr, swrConvert, swr_convert);
	LOAD(swr, swrFree, swr_free);
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

// isSupported also initialize the libav
bool isSupported()
{
	static bool initialized = false;
	if (initialized) return supportCheck;

	initialized = true;
	memset(&avF, 0, sizeof(libavFunctions));
	if (!initAVDLL())
		return supportCheck = false;
	// register very basic functions
	avF.codecVersion = (decltype(avcodec_version)*) avF.lavc->getSymbol("avcodec_version");
	avF.formatVersion = (decltype(avformat_version)*) avF.lavf->getSymbol("avformat_version");
	avF.utilVersion = (decltype(avutil_version)*) avF.lavu->getSymbol("avutil_version");
	// check version
	if (
		(avF.codecVersion() >> 16) < LIBAVCODEC_VERSION_MAJOR ||
		(avF.formatVersion() >> 16) < LIBAVFORMAT_VERSION_MAJOR ||
		(avF.utilVersion() >> 16) < LIBAVUTIL_VERSION_MAJOR
	)
	{
		return supportCheck = freeLibAV();
	}

	if (!initLibAVFunctions())
		return supportCheck = freeLibAV();

	libavSatisfied = true;
	// initialize
	// av_register_all();
	avF.registerAll();
	// avcodec_register_all();
	avF.codecRegisterAll();

	// make sure there's matroska muxer
	{
		AVFormatContext *x = nullptr;
		if (avF.formatAllocOutputContext(&x, nullptr, "matroska", nullptr) < 0)
			// no muxer
			return supportCheck = freeLibAV();
		avF.formatFreeContext(x);
	}

	// check libx264. Will use fallbacks
	if (avF.codecFindEncoderByName(usedEncoder = "libx264") == nullptr)
		// okay. Try libvpx-vp9 if it's not mp4 muxer
		if (avF.codecFindEncoderByName(usedEncoder = "libvpx-vp9") == nullptr)
			// okay. Try mpeg4
			if (avF.codecFindEncoderByName(usedEncoder = "mpeg4") == nullptr)
				// no video encoder??
				return supportCheck = freeLibAV();

	return supportCheck = true;
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
	if (!supportCheck) return false;

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
	AVCodec *vCodec = avF.codecFindEncoderByName(usedEncoder);
	AVStream *vStream = avF.formatNewStream(g_FormatContext, vCodec);
	vStream->id = g_FormatContext->nb_streams - 1;
	vStream->time_base = {1, framerate};
	// new codec context
	g_VCodecContext = avF.codecAllocContext(vCodec);
	if (g_VCodecContext == nullptr)
	{
		clean();
		return false;
	}
	g_VCodecContext->width = width;
	g_VCodecContext->height = height;
	g_VCodecContext->time_base = {1, framerate};
	g_VCodecContext->pix_fmt = AV_PIX_FMT_YUV420P;
	// x264-specific: use qp=0 and preset=slow
	if (strcmp(usedEncoder, "libx264") == 0)
	{
		g_VCodecContext->pix_fmt = AV_PIX_FMT_YUV444P;
		avF.optSet(g_VCodecContext->priv_data, "crf", "0", 0);
		avF.optSet(g_VCodecContext->priv_data, "preset", "medium", 0);
	}
	// libvpx-vp9-specific: use lossless=1
	else if (strcmp(usedEncoder, "libvpx-vp9") == 0)
	{
		g_VCodecContext->pix_fmt = AV_PIX_FMT_GBRP;
		avF.optSet(g_VCodecContext->priv_data, "lossless", "1", 0);
	}
	// this cause problem in H264 decoding (but the encoding is ok)
	//if (g_FormatContext->oformat->flags & AVFMT_GLOBALHEADER)
		//g_VCodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	avF.codecParametersFromContext(vStream->codecpar, g_VCodecContext);
	if (avF.codecOpen(g_VCodecContext, vCodec, nullptr) < 0)
	{
		clean();
		return false;
	}
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
		char temp[128];
		avF.strerror(ret, temp, 127);
		fprintf(stderr, temp);
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
	// video encode
	g_VFrame->pts = g_VCodecContext->frame_number;
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

	// done
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
	AVCodec *aCodec = nullptr, *vCodec = nullptr;
	AVCodecContext *aCodecCtx = nullptr, *vCodecCtx = nullptr;
	SwrContext *swr = nullptr;
	SwsContext *sws = nullptr;
	AVPacket *packet = nullptr;
	AVFrame *tempFrame = nullptr;

	// assume it's already open
	if (avF.formatFindStreamInfo(fmtContext, nullptr) < 0)
		return nullptr;

	// use "video" stream for cover art
	AVStream *aStream = nullptr, *vStream = nullptr;
	for (int i = 0; i < fmtContext->nb_streams && aStream == nullptr && vStream == nullptr; i++)
	{
		auto x = fmtContext->streams[i]->codecpar->codec_type;
		if (x == AVMEDIA_TYPE_AUDIO && aStream == nullptr)
			aStream = fmtContext->streams[i];
		else if (x == AVMEDIA_TYPE_VIDEO && vStream == nullptr)
			vStream = fmtContext->streams[i];
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
	if (avF.codecOpen(aCodecCtx, aCodec, nullptr) < 0 || (vCodec && avF.codecOpen(vCodecCtx, vCodec, nullptr) < 0))
		goto cleanup;

	// init swr
	swr = avF.swrAllocSetOpts(nullptr,
		AV_CH_LAYOUT_STEREO,
		AV_SAMPLE_FMT_S16,
		aStream->codecpar->sample_rate,
		aStream->codecpar->channel_layout,
		AVSampleFormat(aStream->codecpar->format),
		aStream->codecpar->sample_rate,
		0, nullptr
	);
	if (avF.swrInit(swr) < 0)
		goto cleanup;
	if (vCodecCtx)
	{
		sws = avF.swsGetContext(
			vStream->codecpar->width, vStream->codecpar->height,
			AVPixelFormat(vStream->codecpar->format),
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
	int rval = avF.readPacket(fmtContext, packet);
	while (rval >= 0)
	{
		bool isV = vStream && packet->stream_index == vStream->id;
		bool isA = packet->stream_index == aStream->id;
		bool processPacket = isV || isA;
		if (processPacket)
		{
			AVCodecContext *ctx = isV ? vCodecCtx : aCodecCtx;
			int r = avF.codecSendPacket(ctx, packet);
			if (r < 0)
			{
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
					deleteQueue(ret);
					goto cleanup;
				}
				
				if (packet->stream_index == aStream->id)
				{
					// audio decode
					AVFrame *frame = avF.frameAlloc();
					frame->format = AV_SAMPLE_FMT_S16;
					frame->nb_samples = tempFrame->nb_samples;
					frame->channel_layout = AV_CH_LAYOUT_STEREO;
					frame->sample_rate = tempFrame->sample_rate;
					// swr_convert_frame will call av_frame_get_buffer
					if (avF.frameGetBuffer(frame, 0) < 0)
					{
						avF.frameFree(&frame);
						deleteQueue(ret);
						goto cleanup;
					}
					if (avF.swrConvertFrame(swr, frame, tempFrame) < 0)
					{
						avF.frameFree(&frame);
						deleteQueue(ret);
						goto cleanup;
					}
					ret->push(frame);
					audioLen += frame->nb_samples;
				}
				else
				{
					// video decode
					int linesize[] = {tempFrame->width * tempFrame->height * 4};
					uint8_t *imageData = (uint8_t*) avF.malloc(linesize[0]);
					uint8_t *dst[] = {imageData};
					if (avF.swsScale(sws, tempFrame->data, tempFrame->linesize, 0, tempFrame->height, dst, linesize) < 0)
					{
						deleteQueue(ret);
						goto cleanup;
					}
					// set image data
					sInfo->coverArtWidth = tempFrame->width;
					sInfo->coverArtHeight = tempFrame->height;
					sInfo->coverArt = (char*) imageData;
					// done with video decoding
					avF.swsFreeContext(sws); sws = nullptr;
					avF.codecFreeContext(&vCodecCtx);
					vStream = nullptr;
				}
			}
		}

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
				frame->channel_layout = AV_CH_LAYOUT_STEREO;
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
	int64_t delay = avF.swrGetDelay(swr, aStream->codecpar->sample_rate);
	if (delay > 0)
	{
		AVFrame *frame = avF.frameAlloc();
		frame->format = AV_SAMPLE_FMT_S16;
		frame->channel_layout = AV_CH_LAYOUT_STEREO;
		frame->nb_samples = delay;
		if (avF.frameGetBuffer(frame, 0) < 0)
		{
			avF.frameFree(&frame);
			deleteQueue(ret);
			goto cleanup;
		}
		avF.swrConvert(swr, frame->data, delay, nullptr, 0);
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
	std::queue<AVFrame*> *queue = loadAudioMainRoutine(fmtContext, info, smpLen);
	if (queue == nullptr)
		goto cleanup;

	short *sample = loadAudioConcat(queue, smpLen);
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

const std::map<std::string, void*> &getFunctions()
{
	static std::map<std::string, void*> funcs = {
		{"startEncodingSession", startSession},
		{"supplyEncoder", supply},
		{"endEncodingSession", endSession},
		{"av_free", nullptr},
		{"loadAudioFile", loadAudioFile}
	};
	funcs["av_free"] = avF.free;
	return funcs;
}

}
}
#endif
