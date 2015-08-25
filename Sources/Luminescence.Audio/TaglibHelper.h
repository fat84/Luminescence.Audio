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
         array<byte>^ buffer = gcnew array<byte>(data.size());
         pin_ptr<byte> buffer_start = &buffer[0];
         memcpy(buffer_start, data.data(), buffer->Length);
         return buffer;
      }

      static TagLib::ByteVector ConvertManagedArrayToByteVector(array<byte>^ data)
      {
         pin_ptr<byte> p = &data[0];
         unsigned char *pby = p;
         const char *pch = reinterpret_cast<char*>(pby);
         TagLib::ByteVector buffer(pch, data->Length);
         return buffer;
      }

      static TagLib::String EncodeBase64(const TagLib::ByteVector& data)
      {
         array<byte>^ buffer = ConvertByteVectorToManagedArray(data);
         String^ base64 = Convert::ToBase64String(buffer);
         TagLib::String s(msclr::interop::marshal_as<std::string>(base64));
         return s;
      }

      static TagLib::ByteVector DecodeBase64(const TagLib::String& base64)
      {
         String^ b64 = gcnew String(base64.toCString());
         array<byte>^ bytes = Convert::FromBase64String(b64);
         pin_ptr<byte> ubytes = &bytes[0];
         TagLib::ByteVector bv(reinterpret_cast<char*>(ubytes), bytes->Length);
         return bv;
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
         if (vendor->StartsWith("AO; aoTuV")) return "aoTuV";

         if (vendor == "Xiphophorus libVorbis I 20000508") return "1.0 Beta 1/2";
         if (vendor == "Xiphophorus libVorbis I 20001031") return "1.0 Beta 3";
         if (vendor == "Xiphophorus libVorbis I 20010225") return "1.0 Beta 4";
         if (vendor == "Xiphophorus libVorbis I 20010615") return "1.0 RC1";
         if (vendor == "Xiphophorus libVorbis I 20010813") return "1.0 RC2";
         if (vendor == "Xiphophorus libVorbis I 20011217") return "1.0 RC3";
         if (vendor == "Xiphophorus libVorbis I 20011231") return "1.0 RC3";

         // https://trac.xiph.org/browser/trunk/vorbis/CHANGES
         if (vendor == "Xiph.Org libVorbis I 20020717") return "1.0.0";
         if (vendor == "Xiph.Org libVorbis I 20030909") return "1.0.1";
         if (vendor == "Xiph.Org libVorbis I 20040629") return "1.1.0";
         if (vendor == "Xiph.Org libVorbis I 20050304") return "1.1.1/2";
         if (vendor == "Xiph.Org libVorbis I 20070622") return "1.2.0";
         if (vendor == "Xiph.Org libVorbis I 20080501") return "1.2.1";
         if (vendor == "Xiph.Org libVorbis I 20090624") return "1.2.2";
         if (vendor == "Xiph.Org libVorbis I 20090709") return "1.2.3";
         if (vendor == "Xiph.Org libVorbis I 20100325 (Everywhere)") return "1.3.1";
         if (vendor == "Xiph.Org libVorbis I 20101101 (Schaufenugget)") return "1.3.2";
         if (vendor == "Xiph.Org libVorbis I 20120203 (Omnipresent)") return "1.3.3";
         if (vendor == "Xiph.Org libVorbis I 20140122 (Turpakäräjiin)") return "1.3.4";
         if (vendor == "Xiph.Org libVorbis I 20150105 (????)") return "1.3.5";

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

         Picture(array<byte>^ data, String^ mime, PictureType type, String^ description)
         {
            Data = data;
            PictureFormat = GetFormatFromMimeType(mime);
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

         static Format GetFormatFromMimeType(String^ mime)
         {
            if (String::Equals(mime, "image/jpeg", StringComparison::OrdinalIgnoreCase))
               return Format::JPEG;

            if (String::Equals(mime, "image/png", StringComparison::OrdinalIgnoreCase))
               return Format::PNG;

            if (String::Equals(mime, "image/gif", StringComparison::OrdinalIgnoreCase))
               return Format::GIF;

            if (String::Equals(mime, "image/bmp", StringComparison::OrdinalIgnoreCase))
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