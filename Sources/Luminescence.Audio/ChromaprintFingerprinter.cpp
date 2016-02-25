#include "stdafx.h"

extern "C"
{
#include <libavformat/avformat.h>
}

#include "fingerprinter.h"
#include "fingerprinter_configuration.h"

#include "Interop.h"
#include "ResourceStrings.h"

using namespace System;

// defined in decode_audio_file.cpp
int decode_audio_file(ChromaprintContext *chromaprint_ctx, const char *file_name, int max_length, int *duration, String^% error);

// defined in match_fingerprints.cpp
float match_fingerprints3(int *a, int asize, int *b, int bsize, int maxoffset);

namespace Luminescence
{
   namespace Audio
   {
      public ref class ChromaprintFingerprinter abstract sealed
      {
      private:
         static ChromaprintFingerprinter() { av_register_all(); }

      public:
         static array<int>^ GetFingerprint(String^ path) { return GetFingerprint(path, 120); }
         static array<int>^ GetFingerprint(String^ path, int length)
         {
            ChromaprintContext* chromaprint_ctx = chromaprint_new(CHROMAPRINT_ALGORITHM_DEFAULT);

            String^ error = nullptr;
            int duration;
            if (!decode_audio_file(chromaprint_ctx, ManagedStringToNativeUtf8One(path).c_str(), length, &duration, error))
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
               throw gcnew Exception(String::Format(ResourceStrings::GetString("CannotCalculateFingerprint"), "chromaprint_get_raw_fingerprint"));
            }

            array<int>^ mfp = NativeIntArrayToManagedOne(raw_fingerprint, raw_fingerprint_size);
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

         static float MatchFingerprints(array<int>^ fingerprint1, array<int>^ fingerprint2) { return MatchFingerprints(fingerprint1, fingerprint2, -1); }
         static float MatchFingerprints(array<int>^ fingerprint1, array<int>^ fingerprint2, int maxoffset)
         {
            pin_ptr<int> _fingerprint1 = &fingerprint1[0];
            pin_ptr<int> _fingerprint2 = &fingerprint2[0];
            return match_fingerprints3(_fingerprint1, fingerprint1->Length, _fingerprint2, fingerprint2->Length, maxoffset);
         };

         static float CompareFile(String^ path1, String^ path2)
         {
            return MatchFingerprints(GetFingerprint(path1, 0), GetFingerprint(path2, 0), -1);
         }

         static property Version^ ChromaprintAlgorithmVersion
         {
            Version^ get() { return gcnew Version(CHROMAPRINT_VERSION_MAJOR, CHROMAPRINT_VERSION_MINOR, CHROMAPRINT_VERSION_PATCH); }
         }
      };
   }
}