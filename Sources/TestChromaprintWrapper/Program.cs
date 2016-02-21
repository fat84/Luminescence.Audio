using Luminescence.Audio;
using Metatogger.Business;
using Metatogger.Data;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Net;
using System.Text;
using System.Xml.Linq;

namespace TestChromaprintWrapper
{
   class Program
   {
      static void Main(string[] args)
      {
         //TestFingerprinting();
         BenchmarkFingerprintComparer();
         //TestLookup();

         //var s = ChromaprintFingerprinter.EncodeFingerprint(new int[0]);

         Console.Write("Press any key to continue . . . ");
         Console.ReadKey(true);
      }

      private static void BenchmarkFingerprintComparer()
      {
         var sw = new Stopwatch();
         var fingerprint1 = new int[1981];
         var fingerprint2 = new int[2016];

         sw.Start();
         for (int i = 0; i < 100; i++)
         {
            FingerprintComparer.MatchFingerprints3(fingerprint1, fingerprint2);
         }
         sw.Stop();
         Console.WriteLine(sw.ElapsedMilliseconds);

         //sw.Restart();
         //for (int i = 0; i < 100; i++)
         //{
         //   FingerprintComparer.MatchFingerprints4(fingerprint1, fingerprint2);
         //}
         //sw.Stop();
         //Console.WriteLine(sw.ElapsedMilliseconds);
      }

