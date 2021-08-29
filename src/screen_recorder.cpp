#include "screen_recorder.h"

#include <stdio.h>
#include <time.h>

#include <iomanip>
#include <functional>
#include <fstream>

ScreenRecorder::ScreenRecorder(AVCodecContext* video_encoder_codec_context)
	: output_filename_("output.mp4") {
	InitVideo(video_encoder_codec_context);
	InitAudio();
	InitWriter();
}
int ScreenRecorder::InitWriter(){
	// writer init
	avformat_alloc_output_context2(&output_format_context_, NULL, NULL, output_filename_.c_str());
	if (!output_format_context_)
	{
		std::cout << "\nerror in allocating av format output context";
		return -1;
	}
	out_video_stream_ = avformat_new_stream(output_format_context_, NULL);
	out_video_stream_->id = output_format_context_->nb_streams - 1;
	out_video_stream_->time_base = video_encoder_codec_context_->time_base;
	int ret = avcodec_parameters_from_context(out_video_stream_->codecpar, video_encoder_codec_context_);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "Failed to copy encoder parameters to output stream\n");
		return ret;
	}

	out_audio_stream_ = avformat_new_stream(output_format_context_, NULL);
	out_audio_stream_->time_base = audio_encoder_codec_context_->time_base;
	out_audio_stream_->id = output_format_context_->nb_streams - 1;
	ret = avcodec_parameters_from_context(out_audio_stream_->codecpar, audio_encoder_codec_context_);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "Failed to copy encoder parameters to output stream\n");
		return ret;
	}

	if (output_format_context_->oformat->flags & AVFMT_GLOBALHEADER)
	{
		output_format_context_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	}

	/* create empty video file */
	if (!(output_format_context_->flags & AVFMT_NOFILE))
	{
		if (avio_open2(&output_format_context_->pb, output_filename_.c_str(), AVIO_FLAG_WRITE, NULL, NULL) < 0)
		{
			std::cout << "\nerror in creating the video file";
			exit(1);
		}
	}

	if (!output_format_context_->nb_streams)
	{
		std::cout << "\noutput file dose not contain any stream";
		exit(1);
	}

	/* imp: mp4 container or some advanced container file required header information*/
	ret = avformat_write_header(output_format_context_, NULL);
	if (ret < 0)
	{
		std::cout << "\nerror in writing the header context";
		exit(1);
	}
	return 0;
}

