using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using Metatogger.Data;

namespace Metatogger.Business
{
   public static class Fingerprinter
   {
      private static readonly int[] bitcounts = InitializeBitcounts();

      public static List<AudioFile> GetDuplicates(List<AudioFile> files, AudioFile file, float level)
      {
         var candidates = files/*.AsParallel()*/.Where(af =>
                          af.SimilarityGroupId == 0 && af != file/* &&
                          af.Fingerprint.Intersect(file.Fingerprint).FirstOrDefault() != 0*/).ToList();

         if (candidates.Count == 0)
            return candidates;

         var dups = new List<AudioFile>();
         var matrix = new float[candidates.Count];
         Parallel.For(0, matrix.Length, j => { matrix[j] = ComputeScore(candidates[j].Fingerprint, file.Fingerprint); });
         for (int j = 0; j < matrix.Length; j++)
         {
            if (matrix[j] >= level)
               dups.Add(candidates[j]);
         }

         return dups;
      }

      private static float ComputeScore(int[] a, int[] b)
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
               int biterror = PrecomputedBitcount(a[i] ^ b[j]);
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

      private static int[] InitializeBitcounts()
      {
         var bitcounts = new int[65536];
         int p1 = -1;
         int p2 = -1;

         for (int i = 1; i < bitcounts.Length; i++, p1++)
         {
            if (p1 == p2)
            {
               p1 = 0;
               p2 = i;
            }

            bitcounts[i] = bitcounts[p1] + 1;
         }

         return bitcounts;
      }

      private static int PrecomputedBitcount(int value) => bitcounts[value & 65535] + bitcounts[(value >> 16) & 65535];
   }
}