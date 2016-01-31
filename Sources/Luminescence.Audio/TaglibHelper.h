#pragma once

#include "mp4coverart.h"

using namespace System;
using namespace System::Collections::Generic;

namespace Luminescence
{
   namespace Audio
   {
      #pragma region Free functions

      static array<byte>^ ConvertByteVectorToManagedArray(const TagLib::ByteVector& data)
      {
         if (data.size() == 0)
            return gcnew array<byte>(0);

         array<byte>^ buffer = gcnew array<byte>(data.size());
         pin_ptr<byte> buffer_start = &buffer[0];
         memcpy(buffer_start, data.data(), buffer->Length);
         return buffer;
      }

      static TagLib::ByteVector ConvertManagedArrayToByteVector(array<byte>^ data)
      {
         if (data->Length == 0)
         {
            TagLib::ByteVector buffer;
            return buffer;
         }

         pin_ptr<byte> p = &data[0];
         unsigned char *pby = p;
         const char *pch = reinterpret_cast<char*>(pby);
         TagLib::ByteVector buffer(pch, data->Length);
         return buffer;
      }

      static TagLib::PropertyMap DictionaryToMap(Dictionary<String^, List<String^>^>^ dic)
      {
         TagLib::PropertyMap map;

         for each(auto kvp in dic)
         {
            TagLib::String key = msclr::interop::marshal_as<std::wstring>(kvp.Key);
            TagLib::StringList values;

            for each(auto value in kvp.Value)
            {
               values.append(msclr::interop::marshal_as<std::wstring>(value));
            }

            map.insert(key, values);
         }

         return map;
      }

      static Dictionary<String^, List<String^>^>^ MapToDictionary(TagLib::PropertyMap map)
      {
         auto tags = gcnew Dictionary<String^, List<String^>^>(map.size());

         for (auto it = map.begin(); it != map.end(); it++)
         {
            TagLib::String tag = it->first;
            TagLib::StringList values = it->second;

            auto ntag = gcnew String(tag.toCWString());
            tags->Add(ntag, gcnew List<String^>(values.size()));

            for (auto it2 = values.begin(); it2 != values.end(); it2++)
               tags[ntag]->Add(gcnew String(it2->toCWString()));
         }

         return tags;
      }

      static String^ GetVorbisVersionFromVendor(String^ vendor)
      {
         if (vendor->StartsWith("AO; aoTuV")) return "Vorbis aoTuV";

         if (vendor == "Xiphophorus libVorbis I 20000508") return "Vorbis 1.0 Beta 1/2";
         if (vendor == "Xiphophorus libVorbis I 20001031") return "Vorbis 1.0 Beta 3";
         if (vendor == "Xiphophorus libVorbis I 20010225") return "Vorbis 1.0 Beta 4";
         if (vendor == "Xiphophorus libVorbis I 20010615") return "Vorbis 1.0 RC1";
         if (vendor == "Xiphophorus libVorbis I 20010813") return "Vorbis 1.0 RC2";
         if (vendor == "Xiphophorus libVorbis I 20011217") return "Vorbis 1.0 RC3";
         if (vendor == "Xiphophorus libVorbis I 20011231") return "Vorbis 1.0 RC3";

         // https://trac.xiph.org/browser/trunk/vorbis/CHANGES
         if (vendor == "Xiph.Org libVorbis I 20020717") return "Vorbis 1.0.0";
         if (vendor == "Xiph.Org libVorbis I 20030909") return "Vorbis 1.0.1";
         if (vendor == "Xiph.Org libVorbis I 20040629") return "Vorbis 1.1.0";
         if (vendor == "Xiph.Org libVorbis I 20050304") return "Vorbis 1.1.1/2";
         if (vendor == "Xiph.Org libVorbis I 20070622") return "Vorbis 1.2.0";
         if (vendor == "Xiph.Org libVorbis I 20080501") return "Vorbis 1.2.1";
         if (vendor == "Xiph.Org libVorbis I 20090624") return "Vorbis 1.2.2";
         if (vendor == "Xiph.Org libVorbis I 20090709") return "Vorbis 1.2.3";
         if (vendor == "Xiph.Org libVorbis I 20100325 (Everywhere)") return "Vorbis 1.3.1";
         if (vendor == "Xiph.Org libVorbis I 20101101 (Schaufenugget)") return "Vorbis 1.3.2";
         if (vendor == "Xiph.Org libVorbis I 20120203 (Omnipresent)") return "Vorbis 1.3.3";
         if (vendor == "Xiph.Org libVorbis I 20140122 (Turpakäräjiin)") return "Vorbis 1.3.4";
         if (vendor == "Xiph.Org libVorbis I 20150105 (????)") return "Vorbis 1.3.5";

         return nullptr;
      }

      #pragma endregion

      #pragma region Managed classes

      public enum class Id3Version
      {
         id3v23 = 3,
         id3v24 = 4
      };

      public ref class TaglibSettings abstract sealed
      {
      public:
         static Id3Version MinId3Version = Id3Version::id3v23;
         static Id3Version MaxId3Version = Id3Version::id3v24;
      };

