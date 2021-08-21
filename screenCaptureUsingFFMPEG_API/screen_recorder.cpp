#include "screen_recorder.h"

#include <functional>
#include <fstream>
#include <string>

ScreenRecorder::ScreenRecorder(AVCodecContext* encoder_codec_context)
	: output_filename_("output.mp4") {
	init(encoder_codec_context);
}

int ScreenRecorder::init(AVCodecContext* encoder_codec_context) {

	// video decoder init
	AVInputFormat* input_format = av_find_input_format("gdigrab");

	AVDictionary* input_options = NULL;
	av_dict_set(&input_options, "analyzeduration", "100000000", NULL);
	av_dict_set(&input_options, "probesize", "100000000", NULL);

	int ret;
	ret = avformat_open_input(&video_input_format_context_, "desktop", input_format, &input_options);
	if (ret != 0)
	{
		std::cout << "\nerror in opening input device";
		return ret;
	}
	ret = avformat_find_stream_info(video_input_format_context_, &input_options);
	if (ret < 0)
	{
		std::cout << "\nunable to find the stream information";
		return ret;
	}

	AVCodec* decoder_codec;

	int video_stream_index = av_find_best_stream(video_input_format_context_, AVMEDIA_TYPE_VIDEO, -1, -1, &decoder_codec, 0);
	if (video_stream_index == -1)
	{
		std::cout << "\nunable to find the video stream index. (-1)";
		return -1;
	}


	video_decoder_codec_context_ = avcodec_alloc_context3(decoder_codec);
	if (!video_decoder_codec_context_) {
		std::cout << "Could not allocate video decoder context\n";
		return -1;
	}

	avcodec_parameters_to_context(video_decoder_codec_context_, video_input_format_context_->streams[video_stream_index]->codecpar);

	ret = avcodec_open2(video_decoder_codec_context_, decoder_codec, NULL);
	if (ret < 0) {
		std::cout << "Could not open decoder\n";
		return ret;
	}

	// audio decoder init
	AVInputFormat* audio_input_format = av_find_input_format("dshow");
	ret = avformat_open_input(&audio_input_format_context_, "Audio=마이크(Realtek(R) Audio)", audio_input_format, NULL);
	if (ret != 0)
	{
		av_log(NULL, AV_LOG_ERROR, "Could not input format");
	}

	ret = avformat_find_stream_info(audio_input_format_context_, NULL);
	if (ret < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "Could not find stream");
	}

	AVCodec* input_audio_codec;

	int audio_stream_index = av_find_best_stream(audio_input_format_context_, AVMEDIA_TYPE_AUDIO, -1, -1, &input_audio_codec, 0);
	if (audio_stream_index < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "Could not find best audio stream\n");
	}

	if (avcodec_alloc_context3(input_audio_codec) == NULL)
	{
		av_log(NULL, AV_LOG_ERROR, "Could not alloc audio codec context\n");
	}

	avcodec_parameters_to_context(audio_decoder_codec_context_, audio_input_format_context_->streams[audio_stream_index]->codecpar);

	if (avcodec_open2(audio_decoder_codec_context_, decoder_codec, NULL) < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "Could not alloc audio codec context\n");
	}


	// video encoder init
	AVFormatContext* video_output_format_context = NULL;
	avformat_alloc_output_context2(&video_output_format_context, NULL, NULL, output_filename_.c_str());
	if (!video_output_format_context)
	{
		std::cout << "\nerror in allocating av format output context";
		return -1;
	}
	AVCodec* video_encoder_codec = avcodec_find_encoder(AV_CODEC_ID_MPEG4);
	if (!video_encoder_codec) {
		std::cout << "Codec AV_CODEC_ID_H264 not found\n";
		return -1;
	}
	video_encoder_codec_context_ = avcodec_alloc_context3(video_encoder_codec);
	if (!video_encoder_codec_context_) {
		std::cout << "Could not allocate video pEncoderCodec context\n";
		return -1;
	}
	video_encoder_codec_context_->bit_rate		= encoder_codec_context->bit_rate;
	video_encoder_codec_context_->width			= encoder_codec_context->width;
	video_encoder_codec_context_->height			= encoder_codec_context->height;
	video_encoder_codec_context_->pix_fmt			= encoder_codec_context->pix_fmt;
	video_encoder_codec_context_->time_base.num	= encoder_codec_context->time_base.num;
	video_encoder_codec_context_->time_base.den	= encoder_codec_context->time_base.den;
	video_encoder_codec_context_->framerate.num	= encoder_codec_context->framerate.num;
	video_encoder_codec_context_->framerate.den	= encoder_codec_context->framerate.den;
	video_encoder_codec_context_->gop_size		= encoder_codec_context->gop_size;
	video_encoder_codec_context_->max_b_frames	= encoder_codec_context->max_b_frames;

	ret = avcodec_open2(video_encoder_codec_context_, video_encoder_codec, NULL);
	if (ret < 0) {
		std::cout << "Could not open pEncoderCodec\n";
		return ret;
	}

	// audio encoder init

	AVFormatContext* audio_output_format_context = NULL;
	avformat_alloc_output_context2(&audio_output_format_context, NULL, NULL, output_filename_.c_str());
	if (!audio_output_format_context)
	{
		av_log(NULL, AV_LOG_ERROR, "Empty format context\n");
	}

	AVCodec* audio_encoder_codec_ = avcodec_find_encoder(AV_CODEC_ID_AAC);
	if (!audio_encoder_codec_)
	{
		av_log(NULL, AV_LOG_ERROR, "encoder codec not found\n");
	}

	audio_encoder_codec_context_ = avcodec_alloc_context3(audio_encoder_codec_);
	if (!audio_decoder_codec_context_)
	{
		av_log(NULL, AV_LOG_ERROR, "Could not allocate audio codec context\n");
	}

	audio_encoder_codec_context_->bit_rate = 64000;
	audio_encoder_codec_context_->sample_fmt = AV_SAMPLE_FMT_S16;

	/*------------------------*/
	const enum AVSampleFormat* p = audio_encoder_codec_->sample_fmts;
	while (*p != AV_SAMPLE_FMT_NONE)
	{
		if (*p == audio_encoder_codec_context_->sample_fmt)
		{
			ret = 1;
			break;
		}
		p++;
	}

	if (ret <= 0)
	{
		fprintf(stderr, "Encoder does not support sample format %s",
			av_get_sample_fmt_name(audio_encoder_codec_context_->sample_fmt));
		exit(1);
	}

	audio_encoder_codec_context_->sample_rate = 0;
	if (!audio_encoder_codec_->supported_samplerates)
	{
		audio_encoder_codec_context_->sample_rate = 44100;
	}
	else
	{
		const int* p = audio_encoder_codec_->supported_samplerates;
		while (*p)
		{
			if (!audio_encoder_codec_context_->sample_rate || abs(44100 - *p) < abs(44100 - audio_encoder_codec_context_->sample_rate))
				audio_encoder_codec_context_->sample_rate = *p;
			p++;
		}
	}

	if (!audio_encoder_codec_->channel_layouts)
		audio_encoder_codec_context_->channel_layout = AV_CH_LAYOUT_STEREO;
	else {
		int best_nb_channels = 0;
		const uint64_t* p = audio_encoder_codec_->channel_layouts;
		while (*p) {
			int nb_channels = av_get_channel_layout_nb_channels(*p);
			if (nb_channels > best_nb_channels) {
				audio_encoder_codec_context_->channel_layout = *p;
				best_nb_channels = nb_channels;
			}
			p++;
		}
	}

	audio_encoder_codec_context_->channels = av_get_channel_layout_nb_channels(audio_encoder_codec_context_->channel_layout);

	if (avcodec_open2(audio_encoder_codec_context_, audio_encoder_codec_, NULL) < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "Could not open audio encoder\n");
	}
	/*------------------------*/

	// scaler init
	sws_context_ = sws_getContext(
		video_decoder_codec_context_->width, video_decoder_codec_context_->height, video_decoder_codec_context_->pix_fmt,
		video_encoder_codec_context_->width, video_encoder_codec_context_->height, video_encoder_codec_context_->pix_fmt,
		SWS_BICUBIC, NULL, NULL, NULL);

	return 0;
}

