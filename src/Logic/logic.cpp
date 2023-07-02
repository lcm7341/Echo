#include "logic.hpp"
#include "../Hooks/hooks.hpp"
#include <stdio.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <algorithm>  // for std::sort and std::min_element
#include <cmath>      // for std::abs
#include <random>

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

double Logic::xpos_calculation() {
    return previous_xpos + ((60.f * player_speed * player_acceleration) * (1.f / fps));
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

    if (!input.isPlayer2 ^ gamevar && !play_player_1)
        return;
    if (!(!input.isPlayer2 ^ gamevar) && !play_player_2)
        return;

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
    std::vector<std::pair<float, std::string>> cps_percents;
    std::string highest_cps = "0";
    int highest_cps_real = 0;

    for (Frame& frame : inputs) {
        if (!frame.pressingDown) {
            continue;
        }

        std::vector<Frame> inputFramesWithinASecond;
        int firstClickFrame = frame.number;
        int frameOneSecondLater = firstClickFrame + get_fps();

        int second_cps = 0;
        for (Frame& checkFrame : inputs) {
            if (checkFrame.number >= firstClickFrame && checkFrame.number <= frameOneSecondLater && checkFrame.pressingDown) {
                second_cps++;
                inputFramesWithinASecond.push_back(checkFrame);
            }
        }
        if (second_cps > highest_cps_real) {
            highest_cps_real = second_cps;
            highest_cps = std::to_string(highest_cps_real) + " (" + std::to_string(firstClickFrame) + " to " + std::to_string(frameOneSecondLater) + ")";
        }

        if (inputFramesWithinASecond.size() < 4) {
            continue;
        }

        for (size_t j = 0; j < inputFramesWithinASecond.size(); ++j) {
            int numClicks = j;
            int framesBetweenClicks = inputFramesWithinASecond[j].number - firstClickFrame;
            float timeBetweenClicks = static_cast<float>(framesBetweenClicks) / get_fps();
            float cps = numClicks / timeBetweenClicks;

            float current_percent = 0.f;

            if (inputFramesWithinASecond[j].xPosition != 0) {
                current_percent = std::round(((inputFramesWithinASecond[j].xPosition / end_portal_position) * 100.f) * 100) / 100;

                if (cps != 0 && numClicks + 1 > 3) {

                    // Rule 2: CPS must not exceed 18 clicks per second rate in any 1/3rd of a second to 1 second.
                    if (cps > 20 && timeBetweenClicks >= 1.0f / 3.0f) {
                        // cps_percents.push_back({ current_percent, "Rule 2 violation: " + std::to_string(cps) + " cps rate for the " + std::to_string(numClicks + 1) + " click stint from frame " + std::to_string(firstClickFrame) + " to " + std::to_string(inputFramesWithinASecond[j].number) + " (" + std::to_string(timeBetweenClicks) + "s)" });
                        cps_percents.push_back({ current_percent, "Rule 2" });
                    }
                }
            }
            else {
                if (cps != 0 && numClicks + 1 > 3) {

                    // Rule 2: CPS must not exceed 18 clicks per second rate in any 1/3rd of a second to 1 second.
                    if (cps > 20 && timeBetweenClicks >= 1.0f / 3.0f) {
                        // cps_percents.push_back({ current_percent, "Rule 2 violation: " + std::to_string(cps) + " cps rate for the " + std::to_string(numClicks + 1) + " click stint from frame " + std::to_string(firstClickFrame) + " to " + std::to_string(inputFramesWithinASecond[j].number) + " (" + std::to_string(timeBetweenClicks) + "s)" });
                        cps_percents.push_back({ inputFramesWithinASecond[j].number, "Rule 2" });
                    }
                }
            }
        }
    }

    // Remove cps_percents within 1 of each other.
    std::vector<std::pair<float, std::string>> unique_cps_percents;
    for (auto& percent : cps_percents) {
        bool should_push = true;
        for (auto& unique_percent : unique_cps_percents) {
            if (std::abs(percent.first - unique_percent.first) <= 0.4) {
                should_push = false;
            }
        }
        if (should_push) {
            unique_cps_percents.push_back(percent);
        }
    }

    cps_over_percents = unique_cps_percents;

    // The maximum CPS as a string
    return highest_cps;

}

