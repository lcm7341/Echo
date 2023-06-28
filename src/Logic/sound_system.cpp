#include "sound_system.hpp"

void SoundSystem::init() {
	FMOD_System_Create(&system, 2.02);
	FMOD_System_Init(system, 10, FMOD_INIT_NORMAL, nullptr);
}

void SoundSystem::update() {
	FMOD_System_Update(system);
}

void SoundSystem::play_sound(FMOD_CHANNEL* channel, const char* path, float volume, bool loop) {
	FMOD_SOUND* sound;

	FMOD_System_CreateSound(system, path, FMOD_DEFAULT, nullptr, &sound);
	if (loop) {
		FMOD_Sound_SetMode(sound, FMOD_LOOP_NORMAL);
	}
	FMOD_System_PlaySound(system, sound, nullptr, false, &channel);
	FMOD_Channel_SetVolume(channel, volume);
}