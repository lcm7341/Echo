#ifndef AUDIOPITCH_HACK_H
#define AUDIOPITCH_HACK_H

class AudiopitchHack {
public:
    static AudiopitchHack& getInstance();
    AudiopitchHack(AudiopitchHack const&) = delete;
    void operator=(AudiopitchHack const&) = delete;

    bool isEnabled() const;
    void setEnabled(bool enabled);
    void setPitch(float freq);

    bool initialized = false;
    bool enabled;
    void* channel;
    float speed;

private:
    AudiopitchHack();
    void initialize();
};

void* __stdcall setVolumeHook(void* t_channel, float volume);

#endif // AUDIOPITCH_HACK_H