int Logic::find_closest_input() {
    if (inputs.empty()) {
        return 0;
    }

    unsigned current_frame = get_frame();
    auto closest_it = std::min_element(inputs.begin(), inputs.end(),
        [current_frame](const Frame& a, const Frame& b) {
            return std::abs(static_cast<int>(a.number - current_frame)) < std::abs(static_cast<int>(b.number - current_frame));
        });
    return std::distance(inputs.begin(), closest_it);
}

namespace fs = std::filesystem;

std::vector<std::string> getWavFilesInDirectory(const std::string& directoryPath) {
    std::vector<std::string> wavFiles;

    for (const auto& entry : fs::directory_iterator(directoryPath)) {
        if (entry.is_regular_file() && entry.path().extension() == ".wav") {
            wavFiles.push_back(entry.path().string());
        }
    }

    return wavFiles;
}

std::string getRandomWavFile(const std::vector<std::string>& wavFiles) {
    std::random_device rd;
    std::mt19937 gen(rd()); // this shit dawg

    std::uniform_int_distribution<> dis(0, wavFiles.size() - 1);
    int randomIndex = dis(gen);

    return wavFiles[randomIndex];
}

bool Logic::play_macro(int& offset) {
    if (is_playing()) {
        auto& inputs = get_inputs();
        unsigned current_frame = get_frame()/* - offset*/;
        bool ret = false;

        while (inputs[replay_pos].number <= current_frame && replay_pos < inputs.size()) {
            auto& input = inputs[replay_pos];

            if (clickbot_enabled) {

                std::vector<std::string> reg_clicks = getWavFilesInDirectory(".echo\\clickbot\\clicks");
                std::vector<std::string> reg_releases = getWavFilesInDirectory(".echo\\clickbot\\releases");

                auto oldSFX = gd::FMODAudioEngine::sharedEngine()->m_fEffectsVolume;
                gd::FMODAudioEngine::sharedEngine()->m_fEffectsVolume = clickbot_volume;
                if (input.pressingDown) {
                    gd::GameSoundManager soundmanager;
                    soundmanager.playSound(getRandomWavFile(reg_clicks));
                }
                else {
                    gd::GameSoundManager soundmanager;
                    soundmanager.playSound(getRandomWavFile(reg_releases));
                }
                gd::FMODAudioEngine::sharedEngine()->m_fEffectsVolume = oldSFX;
            }

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

void Logic::offset_frames(int offset) {
    for (auto& frame : inputs) {
        // Ensure that the offset does not result in a negative frame number
        if (static_cast<int>(frame.number) + offset >= 0) {
            frame.number += offset;
        }
    }
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

    std::string file_format = format == SIMPLE ? "SIMPLE" : "DEBUG";

    w_b(file_format);

    w_b(fps);
    w_b(end_portal_position);

    for (auto& input : inputs) {
        w_b(input.number);
        w_b(input.pressingDown);
        w_b(input.isPlayer2);

        if (format == DEBUG) {
            w_b(input.xPosition);
            w_b(input.yVelocity);
            w_b(input.xVelocity);
            w_b(input.yPosition);
            w_b(input.rotation);
        }
    }

    file.close();
}

void Logic::read_file(const std::string& filename, bool is_path = false) {
    std::string dir = ".echo\\";
    std::string ext = ".bin";
    
    std::string full_filename = is_path ? filename : dir + filename + ext;

    std::ifstream temp_file(full_filename, std::ios::binary);
    if (!temp_file.is_open()) {
        error = "Error reading file '" + filename + "'!";
        return;
    }

    std::string file_format = "SIMPLE";

    temp_file.read(reinterpret_cast<char*>(&file_format), file_format.size());
    temp_file.close();

    std::ifstream file(full_filename, std::ios::binary);
    if (!file.is_open()) {
        error = "Error reading file '" + filename + "'!";
        return;
    }
    error = "";

    if (is_recording()) toggle_recording();
    if (!is_playing()) toggle_playing();

    if (!is_path)
        inputs.clear();


    if (file_format == "DEBUG") {
        r_b(file_format);
        format = DEBUG;
    }
    else if (file_format == "SIMPLE") {
        r_b(file_format);
        format = SIMPLE;
    }

    r_b(fps);
    r_b(end_portal_position);

    while (true) {
        Frame input;
        r_b(input.number);
        r_b(input.pressingDown);
        r_b(input.isPlayer2);

        if (format == DEBUG) {
            r_b(input.xPosition);
            r_b(input.yVelocity);
            r_b(input.xVelocity);
            r_b(input.yPosition);
            r_b(input.rotation);
        }

        if (file.eof()) {
            break;
        }

        inputs.push_back(input);
    }

    file.close();
}

void Logic::remove_inputs(unsigned frame, bool player_1) {
    auto it = std::remove_if(inputs.begin(), inputs.end(),
        [frame, player_1](const Frame& input) {
            if (!input.isPlayer2 == player_1)
                return input.number >= frame;
            else
                return false;
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

    state["version"] = format == SIMPLE ? "SIMPLE" : "DEBUG";

    state["fps"] = fps;
    state["end_xpos"] = end_portal_position;

    // Create a JSON array to store the input data
    json json_inputs = json::array();
    for (auto& input : inputs) {
        // Each input is a JSON object
        json json_input;
        json_input["frame"] = input.number;
        json_input["holding"] = input.pressingDown;

        if (input.isPlayer2)
            json_input["player_2"] = input.isPlayer2;

        if (format == DEBUG) {
            json_input["x_position"] = input.xPosition;
            json_input["y_vel"] = input.yVelocity;
            json_input["x_vel"] = input.xVelocity;
            json_input["rotation"] = input.rotation;
        }
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

    if (state.contains("version") && !state["version"].is_null()) {
        std::string version = state["version"];
        if (version == "ECHO_V1") format = SIMPLE;
        if (version == "DEBUG") format = DEBUG;
    }
    else {
        format = SIMPLE;
    }

    if (is_recording()) toggle_recording();
    if (!is_playing()) toggle_playing();

    // Extract the state data from the JSON object
    fps = state["fps"].get<double>();
    
    end_portal_position = state["end_xpos"].get<double>();

    inputs.clear();
    // Extract the input data from the JSON object
    for (auto& json_input : state["inputs"]) {
        Frame input;
        input.number = json_input["frame"].get<unsigned>();
        input.pressingDown = json_input["holding"].get<bool>();

        if (json_input.contains("player_2"))
            input.isPlayer2 = json_input["player_2"].get<bool>();

        if (format == FORMATS::DEBUG) {
            input.xPosition = json_input["x_position"].get<float>();
            input.yVelocity = json_input["y_vel"].get<double>();
            input.xVelocity = json_input["x_vel"].get<double>();
            input.rotation = json_input["rotation"].get<float>();
        }
        inputs.push_back(input);
    }

    file.close();
}

void Logic::sort_inputs() {
    std::map<unsigned, std::vector<Frame>> frameMap;

    for (const auto& frame : inputs) {
        frameMap[frame.number].push_back(frame);
    }

    inputs.clear();

    std::map<bool, std::deque<Frame>> pressQueues;
    std::map<bool, std::deque<Frame>> releaseQueues;

    for (const auto& [frameNumber, frames] : frameMap) {
        std::vector<Frame> mergedFrames;

        for (bool isPlayer2 : { false, true }) {
            std::deque<Frame>& pressQueue = pressQueues[isPlayer2];
            std::deque<Frame>& releaseQueue = releaseQueues[isPlayer2];

            for (const auto& frame : frames) {
                if (frame.isPlayer2 != isPlayer2) continue;

                if (frame.pressingDown) {
                    pressQueue.push_back(frame);
                }
                else {
                    if (!pressQueue.empty()) {
                        mergedFrames.push_back(pressQueue.front());
                        pressQueue.pop_front();
                        mergedFrames.push_back(frame);
                    }
                    else {
                        releaseQueue.push_back(frame);
                    }
                }
            }

            while (!pressQueue.empty() && !releaseQueue.empty()) {
                mergedFrames.push_back(pressQueue.front());
                pressQueue.pop_front();
                mergedFrames.push_back(releaseQueue.front());
                releaseQueue.pop_front();
            }

            inputs.insert(inputs.end(), mergedFrames.begin(), mergedFrames.end());
            mergedFrames.clear();
        }
    }

    // If there are any unpaired 'click' frames, append them to the inputs
    for (bool isPlayer2 : { false, true }) {
        std::deque<Frame>& pressQueue = pressQueues[isPlayer2];
        inputs.insert(inputs.end(), pressQueue.begin(), pressQueue.end());
        pressQueue.clear();
    }

    // Sort the inputs by frames, skipping inputs with the same frame number
    std::stable_sort(inputs.begin(), inputs.end(), [](const Frame& a, const Frame& b) {
        if (a.number == b.number) {
            return false;  // Skip inputs with the same frame number
        }
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