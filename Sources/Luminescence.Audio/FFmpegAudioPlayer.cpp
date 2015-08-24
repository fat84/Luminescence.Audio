#include "StdAfx.h"
#include "FFmpegAudioPlayer.h"

#include <msclr\marshal.h>
#include <msclr\marshal_cppstd.h>

//#define DX_SDK_INSTALLED  //the system must have the DirectX SDK Developer Runtime installed for debugging to be supported

namespace Luminescence
{
   namespace Audio
   {
      FFmpegAudioPlayer::FFmpegAudioPlayer() :
         pXAudio2(NULL), pMasteringVoice(NULL), pSourceVoice(NULL), container(NULL), codec_context(NULL), swr_ctx(NULL), volume(1.0f), audioStream(-1)
      {
         //CoInitializeEx(NULL, COINIT_APARTMENTTHREADED); /*COINIT_MULTITHREADED*/ /*COINIT_APARTMENTTHREADED*/
         InitializeAudioEngine();
#if defined(_DEBUG)
         av_log_set_level(AV_LOG_WARNING);
#endif
         av_register_all();
      }

      void FFmpegAudioPlayer::Release()
      {
         Stop();
         ShutdownAudioEngine();
         //CoUninitialize();
      }

      void FFmpegAudioPlayer::ShutdownAudioEngine()
      {
         if (pMasteringVoice != NULL)
         {
            pMasteringVoice->DestroyVoice();
            pMasteringVoice = NULL;
         }

         if (pXAudio2 != NULL)
         {
            pXAudio2->Release();
            pXAudio2 = NULL;
         }

         if (buffersToRelease != NULL)
         {
            delete buffersToRelease;
            buffersToRelease = NULL;
         }
      }

      void FFmpegAudioPlayer::CleanUpFFmpegResource()
      {
         if (swr_ctx != NULL)
         {
            pin_ptr<SwrContext *> ptr = &swr_ctx;
            swr_free(ptr);
            swr_ctx = NULL;
         }

         if (codec_context != NULL)
         {
            avcodec_close(codec_context);
            codec_context = NULL;
         }

         if (container != NULL)
         {
            pin_ptr<AVFormatContext *> ptr = &container;
            avformat_close_input(ptr);
            container = NULL;
         }

         audioStream = -1;
      }

