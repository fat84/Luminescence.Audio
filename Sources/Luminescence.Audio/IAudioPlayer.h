#pragma once

using namespace System;

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

      public interface class IAudioPlayer : public IDisposable
      {
         void Play(String^ path);
         void Stop();

         event EventHandler<PathEventArgs^>^ PlayerStopped;
         event EventHandler<PathEventArgs^>^ PlayerStarted;

         property String^ PlayingFile { String^ get(); }
         property float Volume;
         property bool Muted;
         property bool Paused;
      };
   }
}