#pragma once

#include "TaglibHelper.h"

using namespace System;
using namespace System::IO;

namespace Luminescence
{
   namespace Audio
   {
      [FileFormat("MP3", ".mp3")]
      [FileFormat("FLAC (Free Lossless Audio Codec)", ".flac")]
      public ref class TaglibTagger
      {
      private:
         String^ fullPath;

         String^ codec;
         String^ codecVersion;
         byte channels;
         int duration;
         int bitrate;
         int sampleRate;
         byte bitsPerSample;

         Dictionary<String^, List<String^>^>^ tags;
         List<Picture^>^ pictures;

         static Dictionary<String^, List<String^>^>^ MapToDictionary(TagLib::PropertyMap map);
         static TagLib::PropertyMap DictionaryToMap(Dictionary<String^, List<String^>^>^ dic);

         void ReadTags(String^ path);

         void ReadFlacFile(String^ path);
         bool WriteFlacFile();

         void ReadMp3File(String^ path);
         bool WriteMp3File();

      public:
         TaglibTagger(String^ path) { ReadTags(path); }

         property String^ Source 
         { 
            String^ get() { return fullPath; } 
            void set(String^ value) 
            { 
               if (!String::Equals(fullPath, value, StringComparison::OrdinalIgnoreCase))
                  ReadTags(value);
            }
         }

         property String^ Codec { String^ get() { return codec; } }
         property String^ CodecVersion { String^ get() { return codecVersion; } }
         property byte Channels { byte get() { return channels; } }
         property int Duration { int get() { return duration; } }
         property int Bitrate { int get() { return bitrate; } }
         property int SampleRate { int get() { return sampleRate; } }
         property byte BitsPerSample { byte get() { return bitsPerSample; } }

         property Dictionary<String^, List<String^>^>^ Tags { Dictionary<String^, List<String^>^>^ get() { return tags; } }
         property List<Picture^>^ Pictures { List<Picture^>^ get() { return pictures; } } // return nullptr if the format doesn't support cover, return empty collection if there is no cover

         bool SaveTags();
      };
   }
}

