#include "audiopitchHack.hpp"
#include <Windows.h> 
#include <MinHook.h>

// Global function pointers
void* (__stdcall* setVolume)(void* t_channel, float volume);
void* (__stdcall* setFrequency)(void* t_channel, float frequency);
void* __stdcall setVolumeHook(void* t_channel, float volume);

AudiopitchHack::AudiopitchHack() : channel(nullptr), speed(1.f), enabled(false) {}


bool AudiopitchHack::isEnabled() const {
    return enabled;
}

void AudiopitchHack::setEnabled(bool enabled) {
    this->enabled = enabled;
}

void AudiopitchHack::setPitch(float freq) {
    if (!enabled) return;
    speed = freq;
    setFrequency(channel, freq);
}

void* __stdcall setVolumeHook(void* t_channel, float volume) {
    AudiopitchHack& self = AudiopitchHack::getInstance();
    self.channel = t_channel;
    if (self.speed != 1.f) setFrequency(self.channel, self.speed);
    return setVolume(self.channel, volume);
}

void AudiopitchHack::initialize() {
    setFrequency = (decltype(setFrequency))GetProcAddress(GetModuleHandle("fmod.dll"), "?setPitch@ChannelControl@FMOD@@QAG?AW4FMOD_RESULT@@M@Z");
    void* hookAddr = GetProcAddress(GetModuleHandle("fmod.dll"), "?setVolume@ChannelControl@FMOD@@QAG?AW4FMOD_RESULT@@M@Z");

    MH_CreateHook(
        hookAddr,
        &setVolumeHook,
        reinterpret_cast<LPVOID*>(&setVolume)
    );
}