#include "screen_recorder.h"

#include <chrono>
#include <thread>

int main()
{
	avdevice_register_all();

	AVCodecContext video_encoder_codec_context;

	video_encoder_codec_context.bit_rate = 400000;
	video_encoder_codec_context.width = 1920;
	video_encoder_codec_context.height = 1080;
	video_encoder_codec_context.pix_fmt = AV_PIX_FMT_YUV420P;
	video_encoder_codec_context.time_base.num = 1;
	video_encoder_codec_context.time_base.den = 30;
	video_encoder_codec_context.framerate.num = 30;
	video_encoder_codec_context.framerate.den = 1;
	video_encoder_codec_context.gop_size = 3;
	video_encoder_codec_context.max_b_frames = 2;

	ScreenRecorder desktop_capture(&video_encoder_codec_context);

	desktop_capture.Start();
	std::this_thread::sleep_for(std::chrono::seconds(100));
	desktop_capture.Stop();
	
	return 0;
}