      private static void TestLookup()
      {
         string file = @"E:\Musique\Audiothèque\FLAC\Kyo\Le chemin\01 - Le chemin.flac";
         int[] fp1 = ChromaprintFingerprinter.GetFingerprint(file, 60);
         string fp2 = ChromaprintFingerprinter.EncodeFingerprint(fp1);
         //string fp3 = new StringBuilder(fp2).Replace('A', 'Q', 1, 1).ToString();
         int seconds = 210;

         //string bad =  "AAABz9GSJEkkKYmS4Tp6xvjx-tBpXMfRpUP1w3yGHmUPcWTwhnhgiYNeeBcu5HhbxA2PZuEFnSd-9FECq-gJPbtQ_fiLfkf3wvnRl9AO__jxCj7-QjzyoFek4xcmPnA2-qjUDz9yTsR56PFRJp4GP8ePLj2qY-NRfkL2oXEx6cFThPCh7_iRVnzQbX9wHc0UT0I_5Duao1KVGOsPlpdQLcqFHxf5oqfRFVpySvjR_Ch7_NCO87h05ChL7Do65vDIFC2H8zhyKnhqXJ6GdsuLw4c276h-TGEcfAfjmWgeYzy6HT6u5UhvhCceZSp0PD5iZzka9Zh05NOEZmsgxZTx50ifa8hxpRv0E1vHKciFHk0cFY0moRKzabgbDb3gH_2RH8k3lGuEKc_x3_hRPehX5ODRfDX6JCtOIVyOMFyP-4XyHWkaF9fRt0VzD7sOJlUkHX2Q9Ed-9PnR8EpwijfCJJFi9PmhLySPhmOC-sIZiI8m_ML3INJ3pI6UknCNnkHzyNDEpchz400d9IN99PiSo9kyDf0xXTrS_HCC79D64Tx4o1YSIReeCKH84Q0ORT7CNMKPmQma62inD7xoodlEJNOJXEHzo3qCMEn4omWg_UPKqBN69EYz5kPyDrkeCb2OKd2i4RJ6PB3CQ5Wo5shTMUH7Hf6IHsd5NNcQnjFGo8oP__DJQz8OJk9s5BROotERSmSOjyaUv0LqsziV4zmaK5jIFLXAxEk0xIfeIcyFqiEVXL_hGLnQUOXRJ8SHSLnRr_jx5IU35CH0N0gfHtdQ_Wj0ZniT7ENZXPDRrijZIMxn7Dkqo0nIoG_QVDsO_mi6yOibDJqyHf6DSiX2fdAyVEyCXDyqhD2aRyf2H09EMJkchE-g60GYK3grNHHwHE9I9DrSMoJ2ZMd7_Ch7WEftI24DqTrCEteio9ePLxIVIj-uW1CjK6goVoYnhYZuuElwNDfyIT4e4lGWKWhEyUF5XKic47xQ0ZygO0XXhAgTMzP-FN_x4_lQiynCxBlxPgivIEyNL0e64zu-hCTO48_BieLxIzy0MEZ-MNcc-Bqc53gYUfCPh0c6MmFI6DWaijk-K8g_PNXRhKjTD72UBSXzgT_-YIx27EsUJ6iThkJTZUd-aM_R03iaCk-CUwITPUcPVVfwLA8m0UbTrLgSHR3-Y8-DUsKVB6EkJWh-_HgPTv3AZs_R23CcBfWRM1B1hB1K63iipLg9XETcZ0I3jynO8PiF64YtH1klhDvxhThXzFex3kG1HPZu9Dmu47mh5UGtHKp1POjyUMWfDJo6of8Gtgvy5EDfo1bgnWiX6wCGhEEAKADJIUAbILJzTAgDBDBCAgQQMEQQJABBwgJgBBMAAGaEAMwAh5wAikDkhQBGCIGkAgQASghihAmlEBDEMCOUAYYJ4pRVwhAEAiXEYWAUMchQBpQVhkwhgDUMAQAoMkgxJRUwjgFBBRBCEGM0BgBAAIhQRBJkkALCCAUMQIAIIQBQRDGDBAECAAGY4MYQIhSgBCEkKAHEggG8cAIYJIgUwJAHpCHAMAaEQEYo6IgAADgEAUIIBAQcMEIgYBkECEABCEYKAYYcA4oowBBABAAvBJJKMGIAEAYwA5QADAAGCINMaAKQJcQYjCCzxBBACAcGMM4II8IQKIghgFgABTOEUAegMQQYARgwUAkHADFACAIMcMwACQSABggFiJDCCYoAQMIR4wgQwhCjDBGAAYKAwkAbRcAhjhFkADAEEIcEQYgAxBAFBgiBlACCAA";
         string good = "AQABz9GSJEkkKYmS4Tp6xvjx-tBpXMfRpUP1w3yGHmUPcWTwhnhgiYNeeBcu5HhbxA2PZuEFnSd-9FECq-gJPbtQ_fiLfkf3wvnRl9AO__jxCj7-QjzyoFek4xcmPnA2-qjUDz9yTsR56PFRJp4GP8ePLj2qY-NRfkL2oXEx6cFThPCh7_iRVnzQbX9wHc0UT0I_5Duao1KVGOsPlpdQLcqFHxf5oqfRFVpySvjR_Ch7_NCO87h05ChL7Do65vDIFC2H8zhyKnhqXJ6GdsuLw4c276h-TGEcfAfjmWgeYzy6HT6u5UhvhCceZSp0PD5iZzka9Zh05NOEZmsgxZTx50ifa8hxpRv0E1vHKciFHk0cFY0moRKzabgbDb3gH_2RH8k3lGuEKc_x3_hRPehX5ODRfDX6JCtOIVyOMFyP-4XyHWkaF9fRt0VzD7sOJlUkHX2Q9Ed-9PnR8EpwijfCJJFi9PmhLySPhmOC-sIZiI8m_ML3INJ3pI6UknCNnkHzyNDEpchz400d9IN99PiSo9kyDf0xXTrS_HCC79D64Tx4o1YSIReeCKH84Q0ORT7CNMKPmQma62inD7xoodlEJNOJXEHzo3qCMEn4omWg_UPKqBN69EYz5kPyDrkeCb2OKd2i4RJ6PB3CQ5Wo5shTMUH7Hf6IHsd5NNcQnjFGo8oP__DJQz8OJk9s5BROotERSmSOjyaUv0LqsziV4zmaK5jIFLXAxEk0xIfeIcyFqiEVXL_hGLnQUOXRJ8SHSLnRr_jx5IU35CH0N0gfHtdQ_Wj0ZniT7ENZXPDRrijZIMxn7Dkqo0nIoG_QVDsO_mi6yOibDJqyHf6DSiX2fdAyVEyCXDyqhD2aRyf2H09EMJkchE-g60GYK3grNHHwHE9I9DrSMoJ2ZMd7_Ch7WEftI24DqTrCEteio9ePLxIVIj-uW1CjK6goVoYnhYZuuElwNDfyIT4e4lGWKWhEyUF5XKic47xQ0ZygO0XXhAgTMzP-FN_x4_lQiynCxBlxPgivIEyNL0e64zu-hCTO48_BieLxIzy0MEZ-MNcc-Bqc53gYUfCPh0c6MmFI6DWaijk-K8g_PNXRhKjTD72UBSXzgT_-YIx27EsUJ6iThkJTZUd-aM_R03iaCk-CUwITPUcPVVfwLA8m0UbTrLgSHR3-Y8-DUsKVB6EkJWh-_HgPTv3AZs_R23CcBfWRM1B1hB1K63iipLg9XETcZ0I3jynO8PiF64YtH1klhDvxhThXzFex3kG1HPZu9Dmu47mh5UGtHKp1POjyUMWfDJo6of8Gtgvy5EDfo1bgnWiX6wCGhEEAKADJIUAbILJzTAgDBDBCAgQQMEQQJABBwgJgBBMAAGaEAMwAh5wAikDkhQBGCIGkAgQASghihAmlEBDEMCOUAYYJ4pRVwhAEAiXEYWAUMchQBpQVhkwhgDUMAQAoMkgxJRUwjgFBBRBCEGM0BgBAAIhQRBJkkALCCAUMQIAIIQBQRDGDBAECAAGY4MYQIhSgBCEkKAHEggG8cAIYJIgUwJAHpCHAMAaEQEYo6IgAADgEAUIIBAQcMEIgYBkECEABCEYKAYYcA4oowBBABAAvBJJKMGIAEAYwA5QADAAGCINMaAKQJcQYjCCzxBBACAcGMM4II8IQKIghgFgABTOEUAegMQQYARgwUAkHADFACAIMcMwACQSABggFiJDCCYoAQMIR4wgQwhCjDBGAAYKAwkAbRcAhjhFkADAEEIcEQYgAxBAFBgiBlACCAA";

         if (good != fp2)
         {
            Debugger.Break();
         }

         XElement xml2 = AcoustidServices.SearchFromFingerprint(seconds, fp2);
         Console.WriteLine(xml2.ToString());
      }

