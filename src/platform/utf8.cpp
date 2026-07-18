#include "utf8.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

std::string wide_to_utf8(const wchar_t* wide)
{
    int size = WideCharToMultiByte(CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, nullptr);
    if (size <= 0)
    {
        return std::string();
    }
    std::string result(static_cast<size_t>(size) - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide, -1, result.data(), size, nullptr, nullptr);
    return result;
}

std::wstring utf8_to_wide(const std::string& utf8)
{
    int size = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    if (size <= 0)
    {
        return std::wstring();
    }
    std::wstring result(static_cast<size_t>(size) - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, result.data(), size);
    return result;
}
