#pragma once

#include "mp4coverart.h"

using namespace System;
using namespace System::Collections::Generic;

namespace Luminescence
{
   namespace Audio
   {
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
   }
}