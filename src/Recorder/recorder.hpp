#pragma once
#include <thread>
#include <mutex>
#include <queue>
#include <vector>
#include "../subprocess.hpp"
#include <string>
#include <cocos2d.h>

#include <gd.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

using namespace cocos2d;

using u8 = uint8_t;

class MyRenderTexture {
public:
    unsigned m_width, m_height;
    int m_old_fbo, m_old_rbo;
    unsigned m_fbo;
    CCTexture2D* m_texture;
    void begin();
    void end();
    void capture(std::mutex& lock, std::vector<u8>& data, volatile bool& lul);
};

class Recorder {
public:
    Recorder();
    std::vector<u8> m_current_frame;
    volatile bool m_frame_has_data;
    std::mutex m_lock;
    MyRenderTexture m_renderer;
    unsigned m_width, m_height;
    unsigned m_fps;
    bool m_recording = false;
    double m_last_frame_t, m_extra_t;
    bool m_until_end = true;
    std::string m_codec = "", m_extra_args = "", m_vf_args = "", m_extra_audio_args = "";
    int m_bitrate = 12;
    float m_after_end_duration = 3.f;
    float m_after_end_extra_time;
    float m_song_start_offset;
    bool m_finished_level;
    bool m_include_audio = true;
    std::string directory = ".echo/renders/";
    std::string extension = ".mp4";
    std::string video_name = "output";
    std::string m_ffmpeg_path = "ffmpeg";
    float v_fade_in_time = 2;
    float v_fade_out_time = 2;
    float a_fade_in_time = 2;
    float a_fade_out_time = 2;
    bool fade_audio = true;
    bool fade_video = true;
    bool color_fix = true;
    bool ssb_fix = true;
    bool real_time_rendering = true;

    void start(const std::string& path);
    void stop();
    void capture_frame();
    void handle_recording(gd::PlayLayer*, float dt);
    void update_song_offset(gd::PlayLayer*);
};