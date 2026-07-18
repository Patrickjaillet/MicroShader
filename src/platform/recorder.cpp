#include "recorder.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <algorithm>
#include <cstring>
#include <sstream>
#include <vector>

#include <gif.h>

#include "utf8.h"

namespace
{
    std::string ascii_temp_file_path(const char* filename)
    {
        wchar_t temp_dir[MAX_PATH];
        DWORD len = GetTempPathW(MAX_PATH, temp_dir);
        if (len == 0 || len >= MAX_PATH)
        {
            return std::string();
        }

        wchar_t short_dir[MAX_PATH];
        DWORD short_len = GetShortPathNameW(temp_dir, short_dir, MAX_PATH);
        if (short_len == 0 || short_len >= MAX_PATH)
        {
            return std::string();
        }

        std::string result = wide_to_utf8(short_dir);
        result += filename;
        return result;
    }

    bool move_to_destination(const std::string& src_ascii_path, const std::string& dest_utf8_path)
    {
        std::wstring src_wide = utf8_to_wide(src_ascii_path);
        std::wstring dest_wide = utf8_to_wide(dest_utf8_path);
        return MoveFileExW(src_wide.c_str(), dest_wide.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED) != 0;
    }

    void flip_rows_rgba(const unsigned char* src, std::vector<unsigned char>& dst, int width, int height)
    {
        size_t row_bytes = static_cast<size_t>(width) * 4;
        dst.resize(row_bytes * static_cast<size_t>(height));
        for (int y = 0; y < height; ++y)
        {
            const unsigned char* src_row = src + static_cast<size_t>(height - 1 - y) * row_bytes;
            unsigned char* dst_row = dst.data() + static_cast<size_t>(y) * row_bytes;
            std::memcpy(dst_row, src_row, row_bytes);
        }
    }
}

bool ffmpeg_available()
{
    static bool checked = false;
    static bool available = false;
    if (checked)
    {
        return available;
    }
    checked = true;

    STARTUPINFOW startup_info{};
    startup_info.cb = sizeof(startup_info);
    startup_info.dwFlags = STARTF_USESHOWWINDOW;
    startup_info.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION process_info{};

    wchar_t command_line[] = L"ffmpeg -version";
    BOOL created = CreateProcessW(nullptr, command_line, nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &startup_info, &process_info);
    if (!created)
    {
        return false;
    }

    WaitForSingleObject(process_info.hProcess, 5000);
    DWORD exit_code = 1;
    GetExitCodeProcess(process_info.hProcess, &exit_code);
    CloseHandle(process_info.hProcess);
    CloseHandle(process_info.hThread);

    available = (exit_code == 0);
    return available;
}

ViewportRecorder::~ViewportRecorder()
{
    cancel();
}

