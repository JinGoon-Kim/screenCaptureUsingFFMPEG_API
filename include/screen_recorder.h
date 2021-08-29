#ifndef __SCREEN_RECORDER_H__
#define __SCREEN_RECORDER_H__

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavfilter/avfilter.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavutil/audio_fifo.h>
#include <libavutil/avassert.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

#include <stdio.h>
#include <windows.h>
#include <iostream>
#include <list>

#include "thread_safe_data.hpp"

class ScreenRecorder {
public:
	ScreenRecorder() {}
	ScreenRecorder(AVCodecContext* video_encoder_codec_context);
	~ScreenRecorder();
	void Start();
	void Stop();
	//std::string GetQueueStatus();
private:
	// initialize decoder
	// initialize scaler
	// initialize encoder
	int InitVideo(AVCodecContext* video_encoder_codec_context);
	int InitAudio();
	
	int InitWriter();

	void DecodeVideo();
	void ScaleVideo();
	void EncodeVideo();
	void DecodeAudio();
	void ResampleAudio();
	void EncodeAudio();
	void Writer();

	std::list<std::shared_ptr<std::thread>> threads_;

	ThreadSafeFlag video_decoder_flag_;	// main,	Decoder	쓰레드에서 접근
	ThreadSafeFlag video_scaler_flag_;	// Decoder, Scaler	쓰레드에서 접근
	ThreadSafeFlag video_encoder_flag_;	// Scaler,	Encoder	쓰레드에서 접근
	ThreadSafeFlag audio_decoder_flag_;	// main,	Decoder	쓰레드에서 접근
	ThreadSafeFlag audio_scaler_flag_;	// Decoder, Scaler	쓰레드에서 접근
	ThreadSafeFlag audio_encoder_flag_;	// Scaler,	Encoder	쓰레드에서 접근
	ThreadSafeFlag writer_flag_;	// Encoder, Writer	쓰레드에서 접근
	
	// decoder
	AVFormatContext* input_video_format_context_ = NULL;
	AVCodecContext* video_decoder_codec_context_ = NULL;
	AVFormatContext* input_audio_format_context_ = NULL;
	AVCodecContext* audio_decoder_codec_context_ = NULL;

	// scaler
	SwsContext* sws_context_ = NULL;
	SwrContext* swr_context_ = NULL;


	// encoder
	AVCodecContext* video_encoder_codec_context_ = NULL;
	AVCodecContext* audio_encoder_codec_context_ = NULL;

	// writer
	AVFormatContext* output_format_context_ = NULL;
	AVStream* out_video_stream_ = NULL;
	AVStream* out_audio_stream_ = NULL;

	std::string output_filename_;

	// data queue
	ThreadSafeQueue<AVFrame* > video_decoded_frame_queue_, video_scaled_frame_queue_;
	ThreadSafeQueue<AVFrame* > audio_decoded_frame_queue_, audio_resampled_frame_queue_;
	ThreadSafeQueue<AVPacket *> output_packet_queue_;

	
};

#endif