#include "logic.hpp"
#include "../Hooks/hooks.hpp"
#include <stdio.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

#define PLAYLAYER gd::GameManager::sharedState()->getPlayLayer()

unsigned Logic::get_frame() {
    if (PLAYLAYER) {
        return round(PLAYLAYER->m_time * get_fps());
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
        add_input({ get_frame(), down, twoplayer && !player1 });
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
        auto& inputs = get_inputs();
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
    auto& inputs = get_inputs();

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

    w_b(fps)

        for (auto& input : inputs) {
            w_b(input.frame);
            w_b(input.down);
            w_b(input.player1);
        }

    file.close();

    /* Pipe Dream
    json obj;
    obj["fps"] = fps;
    obj["inputs"] = json::array();

    for (const auto& input : inputs) {
        obj["inputs"].push_back(json{
            {
                "frame", input.frame
            },
            {
                "down", input.down
            },
            {
                "player1", input.player1
            }
        });
    }

    std::ofstream json_file(dir + filename + ".json");
    json_file << obj.dump(4);
    json_file.close();*/
}

void Logic::read_file(const std::string& filename) {
    std::string dir = ".echo\\";
    std::string ext = ".bin";

    std::string full_filename = dir + filename + ext;

    std::ifstream file(full_filename, std::ios::binary);
    if (!file.is_open()) {
        error = "Error reading file '" + filename + "'!";
        return;
    }
    error = "";

    inputs.clear();

    r_b(fps)

        while (true) {
            Input input;
            r_b(input.frame);
            r_b(input.down);
            r_b(input.player1);

            if (file.eof()) {
                break;
            }

            inputs.push_back(input);
        }

    file.close();
}

void Logic::remove_inputs(unsigned frame) {
    auto it = std::remove_if(inputs.begin(), inputs.end(),
        [frame](const Input& input) {
            return input.frame > frame;
        });
    inputs.erase(it, inputs.end());
}

void Logic::handle_checkpoint_data() {
    if (PLAYLAYER) {
        if (checkpoints.size() > 0) {
            Checkpoint& data = checkpoints.back();


            PLAYLAYER->m_player1->setRotation(data.player_1.rotation);
            PLAYLAYER->m_player1->m_yAccel = data.player_1.y_accel;

            if (PLAYLAYER->m_isDualMode) {
                PLAYLAYER->m_player2->setRotation(data.player_2.rotation);
                PLAYLAYER->m_player2->m_yAccel = data.player_2.y_accel;
            }

        }
    }
}