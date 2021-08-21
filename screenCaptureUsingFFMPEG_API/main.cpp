#include "screen_recorder.h"

int main()
{
	avdevice_register_all();
	AVCodecContext encoder_codec_context;

	encoder_codec_context.bit_rate = 400000;
	encoder_codec_context.width = 1920;
	encoder_codec_context.height = 1080;
	encoder_codec_context.pix_fmt = AV_PIX_FMT_YUV420P;
	encoder_codec_context.time_base.num = 1;
	encoder_codec_context.time_base.den = 30;
	encoder_codec_context.framerate.num = 30;
	encoder_codec_context.framerate.den = 1;
	encoder_codec_context.gop_size = 3;
	encoder_codec_context.max_b_frames = 2;

	ScreenRecorder desktop_capture(&encoder_codec_context);
	desktop_capture.Start();	// Thread Start [DecodeVideo, ScaleVideo, EncodeVideo, Writer]

	int progeress = 0;

	while (++progeress <= 20) {
		Sleep(100);
		std::cout << desktop_capture.GetQueueStatus() << "\n";
	}

	desktop_capture.Stop();

	return 0;
}