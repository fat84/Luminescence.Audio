#include "stdafx.h"
#include "ChromaprintFingerprinter.h"

#define HAVE_SWRESAMPLE
#define MIN(a, b) ((a) < (b) ? (a) : (b))

namespace Luminescence
{
   namespace Audio 
   {
      //https://bitbucket.org/acoustid/chromaprint/src/3a2cd633e0f220295239f0056a34c8ce0d51ce9a/examples/fpcalc.c?at=master 
      //modifi� le 22/07/14 : suppression des appels de fonction d�pr�ci�e
      int ChromaprintFingerprinter::decode_audio_file(ChromaprintContext *chromaprint_ctx, const char *file_name, int max_length, int *duration, String^% error)
	   {
		   int ok = 0, remaining, length, consumed, codec_ctx_opened = 0, got_frame, stream_index;
		   AVFormatContext *format_ctx = NULL;
		   AVCodecContext *codec_ctx = NULL;
		   AVCodec *codec = NULL;
		   AVStream *stream = NULL;
		   AVFrame *frame = NULL;
#if defined(HAVE_SWRESAMPLE)
		   SwrContext *convert_ctx = NULL;
#elif defined(HAVE_AVRESAMPLE)
		   AVAudioResampleContext *convert_ctx = NULL;
#endif
		   int max_dst_nb_samples = 0, dst_linsize = 0;
		   uint8_t *dst_data[1] = { NULL };
		   uint8_t **data;
		   AVPacket packet;

		   /*if (!strcmp(file_name, "-")) {
			   file_name = "pipe:0";
		   }*/

		   if (avformat_open_input(&format_ctx, file_name, NULL, NULL) != 0) {
			   //fprintf(stderr, "ERROR: couldn't open the file\n");
            error = "Couldn't open the file.";
			   goto done;
		   }

		   if (avformat_find_stream_info(format_ctx, NULL) < 0) {
			   //fprintf(stderr, "ERROR: couldn't find stream information in the file\n");
            error = "Couldn't find stream information in the file.";
			   goto done;
		   }

		   stream_index = av_find_best_stream(format_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, &codec, 0);
		   if (stream_index < 0) {
			   //fprintf(stderr, "ERROR: couldn't find any audio stream in the file\n");
            error = "Couldn't find any audio stream in the file.";
			   goto done;
		   }

		   stream = format_ctx->streams[stream_index];

		   codec_ctx = stream->codec;
		   codec_ctx->request_sample_fmt = AV_SAMPLE_FMT_S16;

		   if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
			   //fprintf(stderr, "ERROR: couldn't open the codec\n");
            error = "Couldn't open the codec.";
			   goto done;
		   }
		   codec_ctx_opened = 1;

		   if (codec_ctx->channels <= 0) {
			   //fprintf(stderr, "ERROR: no channels found in the audio stream\n");
            error = "No channels found in the audio stream.";
			   goto done;
		   }

		   if (codec_ctx->sample_fmt != AV_SAMPLE_FMT_S16) {
			   int64_t channel_layout = codec_ctx->channel_layout;
			   if (!channel_layout) {
				   channel_layout = av_get_default_channel_layout(codec_ctx->channels);
			   }
#if defined(HAVE_SWRESAMPLE)
			   convert_ctx = swr_alloc_set_opts(NULL,
				   channel_layout, AV_SAMPLE_FMT_S16, codec_ctx->sample_rate,
				   channel_layout, codec_ctx->sample_fmt, codec_ctx->sample_rate,
				   0, NULL);
			   if (!convert_ctx) {
				   //fprintf(stderr, "ERROR: couldn't allocate audio converter\n");
               error = "Couldn't allocate audio converter.";
				   goto done;
			   }
			   if (swr_init(convert_ctx) < 0) {
				   //fprintf(stderr, "ERROR: couldn't initialize the audio converter\n");
               error = "Couldn't initialize the audio converter.";
				   goto done;
			   }
#elif defined(HAVE_AVRESAMPLE)
			   convert_ctx = avresample_alloc_context();
			   av_opt_set_int(convert_ctx, "out_channel_layout", channel_layout, 0);
			   av_opt_set_int(convert_ctx, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);
			   av_opt_set_int(convert_ctx, "out_sample_rate", codec_ctx->sample_rate, 0);
			   av_opt_set_int(convert_ctx, "in_channel_layout", channel_layout, 0);
			   av_opt_set_int(convert_ctx, "in_sample_fmt", codec_ctx->sample_fmt, 0);
			   av_opt_set_int(convert_ctx, "in_sample_rate", codec_ctx->sample_rate, 0);
			   if (!convert_ctx) {
				   fprintf(stderr, "ERROR: couldn't allocate audio converter\n");
				   goto done;
			   }
			   if (avresample_open(convert_ctx) < 0) {
				   fprintf(stderr, "ERROR: couldn't initialize the audio converter\n");
				   goto done;
			   }
#endif
		   }

