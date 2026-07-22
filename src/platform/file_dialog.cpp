#include "file_dialog.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <commdlg.h>
#include <cwchar>

#include "utf8.h"

std::optional<std::string> show_open_file_dialog_hwnd(HWND owner, const wchar_t* filter, const wchar_t* default_ext, const wchar_t* default_path)
{
    wchar_t path_buffer[MAX_PATH] = L"";
    if (default_path != nullptr)
    {
        wcsncpy_s(path_buffer, default_path, MAX_PATH - 1);
    }

    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = path_buffer;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = default_ext;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (!GetOpenFileNameW(&ofn))
    {
        return std::nullopt;
    }
    return wide_to_utf8(path_buffer);
}

std::optional<std::string> show_save_file_dialog_hwnd(HWND owner, const wchar_t* filter, const wchar_t* default_ext, const wchar_t* default_name)
{
    wchar_t path_buffer[MAX_PATH] = L"";
    if (default_name != nullptr)
    {
        wcsncpy_s(path_buffer, default_name, MAX_PATH - 1);
    }

    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = path_buffer;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = default_ext;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (!GetSaveFileNameW(&ofn))
    {
        return std::nullopt;
    }
    return wide_to_utf8(path_buffer);
}