      private static void TestFingerprinting()
      {
         var files = Directory.EnumerateFiles(@"E:\NAS\music\FLAC\Adele\21", "*.flac").Select(s => new AudioFile(s)).ToList();

         foreach (AudioFile file in files)
         {
            file.Fingerprint = ChromaprintFingerprinter.GetFingerprint(file.FullPath, 0);
         }

         //float mf1 = FingerprintComparer.MatchFingerprints(files[0].Fingerprint, files[1].Fingerprint);
         //float mf2 = FingerprintComparer.MatchFingerprints3(files[0].Fingerprint, files[1].Fingerprint);

         int id = 1;
         foreach (AudioFile file in files)
         {
            if (file.SimilarityGroupId == 0)
            {
               var duplicates = FingerprintComparer.GetDuplicates(files, file, 0.9f);

               if (duplicates.Count > 0)
               {
                  file.SimilarityGroupId = id;
                  foreach (var af in duplicates)
                     af.SimilarityGroupId = id;
                  id++;
               }
            }
         }

         foreach (var group in files.GroupBy(af => af.SimilarityGroupId))
         {
            foreach (var file in group)
            {
               Console.WriteLine($"[{file.SimilarityGroupId}] {file.FullPath} ({file.Fingerprint.Length * 4} octets)");
            }

            Console.WriteLine("-----------------------------------------------");
         }
      }
   }

   public static class AcoustidServices
   {
      public static XElement SearchFromFingerprint(int duration, string fingerprint)
      {
         var ut = new UriTemplate("v2/lookup?client=JnD0gnNt&meta=recordings&format=xml&duration={0}&fingerprint={1}");
         Uri url = ut.BindByPosition(new Uri("http://api.acoustid.org"), duration.ToString(), fingerprint);

         using (var web = new WebClient())
         {
            web.Encoding = Encoding.UTF8;
            return XElement.Parse(web.DownloadString(url));
         }
      }
   }
}