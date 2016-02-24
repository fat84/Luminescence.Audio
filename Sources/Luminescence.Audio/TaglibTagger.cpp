#include "stdafx.h"

#include <taglib.h>
#include <tstring.h>
#include <tpropertymap.h>
#include <flacfile.h>
#include <flacproperties.h>
#include <xiphcomment.h>
#include <mpegfile.h>
#include <attachedpictureframe.h>
#include <id3v2tag.h>
#include <vorbisfile.h>
#include <asffile.h>
#include <mp4file.h>

#include "Interop.h"
#include "TaglibHelper.cpp" // contains only managed resources used by TaglibTagger class

using namespace System;
using namespace System::IO;
using namespace System::Linq;

using namespace Luminescence::Audio;

namespace Luminescence
{
   namespace Audio
   {
      [FileFormat("MP3 (MPEG-1/2 Audio Layer 3)", ".mp3")]
      [FileFormat("FLAC (Free Lossless Audio Codec)", ".flac")]
      [FileFormat("Ogg Vorbis", ".ogg")]
      [FileFormat("WMA (Windows Media Audio)", ".wma")]
      [FileFormat("AAC (Advanced Audio Coding) - ALAC (Apple Lossless Audio Codec)", ".m4a")]
      public ref class TaglibTagger
      {
      private:
         String^ fullPath;

         String^ codec;
         String^ codecVersion;
         byte channels;
         int duration;
         int bitrate;
         int sampleRate;
         byte bitsPerSample;

         Dictionary<String^, List<String^>^>^ tags;
         List<Picture^>^ pictures;

         void ReadTags(String^ path)
         {
            if (!File::Exists(path))
               throw gcnew FileNotFoundException("The file is not found.", path);

            String^ extension = Path::GetExtension(path);
            if (String::Equals(extension, ".mp3", StringComparison::OrdinalIgnoreCase))
               ReadMp3File(path);

            else if (String::Equals(extension, ".flac", StringComparison::OrdinalIgnoreCase))
               ReadFlacFile(path);

            else if (String::Equals(extension, ".ogg", StringComparison::OrdinalIgnoreCase))
               ReadOggFile(path);

            else if (String::Equals(extension, ".wma", StringComparison::OrdinalIgnoreCase))
               ReadWmaFile(path);

            else if (String::Equals(extension, ".m4a", StringComparison::OrdinalIgnoreCase))
               ReadM4aFile(path);

            else
               throw gcnew NotSupportedException("The file format is not supported.");

            fullPath = path;
         }

         void ReadFlacFile(String^ path)
         {
            TagLib::FileName fileName(msclr::interop::marshal_as<std::wstring>(path).c_str());

            TagLib::FLAC::File file(fileName, true, TagLib::AudioProperties::ReadStyle::Average);
            if (!file.isValid())
               throw gcnew IOException("The file is not a valid FLAC file.");

            TagLib::FLAC::Properties *properties = file.audioProperties();
            if (properties == NULL)
               throw gcnew FileFormatException("There is no audio properties in the file.");

            if (!file.hasXiphComment())
               throw gcnew FileFormatException("There is no Xiph Comment in the file.");

            TagLib::Ogg::XiphComment *xiph = file.xiphComment();
            TagLib::StringList buffer = xiph->vendorID().split();
            codecVersion = buffer.size() > 2 ? "FLAC " + gcnew String(buffer[2].toCWString()) : "FLAC";
            codec = "FLAC";

            bitrate = properties->bitrate(); // in kb/s
            duration = properties->lengthInSeconds(); // in seconds
            sampleRate = properties->sampleRate(); // in Hertz
            channels = (byte)properties->channels(); // number of audio channels
            bitsPerSample = properties->bitsPerSample(); // in bits

            tags = PropertyMapToManagedDictionary(file.properties());

            TagLib::List<TagLib::FLAC::Picture*> arts = file.pictureList();
            pictures = gcnew List<Picture^>(arts.size());
            for (auto it = arts.begin(); it != arts.end(); it++)
            {
               TagLib::FLAC::Picture *pic = *it;

               pictures->Add(gcnew Picture(
                  ByteVectorToManagedArray(pic->data()),
                  (PictureType)pic->type(),
                  gcnew String(pic->description().toCString())));
            }
         }

         List<String^>^ WriteFlacFile()
         {
            String^ path = fullPath;
            TagLib::FileName fileName(msclr::interop::marshal_as<std::wstring>(path).c_str());

            TagLib::FLAC::File file(fileName, false);
            if (!file.isValid() || file.readOnly())
               throw gcnew IOException("The file cannot be opened for writing.");

            TagLib::Ogg::XiphComment *xiph = file.xiphComment(true);
            TagLib::PropertyMap map = xiph->setProperties(ManagedDictionaryToPropertyMap(tags));

            file.removePictures();
            for each(auto picture in pictures)
            {
               TagLib::FLAC::Picture *pic = new TagLib::FLAC::Picture();
               pic->setData(ManagedArrayToByteVector(picture->Data));
               pic->setMimeType(msclr::interop::marshal_as<std::string>(picture->GetMimeType()).c_str());
               pic->setType((TagLib::FLAC::Picture::Type)picture->Type);
               if (picture->Description != nullptr)
                  pic->setDescription(msclr::interop::marshal_as<std::wstring>(picture->Description).c_str());

               file.addPicture(pic); //The file takes ownership of the picture and will handle freeing its memory.
            }

            file.strip(TagLib::FLAC::File::ID3v1 | TagLib::FLAC::File::ID3v2);
            file.save();
            return PropertyMapToManagedList(map);
         }

         void ReadMp3File(String^ path)
         {
            TagLib::FileName fileName(msclr::interop::marshal_as<std::wstring>(path).c_str());

            TagLib::MPEG::File file(fileName, true, TagLib::AudioProperties::ReadStyle::Average);
            if (!file.isValid())
               throw gcnew IOException("The file is not a valid MP3 file.");

            TagLib::MPEG::Properties *properties = file.audioProperties();
            if (properties == NULL)
               throw gcnew FileFormatException("There is no audio properties in the file.");

            codec = codecVersion = "MP3";

            bitrate = properties->bitrate(); // in kb/s
            duration = properties->lengthInSeconds(); // in seconds
            sampleRate = properties->sampleRate(); // in Hertz
            channels = (byte)properties->channels(); // number of audio channels
            bitsPerSample = 0; // in bits

            tags = PropertyMapToManagedDictionary(file.properties());

            if (!file.hasID3v2Tag())
            {
               pictures = gcnew List<Picture^>(1);
               return;
            }

            TagLib::ID3v2::Tag *tag = file.ID3v2Tag(false);
            TagLib::ID3v2::FrameList arts = tag->frameListMap()["APIC"];
            pictures = gcnew List<Picture^>(arts.size());
            for (auto it = arts.begin(); it != arts.end(); it++)
            {
               TagLib::ID3v2::Frame *frame = *it;
               TagLib::ID3v2::AttachedPictureFrame *pic = static_cast<TagLib::ID3v2::AttachedPictureFrame*>(frame);

               if (pic->picture().size() == 0) continue;

               pictures->Add(gcnew Picture(
                  ByteVectorToManagedArray(pic->picture()),
                  (PictureType)pic->type(),
                  gcnew String(pic->description().toCString())));
            }
         }

         List<String^>^ WriteMp3File()
         {
            String^ path = fullPath;
            TagLib::FileName fileName(msclr::interop::marshal_as<std::wstring>(path).c_str());

            TagLib::MPEG::File file(fileName, false);
            if (!file.isValid() || file.readOnly())
               throw gcnew IOException("The file cannot be opened for writing.");

            TagLib::ID3v2::Tag *id3 = file.ID3v2Tag(true);
            id3->setProperties(ManagedDictionaryToPropertyMap(tags));
            // ID3 implements the complete PropertyMap interface, so an empty map is always returned

            id3->removeFrames("APIC");
            for each(auto picture in pictures)
            {
               array<byte>^ data = picture->Data;
               pin_ptr<byte> p = &data[0];
               unsigned char *pby = p;
               char *pch = reinterpret_cast<char*>(pby);
               TagLib::ByteVector buffer(pch, picture->Data->Length);

               TagLib::ID3v2::AttachedPictureFrame *frame = new TagLib::ID3v2::AttachedPictureFrame();
               frame->setPicture(buffer);
               frame->setMimeType(msclr::interop::marshal_as<std::string>(picture->GetMimeType()).c_str());
               frame->setType((TagLib::ID3v2::AttachedPictureFrame::Type)picture->Type);
               if (picture->Description != nullptr)
                  frame->setDescription(msclr::interop::marshal_as<std::wstring>(picture->Description).c_str());

               id3->addFrame(frame); // At this point the tag takes ownership of the frame and will handle freeing its memory.
            }

            int tagVersion = id3->header()->majorVersion();

            int minVersion = (int)TaglibSettings::MinId3Version;
            int maxVersion = (int)TaglibSettings::MaxId3Version;

            if (tagVersion < minVersion)
               tagVersion = minVersion;

            if (tagVersion > maxVersion)
               tagVersion = maxVersion;

            array<String^>^ unsupportedFramesId3v23 = { "RELEASEDATE", "TAGGINGDATE", "MOOD", "PRODUCEDNOTICE", "ALBUMSORT", "TITLESORT", "ARTISTSORT" };
            if (tagVersion < 4 && Enumerable::Any<String^>(unsupportedFramesId3v23, gcnew Func<String^, bool>(tags, &Dictionary<String^, List<String^>^>::ContainsKey)))
               tagVersion = 4;

            file.save(2, true, tagVersion, false);
            return nullptr;
         }

         void ReadOggFile(String^ path)
         {
            TagLib::FileName fileName(msclr::interop::marshal_as<std::wstring>(path).c_str());

            TagLib::Vorbis::File file(fileName, true, TagLib::AudioProperties::ReadStyle::Average);
            if (!file.isValid())
               throw gcnew IOException("The file is not a valid Ogg Vorbis file.");

            TagLib::Vorbis::Properties *properties = file.audioProperties();
            if (properties == NULL)
               throw gcnew FileFormatException("There is no audio properties in the file.");

            TagLib::Ogg::XiphComment *xiph = file.tag();
            String^ vendor = gcnew String(xiph->vendorID().toCWString());
            codecVersion = GetVorbisVersionFromVendor(vendor);
            codec = "Vorbis";

            bitrate = properties->bitrate(); // in kb/s
            duration = properties->lengthInSeconds(); // in seconds
            sampleRate = properties->sampleRate(); // in Hertz
            channels = (byte)properties->channels(); // number of audio channels

            TagLib::List<TagLib::FLAC::Picture*> arts = xiph->pictureList();
            pictures = gcnew List<Picture^>(arts.size());
            for (auto it = arts.begin(); it != arts.end(); it++)
            {
               TagLib::FLAC::Picture *pic = *it;

               pictures->Add(gcnew Picture(
                  ByteVectorToManagedArray(pic->data()),
                  (PictureType)pic->type(),
                  gcnew String(pic->description().toCString())));
            }

            tags = PropertyMapToManagedDictionary(file.properties());
         }

         List<String^>^ WriteOggFile()
         {
            String^ path = fullPath;
            TagLib::FileName fileName(msclr::interop::marshal_as<std::wstring>(path).c_str());

            TagLib::Vorbis::File file(fileName, false);
            if (!file.isValid() || file.readOnly())
               throw gcnew IOException("The file cannot be opened for writing.");

            TagLib::Ogg::XiphComment *xiph = file.tag();
            TagLib::PropertyMap map = xiph->setProperties(ManagedDictionaryToPropertyMap(tags));

            xiph->removeAllPictures();
            for each(auto picture in pictures)
            {
               TagLib::FLAC::Picture *pic = new TagLib::FLAC::Picture();
               pic->setData(ManagedArrayToByteVector(picture->Data));
               pic->setMimeType(msclr::interop::marshal_as<std::string>(picture->GetMimeType()).c_str());
               pic->setType((TagLib::FLAC::Picture::Type)picture->Type);
               if (picture->Description != nullptr)
                  pic->setDescription(msclr::interop::marshal_as<std::wstring>(picture->Description).c_str());

               xiph->addPicture(pic); //The file takes ownership of the picture and will handle freeing its memory.
            }

            file.save();
            return PropertyMapToManagedList(map);
         }

         void ReadWmaFile(String^ path)
         {
            TagLib::FileName fileName(msclr::interop::marshal_as<std::wstring>(path).c_str());

            TagLib::ASF::File file(fileName, true, TagLib::AudioProperties::ReadStyle::Average);
            if (!file.isValid())
               throw gcnew IOException("The file is not a valid WMA file.");

            TagLib::ASF::Properties *properties = file.audioProperties();
            if (properties == NULL)
               throw gcnew FileFormatException("There is no audio properties in the file.");

            codecVersion = !properties->codecName().isEmpty() ? gcnew String(properties->codecName().toCString()) : "WMA";
            codec = "WMA";

            bitrate = properties->bitrate(); // in kb/s
            duration = properties->lengthInSeconds(); // in seconds
            sampleRate = properties->sampleRate(); // in Hertz
            channels = (byte)properties->channels(); // number of audio channels
            bitsPerSample = properties->bitsPerSample(); // in bits

            tags = PropertyMapToManagedDictionary(file.properties());

            TagLib::ASF::Tag* asf = file.tag();
            TagLib::ASF::AttributeList& arts = asf->attributeListMap()["WM/Picture"];
            pictures = gcnew List<Picture^>(arts.size());
            for (auto it = arts.begin(); it != arts.end(); it++)
            {
               TagLib::ASF::Attribute attr = *it;
               TagLib::ASF::Picture pic = attr.toPicture();

               pictures->Add(gcnew Picture(
                  ByteVectorToManagedArray(pic.picture()),
                  (PictureType)pic.type(),
                  gcnew String(pic.description().toCString())));
            }
         }

         List<String^>^ WriteWmaFile()
         {
            String^ path = fullPath;
            TagLib::FileName fileName(msclr::interop::marshal_as<std::wstring>(path).c_str());

            TagLib::ASF::File file(fileName, false);
            if (!file.isValid() || file.readOnly())
               throw gcnew IOException("The file cannot be opened for writing.");

            TagLib::PropertyMap map = file.setProperties(ManagedDictionaryToPropertyMap(tags));

            TagLib::ASF::Tag* asf = file.tag();
            asf->removeItem("WM/Picture");
            for each(auto picture in pictures)
            {
               TagLib::ASF::Picture pic;
               pic.setPicture(ManagedArrayToByteVector(picture->Data));
               pic.setMimeType(msclr::interop::marshal_as<std::string>(picture->GetMimeType()).c_str());
               pic.setType((TagLib::ASF::Picture::Type)picture->Type);
               if (picture->Description != nullptr)
                  pic.setDescription(msclr::interop::marshal_as<std::wstring>(picture->Description).c_str());

               asf->addAttribute("WM/Picture", pic);
            }

            file.save();
            return PropertyMapToManagedList(map);
         }

         void ReadM4aFile(String^ path)
         {
            TagLib::FileName fileName(msclr::interop::marshal_as<std::wstring>(path).c_str());

            TagLib::MP4::File file(fileName, true, TagLib::AudioProperties::ReadStyle::Average);
            if (!file.isValid())
               throw gcnew IOException("The file is not a valid M4A file.");

            TagLib::MP4::Properties *properties = file.audioProperties();
            if (properties == NULL)
               throw gcnew FileFormatException("There is no audio properties in the file.");

            switch (properties->codec())
            {
            case TagLib::MP4::Properties::Codec::AAC:
               codec = codecVersion = "AAC";
               break;
            case TagLib::MP4::Properties::Codec::ALAC:
               codec = codecVersion = "ALAC";
               break;
            case TagLib::MP4::Properties::Codec::Unknown:
               throw gcnew IOException("The file is not a valid M4A file.");
            }

            bitrate = properties->bitrate(); // in kb/s
            duration = properties->lengthInSeconds(); // in seconds
            sampleRate = properties->sampleRate(); // in Hertz
            channels = (byte)properties->channels(); // number of audio channels
            bitsPerSample = properties->bitsPerSample(); // in bits

            tags = PropertyMapToManagedDictionary(file.properties());

            TagLib::MP4::Tag *tag = file.tag();
            TagLib::MP4::CoverArtList arts = tag->item("covr").toCoverArtList();
            pictures = gcnew List<Picture^>(arts.size());
            for (auto it = arts.begin(); it != arts.end(); it++)
            {
               TagLib::MP4::CoverArt pic = *it;

               pictures->Add(gcnew Picture(
                  ByteVectorToManagedArray(pic.data()),
                  (Format)pic.format(),
                  PictureType::FrontCover,
                  nullptr));
            }
         }

         List<String^>^ WriteM4aFile()
         {
            String^ path = fullPath;
            TagLib::FileName fileName(msclr::interop::marshal_as<std::wstring>(path).c_str());

            TagLib::MP4::File file(fileName, false);
            if (!file.isValid() || file.readOnly())
               throw gcnew IOException("The file cannot be opened for writing.");

            TagLib::PropertyMap map = file.setProperties(ManagedDictionaryToPropertyMap(tags));

            TagLib::MP4::Tag* atom = file.tag();
            atom->removeItem("covr");
            if (pictures->Count != 0)
            {
               TagLib::MP4::CoverArtList arts;
               for each(auto picture in pictures)
               {
                  TagLib::MP4::CoverArt pic((TagLib::MP4::CoverArt::Format)picture->PictureFormat, ManagedArrayToByteVector(picture->Data));
                  arts.append(pic);
               }

               atom->setItem("covr", arts);
            }

            file.save();
            return PropertyMapToManagedList(map);
         }

      public:
         TaglibTagger(String^ path) { ReadTags(path); }

         property String^ Source
         {
            String^ get() { return fullPath; }
            void set(String^ value)
            {
               if (!String::Equals(fullPath, value, StringComparison::OrdinalIgnoreCase))
                  ReadTags(value);
            }
         }

         property String^ Codec { String^ get() { return codec; } }
         property String^ CodecVersion { String^ get() { return codecVersion; } }
         property byte Channels { byte get() { return channels; } }
         property int Duration { int get() { return duration; } }
         property int Bitrate { int get() { return bitrate; } }
         property int SampleRate { int get() { return sampleRate; } }
         property byte BitsPerSample { byte get() { return bitsPerSample; } }

         property Dictionary<String^, List<String^>^>^ Tags { Dictionary<String^, List<String^>^>^ get() { return tags; } }
         property List<Picture^>^ Pictures { List<Picture^>^ get() { return pictures; } } // return nullptr if the format doesn't support cover, return empty collection if there is no cover

         List<String^>^ SaveTags()
         {
            if (!File::Exists(fullPath))
               throw gcnew FileNotFoundException("The file is not found.", fullPath);

            if ((File::GetAttributes(fullPath) & FileAttributes::ReadOnly) == FileAttributes::ReadOnly)
               File::SetAttributes(fullPath, FileAttributes::Normal);

            String^ extension = Path::GetExtension(fullPath);
            if (String::Equals(extension, ".mp3", StringComparison::OrdinalIgnoreCase))
               return WriteMp3File();

            if (String::Equals(extension, ".flac", StringComparison::OrdinalIgnoreCase))
               return WriteFlacFile();

            if (String::Equals(extension, ".ogg", StringComparison::OrdinalIgnoreCase))
               return WriteOggFile();

            if (String::Equals(extension, ".wma", StringComparison::OrdinalIgnoreCase))
               return WriteWmaFile();

            if (String::Equals(extension, ".m4a", StringComparison::OrdinalIgnoreCase))
               return WriteM4aFile();

            throw gcnew NotSupportedException("The file format is not supported.");
         }
      };
   }
}