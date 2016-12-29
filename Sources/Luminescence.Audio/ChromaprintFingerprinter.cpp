#include "stdafx.h"

#define USE_SWRESAMPLE
#define HAVE_AV_PACKET_UNREF
#define HAVE_AV_FRAME_ALLOC
#define HAVE_AV_FRAME_FREE

#include <fingerprinter.h>
#include <fingerprinter_configuration.h>
#include <chromaprint.h>
#include <audio/ffmpeg_audio_reader.h>

#include "Interop.h"
#include "ResourceStrings.h"

using namespace System;
using namespace chromaprint;

// defined in match_fingerprints.cpp
float match_fingerprints3(uint32_t *a, int asize, uint32_t *b, int bsize, int maxoffset);

namespace Luminescence
{
   namespace Audio
   {
      public ref class ChromaprintFingerprinter abstract sealed
      {
      private:
         static void ThrowCannotDecodeAudioFingerprintException(const char* message)
         {
            throw gcnew Exception(String::Format(ResourceStrings::GetString("CannotDecodeAudioFingerprint"), gcnew String(message)));
         }

         static void ThrowCannotCalculateFingerprintException(const char* message)
         {
            throw gcnew Exception(String::Format(ResourceStrings::GetString("CannotCalculateFingerprint"), gcnew String(message)));
         }

         // https://github.com/acoustid/chromaprint/blob/v1.4.1/src/cmd/fpcalc.cpp
         static void ComputeFingerprint(String^ path, int length, ChromaprintContext* chromaprint_ctx)
         {
            FFmpegAudioReader reader;
            reader.SetOutputChannels(chromaprint_get_num_channels(chromaprint_ctx));
            reader.SetOutputSampleRate(chromaprint_get_sample_rate(chromaprint_ctx));

            if (!reader.Open(ManagedStringToNativeUtf8One(path)))
               ThrowCannotDecodeAudioFingerprintException(reader.GetError().c_str());

            if (!chromaprint_start(chromaprint_ctx, reader.GetSampleRate(), reader.GetChannels()))
               ThrowCannotDecodeAudioFingerprintException("Could not initialize the fingerprinting process");

            size_t stream_size = 0;
            const size_t stream_limit = length * reader.GetSampleRate();

            while (!reader.IsFinished())
            {
               const int16_t *frame_data = nullptr;
               size_t frame_size = 0;
               if (!reader.Read(&frame_data, &frame_size))
                  ThrowCannotDecodeAudioFingerprintException(reader.GetError().c_str());

               bool stream_done = false;
               if (stream_limit > 0)
               {
                  const size_t remaining = stream_limit - stream_size;
                  if (frame_size > remaining)
                  {
                     frame_size = remaining;
                     stream_done = true;
                  }
               }

               stream_size += frame_size;

               if (frame_size == 0)
               {
                  if (stream_done)
                     break;
                  else
                     continue;
               }

               if (!chromaprint_feed(chromaprint_ctx, frame_data, frame_size * reader.GetChannels()))
                  ThrowCannotCalculateFingerprintException("Could not process audio data");

               if (stream_done)
                  break;
            }

            if (!chromaprint_finish(chromaprint_ctx))
               ThrowCannotCalculateFingerprintException("Could not finish the fingerprinting process");

            int size;
            if (!chromaprint_get_raw_fingerprint_size(chromaprint_ctx, &size))
               ThrowCannotCalculateFingerprintException("Could not get the fingerprinting size");

            if (size <= 0)
               ThrowCannotCalculateFingerprintException("Empty fingerprint");
         };

      public:
         static array<uint32_t>^ GetRawFingerprint(String^ path) { return GetRawFingerprint(path, 120); }
         static array<uint32_t>^ GetRawFingerprint(String^ path, int length)
         {
            ChromaprintContext *chromaprint_ctx = chromaprint_new(CHROMAPRINT_ALGORITHM_DEFAULT);
            uint32_t *raw_fp_data = nullptr;
            try
            {
               ComputeFingerprint(path, length, chromaprint_ctx);

               int raw_fp_size = 0;
               if (!chromaprint_get_raw_fingerprint(chromaprint_ctx, &raw_fp_data, &raw_fp_size))
                  ThrowCannotCalculateFingerprintException("Could not get the fingerprinting");

               return NativeIntArrayToManagedOne(raw_fp_data, raw_fp_size);
            }
            finally
            {
               chromaprint_dealloc(raw_fp_data);
               chromaprint_free(chromaprint_ctx);
            }
         };

         static String^ GetEncodedFingerprint(String^ path) { return GetEncodedFingerprint(path, 120); }
         static String^ GetEncodedFingerprint(String^ path, int length)
         {
            ChromaprintContext *chromaprint_ctx = chromaprint_new(CHROMAPRINT_ALGORITHM_DEFAULT);
            char *fp = nullptr;
            try
            {
               ComputeFingerprint(path, length, chromaprint_ctx);

               if (!chromaprint_get_fingerprint(chromaprint_ctx, &fp))
                  ThrowCannotCalculateFingerprintException("Could not get the fingerprinting");

               return gcnew String(fp);
            }
            finally
            {
               chromaprint_dealloc(fp);
               chromaprint_free(chromaprint_ctx);
            }
         };

         static String^ EncodeFingerprintBase64(array<uint32_t>^ fingerprint)
         {
            pin_ptr<uint32_t> _fingerprint = &fingerprint[0];
            char *encoded;
            int encoded_size;
            chromaprint_encode_fingerprint(_fingerprint, fingerprint->Length, CHROMAPRINT_ALGORITHM_DEFAULT, &encoded, &encoded_size, 1);
            String^ fp = gcnew String(encoded, 0, encoded_size);
            chromaprint_dealloc(encoded);
            return fp;
         };

         static array<uint32_t>^ DecodeFingerprintBase64(String^ fingerprint)
         {
            uint32_t *fp;
            int length, algorithm;
            std::string encoded_fp = msclr::interop::marshal_as<std::string>(fingerprint);
            chromaprint_decode_fingerprint(encoded_fp.c_str(), encoded_fp.size(), &fp, &length, &algorithm, 1);
            array<uint32_t>^ mfp = NativeIntArrayToManagedOne(fp, length);
            chromaprint_dealloc(fp);
            return mfp;
         }

         static float MatchFingerprints(array<uint32_t>^ fingerprint1, array<uint32_t>^ fingerprint2) { return MatchFingerprints(fingerprint1, fingerprint2, -1); }
         static float MatchFingerprints(array<uint32_t>^ fingerprint1, array<uint32_t>^ fingerprint2, int maxoffset)
         {
            pin_ptr<uint32_t> _fingerprint1 = &fingerprint1[0];
            pin_ptr<uint32_t> _fingerprint2 = &fingerprint2[0];
            return match_fingerprints3(_fingerprint1, fingerprint1->Length, _fingerprint2, fingerprint2->Length, maxoffset);
         };

         static float CompareFile(String^ path1, String^ path2)
         {
            return MatchFingerprints(GetRawFingerprint(path1, 0), GetRawFingerprint(path2, 0), -1);
         }

         static property Version^ ChromaprintAlgorithmVersion
         {
            Version^ get() { return gcnew Version(CHROMAPRINT_VERSION_MAJOR, CHROMAPRINT_VERSION_MINOR, CHROMAPRINT_VERSION_PATCH); }
         }
      };
   }
}