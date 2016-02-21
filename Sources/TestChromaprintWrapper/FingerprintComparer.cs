using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using Metatogger.Data;

namespace Metatogger.Business
{
   public static class FingerprintComparer
   {
      public static List<AudioFile> GetDuplicates(List<AudioFile> files, AudioFile file, float level)
      {
         //int[] fingerprint = await GetFingerprintAsync(file.FullPath, 0);
         //var candidates = await Task.Run(() =>
         //  files.AsParallel().Where(af => af.SimilarityGroupId == 0 && af != file &&
         //  GetFingerprintAsync(af.FullPath, 0).Result.Intersect(fingerprint).FirstOrDefault() != 0).ToList());

         var candidates = files.Where(af => af.SimilarityGroupId == 0 && af != file).ToList();

         if (candidates.Count == 0)
            return candidates;

         var dups = new List<AudioFile>();
         var matrix = new float[candidates.Count];

         Parallel.For(0, matrix.Length, i => { matrix[i] = MatchFingerprints3(candidates[i].Fingerprint, file.Fingerprint); });
         for (int i = 0; i < matrix.Length; i++)
         {
            if (matrix[i] >= level)
               dups.Add(candidates[i]);
         }

         return dups;
      }

      public static float MatchFingerprints(int[] a, int[] b)
      {
         const int ACOUSTID_MAX_BIT_ERROR = 2;
         const int ACOUSTID_MAX_ALIGN_OFFSET = 120;

         int maxsize = Math.Max(a.Length, b.Length);
         int numcounts = maxsize * 2 + 1;
         int[] counts = new int[numcounts];

         for (int i = 0; i < a.Length; i++)
         {
            int jbegin = Math.Max(0, i - ACOUSTID_MAX_ALIGN_OFFSET);
            int jend = Math.Min(b.Length, i + ACOUSTID_MAX_ALIGN_OFFSET);
            for (int j = jbegin; j < jend; j++)
            {
               int biterror = PopCount(a[i] ^ b[j]);
               if (biterror <= ACOUSTID_MAX_BIT_ERROR)
               {
                  int offset = i - j + maxsize;
                  counts[offset]++;
               }
            }
         }

         int topcount = 0;
         for (int i = 0; i < numcounts; i++)
            if (counts[i] > topcount)
               topcount = counts[i];

         return (float)topcount / Math.Min(a.Length, b.Length);
      }

      // http://stackoverflow.com/questions/109023/how-to-count-the-number-of-set-bits-in-a-32-bit-integer
      private static int PopCount(int i)
      {
         i = i - ((i >> 1) & 0x55555555);
         i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
         return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
      }

      // https://bitbucket.org/acoustid/pg_acoustid  -->  acoustid_compare.c
      public static float MatchFingerprints3(int[] a, int[] b, int maxoffset = -1)
      {
         int jbegin = 0;
         int jend = b.Length;
         int numcounts = a.Length + b.Length + 1;
         var counts = new int[numcounts];

         for (int i = 0; i < a.Length; i++)
         {
            if (maxoffset > -1)
            {
               jbegin = Math.Max(0, i - maxoffset);
               jend = Math.Min(b.Length, i + maxoffset);
            }

            for (int j = jbegin; j < jend; j++)
            {
               int biterror = PopCount(a[i] ^ b[j]);
               // Randomly selected blocks share around half their bits, so only count errors less than 16 bits
               if (biterror < 16)
                  counts[i - j + b.Length] += 16 - biterror;
            }
         }

         int topcount = 0;
         for (int i = 0; i < numcounts; i++)
         {
            if (counts[i] > topcount)
               topcount = counts[i];
         }

         return topcount / (16f * Math.Min(a.Length, b.Length));
      }
   }
}