      public enum class PictureType
      {
         //! A type not enumerated below
         Other = TagLib::FLAC::Picture::Type::Other,
         //! 32x32 PNG image that should be used as the file icon
         FileIcon = TagLib::FLAC::Picture::Type::FileIcon,
         //! File icon of a different size or format
         OtherFileIcon = TagLib::FLAC::Picture::Type::OtherFileIcon,
         //! Front cover image of the album
         FrontCover = TagLib::FLAC::Picture::Type::FrontCover,
         //! Back cover image of the album
         BackCover = TagLib::FLAC::Picture::Type::BackCover,
         //! Inside leaflet page of the album
         LeafletPage = TagLib::FLAC::Picture::Type::LeafletPage,
         //! Image from the album itself
         Media = TagLib::FLAC::Picture::Type::Media,
         //! Picture of the lead artist or soloist
         LeadArtist = TagLib::FLAC::Picture::Type::LeadArtist,
         //! Picture of the artist or performer
         Artist = TagLib::FLAC::Picture::Type::Artist,
         //! Picture of the conductor
         Conductor = TagLib::FLAC::Picture::Type::Conductor,
         //! Picture of the band or orchestra
         Band = TagLib::FLAC::Picture::Type::Band,
         //! Picture of the composer
         Composer = TagLib::FLAC::Picture::Type::Composer,
         //! Picture of the lyricist or text writer
         Lyricist = TagLib::FLAC::Picture::Type::Lyricist,
         //! Picture of the recording location or studio
         RecordingLocation = TagLib::FLAC::Picture::Type::RecordingLocation,
         //! Picture of the artists during recording
         DuringRecording = TagLib::FLAC::Picture::Type::DuringRecording,
         //! Picture of the artists during performance
         DuringPerformance = TagLib::FLAC::Picture::Type::DuringPerformance,
         //! Picture from a movie or video related to the track
         MovieScreenCapture = TagLib::FLAC::Picture::Type::MovieScreenCapture,
         //! Picture of a large, coloured fish
         ColouredFish = TagLib::FLAC::Picture::Type::ColouredFish,
         //! Illustration related to the track
         Illustration = TagLib::FLAC::Picture::Type::Illustration,
         //! Logo of the band or performer
         BandLogo = TagLib::FLAC::Picture::Type::BandLogo,
         //! Logo of the publisher (record company)
         PublisherLogo = TagLib::FLAC::Picture::Type::PublisherLogo
      };

      public enum class Format
      {
         //Unknown = TagLib::MP4::CoverArt::Format::Unknown,
         JPEG = TagLib::MP4::CoverArt::Format::JPEG,
         PNG = TagLib::MP4::CoverArt::Format::PNG,
         GIF = TagLib::MP4::CoverArt::Format::GIF,
         BMP = TagLib::MP4::CoverArt::Format::BMP
      };

      public ref class Picture
      {
      public:
         Picture(array<byte>^ data, Format format, PictureType type, String^ description)
         {
            Data = data;
            PictureFormat = format;
            Type = type;
            Description = description;
         }

         Picture(array<byte>^ data, PictureType type, String^ description)
         {
            Data = data;
            PictureFormat = GetFormatFromData(data);
            Type = type;
            Description = description;
         }

         property array<byte>^ Data;
         property PictureType Type;
         property Format PictureFormat;
         property String^ Description;

         String^ GetMimeType() { return GetMimeTypeFromFormat(PictureFormat); }

         static String^ GetMimeTypeFromFormat(Format format)
         {
            switch (format)
            {
            case Format::JPEG:
               return "image/jpeg";
            case Format::PNG:
               return "image/png";
            case Format::GIF:
               return "image/gif";
            case Format::BMP:
               return "image/bmp";
            default:
               throw gcnew NotSupportedException("The picture format is not supported.");
            }
         }

         static Format GetFormatFromData(array<byte>^ data)
         {
            // https://en.wikipedia.org/wiki/List_of_file_signatures

            if (data->Length > 3 &&
               data[0] == 0xFF &&
               data[1] == 0xD8 &&
               data[2] == 0xFF)
               return Format::JPEG;

            if (data->Length > 8 &&
               data[0] == 0x89 &&
               data[1] == 0x50 &&
               data[2] == 0x4E &&
               data[3] == 0x47 &&
               data[4] == 0x0D &&
               data[5] == 0x0A &&
               data[6] == 0x1A &&
               data[7] == 0x0A)
               return Format::PNG;

            if (data->Length > 4 &&
               data[0] == 0x47 &&
               data[1] == 0x49 &&
               data[2] == 0x46 &&
               data[3] == 0x38)
               return Format::GIF;

            if (data->Length > 2 &&
               data[0] == 0x42 &&
               data[1] == 0x4D)
               return Format::BMP;

            throw gcnew NotSupportedException("The mime type is not supported");
         }

         byte GetAtomDataType() { return (byte)PictureFormat; }
      };

      [AttributeUsage(AttributeTargets::Class, AllowMultiple = true)]
      public ref class FileFormatAttribute sealed : Attribute
      {
      private:
         initonly String^ _extension;
         initonly String^ _file;

      public:
         FileFormatAttribute(String^ file, String^ extension)
         {
            _file = file;
            _extension = extension;
         }

         property String^ Extension { String^ get() { return _extension; } }
         property String^ File { String^ get() { return _file; } }
      };

      #pragma endregion
   }
}