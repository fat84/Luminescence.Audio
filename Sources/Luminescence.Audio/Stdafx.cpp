// stdafx.cpp : fichier source incluant simplement les fichiers Include standard
// ChromaprintWrapper.pch représente l'en-tête précompilé
// stdafx.obj contient les informations de type précompilées

#include "stdafx.h"

void MarshalString(System::String^ s, std::string& os)
{
	using namespace System;
	using namespace System::Runtime::InteropServices;

	const char* chars = (const char*)(Marshal::StringToHGlobalAnsi(s)).ToPointer(); 
	os = chars; 
	Marshal::FreeHGlobal(IntPtr((void*)chars)); 
}