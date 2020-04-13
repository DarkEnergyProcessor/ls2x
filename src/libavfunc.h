// Video encoding
// Part of Live Simulator: 2 Extensions
// See copyright notice in LS2X main.cpp

// Please do not add include guard, this may included multiple times.

#ifndef LS2X_NOLOAD_VERSIONING
// versioning
LOAD_AVFUNC(lavc, codecVersion, avcodec_version);
LOAD_AVFUNC(lavf, formatVersion, avformat_version);
LOAD_AVFUNC(lavu, utilVersion, avutil_version);
#endif

// lavf
LOAD_AVFUNC(lavf, formatOpenInput, avformat_open_input);
LOAD_AVFUNC(lavf, formatAllocContext, avformat_alloc_context);
LOAD_AVFUNC(lavf, formatAllocOutputContext, avformat_alloc_output_context2);
LOAD_AVFUNC(lavf, formatFindStreamInfo, avformat_find_stream_info);
LOAD_AVFUNC(lavf, readPacket, av_read_frame);
LOAD_AVFUNC(lavf, ioOpen, avio_open);
LOAD_AVFUNC(lavf, ioClose, avio_closep);
LOAD_AVFUNC(lavf, interleavedWriteFrame, av_interleaved_write_frame);
LOAD_AVFUNC(lavf, writeTrailer, av_write_trailer);
LOAD_AVFUNC(lavf, formatWriteHeader, avformat_write_header);
LOAD_AVFUNC(lavf, formatFreeContext, avformat_free_context);
LOAD_AVFUNC(lavf, formatNewStream, avformat_new_stream);
LOAD_AVFUNC(lavf, dumpFormat, av_dump_format);
LOAD_AVFUNC(lavf, formatCloseInput, avformat_close_input);
LOAD_AVFUNC(lavf, seekFrame, av_seek_frame);
LOAD_AVFUNC(lavf, ioAllocContext, avio_alloc_context);
LOAD_AVFUNC(lavf, ioFreeContext, avio_context_free);
#if LIBAVFORMAT_VERSION_MAJOR < 58
LOAD_AVFUNC(lavf, registerAll, av_register_all);
#endif
// lavc
LOAD_AVFUNC(lavc, codecAllocContext, avcodec_alloc_context3);
LOAD_AVFUNC(lavc, codecFindEncoderByName, avcodec_find_encoder_by_name);
LOAD_AVFUNC(lavc, codecFindDecoder, avcodec_find_decoder);
LOAD_AVFUNC(lavc, codecOpen, avcodec_open2);
LOAD_AVFUNC(lavc, packetFree, av_packet_free);
LOAD_AVFUNC(lavc, codecFreeContext, avcodec_free_context);
LOAD_AVFUNC(lavc, packetAlloc, av_packet_alloc);
LOAD_AVFUNC(lavc, packetRescaleTS, av_packet_rescale_ts);
LOAD_AVFUNC(lavc, packetUnref, av_packet_unref);
LOAD_AVFUNC(lavc, flushBuffers, avcodec_flush_buffers);
LOAD_AVFUNC(lavc, codecSendPacket, avcodec_send_packet);
LOAD_AVFUNC(lavc, codecReceivePacket, avcodec_receive_packet);
LOAD_AVFUNC(lavc, codecSendFrame, avcodec_send_frame);
LOAD_AVFUNC(lavc, codecReceiveFrame, avcodec_receive_frame);
LOAD_AVFUNC(lavc, codecParametersFromContext, avcodec_parameters_from_context);
LOAD_AVFUNC(lavc, codecParametersToContext, avcodec_parameters_to_context);
#if LIBAVCODEC_VERSION_MAJOR < 58
LOAD_AVFUNC(lavc, codecRegisterAll, avcodec_register_all);
#endif
// lavu
LOAD_AVFUNC(lavu, frameAlloc, av_frame_alloc);
LOAD_AVFUNC(lavu, frameGetBuffer, av_frame_get_buffer);
LOAD_AVFUNC(lavu, frameMakeWritable, av_frame_make_writable);
LOAD_AVFUNC(lavu, frameFree, av_frame_free);
LOAD_AVFUNC(lavu, frameUnref, av_frame_unref);
LOAD_AVFUNC(lavu, optSet, av_opt_set);
LOAD_AVFUNC(lavu, dictCount, av_dict_count);
LOAD_AVFUNC(lavu, dictGet, av_dict_get);
LOAD_AVFUNC(lavu, strdup, av_strdup);
LOAD_AVFUNC(lavu, malloc, av_mallocz);
LOAD_AVFUNC(lavu, free, av_free);
LOAD_AVFUNC(lavu, strerror, av_strerror);
LOAD_AVFUNC(lavu, logSetLevel, av_log_set_level);
LOAD_AVFUNC(lavu, getDefaultChannelLayout, av_get_default_channel_layout);
// sws
LOAD_AVFUNC(sws, swsGetContext, sws_getContext);
LOAD_AVFUNC(sws, swsScale, sws_scale);
LOAD_AVFUNC(sws, swsFreeContext, sws_freeContext);
// swr
LOAD_AVFUNC(swr, swrAllocSetOpts, swr_alloc_set_opts);
LOAD_AVFUNC(swr, swrInit, swr_init);
LOAD_AVFUNC(swr, swrConvertFrame, swr_convert_frame);
LOAD_AVFUNC(swr, swrGetDelay, swr_get_delay);
LOAD_AVFUNC(swr, swrConvert, swr_convert);
LOAD_AVFUNC(swr, swrFree, swr_free);