bool ViewportRecorder::start(RecordingFormat format, int in_width, int in_height, int fps)
{
    if (recording)
    {
        return false;
    }

    width = in_width;
    height = in_height;
    frames_written = 0;
    current_format = format;

    if (format == RecordingFormat::Gif)
    {
        gif_temp_path = ascii_temp_file_path("ushader_recording.gif");
        if (gif_temp_path.empty())
        {
            return false;
        }

        GifWriter* writer = new GifWriter();
        int delay_hundredths = fps > 0 ? std::max(1, 100 / fps) : 4;
        if (!GifBegin(writer, gif_temp_path.c_str(), static_cast<uint32_t>(width), static_cast<uint32_t>(height), static_cast<uint32_t>(delay_hundredths)))
        {
            delete writer;
            return false;
        }

        gif_writer = writer;
        recording = true;
        return true;
    }

    if (!ffmpeg_available())
    {
        return false;
    }

    SECURITY_ATTRIBUTES pipe_attributes{};
    pipe_attributes.nLength = sizeof(pipe_attributes);
    pipe_attributes.bInheritHandle = TRUE;

    HANDLE read_handle = nullptr;
    HANDLE write_handle = nullptr;
    if (!CreatePipe(&read_handle, &write_handle, &pipe_attributes, 16 * 1024 * 1024))
    {
        return false;
    }
    SetHandleInformation(write_handle, HANDLE_FLAG_INHERIT, 0);

    gif_temp_path = ascii_temp_file_path(format == RecordingFormat::Mp4 ? "ushader_recording.mp4" : "ushader_recording.webm");

    std::wstringstream command;
    command << L"ffmpeg -y -f rawvideo -pixel_format rgba -video_size " << width << L"x" << height
            << L" -framerate " << fps << L" -i - -vf vflip ";
    if (format == RecordingFormat::Mp4)
    {
        command << L"-c:v libx264 -pix_fmt yuv420p -movflags +faststart ";
    }
    else
    {
        command << L"-c:v libvpx-vp9 -pix_fmt yuv420p ";
    }
    command << L"\"" << utf8_to_wide(gif_temp_path) << L"\"";

    std::wstring command_str = command.str();
    std::vector<wchar_t> command_buffer(command_str.begin(), command_str.end());
    command_buffer.push_back(L'\0');

    STARTUPINFOW startup_info{};
    startup_info.cb = sizeof(startup_info);
    startup_info.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    startup_info.wShowWindow = SW_HIDE;
    startup_info.hStdInput = read_handle;
    startup_info.hStdOutput = nullptr;
    startup_info.hStdError = nullptr;

    PROCESS_INFORMATION process_info{};
    BOOL created = CreateProcessW(nullptr, command_buffer.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &startup_info, &process_info);
    CloseHandle(read_handle);

    if (!created)
    {
        CloseHandle(write_handle);
        return false;
    }

    CloseHandle(process_info.hThread);
    ffmpeg_process = process_info.hProcess;
    ffmpeg_stdin_write = write_handle;
    recording = true;
    return true;
}

void ViewportRecorder::add_frame(const unsigned char* rgba, int frame_width, int frame_height)
{
    if (!recording || frame_width != width || frame_height != height)
    {
        return;
    }

    if (current_format == RecordingFormat::Gif)
    {
        std::vector<unsigned char> flipped;
        flip_rows_rgba(rgba, flipped, width, height);
        GifWriter* writer = static_cast<GifWriter*>(gif_writer);
        GifWriteFrame(writer, flipped.data(), static_cast<uint32_t>(width), static_cast<uint32_t>(height), 4);
    }
    else if (ffmpeg_stdin_write != nullptr)
    {
        DWORD written = 0;
        WriteFile(static_cast<HANDLE>(ffmpeg_stdin_write), rgba, static_cast<DWORD>(width) * static_cast<DWORD>(height) * 4, &written, nullptr);
    }

    frames_written += 1;
}

void ViewportRecorder::stop_ffmpeg_process()
{
    if (ffmpeg_stdin_write != nullptr)
    {
        CloseHandle(static_cast<HANDLE>(ffmpeg_stdin_write));
        ffmpeg_stdin_write = nullptr;
    }
    if (ffmpeg_process != nullptr)
    {
        WaitForSingleObject(static_cast<HANDLE>(ffmpeg_process), 30000);
        CloseHandle(static_cast<HANDLE>(ffmpeg_process));
        ffmpeg_process = nullptr;
    }
}

bool ViewportRecorder::stop(const std::string& destination_utf8_path)
{
    if (!recording)
    {
        return false;
    }

    bool success = false;

    if (current_format == RecordingFormat::Gif)
    {
        GifWriter* writer = static_cast<GifWriter*>(gif_writer);
        GifEnd(writer);
        delete writer;
        gif_writer = nullptr;
    }
    else
    {
        stop_ffmpeg_process();
    }

    success = move_to_destination(gif_temp_path, destination_utf8_path);
    gif_temp_path.clear();
    recording = false;
    return success;
}

void ViewportRecorder::cancel()
{
    if (!recording)
    {
        return;
    }

    if (current_format == RecordingFormat::Gif)
    {
        if (gif_writer != nullptr)
        {
            GifWriter* writer = static_cast<GifWriter*>(gif_writer);
            GifEnd(writer);
            delete writer;
            gif_writer = nullptr;
        }
    }
    else
    {
        stop_ffmpeg_process();
    }

    if (!gif_temp_path.empty())
    {
        std::wstring wide_path = utf8_to_wide(gif_temp_path);
        DeleteFileW(wide_path.c_str());
        gif_temp_path.clear();
    }

    recording = false;
}
