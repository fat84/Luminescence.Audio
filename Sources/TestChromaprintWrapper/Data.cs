using System;

namespace Metatogger.Data
{
   public class AudioFile : IComparable<AudioFile>
   {
      public string FullPath { get; }
      public int[] Fingerprint { get; set; }
      public int SimilarityGroupId { get; set; }

      public AudioFile(string path)
      {
         FullPath = path;
      }

      public int CompareTo(AudioFile other) => String.Compare(FullPath, other.FullPath, true);
   }
}