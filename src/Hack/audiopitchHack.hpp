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

    void* channel;
    float speed;
    bool initialized;
    bool enabled;

private:
    AudiopitchHack();
    void initialize();
};

void* __stdcall volumeAdjustHook(void* t_channel, float volume);

#endif // AUDIOPITCH_HACK_H
