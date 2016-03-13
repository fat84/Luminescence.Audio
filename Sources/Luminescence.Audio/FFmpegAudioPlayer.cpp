#include "stdafx.h"

#include <XAudio2.h>

#include <vector>
#include <deque>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>
}

#include "Interop.h"
#include "ResourceStrings.h"

#define XAUDIO2_QUEUED_BUFFERS 3
#define XAUDIO2_BUFFER_SECONDS 3
//#define DX_SDK_INSTALLED  //the system must have the DirectX SDK Developer Runtime installed for debugging to be supported

using namespace System;
using namespace System::IO;
using namespace System::Threading;
using namespace System::Threading::Tasks;

namespace Luminescence
{
   namespace Audio
   {
      #pragma region Helper

      public ref class PathEventArgs : EventArgs
      {
      private:
         String^ path;

      public:
         PathEventArgs(String^ p)
         {
            path = p;
         }

         property String^ Path
         {
            String^ get() { return path; }
         }
      };

      #pragma endregion

      public ref class FFmpegAudioPlayer
      {
      private:
         IXAudio2* pXAudio2;
         IXAudio2MasteringVoice* pMasteringVoice;
         IXAudio2SourceVoice* pSourceVoice;

         AVFormatContext* container;
         AVCodecContext* codec_context;
         SwrContext* swr_ctx;

         std::deque<XAUDIO2_BUFFER*>* buffersToRelease;

         bool isPaused;
         bool isMuted;
         float volume;
         int audioStream;
         UINT32 bufferSize;

         String^ file;
         Task^ voiceFeederTask;
         CancellationTokenSource^ cancellationToken;
         SynchronizationContext^ context;

         void Release()
         {
            Stop();
            ShutdownAudioEngine();
            //CoUninitialize();
         }

         void InitializeAudioEngine()
         {
            UINT32 flags = 0;

#if defined(_DEBUG) && defined(DX_SDK_INSTALLED)
            flags |= XAUDIO2_DEBUG_ENGINE;
#endif

            HRESULT hr;
            pin_ptr<IXAudio2*> _pXAudio2 = &pXAudio2;
            if (FAILED(hr = XAudio2Create(_pXAudio2, flags)))
               throw gcnew Exception(String::Format(ResourceStrings::GetString("CannotUseAudioEngine"), "XAudio2Create", hr));

            pin_ptr<IXAudio2MasteringVoice*> _pMasteringVoice = &pMasteringVoice;
            if (FAILED(hr = pXAudio2->CreateMasteringVoice(_pMasteringVoice)))
            {
               pXAudio2->Release();
               pXAudio2 = NULL;
               throw gcnew Exception(String::Format(ResourceStrings::GetString("CannotUseAudioEngine"), "IXAudio2::CreateMasteringVoice", hr));
            }

            buffersToRelease = new std::deque<XAUDIO2_BUFFER*>();

#if defined(_DEBUG) && defined(DX_SDK_INSTALLED)
            XAUDIO2_DEBUG_CONFIGURATION debug = { 0 };
            debug.TraceMask = XAUDIO2_LOG_WARNINGS /*| XAUDIO2_LOG_API_CALLS | XAUDIO2_LOG_LOCKS | XAUDIO2_LOG_STREAMING | XAUDIO2_LOG_TIMING*/;
            debug.BreakMask = XAUDIO2_LOG_ERRORS;
            debug.LogTiming = 0;
            pXAudio2->SetDebugConfiguration(&debug);
#endif
         }

         void ShutdownAudioEngine()
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

         void CleanUpFFmpegResource()
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

         // ATTENTION : exécution de la méthode dans un thread de travail
         void FeedSourceVoice()
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

         // ATTENTION : exécution de la méthode dans un thread de travail
         void StopAudioEngine()
         {
            CleanUpSourceVoiceResource();
            CleanUpFFmpegResource();

            isPaused = false;
            String^ path = file;
            file = nullptr;

            RaiseStoppedEvent(path);
         }

         void CleanUpSourceVoiceResource()
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

         bool FeedSourceVoiceBuffer(int buffersToQueue, UINT32 minBufferSize)
         {
            AVFrame* frame = av_frame_alloc();
            int got_frame, consumed, audio_buf_size;
            AVPacket readingPacket;

            uint8_t *audio_buf;
            std::vector<BYTE> bytes;
            int buffersQueued = 0;
            bool eofReached = false;

            while (buffersQueued < buffersToQueue)
            {
               if (av_read_frame(container, &readingPacket) < 0)
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

               if (readingPacket.stream_index == audioStream)
               {
                  AVPacket decodingPacket = readingPacket;
                  while (decodingPacket.size > 0)
                  {
                     got_frame = 0;
                     consumed = avcodec_decode_audio4(codec_context, frame, &got_frame, &decodingPacket);
                     if (consumed < 0)
                     {
                        av_packet_unref(&readingPacket);
                        av_frame_free(&frame);
                        throw gcnew Exception(String::Format(ResourceStrings::GetString("CannotDecodeAudio"), "avcodec_decode_audio4"));
                     }

                     decodingPacket.data += consumed;
                     decodingPacket.size -= consumed;

                     if (got_frame)
                     {
                        if (swr_ctx != NULL)
                        {
                           if (av_samples_alloc(&audio_buf, &audio_buf_size, codec_context->channels, frame->nb_samples, AV_SAMPLE_FMT_S16, 0) < 0)
                           {
                              av_packet_unref(&readingPacket);
                              av_frame_free(&frame);
                              throw gcnew Exception(String::Format(ResourceStrings::GetString("CannotDecodeAudio"), "av_samples_alloc"));
                           }

                           if (swr_convert(swr_ctx, &audio_buf, frame->nb_samples, (const uint8_t **)frame->extended_data, frame->nb_samples) < 0)
                           {
                              av_packet_unref(&readingPacket);
                              av_frame_free(&frame);
                              av_freep(&audio_buf);
                              throw gcnew Exception(String::Format(ResourceStrings::GetString("CannotDecodeAudio"), "swr_convert"));
                           }
                        }
                        else
                        {
                           audio_buf = frame->extended_data[0];
                           audio_buf_size = frame->linesize[0];
                        }

                        if (audio_buf_size > 0)
                           bytes.insert(bytes.end(), &audio_buf[0], &audio_buf[audio_buf_size]);

                        if (swr_ctx != NULL)
                           av_freep(&audio_buf);
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

               av_packet_unref(&readingPacket);
            }

            av_packet_unref(&readingPacket);
            av_frame_free(&frame);

            return eofReached;
         }

         void StoppedEventCallback(Object^ state)
         {
            PlayerStopped(this, static_cast<PathEventArgs^>(state));
         }

         void RaiseStoppedEvent(String^ path)
         {
            PathEventArgs^ e = gcnew PathEventArgs(path);
            if (context != nullptr)
               context->Post(gcnew SendOrPostCallback(this, &FFmpegAudioPlayer::StoppedEventCallback), e);
            else
               PlayerStopped(this, e);
         }

         void PlayingEventCallback(Object^ state)
         {
            PlayerStarted(this, static_cast<PathEventArgs^>(state));
         }

         void RaisePlayingEvent(String^ path)
         {
            PathEventArgs^ e = gcnew PathEventArgs(path);
            if (context != nullptr)
               context->Post(gcnew SendOrPostCallback(this, &FFmpegAudioPlayer::PlayingEventCallback), e);
            else
               PlayerStarted(this, e);
         }

      protected:
         !FFmpegAudioPlayer() { Release(); }

      public:
         FFmpegAudioPlayer() :
            pXAudio2(NULL), pMasteringVoice(NULL), pSourceVoice(NULL), container(NULL), codec_context(NULL), swr_ctx(NULL), volume(1.0f), audioStream(-1)
         {
            //CoInitializeEx(NULL, COINIT_APARTMENTTHREADED); /*COINIT_MULTITHREADED*/ /*COINIT_APARTMENTTHREADED*/
            InitializeAudioEngine();
#if defined(_DEBUG)
            av_log_set_level(AV_LOG_WARNING);
#endif
            av_register_all();
         }

         ~FFmpegAudioPlayer() { Release(); }

         void Play(String^ path)
         {
            if (pXAudio2 == NULL)
               throw gcnew ObjectDisposedException("pXAudio2");

            Stop();

            pin_ptr<AVFormatContext*> _container = &container;

            if (avformat_open_input(_container, ManagedStringToNativeUtf8One(path).c_str(), NULL, NULL) < 0)
               throw gcnew IOException(String::Format(ResourceStrings::GetString("CannotDecodeAudio"), "avformat_open_input"));

            if (avformat_find_stream_info(container, NULL) < 0)
            {
               CleanUpFFmpegResource();
               throw gcnew FormatException(String::Format(ResourceStrings::GetString("CannotDecodeAudio"), "avformat_find_stream_info"));
            }

            AVCodec* codec;
            audioStream = av_find_best_stream(container, AVMEDIA_TYPE_AUDIO, -1, -1, &codec, 0);

            if (audioStream < 0)
            {
               CleanUpFFmpegResource();
               throw gcnew FormatException(String::Format(ResourceStrings::GetString("CannotDecodeAudio"), "av_find_best_stream"));
            }

            codec_context = container->streams[audioStream]->codec;
            codec_context->request_sample_fmt = AV_SAMPLE_FMT_S16;

            if (avcodec_open2(codec_context, codec, NULL) < 0)
            {
               CleanUpFFmpegResource();
               throw gcnew FormatException(String::Format(ResourceStrings::GetString("CannotDecodeAudio"), "avcodec_open2"));
            }

            if (codec_context->sample_fmt != AV_SAMPLE_FMT_S16 && codec_context->sample_fmt != AV_SAMPLE_FMT_S32)
            {
               uint64_t channel_layout = codec_context->channel_layout;
               if (!channel_layout)
                  channel_layout = av_get_default_channel_layout(codec_context->channels);

               swr_ctx = swr_alloc_set_opts(NULL,
                  channel_layout, AV_SAMPLE_FMT_S16, codec_context->sample_rate, // out
                  channel_layout, codec_context->sample_fmt, codec_context->sample_rate, // in
                  0, NULL);

               if (!swr_ctx || swr_init(swr_ctx) < 0)
               {
                  CleanUpFFmpegResource();
                  throw gcnew FormatException(String::Format(ResourceStrings::GetString("CannotDecodeAudio"), "swr_alloc_set_opts, swr_init"));
               }
            }

            WAVEFORMATEXTENSIBLE wfx;
            wfx.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
            wfx.Format.nChannels = codec_context->channels;
            wfx.Format.nSamplesPerSec = codec_context->sample_rate;
            wfx.Samples.wValidBitsPerSample = codec_context->bits_per_raw_sample;
            wfx.Format.wBitsPerSample = codec_context->bits_per_raw_sample;
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
               throw gcnew Exception(String::Format(ResourceStrings::GetString("CannotUseAudioEngine"), "IXAudio2::CreateSourceVoice", hr));
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
               throw gcnew Exception(String::Format(ResourceStrings::GetString("CannotUseAudioEngine"), "IXAudio2SourceVoice::Start", hr));
            }

            file = path;

            cancellationToken = gcnew CancellationTokenSource();
            context = SynchronizationContext::Current;
            voiceFeederTask = Task::Factory->StartNew(gcnew Action(this, &FFmpegAudioPlayer::FeedSourceVoice), cancellationToken->Token, TaskCreationOptions::LongRunning, TaskScheduler::Default);

            RaisePlayingEvent(path);
         }

         void Stop()
         {
            if (voiceFeederTask != nullptr && !voiceFeederTask->IsCompleted)
            {
               cancellationToken->Cancel();
               voiceFeederTask->Wait();
            }
         }

         event EventHandler<PathEventArgs^>^ PlayerStopped;
         event EventHandler<PathEventArgs^>^ PlayerStarted;

         property String^ PlayingFile
         {
            String^ get() { return file; }
         }

         property float Volume
         {
            float get() { return volume; }
            void set(float value)
            {
               // A volume level of 1.0 means there is no attenuation or gain and 0 means silence. 
               // Negative levels can be used to invert the audio's phase.
               if (pMasteringVoice == NULL)
                  throw gcnew ObjectDisposedException("pMasteringVoice");

               if (value < 0) value = 0;
               if (value > XAUDIO2_MAX_VOLUME_LEVEL) value = XAUDIO2_MAX_VOLUME_LEVEL;
               if (value == volume && !isMuted) return;
               pMasteringVoice->SetVolume(value);
               isMuted = false;
               volume = value;
            }
         }

         property bool Muted
         {
            bool get() { return isMuted; }
            void set(bool value)
            {
               if (pMasteringVoice == NULL)
                  throw gcnew ObjectDisposedException("pMasteringVoice");

               if (value == isMuted) return;

               if (value)
                  pMasteringVoice->SetVolume(0);
               else
                  pMasteringVoice->SetVolume(volume);

               isMuted = value;
            }
         }

         property bool Paused
         {
            bool get() { return isPaused; }
            void set(bool value)
            {
               if (value == isPaused) return;

               if (pSourceVoice == NULL)
                  throw gcnew ObjectDisposedException("pSourceVoice");

               if (value)
                  pSourceVoice->Stop();
               else
                  pSourceVoice->Start();

               isPaused = value;
            }
         }
      };
   }
}