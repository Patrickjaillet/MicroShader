#include "paths.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <SDL3/SDL.h>

#include <cstdio>
#include <cstdlib>

#include "utf8.h"

std::string asset_path(const std::string& relative_path)
{
    const char* base_path = SDL_GetBasePath();
    std::string result = base_path != nullptr ? base_path : "./";
    result += "assets/";
    result += relative_path;
    return result;
}

std::string app_data_dir()
{
    const wchar_t* appdata = _wgetenv(L"APPDATA");
    if (appdata == nullptr)
    {
        return std::string();
    }
    std::wstring dir = appdata;
    dir += L"\\ushader\\";
    CreateDirectoryW(dir.c_str(), nullptr);
    return wide_to_utf8(dir.c_str());
}

std::string read_utf8_file(const std::string& utf8_path)
{
    std::wstring wide_path = utf8_to_wide(utf8_path);
    FILE* file = nullptr;
    _wfopen_s(&file, wide_path.c_str(), L"rb");
    if (file == nullptr)
    {
        return std::string();
    }

    std::string content;
    char buffer[4096];
    size_t read_count = 0;
    while ((read_count = std::fread(buffer, 1, sizeof(buffer), file)) > 0)
    {
        content.append(buffer, read_count);
    }
    std::fclose(file);
    return content;
}

bool write_utf8_file(const std::string& utf8_path, const std::string& content)
{
    std::wstring wide_path = utf8_to_wide(utf8_path);
    FILE* file = nullptr;
    _wfopen_s(&file, wide_path.c_str(), L"wb");
    if (file == nullptr)
    {
        return false;
    }

    size_t written = std::fwrite(content.data(), 1, content.size(), file);
    std::fclose(file);
    return written == content.size();
}

bool file_exists(const std::string& utf8_path)
{
    std::wstring wide_path = utf8_to_wide(utf8_path);
    DWORD attributes = GetFileAttributesW(wide_path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}
