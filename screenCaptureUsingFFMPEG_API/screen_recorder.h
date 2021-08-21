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
	AVFormatContext *input_video_format_context_ = NULL;
	AVCodecContext *decoder_video_codec_context_ = NULL;

	AVFormatContext *input_audio_format_context_ = NULL;
	AVCodecContext *decoder_audio_codec_context_ = NULL;

	// scaler
	SwsContext* sws_context_;

	// encoder
	AVCodecContext* encoder_codec_context_ = NULL;

	std::string output_filename_;

	// data queue
	ThreadSafeQueue<AVFrame *> decoded_frame_queue, scaled_frame_queue;
	ThreadSafeQueue<AVPacket *> output_packet_queue;

	
};

#endif