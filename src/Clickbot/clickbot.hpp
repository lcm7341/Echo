#include <fmod.hpp>
#include <fmod.h>
#include <string>
#include <chrono>

namespace Clickbot
{
    std::string pickRandomFile(std::string folder, bool player1);

    static inline bool inited = false;
    static inline std::chrono::system_clock::time_point start, now;
    static inline std::chrono::duration<double> cycleTime;

    static inline FMOD::System* system;
    static inline FMOD::Channel* clickChannel;
    static inline FMOD::Sound* clickSound;
    static inline FMOD::Channel* releaseChannel;
    static inline FMOD::Sound* releaseSound;

    static inline FMOD::Channel* clickChannel2;
    static inline FMOD::Sound* clickSound2;
    static inline FMOD::Channel* releaseChannel2;
    static inline FMOD::Sound* releaseSound2;

    static inline FMOD::Channel* noiseChannel;
    static inline FMOD::Sound* noiseSound;
}

