#include <fmod.hpp>
#include <fmod.h>
#include <string>
#include <chrono>

namespace Clickbot
{
    std::string pickRandomFile(std::string path, std::string folder);

    static bool inited = false;

    static FMOD::System* system;
    static FMOD::Channel* clickChannel;
    static FMOD::Sound* clickSound;
    static FMOD::Channel* releaseChannel;
    static FMOD::Sound* releaseSound;

    static FMOD::Channel* clickChannel2;
    static FMOD::Sound* clickSound2;
    static FMOD::Channel* releaseChannel2;
    static FMOD::Sound* releaseSound2;

    static FMOD::Channel* noiseChannel;
    static FMOD::Sound* noiseSound;
}

