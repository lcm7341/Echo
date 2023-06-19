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
        add_input({ get_frame(), down, twoplayer && !player1, PLAYLAYER->m_player1->getPositionY(), PLAYLAYER->m_player1->getPositionX(), PLAYLAYER->m_player1->getRotation(), PLAYLAYER->m_player1->m_yAccel, PLAYLAYER->m_player1->m_xAccel });
    }
}

void Logic::play_input(Frame& input) {
    auto gamevar = gd::GameManager::sharedState()->getGameVariable("0010"); // game var again 
    auto editor = gd::GameManager::sharedState()->getEditorLayer();

    // PLAYLAYER->m_player1->setPositionY(input.yPosition);
    // PLAYLAYER->m_player1->setPositionX(input.xPosition);

    if (PLAYLAYER->m_player1->m_yAccel != input.yVelocity) {
        printf("MISMATCH Y VELOCITY %f:%f\n", PLAYLAYER->m_player1->m_yAccel, input.yVelocity);
    }

    if (PLAYLAYER->m_player1->m_xAccel != input.xVelocity) {
        printf("MISMATCH X VELOCITY %f:%f\n", PLAYLAYER->m_player1->m_xAccel, input.xVelocity);
    }

    if (PLAYLAYER) {
        live_inputs.push_back(input);
        if (input.pressingDown) {
            Hooks::PlayLayer::pushButton(PLAYLAYER, 0, !input.isPlayer2 ^ gamevar);
        }
        else {
            Hooks::PlayLayer::releaseButton(PLAYLAYER, 0, !input.isPlayer2 ^ gamevar);
        }
    }
}

unsigned Logic::count_presses_in_last_second() {
    unsigned frame_limit = round((((PLAYLAYER->m_time) * get_fps()) - get_removed()) - get_fps());
    if (frame_limit > ((((PLAYLAYER->m_time) * get_fps()) - get_removed()) + 2))
        frame_limit = 0;

    std::vector<float> cps_percents;

    unsigned press_count = 0;
    if (is_recording()) {
        for (auto it = inputs.rbegin(); it != inputs.rend(); ++it) {
            if (it->number > frame_limit && it->pressingDown) {
                press_count++;
            }
        }
    }
    else if (is_playing()) {
        for (auto it = live_inputs.rbegin(); it != live_inputs.rend(); ++it) {
            if (it->number > frame_limit && it->pressingDown) {
                press_count++;
            }
        }
    }

    if (press_count > max_cps) {
        over_max_cps = true;
    }

    current_cps = press_count;

    return press_count;
}

std::string Logic::highest_cps() {
    int highest_cps = 0;
    std::string to_return = "0";

    std::vector<float> cps_percents;
    for (Frame frames : inputs) {
        int frame_limit = frames.number - get_fps() > 0 ? frames.number - get_fps() : 0;

        unsigned press_count = 0;
        for (Frame frame : inputs) {
            if (frame.number >= frame_limit && frame.number <= frames.number && frame.pressingDown) {
                press_count++;
            }
        }

        // Check if press_count > max_cps after incrementing press_count in the loop
        if (press_count > max_cps) {
            float current_percent = std::round(((frames.number / end_portal_position) * 100.f) * 100) / 100;
            bool should_push = true;
            for (const auto& percent : cps_percents) {
                if (std::abs(percent - current_percent) <= 1) {
                    should_push = false;
                }
            }
            if (should_push) {
                cps_percents.push_back(current_percent + 0.5f);
            }
        }

        if (press_count > highest_cps) {
            highest_cps = press_count;
            to_return = std::to_string(highest_cps) + " (" + std::to_string(frame_limit) + " to " + std::to_string(frames.number) + ")";
        }
    }

    cps_over_percents = cps_percents;

    return to_return;
}

int Logic::find_closest_input() {
    if (inputs.empty()) {
        return -1;
    }

    unsigned current_frame = get_frame();
    auto closest_it = std::min_element(inputs.begin(), inputs.end(),
        [current_frame](const Frame& a, const Frame& b) {
            return std::abs(static_cast<int>(a.number - current_frame)) < std::abs(static_cast<int>(b.number - current_frame));
        });
    return std::distance(inputs.begin(), closest_it);
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
    w_b(end_portal_position);

    for (auto& input : inputs) {
        w_b(input.number);
        w_b(input.pressingDown);
        w_b(input.isPlayer2);

        /*w_b(input.yPosition);
        w_b(input.xPosition);
        w_b(input.rotation);
        w_b(input.yVelocity);
        w_b(input.xVelocity);*/
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
    r_b(end_portal_position);

    while (true) {
        Frame input;
        r_b(input.number);
        r_b(input.pressingDown);
        r_b(input.isPlayer2);

        /*r_b(input.yPosition);
        r_b(input.xPosition);
        r_b(input.rotation);
        r_b(input.yVelocity);
        r_b(input.xVelocity);*/

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
            return input.number >= frame;
        });
    inputs.erase(it, inputs.end());
}