		   if (stream->duration != AV_NOPTS_VALUE) {
			   *duration = (int)(stream->time_base.num * stream->duration / stream->time_base.den);
		   }
		   else if (format_ctx->duration != AV_NOPTS_VALUE) {
			   *duration = (int)(format_ctx->duration / AV_TIME_BASE);
		   }
		   else {
			   //fprintf(stderr, "ERROR: couldn't detect the audio duration\n");
            error = "Couldn't detect the audio duration.";
			   goto done;
		   }

		   remaining = max_length * codec_ctx->channels * codec_ctx->sample_rate;
		   chromaprint_start(chromaprint_ctx, codec_ctx->sample_rate, codec_ctx->channels);

		   frame = av_frame_alloc();

		   while (1) {
			   if (av_read_frame(format_ctx, &packet) < 0) {
				   break;
			   }

			   if (packet.stream_index == stream_index) {
				   av_frame_unref(frame);

				   got_frame = 0;
				   consumed = avcodec_decode_audio4(codec_ctx, frame, &got_frame, &packet);
				   if (consumed < 0) {
					   //fprintf(stderr, "WARNING: error decoding audio\n");
                  error = "Coudn't decode audio data.";
					   //continue;
                  goto done;
				   }

				   if (got_frame) {
					   data = frame->data;
					   if (convert_ctx) {
						   if (frame->nb_samples > max_dst_nb_samples) {
							   av_freep(&dst_data[0]);
							   if (av_samples_alloc(dst_data, &dst_linsize, codec_ctx->channels, frame->nb_samples, AV_SAMPLE_FMT_S16, 1) < 0) {
								   //fprintf(stderr, "ERROR: couldn't allocate audio converter buffer\n");
                           error = "Couldn't allocate audio converter buffer.";
								   goto done;
							   }
							   max_dst_nb_samples = frame->nb_samples;
						   }
#if defined(HAVE_SWRESAMPLE)
						   if (swr_convert(convert_ctx, dst_data, frame->nb_samples, (const uint8_t **)frame->data, frame->nb_samples) < 0) {
#elif defined(HAVE_AVRESAMPLE)
						   if (avresample_convert(convert_ctx, dst_data, 0, frame->nb_samples, (uint8_t **)frame->data, 0, frame->nb_samples) < 0) {
#endif
							   //fprintf(stderr, "ERROR: couldn't convert the audio\n");
                        error = "Couldn't convert the audio data.";
							   goto done;
						   }
						   data = dst_data;
					   }
					   length = MIN(remaining, frame->nb_samples * codec_ctx->channels);
					   if (!chromaprint_feed(chromaprint_ctx, data[0], length)) {
						   goto done;
					   }

					   if (max_length) {
						   remaining -= length;
						   if (remaining <= 0) {
							   goto finish;
						   }
					   }
				   }
			   }
			   av_free_packet(&packet);
		   }

	   finish:
		   if (!chromaprint_finish(chromaprint_ctx)) {
			   //fprintf(stderr, "ERROR: fingerprint calculation failed\n");
            error = "Couldn't calculate fingerprint.";
			   goto done;
		   }

		   ok = 1;

	   done:
		   if (frame) {
			   av_frame_free(&frame);
		   }
		   if (dst_data[0]) {
			   av_freep(&dst_data[0]);
		   }
		   if (convert_ctx) {
#if defined(HAVE_SWRESAMPLE)
			   swr_free(&convert_ctx);
#elif defined(HAVE_AVRESAMPLE)
			   avresample_free(&convert_ctx);
#endif
		   }
		   if (codec_ctx_opened) {
			   avcodec_close(codec_ctx);
		   }
		   if (format_ctx) {
			   avformat_close_input(&format_ctx);
		   }
		   return ok;
	   }
   }
}