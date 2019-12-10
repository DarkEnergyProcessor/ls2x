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
#include "libavutil/opt.h"
#include "libavutil/frame.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
}

namespace ls2x
{
namespace libav
{

struct libavFunctions
{
	// handle
	ls2x::so *lavc, *lavf, *lavu, *sws, *swr;
	// versioning
	decltype(avcodec_version) *codecVersion; //lavc
	decltype(avformat_version) *formatVersion; // lavf
	decltype(avutil_version) *utilVersion; // lavu
	// registration
#if LIBAVFORMAT_VERSION_MAJOR < 58
	decltype(av_register_all) *registerAll; // lavf
#endif
#if LIBAVCODEC_VERSION_MAJOR < 58
	decltype(avcodec_register_all) *codecRegisterAll; // lavc
#endif
	// format
	decltype(avformat_open_input) *formatOpenInput; // lavf
	decltype(avformat_alloc_context) *formatAllocContext; // lavf
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
	decltype(av_seek_frame) *seekFrame; // lavf
	decltype(avio_alloc_context) *ioAllocContext; // lavf
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
	decltype(avcodec_parameters_to_context) *codecParametersToContext; // lavc
	decltype(av_packet_alloc) *packetAlloc; // lavc
	decltype(av_packet_rescale_ts) *packetRescaleTS; // lavc
	decltype(av_packet_unref) *packetUnref; // lavc
	decltype(avcodec_flush_buffers) *flushBuffers; // lavc
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
	decltype(av_get_default_channel_layout) *getDefaultChannelLayout; // lavu
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
