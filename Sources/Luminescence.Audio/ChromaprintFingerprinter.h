#pragma once

#include <msclr\marshal.h>
#include <msclr\marshal_cppstd.h>

extern "C" 
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>
}

#include "fingerprinter.h"
#include "fingerprinter_configuration.h"
#include "chromaprint.h"

using namespace System;
using namespace System::Runtime::InteropServices;

namespace Luminescence
{
   namespace Audio 
   {
      static array<int>^ ConvertNativeToManagedArray(const int* data, int size)
      {
         array<int>^ buffer = gcnew array<int>(size);
         pin_ptr<int> buffer_start = &buffer[0];
         memcpy(buffer_start, data, size);
         return buffer;
      }

      public ref class ChromaprintFingerprinter abstract sealed
      {
      private:
         static int decode_audio_file(ChromaprintContext *chromaprint_ctx, const char *file_name, int max_length, int *duration, String^% error);
         static ChromaprintFingerprinter() { av_register_all(); }

      public:                       
         static array<int>^ GetFingerprint(String^ path) { return GetFingerprint(path, 120); }
         static array<int>^ GetFingerprint(String^ path, int length)
         {
            std::string file_name = msclr::interop::marshal_as<std::string>(path);
            ChromaprintContext* chromaprint_ctx = chromaprint_new(CHROMAPRINT_ALGORITHM_DEFAULT);

            String^ error = nullptr;
            int duration;
            if (!decode_audio_file(chromaprint_ctx, file_name.c_str(), length, &duration, error))
            {
               chromaprint_free(chromaprint_ctx);
               throw gcnew Exception(error);
            }

            int32_t* raw_fingerprint;
            int raw_fingerprint_size = 0;
            if (!chromaprint_get_raw_fingerprint(chromaprint_ctx, (void **)&raw_fingerprint, &raw_fingerprint_size) || raw_fingerprint_size == 0)
            {
               chromaprint_dealloc(raw_fingerprint);
               chromaprint_free(chromaprint_ctx);
               throw gcnew Exception("Not enough memory to get raw fingerprint.");
            }

            array<int>^ mfp = ConvertNativeToManagedArray(raw_fingerprint, raw_fingerprint_size);
            chromaprint_dealloc(raw_fingerprint);
            chromaprint_free(chromaprint_ctx);

            return mfp;			
         };

         static String^ EncodeFingerprint(array<int>^ fingerprint)
         {
            pin_ptr<int> _fingerprint = &fingerprint[0];
            char *encoded;
            int encoded_size;
            chromaprint_encode_fingerprint(_fingerprint, fingerprint->Length, CHROMAPRINT_ALGORITHM_DEFAULT, (void **)&encoded, &encoded_size, 1);
            String^ fp = gcnew String(encoded, 0, encoded_size);
            chromaprint_dealloc(encoded);
            return fp;
         };

         static property Version^ ChromaprintAlgorithmVersion
         {
            Version^ get() { return gcnew Version(CHROMAPRINT_VERSION_MAJOR, CHROMAPRINT_VERSION_MINOR, CHROMAPRINT_VERSION_PATCH); }
         }
      };
   }	
}