#include "stdafx.h"

#include <msclr\marshal.h>
#include <msclr\marshal_cppstd.h>

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

#include "TaglibTagger.h"

using namespace System;
using namespace System::IO;

namespace Luminescence
{
   namespace Audio
   {      
      void TaglibTagger::ReadTags(String^ path)
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
         else
            throw gcnew NotSupportedException("The file format is not supported.");

         fullPath = path;
      }

      bool TaglibTagger::SaveTags()
      {
         if (!File::Exists(fullPath))
            throw gcnew FileNotFoundException("The file is not found.", fullPath);

         if ((File::GetAttributes(fullPath) & FileAttributes::ReadOnly) == FileAttributes::ReadOnly)
            File::SetAttributes(fullPath, FileAttributes::Normal);

         String^ extension = Path::GetExtension(fullPath);
         if (String::Equals(extension, ".mp3", StringComparison::OrdinalIgnoreCase))
            return WriteMp3File();
         else if (String::Equals(extension, ".flac", StringComparison::OrdinalIgnoreCase))
            return WriteFlacFile();
         else if (String::Equals(extension, ".ogg", StringComparison::OrdinalIgnoreCase))
            return WriteOggFile();

         return true;
      }

      void TaglibTagger::ReadMp3File(String^ path)
      {
         TagLib::FileName fileName(msclr::interop::marshal_as<std::wstring>(path).c_str());

         TagLib::MPEG::File file(fileName, true, TagLib::AudioProperties::ReadStyle::Average);
         if (!file.isValid())
            throw gcnew IOException("The file is not a valid MP3 file.");

         TagLib::MPEG::Properties *properties = file.audioProperties();
         if (properties == NULL)
            throw gcnew FileFormatException("There is no audio properties in the file.");

         codec = "MP3";

         bitrate = properties->bitrate(); // in kb/s
         duration = properties->lengthInSeconds(); // in seconds
         sampleRate = properties->sampleRate(); // in Hertz
         channels = (byte)properties->channels(); // number of audio channels
         bitsPerSample = 0; // in bits

         tags = MapToDictionary(file.properties());

         if (!file.hasID3v2Tag())
            return;

         TagLib::ID3v2::Tag *tag = file.ID3v2Tag(false);
         TagLib::ID3v2::FrameList arts = tag->frameListMap()["APIC"];
         pictures = gcnew List<Picture^>(arts.size());
         for (auto it = arts.begin(); it != arts.end(); it++)
         {
            TagLib::ID3v2::Frame *frame = *it;
            TagLib::ID3v2::AttachedPictureFrame *pic = static_cast<TagLib::ID3v2::AttachedPictureFrame*>(frame);

            array<byte>^ buffer = ConvertByteVectorToManagedArray(pic->picture().data());
            pictures->Add(gcnew Picture(buffer, gcnew String(pic->mimeType().toCString()), (PictureType)pic->type(), gcnew String(pic->description().toCString())));
         }
      }

      void TaglibTagger::ReadFlacFile(String^ path)
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

         codec = "FLAC";
         TagLib::Ogg::XiphComment *xiph = file.xiphComment();
         TagLib::StringList buffer = xiph->vendorID().split();
         if (buffer.size() > 2)
            codecVersion = gcnew String(buffer[2].toCWString());

         bitrate = properties->bitrate(); // in kb/s
         duration = properties->lengthInSeconds(); // in seconds
         sampleRate = properties->sampleRate(); // in Hertz
         channels = (byte)properties->channels(); // number of audio channels
         bitsPerSample = properties->bitsPerSample(); // in bits

         tags = MapToDictionary(file.properties());

