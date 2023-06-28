#pragma once
#include <fmod.h>

class SoundSystem {
	FMOD_SYSTEM* system;
	FMOD_CHANNEL* clicks_channel;
public:

	auto& get_system() { return system; }

	void init();
	void update();

	void play_sound(FMOD_CHANNEL* channel, const char* path, float volume, bool loop);
};