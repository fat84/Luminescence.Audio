#pragma once

using namespace System;
using namespace System::Runtime::InteropServices;
using namespace System::Text;

namespace Luminescence
{
   namespace Audio
   {
      static std::string ConvertManagedPathToNativeString(String^ path)
      {
         array<Byte>^ encodedBytes = Encoding::UTF8->GetBytes(path);
         std::string file_name;
         file_name.resize(encodedBytes->Length);
         Marshal::Copy(encodedBytes, 0, IntPtr((void*)file_name.data()), encodedBytes->Length);
         return file_name;
      }
   }
}