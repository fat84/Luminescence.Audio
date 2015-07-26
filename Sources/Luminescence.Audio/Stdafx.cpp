// stdafx.cpp�: fichier source incluant simplement les fichiers Include standard
// ChromaprintWrapper.pch repr�sente l'en-t�te pr�compil�
// stdafx.obj contient les informations de type pr�compil�es

#include "stdafx.h"

void MarshalString(System::String^ s, std::string& os)
{
	using namespace System;
	using namespace System::Runtime::InteropServices;

	const char* chars = (const char*)(Marshal::StringToHGlobalAnsi(s)).ToPointer(); 
	os = chars; 
	Marshal::FreeHGlobal(IntPtr((void*)chars)); 
}