﻿using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Luminescence.Audio;

namespace TestTaglibWrapper
{
   class Program
   {
      static void Main(string[] args)
      {
         TaglibSettings.MinId3Version = Id3Version.id3v23;
         TaglibSettings.MaxId3Version = Id3Version.id3v24;

         bool workOnCopy = true;
         var paths = new[]
         {
            @"C:\Users\cyber\Downloads\03 - Hey Brother.flac",
            @"C:\Users\cyber\Downloads\00 All that she wants.mp3"
         };

         foreach (string path in paths)
         {
            string realPath = workOnCopy ? WorkOnCopy(path) : path;

            ReadAudioFile(realPath);
            WriteAudioFile(realPath);
            ReadAudioFile(realPath);

            if (workOnCopy)
               File.Delete(realPath);

            Console.WriteLine("----------------------------------------");
         }

         Console.WriteLine("Press any key to exit...");
         Console.ReadKey();
      }

      private static void WriteAudioFile(string path)
      {
         Console.WriteLine($"Ecriture du fichier {path}");

         TaglibTagger tagger = new TaglibTagger(path);
         var tags = tagger.Tags;

         tags.Remove("ARTIST");
         Console.WriteLine(@"Ajout: ARTIST=""artist 1""");
         AddTag(tags, "ARTIST", "artist 1");
         Console.WriteLine(@"Ajout: ARTIST=""artist 2""");
         AddTag(tags, "ARTIST", "artist 2");

         Console.WriteLine(@"Ajout: FAKE=""toto""");
         AddTag(tags, "FAKE", "toto");

         tagger.Pictures.Clear();

         Console.WriteLine("Ajout d'une pochette : FrontCover, JPEG, 385 918 octets, sans description");
         string cover = @"C:\Users\cyber\Downloads\folder.jpg";
         tagger.Pictures.Add(new Picture(File.ReadAllBytes(cover), Format.JPEG, PictureType.FrontCover, null));

         tagger.SaveTags();
         Console.WriteLine();
      }

      private static void ReadAudioFile(string path)
      {
         Console.WriteLine($"Lecture du fichier {path}");

         TaglibTagger tagger = new TaglibTagger(path);

         foreach (var tag in tagger.Tags)
         {
            Console.WriteLine($"{tag.Key}:");
            foreach (var value in tag.Value)
               Console.WriteLine($"   {value}");

            Console.WriteLine();
         }

         foreach (var cover in tagger.Pictures)
         {
            Console.WriteLine("COVER ART:");
            Console.WriteLine($"   Type={cover.Type}");
            Console.WriteLine($"   Description={cover.Description}");
            Console.WriteLine($"   PictureFormat={cover.PictureFormat}");
            Console.WriteLine($"   Size={cover.Data.Length} bytes");

            Console.WriteLine();
         }
      }

      private static string WorkOnCopy(string path)
      {
         string newPath = Path.Combine(Path.GetDirectoryName(path), Path.ChangeExtension(Path.GetRandomFileName(), Path.GetExtension(path)));
         if (File.Exists(newPath))
            return WorkOnCopy(path);

         File.Copy(path, newPath, false);
         return newPath;
      }

      private static void AddTag(IDictionary<string, List<string>> tags, string name, string content)
      {
         if (tags.ContainsKey(name))
         {
            if (!tags[name].Contains(content))
               tags[name].Add(content);
         }
         else
         {
            tags.Add(name, new List<string>(new[] { content }));
         }
      }
   }
}