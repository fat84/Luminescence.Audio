#pragma once

#include <taglib.h>
#include <tstring.h>
#include <tpropertymap.h>

#include <msclr\marshal.h>
#include <msclr\marshal_cppstd.h>

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Runtime::InteropServices;
using namespace System::Text;

static array<byte>^ ByteVectorToManagedArray(const TagLib::ByteVector& data)
{
   if (data.size() == 0)
      return gcnew array<byte>(0);

   array<byte>^ buffer = gcnew array<byte>(data.size());
   pin_ptr<byte> buffer_start = &buffer[0];
   memcpy(buffer_start, data.data(), buffer->Length);
   return buffer;
}

static List<String^>^ PropertyMapToManagedList(const TagLib::PropertyMap& map)
{
   auto tags = gcnew List<String^>(map.size());

   for (auto it = map.begin(); it != map.end(); it++)
   {
      tags->Add(gcnew String(it->first.toCWString()));
   }

   return tags;
}

static TagLib::ByteVector ManagedArrayToByteVector(array<byte>^ data)
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

static TagLib::PropertyMap ManagedDictionaryToPropertyMap(Dictionary<String^, List<String^>^>^ dic)
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

static Dictionary<String^, List<String^>^>^ PropertyMapToManagedDictionary(const TagLib::PropertyMap& map)
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

static std::string ManagedStringToNativeUtf8One(String^ s)
{
   array<Byte>^ encodedBytes = Encoding::UTF8->GetBytes(s);
   std::string utf8;
   utf8.resize(encodedBytes->Length);
   Marshal::Copy(encodedBytes, 0, IntPtr((void*)utf8.data()), encodedBytes->Length);
   return utf8;
}

static array<int>^ NativeIntArrayToManagedOne(const int* data, int size)
{
   array<int>^ buffer = gcnew array<int>(size);
   pin_ptr<int> buffer_start = &buffer[0];
   memcpy(buffer_start, data, size);
   return buffer;
}