         TagLib::List<TagLib::FLAC::Picture*> arts = file.pictureList();
         pictures = gcnew List<Picture^>(arts.size());
         for (auto it = arts.begin(); it != arts.end(); it++)
         {
            TagLib::FLAC::Picture *pic = *it;

            array<byte>^ buffer = ConvertByteVectorToManagedArray(pic->data());
            pictures->Add(gcnew Picture(buffer, gcnew String(pic->mimeType().toCString()), (PictureType)pic->type(), gcnew String(pic->description().toCString())));
         }
      }

      void TaglibTagger::ReadOggFile(String^ path)
      {
         TagLib::FileName fileName(msclr::interop::marshal_as<std::wstring>(path).c_str());

         TagLib::Vorbis::File file(fileName, true, TagLib::AudioProperties::ReadStyle::Average);
         if (!file.isValid())
            throw gcnew IOException("The file is not a valid Ogg Vorbis file.");

         TagLib::Vorbis::Properties *properties = file.audioProperties();
         if (properties == NULL)
            throw gcnew FileFormatException("There is no audio properties in the file.");

         codec = "Vorbis";
         TagLib::Ogg::XiphComment *xiph = file.tag();
         String^ vendor = gcnew String(xiph->vendorID().toCWString());
         codecVersion = GetVorbisVersionFromVendor(vendor);

         bitrate = properties->bitrate(); // in kb/s
         duration = properties->lengthInSeconds(); // in seconds
         sampleRate = properties->sampleRate(); // in Hertz
         channels = (byte)properties->channels(); // number of audio channels

         TagLib::StringList arts = xiph->fieldListMap()["METADATA_BLOCK_PICTURE"];
         xiph->removeField("METADATA_BLOCK_PICTURE");
         tags = MapToDictionary(file.properties());

         pictures = gcnew List<Picture^>(arts.size());
         for (auto it = arts.begin(); it != arts.end(); it++)
         {
            TagLib::String base64 = *it;

            TagLib::FLAC::Picture pic(DecodeBase64(base64));
            array<byte>^ buffer = ConvertByteVectorToManagedArray(pic.data());            
            pictures->Add(gcnew Picture(buffer, gcnew String(pic.mimeType().toCString()), (PictureType)pic.type(), gcnew String(pic.description().toCString())));
         }
      }      

      bool TaglibTagger::WriteMp3File()
      {
         String^ path = fullPath;
         TagLib::FileName fileName(msclr::interop::marshal_as<std::wstring>(path).c_str());

         TagLib::MPEG::File file(fileName, false);
         if (!file.isValid() || file.readOnly())
            throw gcnew IOException("The file cannot be opened for writing.");

         TagLib::ID3v2::Tag *id3 = file.ID3v2Tag(true);
         id3->setProperties(DictionaryToMap(tags));

         id3->removeFrames("APIC");
         for each(auto picture in pictures)
         {
            array<byte>^ data = picture->Data;
            pin_ptr<byte> p = &data[0];
            unsigned char *pby = p;
            const char *pch = reinterpret_cast<char*>(pby);
            TagLib::ByteVector buffer(pch, picture->Data->Length);

            TagLib::ID3v2::AttachedPictureFrame *frame = new TagLib::ID3v2::AttachedPictureFrame;
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

         return file.save(2, true, tagVersion, false);
      }

      bool TaglibTagger::WriteFlacFile()
      {         
         String^ path = fullPath;
         TagLib::FileName fileName(msclr::interop::marshal_as<std::wstring>(path).c_str());

         TagLib::FLAC::File file(fileName, false);
         if (!file.isValid() || file.readOnly())
            throw gcnew IOException("The file cannot be opened for writing.");

         TagLib::Ogg::XiphComment *xiph = file.xiphComment(true);
         TagLib::PropertyMap map = xiph->setProperties(DictionaryToMap(tags));
         if (!map.isEmpty())
         {
            auto invalidTags = gcnew List<String^>(map.size());
            for (auto it = map.begin(); it != map.end(); ++it) 
            {
               invalidTags->Add(gcnew String(it->first.toCWString()));
            }

            throw gcnew InvalidOperationException(String::Format("The following tags are not supported in Xiph Comment tags: ", String::Join(", ", invalidTags)));
         }

         file.removePictures();
         for each(auto picture in pictures)
         {
            TagLib::FLAC::Picture *pic = new TagLib::FLAC::Picture();
            pic->setData(ConvertManagedArrayToByteVector(picture->Data));
            pic->setMimeType(msclr::interop::marshal_as<std::string>(picture->GetMimeType()).c_str());
            pic->setType((TagLib::FLAC::Picture::Type)picture->Type);
            if (picture->Description != nullptr)
              pic->setDescription(msclr::interop::marshal_as<std::wstring>(picture->Description).c_str());

            file.addPicture(pic); //The file takes ownership of the picture and will handle freeing its memory.
         }

         //TODO : disallow ID3 tags saving in FLAC file      
         return file.save();
      }

      bool TaglibTagger::WriteOggFile()
      {
         String^ path = fullPath;
         TagLib::FileName fileName(msclr::interop::marshal_as<std::wstring>(path).c_str());

         TagLib::Vorbis::File file(fileName, false);
         if (!file.isValid() || file.readOnly())
            throw gcnew IOException("The file cannot be opened for writing.");

         TagLib::Ogg::XiphComment *xiph = file.tag();
         TagLib::PropertyMap map = xiph->setProperties(DictionaryToMap(tags));
         if (!map.isEmpty())
         {
            auto invalidTags = gcnew List<String^>(map.size());
            for (auto it = map.begin(); it != map.end(); ++it)
            {
               invalidTags->Add(gcnew String(it->first.toCWString()));
            }

            throw gcnew InvalidOperationException(String::Format("The following tags are not supported in Xiph Comment tags: ", String::Join(", ", invalidTags)));
         }

         xiph->removeField("METADATA_BLOCK_PICTURE");
         for each(auto picture in pictures)
         {
            TagLib::FLAC::Picture pic;
            pic.setData(ConvertManagedArrayToByteVector(picture->Data));
            pic.setMimeType(msclr::interop::marshal_as<std::string>(picture->GetMimeType()).c_str());
            pic.setType((TagLib::FLAC::Picture::Type)picture->Type);
            if (picture->Description != nullptr)
               pic.setDescription(msclr::interop::marshal_as<std::wstring>(picture->Description).c_str());

            xiph->addField("METADATA_BLOCK_PICTURE", EncodeBase64(pic.render()), false);
         }

         return file.save();
      }

      /*TagLib::ASF::File file = File("myfile.wma");
      const TagLib::ASF::AttributeListMap& attrListMap = file->tag()->attributeListMap();
      const TagLib::ASF::AttributeList& attrList = attrListMap["WM/Picture"];
      TagLib::ASF::Picture pic = attrList[0].toPicture();*/      
   }
}