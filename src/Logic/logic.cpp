#include "logic.hpp"
#include "../Hooks/hooks.hpp"
#include <stdio.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <algorithm>  // for std::sort and std::min_element
#include <cmath>      // for std::abs

using json = nlohmann::json;

#define PLAYLAYER gd::GameManager::sharedState()->getPlayLayer()

unsigned Logic::get_frame() {
    if (PLAYLAYER) {
        return round((PLAYLAYER->m_time * get_fps()) - get_removed());
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
        add_input({ get_frame(), down });
    }
}

void Logic::play_input(Frame& input) {
    auto gamevar = gd::GameManager::sharedState()->getGameVariable("0010"); // game var again 
    // i think its for flip 2 player controls?
    auto editor = gd::GameManager::sharedState()->getEditorLayer();

    if (PLAYLAYER) {
        if (input.down)
            Hooks::PlayLayer::pushButton(PLAYLAYER, 0, !input.player1 ^ gamevar);
        else
            Hooks::PlayLayer::releaseButton(PLAYLAYER, 0, !input.player1 ^ gamevar);
    }

bool Logic::play_macro(int& offset) {
    if (is_playing()) {
        auto& inputs = get_inputs();
        unsigned current_frame = get_frame()/* - offset*/;
        bool ret = false;

        while (inputs[replay_pos].number <= current_frame && replay_pos < inputs.size()) {
            play_input(inputs[replay_pos]);
            replay_pos += 1;
            ret = true;
        }

        return ret;
    }
}

void Logic::offset_inputs(int lower, int upper) {
    srand(time(0)); // seed the random number generator with the current time
    std::transform(inputs.begin(), inputs.end(), inputs.begin(),
        [lower, upper](Frame input) {
            int offset = lower + rand() % (upper - lower + 1); // generate random offset
            input.number += offset;  // add offset to the frame
            return input;
        });
}

void Logic::set_replay_pos(unsigned idx) {
    replay_pos = idx;
}

#define w_b(var) file.write(reinterpret_cast<char*>(&var), sizeof(var));
#define r_b(var) file.read(reinterpret_cast<char*>(&var), sizeof(var));

void Logic::write_file(const std::string& filename) {
    std::string dir = ".echo\\";
    std::string ext = ".bin";

    std::string full_filename = dir + filename + ext;
    
    std::ofstream file(full_filename, std::ios::binary);
    if (!file.is_open()) {
        error = "Error writing file '" + filename + "'!";
        return;
    }
    error = "";

    w_b(fps);

    for (auto& input : inputs) {
        w_b(input.number);
        w_b(input.pressingDown);
    }

    file.close();
}

void Logic::read_file(const std::string& filename, bool is_path = false) {
    std::string dir = ".echo\\";
    std::string ext = ".bin";

    std::string full_filename = is_path ? filename : dir + filename + ext;

    std::ifstream file(full_filename, std::ios::binary);
    if (!file.is_open()) {
        error = "Error reading file '" + filename + "'!";
        return;
    }
    error = "";

    if (!is_path)
    inputs.clear();

    r_b(fps);

    while (true) {
        Frame input;
        r_b(input.number);
        r_b(input.pressingDown);

        if (file.eof()) {
            break;
        }

        inputs.push_back(input);
    }

    file.close();
}

void Logic::remove_inputs(unsigned frame) {
    auto it = std::remove_if(inputs.begin(), inputs.end(),
        [frame](const Frame& input) {
            return input.number > frame;
        });
    inputs.erase(it, inputs.end());
}

void Logic::convert_file(const std::string& filename, bool is_path = false) {
    try {
        std::string dir = ".echo\\";
        std::string ext = ".json";

        std::string full_filename = is_path ? filename : dir + filename + ext;

        std::ifstream file(full_filename);
        if (!file.is_open()) {
            error = "Error reading file '" + filename + "'!";
            return;
        }
        error = "";

        json j;
        file >> j;

        fps = j["fps"].get<double>();

        inputs.clear();
        for (auto& macro : j["macro"]) {
            Frame input;

            input.number = macro["frame"].get<unsigned>();
            input.pressingDown = macro["player_1"]["click"].get<int>() != 2;

            inputs.push_back(input);
        }

        file.close();
    }
    catch (std::exception& e) {
        std::cout << "Caught exception: " << e.what() << std::endl;
    }
    catch (...) {
        std::cout << "Caught unknown exception." << std::endl;
    }
}

void Logic::sort_inputs() {
    std::sort(inputs.begin(), inputs.end(), [](const Frame& a, const Frame& b) {
        return a.number < b.number;
        });
}

void Logic::handle_checkpoint_data() {
    if (PLAYLAYER) {
        if (checkpoints.size() > 0) {
            Checkpoint& data = checkpoints.back();
            data.player_1_data.apply(PLAYLAYER->m_player1);
            data.player_2_data.apply(PLAYLAYER->m_player2);
        }
    }
}