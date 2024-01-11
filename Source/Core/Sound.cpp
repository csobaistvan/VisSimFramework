#include "PCH.h"
#include "Sound.h"
#include "Debug.h"

namespace Sound
{
    ////////////////////////////////////////////////////////////////////////////////
    void playSystemSound(std::string const& fileName)
    {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        std::wstring wFileName = converter.from_bytes(fileName.c_str());
        PlaySound(wFileName.c_str(), NULL, SND_ASYNC | SND_ALIAS);
    }
}