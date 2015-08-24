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
         if (String::Equals(extension, ".flac", StringComparison::OrdinalIgnoreCase))
            ReadFlacFile(path);
         else if (String::Equals(extension, ".mp3", StringComparison::OrdinalIgnoreCase))
            ReadMp3File(path);
         else
            throw gcnew NotSupportedException("The file format is not supported.");

         fullPath = path;
      }

      void TaglibTagger::ReadMp3File(String^ path)
      {
         TagLib::FileName fileName(msclr::interop::marshal_as<std::wstring>(path).c_str());

         TagLib::MPEG::File file(fileName, true, TagLib::AudioProperties::ReadStyle::Average);
         if (!file.isValid())
            throw gcnew IOException("The file cannot be opened for reading.");

         TagLib::MPEG::Properties *properties = file.audioProperties();
         if (properties == NULL)
            throw gcnew FileFormatException("There is no audio properties in the file.");

         codec = "MP3";

         bitrate = properties->bitrate(); // in kb/s
         duration = properties->length(); // in seconds
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

            array<byte>^ buffer = gcnew array<byte>(pic->picture().size());
            pin_ptr<byte> buffer_start = &buffer[0];
            memcpy(buffer_start, pic->picture().data(), buffer->Length);

            pictures->Add(gcnew Picture(buffer, gcnew String(pic->mimeType().toCString()), (PictureType)pic->type(), gcnew String(pic->description().toCString())));
         }
      }

      void TaglibTagger::ReadFlacFile(String^ path)
      {
         TagLib::FileName fileName(msclr::interop::marshal_as<std::wstring>(path).c_str());

         TagLib::FLAC::File file(fileName, true, TagLib::AudioProperties::ReadStyle::Average);
         if (!file.isValid())
            throw gcnew IOException("The file cannot be opened for reading.");

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
         duration = properties->length(); // in seconds
         sampleRate = properties->sampleRate(); // in Hertz
         channels = (byte)properties->channels(); // number of audio channels
         bitsPerSample = properties->sampleWidth(); // in bits

         tags = MapToDictionary(file.properties());

         TagLib::List<TagLib::FLAC::Picture*> arts = file.pictureList();
         pictures = gcnew List<Picture^>(arts.size());

         for (auto it = arts.begin(); it != arts.end(); it++)
         {
            TagLib::FLAC::Picture *pic = *it;
            array<byte>^ buffer = gcnew array<byte>(pic->data().size());
            pin_ptr<byte> buffer_start = &buffer[0];
            memcpy(buffer_start, pic->data().data(), buffer->Length);

            pictures->Add(gcnew Picture(buffer, gcnew String(pic->mimeType().toCString()), (PictureType)pic->type(), gcnew String(pic->description().toCString())));
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

            id3->addFrame(frame);
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
            array<byte>^ data = picture->Data;
            pin_ptr<byte> p = &data[0];
            unsigned char *pby = p;
            const char *pch = reinterpret_cast<char*>(pby);
            TagLib::ByteVector buffer(pch, picture->Data->Length);

            TagLib::FLAC::Picture *pic = new TagLib::FLAC::Picture();
            pic->setData(buffer);
            pic->setMimeType(msclr::interop::marshal_as<std::string>(picture->GetMimeType()).c_str());
            pic->setType((TagLib::FLAC::Picture::Type)picture->Type);
            if (picture->Description != nullptr)
              pic->setDescription(msclr::interop::marshal_as<std::wstring>(picture->Description).c_str());

            file.addPicture(pic);
         }

         //TODO : don't save ID3 tags in FLAC file         

         return file.save();
      }

      //TagLib::ByteVector block = pic->render();
      //xiph->addField("METADATA_BLOCK_PICTURE", b64_encode(block.data(), block.size()), true);

      /*TagLib::ASF::File file = File("myfile.flac");
      const TagLib::ASF::AttributeListMap& attrListMap = file->tag()->attributeListMap();
      const TagLib::ASF::AttributeList& attrList = attrListMap["WM/Picture"];
      TagLib::ASF::Picture pic = attrList[0].toPicture();*/

      TagLib::PropertyMap TaglibTagger::DictionaryToMap(Dictionary<String^, List<String^>^>^ dic)
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

      Dictionary<String^, List<String^>^>^ TaglibTagger::MapToDictionary(TagLib::PropertyMap map)
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

      bool TaglibTagger::SaveTags()
      {
         if (!File::Exists(fullPath))
            throw gcnew FileNotFoundException("The file is not found.", fullPath);

         if ((File::GetAttributes(fullPath) & FileAttributes::ReadOnly) == FileAttributes::ReadOnly)
            File::SetAttributes(fullPath, FileAttributes::Normal);

         String^ extension = Path::GetExtension(fullPath);
         if (String::Equals(extension, ".flac", StringComparison::OrdinalIgnoreCase))
            return WriteFlacFile();
         else if (String::Equals(extension, ".mp3", StringComparison::OrdinalIgnoreCase))
            return WriteMp3File();

         return true;
      }
   }
}