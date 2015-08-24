#pragma once

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

#define XAUDIO2_QUEUED_BUFFERS 3
#define XAUDIO2_BUFFER_SECONDS 3

using namespace System;
using namespace System::IO;
using namespace System::Threading;
using namespace System::Threading::Tasks;

namespace Luminescence
{
   namespace Audio
   {    
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

      public ref class FFmpegAudioPlayer
      {
      private:
         void Release();
         void InitializeAudioEngine();
         void ShutdownAudioEngine();
         void CleanUpFFmpegResource();
         void FeedSourceVoice();
         void StopAudioEngine();
         void CleanUpSourceVoiceResource();
         bool FeedSourceVoiceBuffer(int buffersToQueue, UINT32 minBufferSize);

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

      protected:
         !FFmpegAudioPlayer() { Release(); }

      public:
         FFmpegAudioPlayer();
         ~FFmpegAudioPlayer() { Release(); }

         void Play(String^ path);
         void Stop();

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
                  throw gcnew ObjectDisposedException("You cannot use this instance of audio player because it has been disposed.");

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
                  throw gcnew ObjectDisposedException("You cannot use this instance of audio player because it has been disposed.");

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
                  throw gcnew InvalidOperationException("You cannot pause the player because it doesn't play anything.");

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