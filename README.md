# Luminescence.Audio README

Luminescence.Audio is a C++/CLI library for playing (with [FFmpeg](https://github.com/FFmpeg/FFmpeg) and [XAudio2](https://msdn.microsoft.com/en-us/library/windows/desktop/ee415764.aspx)), tagging (with [TagLib](https://github.com/taglib/taglib)) and fingerprinting (with [FFmpeg](https://github.com/FFmpeg/FFmpeg) and [Acoustid Chromaprint](https://bitbucket.org/acoustid/chromaprint/overview)) audio files, in managed code.

This library has been written to be used by my tags editor, [Metatogger](http://www.luminescence-software.org/metatogger.html).

Here is a screenshot of that software:
![Screenshot of Metatogger](http://www.luminescence-software.org/img/metatogger/screenshot2.png)

Feel free to fork the code and [contribute](https://guides.github.com/activities/contributing-to-open-source/) to Luminescence.Audio.

## How to use the tagging API in C♯

```C#
using Luminescence.Audio;

// supports the following audio formats: MP3 (*.mp3), FLAC (*.flac), Ogg Vorbis (*.ogg), WMA (*.wma) and AAC/ALAC (*.m4a)
var tagger = new TaglibTagger(@"C:\MyFolder\MyAudioFile.flac");

// getting technical informations

string codecName = tagger.Codec; // "FLAC"
string codecVersion = tagger.CodecVersion; // "1.2.1"
byte channelsCount = tagger.Channels; // 2
TimeSpan duration = tagger.Duration;
int sampleRateInHertz = tagger.SampleRate; // 44100
int bitsPerSample = tagger.BitsPerSample; // 16

// getting or setting textual tags

// textual tags is stored in a dictionary where the key is the tag name in uppercase
Dictionary<string, List<string>> tagsRepository = tagger.Tags;
string artist = tagger.GetTagValues(TagNameKey.Artist).FirstOrDefault();
tagger.AddTag(TagNameKey.Artist, "Artist 2", "Artist 3");
tagger.ReplaceTag(TagNameKey.Title, "All You Need Is Love");
tagger.RemoveTag(TagNameKey.Comment);
tagger.AddTag("MY_KEY_NAME", "Custom Tag Value");

// getting or setting embedded covers

List<Picture> pictures = tagger.Pictures;
if (pictures != null && pictures.Count != 0)
{
   Picture mainPicture = pictures.FirstOrDefault(p => p.Type == PictureType.FrontCover);
   if (mainPicture == null) mainPicture = pictures[0];

   byte[] data = mainPicture.Data;
   Format pictureFormat = mainPicture.PictureFormat; // Format.JPEG
}

pictures.Clear();

tagger.Save(); // saving changes
```

## How to play audio file in C♯

```C#
using Luminescence.Audio;

// all audio formats supported by FFmpeg can be played
var player = new FFmpegAudioPlayer();
player.Play(@"C:\MyFolder\MyAudioFile.flac");

// a volume level of 1.0 means there is no attenuation or gain and 0 means silence
player.Volume = 0.5;
player.Muted = true;

player.Paused = true;
player.Stop();

player.Dispose();
```

## How to use the fingerprinting API in C♯

```C#
using Luminescence.Audio;

// all audio formats supported by FFmpeg can be fingerprinted
int lengthToFingerprintInSeconds = 120; // 0 to fingerprint all the file
int[] fingerprint1 = ChromaprintFingerprinter.GetFingerprint(@"C:\MyFolder\MyAudioFile1.flac", lengthToFingerprintInSeconds);
int[] fingerprint2 = ChromaprintFingerprinter.GetFingerprint(@"C:\MyFolder\MyAudioFile2.flac", lengthToFingerprintInSeconds);

float comparison = ChromaprintFingerprinter.MatchFingerprints(fingerprint1, fingerprint2);
float concordanceLevel = 0.9; // the file should be the same if the concordance level is > 0.9
bool sameFile = comparison > concordanceLevel;

string readableFingerprint = ChromaprintFingerprinter.EncodeFingerprint(fingerprint1);

bool sameFile2 = ChromaprintFingerprinter.CompareFile(@"C:\MyFolder\MyAudioFile3.flac", @"C:\MyFolder\MyAudioFile4.flac") > concordanceLevel;
```

## Native dependencies required

The Luminescence.Audio requires the following native dependencies to run (in 32 bits only):
- TagLib 1.11 : taglib.dll (use this [NuGet package](https://www.nuget.org/packages/taglibcpp/))
- FFmpeg 3.0 : avcodec-57.dll, avformat-57.dll, avutil-55.dll, swresample-2.dll (an audio-only optimized build is available [here](http://www.luminescence-software.org/download/audio/ffmpeg_audio.zip))
- Chromparint 1.3.1 : use the static version of the chromaprint library (that you can download [here](http://www.luminescence-software.org/download/audio/chromaprint_static_library.zip))
- XAudio 2.7 : xaudio2_7.dll (you can download the redistribuable [here](http://www.luminescence-software.org/download/audio/Jun2010_XAudio_x86.exe)
- The Visual C++ 2015 runtime (that you can download [here](http://www.luminescence-software.org/download/audio/vcredist_x86.exe))

## Licence

Luminescence.Audio is released under the [Mozilla Public License Version 2.0](https://www.mozilla.org/en-US/MPL/2.0/)

_Sylvain Rougeaux (Cyber Sinh)_