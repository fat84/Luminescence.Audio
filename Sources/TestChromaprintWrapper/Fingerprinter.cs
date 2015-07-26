using System;
using System.Collections.Generic;
using MetatOGGer.Data;
using Luminescence.Audio;

namespace MetatOGGer.Business
{
   public static class Fingerprinter
   {
      private static readonly int[] bitcounts = InitializeBitcounts();

      public static void ComputeFingerprinting(IList<AudioFile> fp)
      {
         foreach (AudioFile file in fp)
         {
            if (file.Fingerprint != null) continue;
            //file.Fingerprint = FingerprintDatabase.Instance.GetFingerprint(file.FullPath);
            
            if (file.Fingerprint == null)
            {
               file.Fingerprint = ChromaprintFingerprinter.GetFingerprint(file.FullPath);

               //Console.WriteLine(file.FullPath);
               //Console.WriteLine(ChromaprintFingerprinter.EncodeFingerprint(file.Fingerprint));
               //Console.WriteLine();

               //FingerprintDatabase.Instance.InsertFingerprint(file.FullPath, file.Fingerprint);
            }
         }
      }

      public static List<List<AudioFile>> GetDuplicates(IList<AudioFile> fp, float level)
      {
         foreach (AudioFile file in fp)
            file.SimilarityGroupId = 0;
         
         var duplicates = new List<List<AudioFile>>();
         var indexes = new List<int>();
         float[,] matrix = ComputeSimilarities(fp);
         int id = 1;

         for (int row = 0; row < fp.Count; row++)
         {
            if (indexes.Contains(row)) continue;
            
            var files = new List<AudioFile>();
            for (int col = 0; col < fp.Count; col++)
            {
               if (matrix[row, col] >= level)
               {
                  files.Add(fp[col]);
                  indexes.Add(col);
               }
            }

            if (files.Count > 1)
            {                              
               foreach (AudioFile file in files)
                 file.SimilarityGroupId = id;
               
               duplicates.Add(files);
               id++;
            }
         }

         return duplicates;
      }

      private static float[,] ComputeSimilarities(IList<AudioFile> fp)
      {
         var matrix = new float[fp.Count, fp.Count];

         // initialisation de la matrice à -1
         for (int row = 0; row < fp.Count; row++)
            for (int col = 0; col < fp.Count; col++)
               matrix[row, col] = -1f;

         // remplissage de la diagonale de la matrice à 1
         for (int i = 0; i < fp.Count; i++)
            matrix[i, i] = 1f;

         // comparaison des éléments
         for (int i = 0; i < fp.Count; i++) //Parallel.For
            for (int j = 0; j < fp.Count; j++)
               if (matrix[i, j] == -1f)
                  matrix[i, j] = matrix[j, i] = ComputeScore(fp[i].Fingerprint, fp[j].Fingerprint);
         
         return matrix;
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
         int position1 = -1;
         int position2 = -1;

         for (int i = 1; i < 65536; i++, position1++)
         {
            if (position1 == position2)
            {
               position1 = 0;
               position2 = i;
            }

            bitcounts[i] = bitcounts[position1] + 1;
         }

         return bitcounts;
      }

      private static int PrecomputedBitcount(int value) => bitcounts[value & 65535] + bitcounts[(value >> 16) & 65535];
   }
}