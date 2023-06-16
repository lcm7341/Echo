#include "audiopitchHack.hpp"
#include <Windows.h> 
#include "MinHook.h" 

// Global function pointers
void* (__stdcall* setVolume)(void* t_channel, float volume);
void* (__stdcall* setFrequency)(void* t_channel, float frequency);

AudiopitchHack::AudiopitchHack() : channel(nullptr), speed(1.f), initialized(false), enabled(false) {}

AudiopitchHack& AudiopitchHack::getInstance() {
    static AudiopitchHack instance;
    return instance;
}

bool AudiopitchHack::isEnabled() const {
    return enabled;
}

void AudiopitchHack::setEnabled(bool enabled) {
    this->enabled = enabled;
    if (enabled) {
        if (!initialized) initialize();
    }
}

void AudiopitchHack::setPitch(float freq) {
    if (!initialized || !enabled || channel == nullptr) return;
    speed = freq;
    setFrequency(channel, freq);
}

void AudiopitchHack::initialize() {
    if (initialized) return;
    setFrequency = (decltype(setFrequency))GetProcAddress(GetModuleHandle("fmod.dll"), "?setPitch@ChannelControl@FMOD@@QAG?AW4FMOD_RESULT@@M@Z");
    DWORD hookAddr = (DWORD)GetProcAddress(GetModuleHandle("fmod.dll"), "?setVolume@ChannelControl@FMOD@@QAG?AW4FMOD_RESULT@@M@Z");

    MH_CreateHook(
        (PVOID)hookAddr,
        &volumeAdjustHook,
        (PVOID*)&setVolume
    );

    initialized = true;
}

void* __stdcall volumeAdjustHook(void* t_channel, float volume) {
    AudiopitchHack& self = AudiopitchHack::getInstance();
    self.channel = t_channel;
    if (self.speed != 1.f) setFrequency(self.channel, self.speed);
    return setVolume(self.channel, volume);
}