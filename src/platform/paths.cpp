#include "paths.h"

#include <SDL3/SDL.h>

#include <cstdio>

#include "utf8.h"

std::string asset_path(const std::string& relative_path)
{
    const char* base_path = SDL_GetBasePath();
    std::string result = base_path != nullptr ? base_path : "./";
    result += "assets/";
    result += relative_path;
    return result;
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