ScreenRecorder::~ScreenRecorder() {
	if (video_encoder_codec_context_ != NULL)
		avcodec_free_context(&video_encoder_codec_context_);
	if (video_decoder_codec_context_ != NULL)
		avcodec_free_context(&video_decoder_codec_context_);
	
	ThreadSafeQueue<AVFrame*> decoded_frame_queue, scaled_frame_queue;
	ThreadSafeQueue<AVPacket*> output_packet_queue;

	AVFrame* delete_frame;
	while ((delete_frame = decoded_frame_queue.pop()) != NULL)
		av_frame_free(&delete_frame);
	while ((delete_frame = scaled_frame_queue.pop()) != NULL)
		av_frame_free(&delete_frame);
	AVPacket* delete_packet;
	while ((delete_packet = output_packet_queue.pop()) != NULL)
		av_packet_free(&delete_packet);
}

void ScreenRecorder::Start() {
	Stop();
	decoder_flag_.set(true);
	scaler_flag_.set(true);
	encoder_flag_.set(true); 
	writer_flag_.set(true);

	// List threads_에 인자를 하나 넣음, 그 인자가 Thread (DecodeVideo, ScaleVideo, EncodeVideo, Writer)
	threads_.push_back(std::make_shared<std::thread>(std::bind(&ScreenRecorder::DecodeVideo, this)));
	threads_.push_back(std::make_shared<std::thread>(std::bind(&ScreenRecorder::ScaleVideo, this)));
	threads_.push_back(std::make_shared<std::thread>(std::bind(&ScreenRecorder::EncodeVideo, this)));
	
	threads_.push_back(std::make_shared<std::thread>(std::bind(&ScreenRecorder::DecodeAudio, this)));
	threads_.push_back(std::make_shared<std::thread>(std::bind(&ScreenRecorder::ScaleAudio, this)));
	threads_.push_back(std::make_shared<std::thread>(std::bind(&ScreenRecorder::EncodeAudio, this)));
	
	threads_.push_back(std::make_shared<std::thread>(std::bind(&ScreenRecorder::Writer, this)));
}

