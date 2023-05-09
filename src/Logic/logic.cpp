#include "logic.hpp"
#include "../Hooks/hooks.hpp"
#define PLAYLAYER gd::GameManager::sharedState()->getPlayLayer()

unsigned Logic::get_frame() {
    if (PLAYLAYER) {
        return round(PLAYLAYER->m_time * macro.get_fps());
    }
    return 0;
}

double Logic::get_time() {
    if (PLAYLAYER) {
        return PLAYLAYER->m_time;
    }
    return 0.f;
}

void Logic::record_input(bool down, bool player1) {
    if (is_recording() || is_both()) {
        auto twoplayer = PLAYLAYER->m_levelSettings->m_twoPlayerMode;
        player1 ^= 1 && gd::GameManager::sharedState()->getGameVariable("0010"); // what the fuck ?
        macro.add_input({ get_frame(), down, twoplayer && !player1 });
    }
}

void Logic::play_input(Input& input) {
    auto gamevar = gd::GameManager::sharedState()->getGameVariable("0010"); // game var again 
    // i think its for flip 2 player controls?

    if (input.down)
        Hooks::PlayLayer::pushButton(PLAYLAYER, 0, input.player1 ^ gamevar);
    else
        Hooks::PlayLayer::releaseButton(PLAYLAYER, 0, input.player1 ^ gamevar);
}

void Logic::play_macro() {
    if (is_playing()) {
        auto& inputs = macro.get_inputs();
        unsigned currentFrame = get_frame();
        
        while (inputs[replay_pos].frame <= get_frame() && replay_pos < inputs.size()) {
            play_input(inputs[replay_pos]);
            replay_pos += 1;
        }
    }
}

void Logic::set_replay_pos(unsigned idx) {
    replay_pos = idx;
}

int Logic::find_closest_input() {
    unsigned current_frame = get_frame();
    auto& inputs = macro.get_inputs();

    auto it = std::lower_bound(inputs.begin(), inputs.end(), Input{ current_frame },
        [](const Input& a, const Input& b) {
            return a.frame < b.frame;
        });
    if (it == inputs.begin()) {
        return 0;
    }
    else {
        return std::distance(inputs.begin(), std::prev(it));
    }
}