void Logic::write_file_json(const std::string& filename) {
    std::string dir = ".echo\\";
    std::string ext = ".echo";

    std::string full_filename = dir + filename + ext;

    std::ofstream file(full_filename);
    if (!file.is_open()) {
        error = "Error writing file '" + filename + "'!";
        return;
    }
    error = "";

    // Create a JSON object to store the state data
    json state;

    state["fps"] = fps;
    state["end_xpos"] = end_portal_position;

    // Create a JSON array to store the input data
    json json_inputs = json::array();
    for (auto& input : inputs) {
        // Each input is a JSON object
        json json_input;
        json_input["frame"] = input.number;
        json_input["holding"] = input.pressingDown;
        json_input["player_2"] = input.isPlayer2;
        // Add the input object to the array
        json_inputs.push_back(json_input);
    }

    // Add the array to the state object
    state["inputs"] = json_inputs;

    // Write the JSON object to the file
    file << state.dump(4);  // 4 spaces for indentation

    file.close();
}

void Logic::read_file_json(const std::string& filename, bool is_path = false) {
    std::string dir = ".echo\\";
    std::string ext = ".echo";

    std::string full_filename = is_path ? filename : dir + filename + ext;

    std::ifstream file(full_filename);
    if (!file.is_open()) {
        error = "Error reading file '" + filename + "'!";
        return;
    }
    error = "";

    // Parse the JSON object from the file
    json state;
    file >> state;

    // Extract the state data from the JSON object
    fps = state["fps"].get<double>();
    end_portal_position = state["end_xpos"].get<double>();

    inputs.clear();
    // Extract the input data from the JSON object
    for (auto& json_input : state["inputs"]) {
        Frame input;
        input.number = json_input["frame"].get<unsigned>();
        input.pressingDown = json_input["holding"].get<bool>();
        input.isPlayer2 = json_input["player_2"].get<bool>();
        inputs.push_back(input);
    }

    file.close();
}

void Logic::sort_inputs() {
    std::unordered_map<unsigned, std::vector<Frame>> frameMap;

    for (const auto& frame : inputs) {
        frameMap[frame.number].push_back(frame);
    }

    inputs.clear();

    std::unordered_map<bool, std::deque<Frame>> pressQueues;
    std::unordered_map<bool, std::deque<Frame>> releaseQueues;

    for (const auto& [frameNumber, frames] : frameMap) {
        std::vector<Frame> mergedFrames;

        for (bool isPlayer2 : {false, true}) {
            for (const auto& frame : frames) {
                if (frame.isPlayer2 != isPlayer2) continue;

                if (frame.pressingDown) {
                    pressQueues[isPlayer2].push_back(frame);
                }
                else {
                    if (!pressQueues[isPlayer2].empty()) {
                        mergedFrames.push_back(pressQueues[isPlayer2].front());
                        pressQueues[isPlayer2].pop_front();
                        mergedFrames.push_back(frame);
                    }
                    else {
                        releaseQueues[isPlayer2].push_back(frame);
                    }
                }
            }

            while (!pressQueues[isPlayer2].empty() && !releaseQueues[isPlayer2].empty()) {
                mergedFrames.push_back(pressQueues[isPlayer2].front());
                pressQueues[isPlayer2].pop_front();
                mergedFrames.push_back(releaseQueues[isPlayer2].front());
                releaseQueues[isPlayer2].pop_front();
            }

            inputs.insert(inputs.end(), mergedFrames.begin(), mergedFrames.end());
            mergedFrames.clear();
        }
    }

    // If there are any unpaired 'click' frames, append them to the inputs
    for (bool isPlayer2 : {false, true}) {
        while (!pressQueues[isPlayer2].empty()) {
            inputs.push_back(pressQueues[isPlayer2].front());
            pressQueues[isPlayer2].pop_front();
        }
    }

    // Sort the inputs by frames unless they have the same frame number
    std::sort(inputs.begin(), inputs.end(), [](const Frame& a, const Frame& b) {
        return (a.number != b.number) && (a.number < b.number);
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