void ScreenRecorder::Stop() {
	decoder_flag_.set(false);
	for (auto i = threads_.begin(); i != threads_.end(); ++i) {
		if ((*i)->joinable()) {
			(*i)->join();
		}
	}
}

std::string ScreenRecorder::GetQueueStatus() {
	return std::to_string(decoded_frame_queue.size()) + " " + std::to_string(scaled_frame_queue.size()) + " " + std::to_string(output_packet_queue.size());
}

void ScreenRecorder::DecodeVideo() {

	AVPacket* input_packet = av_packet_alloc();
	if (!input_packet) {
		std::cout << "Could not allocate video packet\n";
		exit(1);
	}
	while (decoder_flag_.get() && av_read_frame(video_input_format_context_, input_packet) == 0) {
		int ret;
		ret = avcodec_send_packet(video_decoder_codec_context_, input_packet);
		if (ret < 0) {
			std::cout << "Error sending a packet for decoding\n";
			exit(1);
		}
		while (ret >= 0) {
			AVFrame* decoded_frame = av_frame_alloc();
			if (!decoded_frame) {
				std::cout << "Could not allocate video pFrame\n";
				exit(1);
			}
			ret = avcodec_receive_frame(video_decoder_codec_context_, decoded_frame);
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
				break;
			else if (ret < 0) {
				std::cout << "Error during decoding\n";
				exit(1);
			}
			decoded_frame_queue.push(decoded_frame);
		}
	}

	/* flush the decoder */
	int ret;
	ret = avcodec_send_packet(video_decoder_codec_context_, NULL);
	if (ret < 0) {
		std::cout << "Error sending a packet for decoding\n";
		exit(1);
	}
	while (ret >= 0) {
		AVFrame* decoded_frame = av_frame_alloc();
		if (!decoded_frame) {
			std::cout << "Could not allocate video pFrame\n";
			exit(1);
		}
		ret = avcodec_receive_frame(video_decoder_codec_context_, decoded_frame);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			break;
		else if (ret < 0) {
			std::cout << "Error during decoding\n";
			exit(1);
		}
		decoded_frame_queue.push(decoded_frame);
	}

	scaler_flag_.set(false);
	av_packet_free(&input_packet);
	return;
}
void ScreenRecorder::ScaleVideo() {
	// Get Original Frame
	AVFrame* decoded_frame = NULL;
	while (1) {
		if (!(decoded_frame = decoded_frame_queue.pop())) {
			Sleep(100);
			if (scaler_flag_.get() == false)
				break;
			continue;
		}
		// Alloc AVFrame
		AVFrame* scaled_frame = av_frame_alloc();
		int nbytes = av_image_get_buffer_size(video_encoder_codec_context_->pix_fmt, video_encoder_codec_context_->width, video_encoder_codec_context_->height, 32);
		uint8_t* video_outbuf = (uint8_t*)av_malloc(nbytes);
		av_image_fill_arrays(scaled_frame->data, scaled_frame->linesize, video_outbuf, video_encoder_codec_context_->pix_fmt, video_encoder_codec_context_->width, video_encoder_codec_context_->height, 1);

		// Convert Origianl Frame to Scaled Frame
		sws_scale(sws_context_, decoded_frame->data, decoded_frame->linesize,
			0, video_decoder_codec_context_->height, scaled_frame->data, scaled_frame->linesize);
		// Push Scaled Frame to Queue
		scaled_frame_queue.push(scaled_frame);
		
		av_frame_free(&decoded_frame);

	}
	encoder_flag_.set(false);
}
void ScreenRecorder::EncodeVideo() {
	AVFrame* scaled_frame = av_frame_alloc();

	int ret;
	while (1) {
		if (!(scaled_frame = scaled_frame_queue.pop())) {
			Sleep(100);
			if (encoder_flag_.get() == false)
				break;
			continue;
		}
		scaled_frame->format = video_encoder_codec_context_->pix_fmt;
		scaled_frame->width = video_encoder_codec_context_->width;
		scaled_frame->height = video_encoder_codec_context_->height;

		ret = avcodec_send_frame(video_encoder_codec_context_, scaled_frame);
		if (ret < 0) {
			std::cout << "Error sending a frame for encoding\n";
			exit(1);
		}
		while (ret >= 0) {
			AVPacket* output_packet = av_packet_alloc();
			if (!output_packet)
				exit(1);
			ret = avcodec_receive_packet(video_encoder_codec_context_, output_packet);
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
				break;
			else if (ret < 0) {
				std::cout << "Error during encoding\n";
				exit(1);
			}
			// send write packet queue
			output_packet_queue.push(output_packet);
		}
		av_frame_free(&scaled_frame);
	}
	/* flush the encoder */
	ret = avcodec_send_frame(video_encoder_codec_context_, NULL);
	if (ret < 0) {
		std::cout << "Error sending a frame for encoding\n";
		exit(1);
	}
	while (ret >= 0) {
		AVPacket* output_packet = av_packet_alloc();
		if (!output_packet)
			exit(1);
		ret = avcodec_receive_packet(video_encoder_codec_context_, output_packet);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			break;
		else if (ret < 0) {
			std::cout << "Error during encoding\n";
			exit(1);
		}
		// send write packet queue
		output_packet_queue.push(output_packet);
	}

	writer_flag_.set(false);

	return;
}
void ScreenRecorder::DecodeAudio()
{
	

	
}
void ScreenRecorder::ScaleAudio()
{

}
void ScreenRecorder::EncodeAudio()
{

}
void ScreenRecorder::Writer() {
	AVPacket* output_packet;
	std::ofstream out_stream;
	out_stream.open(output_filename_.c_str(), std::ios::binary);
	while (1) {
		if (!(output_packet = output_packet_queue.pop())) {
			Sleep(100);
			if (writer_flag_.get() == false)
				break;
			continue;
		}
		out_stream.write(reinterpret_cast<const char*>(output_packet->data), output_packet->size);
	}
	out_stream.close();

	return;
}