      void FFmpegAudioPlayer::Play(String^ path)
      {
         if (pXAudio2 == NULL)
            throw gcnew ObjectDisposedException("You cannot use this instance of audio player because it has been disposed.");

         Stop();

         std::string file_name = msclr::interop::marshal_as<std::string>(path);
         pin_ptr<AVFormatContext*> _container = &container;

         if (avformat_open_input(_container, file_name.c_str(), NULL, NULL) < 0)
            throw gcnew IOException("Failed to play file: the audio file couldn't be opened.");

         if (avformat_find_stream_info(container, NULL) < 0)
         {
            CleanUpFFmpegResource();
            throw gcnew FormatException("Failed to play file: the audio file doesn't contain any file info.");
         }

         AVCodec* codec;
         audioStream = av_find_best_stream(container, AVMEDIA_TYPE_AUDIO, -1, -1, &codec, 0);

         if (audioStream < 0)
         {
            CleanUpFFmpegResource();
            throw gcnew FormatException("Failed to play file: the audio file doesn't contain any audio stream.");
         }

         codec_context = container->streams[audioStream]->codec;
         codec_context->request_sample_fmt = AV_SAMPLE_FMT_S16;

         if (avcodec_open2(codec_context, codec, NULL) < 0)
         {
            CleanUpFFmpegResource();
            throw gcnew FormatException("Failed to play file: couldn't find the needed codec.");
         }

         if (codec_context->sample_fmt != AV_SAMPLE_FMT_S16)
         {
            uint64_t channel_layout = codec_context->channel_layout;
            if (!channel_layout)
               channel_layout = av_get_default_channel_layout(codec_context->channels);

            swr_ctx = swr_alloc_set_opts(NULL,
               channel_layout, AV_SAMPLE_FMT_S16, codec_context->sample_rate, // out
               channel_layout, codec_context->sample_fmt, codec_context->sample_rate, // in
               0, NULL);

            if (!swr_ctx)
            {
               CleanUpFFmpegResource();
               throw gcnew FormatException("Failed to play file: couldn't allocate the audio converter.");
            }

            if (swr_init(swr_ctx) < 0)
            {
               CleanUpFFmpegResource();
               throw gcnew FormatException("Failed to play file: couldn't initialize the audio converter.");
            }
         }

         WAVEFORMATEXTENSIBLE wfx;
         wfx.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
         wfx.Format.nChannels = codec_context->channels;
         wfx.Format.nSamplesPerSec = codec_context->sample_rate;
         wfx.Format.wBitsPerSample = codec_context->bits_per_raw_sample;
         wfx.Samples.wValidBitsPerSample = codec_context->bits_per_raw_sample;
         if (wfx.Samples.wValidBitsPerSample == 0)
            wfx.Format.wBitsPerSample = wfx.Samples.wValidBitsPerSample = 16;
         else if (wfx.Samples.wValidBitsPerSample == 24)
            wfx.Format.wBitsPerSample = 32;
         wfx.Format.cbSize = 22;
         wfx.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
         wfx.dwChannelMask = 0;
         wfx.Format.nBlockAlign = (wfx.Format.nChannels * wfx.Format.wBitsPerSample) / 8;
         wfx.Format.nAvgBytesPerSec = wfx.Format.nSamplesPerSec * wfx.Format.nBlockAlign;

         bufferSize = wfx.Format.nAvgBytesPerSec * XAUDIO2_BUFFER_SECONDS;

         HRESULT hr;
         pin_ptr<IXAudio2SourceVoice*> _pSourceVoice = &pSourceVoice;
         if (FAILED(hr = pXAudio2->CreateSourceVoice(_pSourceVoice, (WAVEFORMATEX*)&wfx, XAUDIO2_VOICE_NOPITCH)))
         {
            CleanUpFFmpegResource();
            throw gcnew Exception(String::Format("Failed to create XAudio2 source voice ({0:X}).", hr));
         }

         try
         {
            FeedSourceVoiceBuffer(XAUDIO2_QUEUED_BUFFERS, bufferSize); /*XAUDIO2_MAX_QUEUED_BUFFERS*/ /*XAUDIO2_MAX_BUFFER_BYTES*/
         }
         catch (Exception^)
         {
            CleanUpSourceVoiceResource();
            CleanUpFFmpegResource();
            throw;
         }

         if (FAILED(hr = pSourceVoice->Start()))
         {
            CleanUpSourceVoiceResource();
            CleanUpFFmpegResource();
            throw gcnew Exception(String::Format("Failed to start XAudio2 source voice ({0:X}).", hr));
         }

         file = path;

         cancellationToken = gcnew CancellationTokenSource();
         context = SynchronizationContext::Current;
         voiceFeederTask = Task::Factory->StartNew(gcnew Action(this, &FFmpegAudioPlayer::FeedSourceVoice), cancellationToken->Token, TaskCreationOptions::LongRunning, TaskScheduler::Default);

         RaisePlayingEvent(path);
      }

      // ATTENTION : exécution de la méthode dans un thread de travail
      void FFmpegAudioPlayer::StopAudioEngine()
      {
         CleanUpSourceVoiceResource();
         CleanUpFFmpegResource();

         isPaused = false;
         String^ path = file;
         file = nullptr;

         RaiseStoppedEvent(path);
      }

      void FFmpegAudioPlayer::CleanUpSourceVoiceResource()
      {
         pSourceVoice->Stop();
         pSourceVoice->FlushSourceBuffers();
         pSourceVoice->DestroyVoice();
         pSourceVoice = NULL;

         for (UINT32 i = 0; i < buffersToRelease->size(); i++)
         {
            XAUDIO2_BUFFER* xbuffer = buffersToRelease->front();
            buffersToRelease->pop_front();
            delete[] xbuffer->pAudioData;
            delete xbuffer;
         }
      }

      // ATTENTION : exécution de la méthode dans un thread de travail
      void FFmpegAudioPlayer::FeedSourceVoice()
      {
         CancellationToken^ token = cancellationToken->Token;
         bool eofReached = false;

         while (true)
         {
            if (token->IsCancellationRequested)
            {
               StopAudioEngine();
               return;
            }

            XAUDIO2_VOICE_STATE state;
            pSourceVoice->GetState(&state);
            if (state.BuffersQueued == 0)
            {
               StopAudioEngine();
               return;
            }

            if (!eofReached && state.BuffersQueued < XAUDIO2_QUEUED_BUFFERS)
            {
               eofReached = FeedSourceVoiceBuffer(1, bufferSize);
               if (!eofReached)
               {
                  XAUDIO2_BUFFER* xbuffer = buffersToRelease->front();
                  buffersToRelease->pop_front();
                  delete[] xbuffer->pAudioData;
                  delete xbuffer;
               }
            }

            //Task::Delay(100, cancellationToken->Token)->Wait();
            Sleep(100);
         }
      }

