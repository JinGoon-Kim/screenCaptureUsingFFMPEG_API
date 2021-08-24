#ifndef __SCREEN_RECORDER_H__
#define __SCREEN_RECORDER_H__

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavfilter/avfilter.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

#include <stdio.h>
#include <windows.h>
#include <iostream>
#include <list>

#include "thread_safe_queue.hpp"
#include "thread_safe_flag.h"

class ScreenRecorder {
public:
	ScreenRecorder(AVCodecContext* encoder_codec_context);
	~ScreenRecorder();
	void Start();
	void Stop();
	std::string GetQueueStatus();
private:
	// initialize decoder
	// initialize scaler
	// initialize encoder
	int init(AVCodecContext* encoder_codec_context);

	void DecodeVideo();
	void ScaleVideo();
	void EncodeVideo();

	void DecodeAudio();
	void ScaleAudio();
	void EncodeAudio();

	void Writer();
	
	std::list<std::shared_ptr<std::thread>> threads_;

	ThreadSafeFlag decoder_flag_;	// main,	DecodeVideo	쓰레드에서 접근
	ThreadSafeFlag scaler_flag_;	// DecodeVideo, ScaleVideo	쓰레드에서 접근
	ThreadSafeFlag encoder_flag_;	// ScaleVideo,	EncodeVideo	쓰레드에서 접근
	ThreadSafeFlag writer_flag_;	// EncodeVideo, Writer	쓰레드에서 접근
	
	// decoder
	AVFormatContext *video_input_format_context_ = NULL;
	AVCodecContext *video_decoder_codec_context_ = NULL;

	AVFormatContext *audio_input_format_context_ = NULL;
	AVCodecContext *audio_decoder_codec_context_ = NULL;

	// scaler
	SwsContext* sws_context_;
	SwrContext* swr_context_;

	// encoder
	AVCodecContext* video_encoder_codec_context_ = NULL;

	AVCodecContext* audio_encoder_codec_context_ = NULL;

	std::string output_filename_;

	// writer
	AVFormatContext* output_format_context_ = NULL;
	AVStream* out_video_stream_;
	AVStream* out_audio_stream_;

	// data queue
	ThreadSafeQueue<AVFrame *> decoded_frame_queue, scaled_frame_queue;
	ThreadSafeQueue<AVPacket *> output_packet_queue;

	
};

#endif