int ScreenRecorder::InitVideo(AVCodecContext* video_encoder_codec_context) {

	AVDictionary* options = NULL;
	av_dict_set(&options, "framerate", "30", 0);
	av_dict_set(&options, "probesize", "100000000", 0);
	av_dict_set(&options, "rtbufsize", "100000000", 0);
	
	av_dict_set(&options,"offset_x","20",0);	//The distance from the left edge of the screen or desktop
    av_dict_set(&options,"offset_y","40",0);	//The distance from the top edge of the screen or desktop
    av_dict_set(&options,"video_size","640x480",0);	//Video frame size. The default is to capture the full screen
	
	// decoder
	int ret;
	AVInputFormat* video_input_format_ = av_find_input_format("gdigrab");
	ret = avformat_open_input(&input_video_format_context_, "desktop", video_input_format_, &options);
	if (ret != 0)
	{
		std::cout << "\nerror in opening input device";
		return ret;
	}
	ret = avformat_find_stream_info(input_video_format_context_, &options);
	if (ret < 0)
	{
		std::cout << "\nunable to find the stream information";
		return ret;
	}
	int out_video_stream_index = av_find_best_stream(input_video_format_context_, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	if (out_video_stream_index == -1)
	{
		std::cout << "\nunable to find the video stream index. (-1)";
		return -1;
	}

	AVCodecParameters* video_parameter_ = input_video_format_context_->streams[out_video_stream_index]->codecpar;
	AVCodec* decoder_codec_ = avcodec_find_decoder(video_parameter_->codec_id);
	if (!decoder_codec_) {
		std::cout << "codec not found\n";
		return -1;
	}
	video_decoder_codec_context_ = avcodec_alloc_context3(decoder_codec_);
	if (!video_decoder_codec_context_) {
		std::cout << "Could not allocate video decoder context\n";
		return -1;
	}

	avcodec_parameters_to_context(video_decoder_codec_context_, video_parameter_);

	ret = avcodec_open2(video_decoder_codec_context_, decoder_codec_, NULL);
	if (ret < 0) {
		std::cout << "Could not open decoder\n";
		return ret;
	}

	// encoder
	AVCodec* video_encoder_codec_ = avcodec_find_encoder(AV_CODEC_ID_MPEG4);
	if (!video_encoder_codec_) {
		std::cout << "Codec AV_CODEC_ID_MPEG4 not found\n";
		return -1;
	}
	video_encoder_codec_context_ = avcodec_alloc_context3(video_encoder_codec_);
	if (!video_encoder_codec_context_) {
		std::cout << "Could not allocate video video encoder context\n";
		return -1;
	}

	video_encoder_codec_context_->bit_rate	  = video_encoder_codec_context->bit_rate;
	video_encoder_codec_context_->width		  = video_encoder_codec_context->width;
	video_encoder_codec_context_->height		  = video_encoder_codec_context->height;
	video_encoder_codec_context_->pix_fmt		  = video_encoder_codec_context->pix_fmt;
	video_encoder_codec_context_->time_base.num = video_encoder_codec_context->time_base.num;
	video_encoder_codec_context_->time_base.den = video_encoder_codec_context->time_base.den;
	video_encoder_codec_context_->framerate.num = video_encoder_codec_context->framerate.num;
	video_encoder_codec_context_->framerate.den = video_encoder_codec_context->framerate.den;
	video_encoder_codec_context_->gop_size	  = video_encoder_codec_context->gop_size;
	video_encoder_codec_context_->max_b_frames  = video_encoder_codec_context->max_b_frames;
	
	ret = avcodec_open2(video_encoder_codec_context_, video_encoder_codec_, &options);
	if (ret < 0) {
		std::cout << "Could not open video encoder\n";
		return ret;
	}

	// scaler init
	sws_context_ = sws_getContext(
		video_decoder_codec_context_->width, video_decoder_codec_context_->height, video_decoder_codec_context_->pix_fmt,
		video_encoder_codec_context_->width, video_encoder_codec_context_->height, video_encoder_codec_context_->pix_fmt,
		SWS_BICUBIC, NULL, NULL, NULL);

	av_dict_free(&options);

	return 0;
}

int ScreenRecorder::InitAudio() {

	AVDictionary* options = NULL;
	//av_dict_set(&options, "samplerate", "44100", 0);

	// decode init (audio)
	AVInputFormat* audio_input_format_ = av_find_input_format("dshow");
	int ret = avformat_open_input(&input_audio_format_context_, "audio=Line In(High Definition Audio Device)", audio_input_format_, &options);
	if (ret != 0)
	{
		std::cout << "\nerror in opening input device";
		return ret;
	}
	ret = avformat_find_stream_info(input_audio_format_context_, &options);
	if (ret < 0)
	{
		std::cout << "\nunable to find the stream information";
		return ret;
	}
	int out_audio_stream_index = av_find_best_stream(input_audio_format_context_, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
	if (out_audio_stream_index == -1)
	{
		std::cout << "\nunable to find the video stream index. (-1)";
		return -1;
	}

	AVCodecParameters* audio_parameter_ = input_audio_format_context_->streams[out_audio_stream_index]->codecpar;
	AVCodec* audio_decoder_codec_ = avcodec_find_decoder(audio_parameter_->codec_id);
	if (!audio_decoder_codec_) {
		std::cout << "codec not found\n";
		return -1;
	}
	audio_decoder_codec_context_ = avcodec_alloc_context3(audio_decoder_codec_);
	if (!audio_decoder_codec_context_) {
		std::cout << "Could not allocate video decoder context\n";
		return -1;
	}

	avcodec_parameters_to_context(audio_decoder_codec_context_, audio_parameter_);
	audio_decoder_codec_context_->channel_layout = av_get_default_channel_layout(audio_decoder_codec_context_->channels);

	ret = avcodec_open2(audio_decoder_codec_context_, audio_decoder_codec_, NULL);
	if (ret < 0) {
		std::cout << "Could not open decoder\n";
		return ret;
	}

	AVCodec* audio_encoder_codec_ = avcodec_find_encoder(AV_CODEC_ID_AAC);
	if (!audio_encoder_codec_) {
		std::cout << "Codec AV_CODEC_ID_MP2 not found\n";
		return -1;
	}
	audio_encoder_codec_context_ = avcodec_alloc_context3(audio_encoder_codec_);
	if (!audio_encoder_codec_context_) {
		std::cout << "Could not allocate audio audio encoder context\n";
		return -1;
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
	audio_encoder_codec_context_->sample_rate = 0;
	if (!audio_encoder_codec_->supported_samplerates)
		audio_encoder_codec_context_->sample_rate = 44100;
	else {
		const int* p = audio_encoder_codec_->supported_samplerates;
		while (*p) {
			if (!audio_encoder_codec_context_->sample_rate || abs(44100 - *p) < abs(44100 - audio_encoder_codec_context_->sample_rate))
				audio_encoder_codec_context_->sample_rate = *p;
			p++;
		}
	}
	audio_encoder_codec_context_->bit_rate = 64000;
	audio_encoder_codec_context_->sample_fmt = audio_encoder_codec_->sample_fmts[0];
	const enum AVSampleFormat* p = audio_encoder_codec_->sample_fmts;
	while (*p != AV_SAMPLE_FMT_NONE) {
		if (*p == audio_encoder_codec_context_->sample_fmt) {
			ret = 1;
			break;
		}
		p++;
	}
	if (ret <= 0) {
		fprintf(stderr, "Encoder does not support sample format %s",
			av_get_sample_fmt_name(audio_encoder_codec_context_->sample_fmt));
		exit(1);
	}


	audio_encoder_codec_context_->time_base.num = 1;
	audio_encoder_codec_context_->time_base.den = audio_encoder_codec_context_->sample_rate;


	ret = avcodec_open2(audio_encoder_codec_context_, audio_encoder_codec_, NULL);
	if (ret < 0) {
		std::cout << "Could not open audio encoder\n";
		return ret;
	}

	// scaler init (audio)
	/* create resampler context */
	swr_context_ = swr_alloc();
	if (!swr_context_) {
		fprintf(stderr, "Could not allocate resampler context\n");
		exit(1);
	}
	/* Set options */
	av_opt_set_int(swr_context_, "in_channel_layout", audio_decoder_codec_context_->channel_layout, 0);
	av_opt_set_int(swr_context_, "in_sample_rate", audio_decoder_codec_context_->sample_rate, 0);
	av_opt_set_sample_fmt(swr_context_, "in_sample_fmt", audio_decoder_codec_context_->sample_fmt, 0);

	av_opt_set_int(swr_context_, "out_channel_layout", audio_encoder_codec_context_->channel_layout, 0);
	av_opt_set_int(swr_context_, "out_sample_rate", audio_encoder_codec_context_->sample_rate, 0);
	av_opt_set_sample_fmt(swr_context_, "out_sample_fmt", audio_encoder_codec_context_->sample_fmt, 0);
	/* initialize the resampling context */
	if ((ret = swr_init(swr_context_)) < 0) {
		fprintf(stderr, "Failed to initialize the resampling context\n");
		exit(1);
	}

	av_dict_free(&options);

	return 0;
}

ScreenRecorder::~ScreenRecorder() {
	if (video_decoder_codec_context_ != NULL)
		avcodec_free_context(&video_encoder_codec_context_);
	if (video_decoder_codec_context_ != NULL)
		avcodec_free_context(&video_decoder_codec_context_);

	AVFrame* delete_frame;
	while ((delete_frame = video_decoded_frame_queue_.Pop()) != NULL)
		av_frame_free(&delete_frame);
	while ((delete_frame = video_scaled_frame_queue_.Pop()) != NULL)
		av_frame_free(&delete_frame);
	while ((delete_frame = audio_decoded_frame_queue_.Pop()) != NULL)
		av_frame_free(&delete_frame);
	AVPacket* delete_packet;
	while ((delete_packet = output_packet_queue_.Pop()) != NULL)
		av_packet_free(&delete_packet);

	sws_freeContext(sws_context_);
	swr_free(&swr_context_);
}

void ScreenRecorder::Start() {
	Stop();
	video_decoder_flag_.Set(true);
	video_scaler_flag_.Set(true);
	video_encoder_flag_.Set(true); 
	audio_decoder_flag_.Set(true);
	audio_scaler_flag_.Set(true);
	audio_encoder_flag_.Set(true);
	writer_flag_.Set(true);

	threads_.push_back(std::make_shared<std::thread>(std::bind(&ScreenRecorder::DecodeVideo, this)));
	threads_.push_back(std::make_shared<std::thread>(std::bind(&ScreenRecorder::ScaleVideo, this)));
	threads_.push_back(std::make_shared<std::thread>(std::bind(&ScreenRecorder::EncodeVideo, this)));
	threads_.push_back(std::make_shared<std::thread>(std::bind(&ScreenRecorder::DecodeAudio, this)));
	threads_.push_back(std::make_shared<std::thread>(std::bind(&ScreenRecorder::ResampleAudio, this)));
	threads_.push_back(std::make_shared<std::thread>(std::bind(&ScreenRecorder::EncodeAudio, this)));
	threads_.push_back(std::make_shared<std::thread>(std::bind(&ScreenRecorder::Writer, this)));;
}

void ScreenRecorder::Stop() {
	video_decoder_flag_.Set(false);
	audio_decoder_flag_.Set(false);
	for (auto i = threads_.begin(); i != threads_.end(); ++i) {
		if ((*i)->joinable()) {
			(*i)->join();
		}
	}
}

void ScreenRecorder::DecodeVideo() {

	AVPacket* input_packet = av_packet_alloc();
	if (!input_packet) {
		std::cout << "Could not allocate video packet\n";
		exit(1);
	}

	clock_t start;
	start = clock();

	int frame_number = 0;

	while (video_decoder_flag_.Get() && av_read_frame(input_video_format_context_, input_packet) == 0) {
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
			++frame_number;
			double du = ((double)(clock() - start) / 1000.0);
			printf("Time: %5.2lf\tFPS: %5.2lf    \r", du, (double)frame_number / du);
			video_decoded_frame_queue_.Push(decoded_frame);
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
		video_decoded_frame_queue_.Push(decoded_frame);
	}

	video_scaler_flag_.Set(false);
	av_packet_free(&input_packet);
	return;
}
void ScreenRecorder::ScaleVideo() {
	// Get Original Frame
	AVFrame* decoded_frame = NULL;
	while (1) {
		if (!(decoded_frame = video_decoded_frame_queue_.Pop())) {
			Sleep(100);
			if (video_scaler_flag_.Get() == false)
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
		video_scaled_frame_queue_.Push(scaled_frame);
		
		av_frame_free(&decoded_frame);
	}
	video_encoder_flag_.Set(false);
}
void ScreenRecorder::EncodeVideo() {
	AVFrame* scaled_frame = av_frame_alloc();

	int ret;
	while (1) {
		if (!(scaled_frame = video_scaled_frame_queue_.Pop())) {
			Sleep(100);
			if (video_encoder_flag_.Get() == false)
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
			if (output_packet->pts != AV_NOPTS_VALUE)
				output_packet->pts = av_rescale_q(output_packet->pts, video_encoder_codec_context_->time_base, out_video_stream_->time_base);
			if (output_packet->dts != AV_NOPTS_VALUE)
				output_packet->dts = av_rescale_q(output_packet->dts, video_encoder_codec_context_->time_base, out_video_stream_->time_base);
			output_packet->stream_index = out_video_stream_->index;
			output_packet_queue_.Push(output_packet);
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
		
		if (output_packet->pts != AV_NOPTS_VALUE)
			output_packet->pts = av_rescale_q(output_packet->pts, video_encoder_codec_context_->time_base, out_video_stream_->time_base);
		if (output_packet->dts != AV_NOPTS_VALUE)
			output_packet->dts = av_rescale_q(output_packet->dts, video_encoder_codec_context_->time_base, out_video_stream_->time_base);
		output_packet->stream_index = out_video_stream_->index;
		output_packet_queue_.Push(output_packet);
	}

	writer_flag_.Set(false);

	return;
}
void ScreenRecorder::DecodeAudio() {
	AVPacket* input_packet = av_packet_alloc();
	if (!input_packet) {
		std::cout << "Could not allocate audio packet\n";
		exit(1);
	}

	clock_t start;
	start = clock();

	while (audio_decoder_flag_.Get() && av_read_frame(input_audio_format_context_, input_packet) == 0) {
		int ret;
		ret = avcodec_send_packet(audio_decoder_codec_context_, input_packet);
		if (ret < 0) {
			std::cout << "Error sending a packet for decoding\n";
			exit(1);
		}
		while (ret >= 0) {
			AVFrame* decoded_frame = av_frame_alloc();
			if (!decoded_frame) {
				std::cout << "Could not allocate audio pFrame\n";
				exit(1);
			}
			static int aaa = 0, bbb = 0;
			++aaa;
			ret = avcodec_receive_frame(audio_decoder_codec_context_, decoded_frame);
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
				break;
			else if (ret < 0) {
				std::cout << "Error during decoding\n";
				exit(1);
			}
			audio_decoded_frame_queue_.Push(decoded_frame);
		}
	}

	/* flush the decoder */
	int ret;
	ret = avcodec_send_packet(audio_decoder_codec_context_, NULL);
	if (ret < 0) {
		std::cout << "Error sending a packet for decoding\n";
		exit(1);
	}
	while (ret >= 0) {
		AVFrame* decoded_frame = av_frame_alloc();
		if (!decoded_frame) {
			std::cout << "Could not allocate audio pFrame\n";
			exit(1);
		}
		ret = avcodec_receive_frame(audio_decoder_codec_context_, decoded_frame);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			break;
		else if (ret < 0) {
			std::cout << "Error during decoding\n";
			exit(1);
		}
		audio_decoded_frame_queue_.Push(decoded_frame);
	}

	audio_scaler_flag_.Set(false);
	av_packet_free(&input_packet);
	return;
}
void ScreenRecorder::ResampleAudio() {
	// Get Original Frame
	AVAudioFifo* audio_fifo = av_audio_fifo_alloc(
		audio_encoder_codec_context_->sample_fmt,
		audio_decoder_codec_context_->channels,
		audio_decoder_codec_context_->sample_rate * 2);
	AVFrame* decoded_frame = NULL;
	__int64 frame_count = 0;
	while (1) {
		if (!(decoded_frame = audio_decoded_frame_queue_.Pop())) {
			Sleep(100);
			if (audio_scaler_flag_.Get() == false)
				break;
			continue;
		}
		uint8_t** converted_samples = nullptr;
		if (av_samples_alloc_array_and_samples(&converted_samples, NULL, audio_encoder_codec_context_->channels, decoded_frame->nb_samples, audio_encoder_codec_context_->sample_fmt, 0) < 0)
			std::cout << "Fail to alloc samples by av_samples_alloc_array_and_samples.\n";
		if (swr_convert(swr_context_, converted_samples, decoded_frame->nb_samples, (const uint8_t**)decoded_frame->extended_data, decoded_frame->nb_samples) < 0)
			std::cout << "Fail to swr_convert.\n";
		/* convert samples from native format to destination codec format, using the resampler */
				/* compute destination number of samples */
		int dst_nb_samples = av_rescale_rnd(swr_get_delay(swr_context_, audio_encoder_codec_context_->sample_rate) + decoded_frame->nb_samples,
			audio_encoder_codec_context_->sample_rate, audio_encoder_codec_context_->sample_rate, AV_ROUND_UP);

		av_assert0(dst_nb_samples == decoded_frame->nb_samples);
		if (av_audio_fifo_space(audio_fifo) < decoded_frame->nb_samples)
			std::cout << "audio buffer is too small.\n";
		if (av_audio_fifo_write(audio_fifo, (void**)converted_samples, decoded_frame->nb_samples) < 0)
			std::cout << "Fail to write fifo\n";

		av_freep(&converted_samples[0]);
		av_frame_free(&decoded_frame);

		int a = av_audio_fifo_size(audio_fifo);
		while (av_audio_fifo_size(audio_fifo) >= audio_encoder_codec_context_->frame_size) {

			// Alloc AVFrame
			AVFrame* resampled_frame = av_frame_alloc();

			resampled_frame->format = audio_encoder_codec_context_->sample_fmt;
			resampled_frame->channel_layout = audio_encoder_codec_context_->channel_layout;
			resampled_frame->channels = av_get_default_channel_layout(audio_encoder_codec_context_->channel_layout);
			resampled_frame->sample_rate = audio_encoder_codec_context_->sample_rate;
			resampled_frame->nb_samples = audio_encoder_codec_context_->frame_size;
			if (resampled_frame->nb_samples) {
				int ret = av_frame_get_buffer(resampled_frame, 0);
				if (ret < 0) {
					fprintf(stderr, "Error allocating an audio buffer\n");
					exit(1);
				}
				ret = av_audio_fifo_read(audio_fifo, (void**)resampled_frame->data, audio_encoder_codec_context_->frame_size);
				if (ret < 0) {
					fprintf(stderr, "Error read audio buffer\n");
					exit(1);
				}
				resampled_frame->pts = frame_count * out_audio_stream_->time_base.den * 1024 / audio_encoder_codec_context_->sample_rate;
				++frame_count;
				/* when we pass a frame to the encoder, it may keep a reference to it
				 * internally;
				 * make sure we do not overwrite it here
				 */
				ret = av_frame_make_writable(resampled_frame);
				if (ret < 0)
					exit(1);

				if (ret < 0) {
					fprintf(stderr, "Error while converting\n");
					exit(1);
				}
				audio_resampled_frame_queue_.Push(resampled_frame);
			}
		}
	}
	audio_encoder_flag_.Set(false);
}
void ScreenRecorder::EncodeAudio() {
	AVFrame* resampled_frame = av_frame_alloc();

	int ret;
	while (1) {
		if (!(resampled_frame = audio_resampled_frame_queue_.Pop())) {
			Sleep(100);
			if (audio_encoder_flag_.Get() == false)
				break;
			continue;
		}

		ret = avcodec_send_frame(audio_encoder_codec_context_, resampled_frame);
		if (ret < 0) {
			std::cout << "Error sending a frame for encoding\n";
			exit(1);
		}
		while (ret >= 0) {
			AVPacket* output_packet = av_packet_alloc();
			if (!output_packet)
				exit(1);
			ret = avcodec_receive_packet(audio_encoder_codec_context_, output_packet);
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
				break;
			else if (ret < 0) {
				std::cout << "Error during encoding\n";
				exit(1);
			}
			if (output_packet->pts != AV_NOPTS_VALUE)
				output_packet->pts = av_rescale_q(output_packet->pts, audio_encoder_codec_context_->time_base, out_audio_stream_->time_base);
			if (output_packet->dts != AV_NOPTS_VALUE)
				output_packet->dts = av_rescale_q(output_packet->dts, audio_encoder_codec_context_->time_base, out_audio_stream_->time_base);
			if (output_packet->duration != AV_NOPTS_VALUE)
				output_packet->duration = av_rescale_q(output_packet->duration, audio_encoder_codec_context_->time_base, out_audio_stream_->time_base);
			output_packet->stream_index = out_audio_stream_->index;
			output_packet_queue_.Push(output_packet);
		}
		av_frame_free(&resampled_frame);
	}
	/* flush the encoder */
	ret = avcodec_send_frame(audio_encoder_codec_context_, NULL);
	if (ret < 0) {
		std::cout << "Error sending a frame for encoding\n";
		exit(1);
	}
	while (ret >= 0) {
		AVPacket* output_packet = av_packet_alloc();
		if (!output_packet)
			exit(1);
		ret = avcodec_receive_packet(audio_encoder_codec_context_, output_packet);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			break;
		else if (ret < 0) {
			std::cout << "Error during encoding\n";
			exit(1);
		}

		if (output_packet->pts != AV_NOPTS_VALUE)
			output_packet->pts = av_rescale_q(output_packet->pts, audio_encoder_codec_context_->time_base, out_audio_stream_->time_base);
		if (output_packet->dts != AV_NOPTS_VALUE)
			output_packet->dts = av_rescale_q(output_packet->dts, audio_encoder_codec_context_->time_base, out_audio_stream_->time_base);
		output_packet->stream_index = out_audio_stream_->index;
		output_packet_queue_.Push(output_packet);
	}

	writer_flag_.Set(false);

	return;
}
void ScreenRecorder::Writer() {
	AVPacket* output_packet;
	while (1) {
		if (!(output_packet = output_packet_queue_.Pop())) {
			Sleep(100);
			if (writer_flag_.Get() == false)
				break;
			continue;
		}
		//av_packet_rescale_ts(pkt, *time_base, st->time_base);
		if (av_interleaved_write_frame(output_format_context_, output_packet) != 0)
		{
			std::cout << "\nerror in writing video frame";
		}
		av_packet_unref(output_packet);
	}
	int ret = av_write_trailer(output_format_context_);
	if (ret < 0)
	{
		std::cout << "\nerror in writing av trailer";
		exit(1);
	}
	return;
}