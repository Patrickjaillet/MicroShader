#pragma once

#include <string>

enum class RecordingFormat
{
    Gif,
    Mp4,
    WebM
};

bool ffmpeg_available();

class ViewportRecorder
{
public:
    ~ViewportRecorder();

    bool start(RecordingFormat format, int width, int height, int fps);
    void add_frame(const unsigned char* rgba, int width, int height);
    bool stop(const std::string& destination_utf8_path);
    void cancel();

    bool is_recording() const { return recording; }
    RecordingFormat format() const { return current_format; }
    int frame_count() const { return frames_written; }

private:
    void stop_ffmpeg_process();

    bool recording = false;
    RecordingFormat current_format = RecordingFormat::Gif;
    int width = 0;
    int height = 0;
    int frames_written = 0;

    void* gif_writer = nullptr;
    std::string gif_temp_path;

    void* ffmpeg_process = nullptr;
    void* ffmpeg_stdin_write = nullptr;
};