      bool FFmpegAudioPlayer::FeedSourceVoiceBuffer(int buffersToQueue, UINT32 minBufferSize)
      {
         AVFrame* frame = av_frame_alloc();
         int got_frame, consumed, audio_buf_size;
         AVPacket packet, packet_temp;

         uint8_t *audio_buf;
         std::vector<BYTE> bytes;
         int buffersQueued = 0;
         bool eofReached = false;
         int max_dst_nb_samples = 0;

         while (buffersQueued < buffersToQueue)
         {
            if (av_read_frame(container, &packet) < 0)
            {
               // End of stream.
               eofReached = true;

               if (bytes.empty())
               {
                  pSourceVoice->Discontinuity();
                  break;
               }

               XAUDIO2_BUFFER* xbuffer = new XAUDIO2_BUFFER();
               xbuffer->AudioBytes = bytes.size();
               xbuffer->pAudioData = new BYTE[bytes.size()];
               xbuffer->Flags = XAUDIO2_END_OF_STREAM;

               buffersToRelease->push_back(xbuffer);
               std::copy(bytes.begin(), bytes.end(), (BYTE*)xbuffer->pAudioData);

               if (SUCCEEDED(pSourceVoice->SubmitSourceBuffer(xbuffer)))
                  buffersQueued++;

               break;
            }

            if (packet.stream_index == audioStream)
            {
               packet_temp = packet;
               while (packet_temp.size > 0)
               {
                  got_frame = 0;
                  consumed = avcodec_decode_audio4(codec_context, frame, &got_frame, &packet_temp);
                  if (consumed >= 0)// && got_frame != 0)
                  {
                     packet_temp.data += consumed;
                     packet_temp.size -= consumed;

                     audio_buf = frame->extended_data[0];
                     audio_buf_size = frame->linesize[0];
                     if (audio_buf_size > 0)
                        bytes.insert(bytes.end(), &audio_buf[0], &audio_buf[audio_buf_size]);
                  }
                  else
                  {
                     av_free_packet(&packet);
                     av_frame_free(&frame);
                     throw gcnew Exception("Failed to play file: couldn't decode the audio frame.");
                  }
               }

               if (bytes.size() >= minBufferSize)
               {
                  XAUDIO2_BUFFER* xbuffer = new XAUDIO2_BUFFER();
                  xbuffer->AudioBytes = bytes.size();
                  xbuffer->pAudioData = new BYTE[bytes.size()];

                  buffersToRelease->push_back(xbuffer);
                  std::copy(bytes.begin(), bytes.end(), (BYTE*)xbuffer->pAudioData);
                  bytes.clear();

                  if (SUCCEEDED(pSourceVoice->SubmitSourceBuffer(xbuffer)))
                     buffersQueued++;
               }
            }

            av_free_packet(&packet);
         }

         av_free_packet(&packet);
         av_frame_free(&frame);

         return eofReached;
      }

      void FFmpegAudioPlayer::Stop()
      {
         if (voiceFeederTask != nullptr && !voiceFeederTask->IsCompleted)
         {
            cancellationToken->Cancel();
            voiceFeederTask->Wait();
         }
      }

      void FFmpegAudioPlayer::InitializeAudioEngine()
      {
         UINT32 flags = 0;

#if defined(_DEBUG) && defined(DX_SDK_INSTALLED)
         flags |= XAUDIO2_DEBUG_ENGINE;
#endif

         HRESULT hr;
         pin_ptr<IXAudio2*> _pXAudio2 = &pXAudio2;
         if (FAILED(hr = XAudio2Create(_pXAudio2, flags)))
            throw gcnew Exception(String::Format("Failed to init XAudio2 engine ({0:X}).", hr));

         pin_ptr<IXAudio2MasteringVoice*> _pMasteringVoice = &pMasteringVoice;
         if (FAILED(hr = pXAudio2->CreateMasteringVoice(_pMasteringVoice)))
         {
            pXAudio2->Release();
            pXAudio2 = NULL;
            throw gcnew Exception(String::Format("Failed to create XAudio2 mastering voice ({0:X}).", hr));
         }

         buffersToRelease = new std::deque<XAUDIO2_BUFFER*>();

#if defined(_DEBUG) && defined(DX_SDK_INSTALLED)
         XAUDIO2_DEBUG_CONFIGURATION debug = {0};
         debug.TraceMask = XAUDIO2_LOG_WARNINGS /*| XAUDIO2_LOG_API_CALLS | XAUDIO2_LOG_LOCKS | XAUDIO2_LOG_STREAMING | XAUDIO2_LOG_TIMING*/;
         debug.BreakMask = XAUDIO2_LOG_ERRORS;
         debug.LogTiming = 0;
         pXAudio2->SetDebugConfiguration(&debug);
#endif
      }
   }
}