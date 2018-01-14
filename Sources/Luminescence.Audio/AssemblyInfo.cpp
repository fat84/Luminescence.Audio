#include "stdafx.h"

using namespace System;
using namespace System::Reflection;
using namespace System::Runtime::CompilerServices;
using namespace System::Runtime::InteropServices;
using namespace System::Security::Permissions;

//
// Les informations générales relatives à un assembly dépendent de
// l'ensemble d'attributs suivant. Changez les valeurs de ces attributs pour modifier les informations
// associées à un assembly.
//
[assembly:AssemblyTitleAttribute("Luminescence.Audio")];
[assembly:AssemblyDescriptionAttribute("A C++/CLI assembly for playing (with FFmpeg and XAudio2), tagging (with TagLib) and fingerprinting (with FFmpeg and Acoustid Chromaprint) audio files.")]
[assembly:AssemblyConfigurationAttribute("")];
[assembly:AssemblyCompanyAttribute("Luminescence Software")];
[assembly:AssemblyProductAttribute("Luminescence.Audio")];
[assembly:AssemblyCopyrightAttribute("Sylvain Rougeaux, © 2011-2017")];
[assembly:AssemblyTrademarkAttribute("")];
[assembly:AssemblyCultureAttribute("")];

//
// Les informations de version pour un assembly se composent des quatre valeurs suivantes :
//
//      Version principale
//      Version secondaire
//      Numéro de build
//      Révision
//
// Vous pouvez spécifier toutes les valeurs ou indiquer les numéros de révision et de build par défaut
// en utilisant '*', comme indiqué ci-dessous :

[assembly:AssemblyVersionAttribute("3.8.0.0")];
[assembly:ComVisible(false)];
[assembly:CLSCompliantAttribute(true)];
