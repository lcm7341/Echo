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
        std::cout << "MISMATCH Y VELOCITY " << PLAYLAYER->m_player1->m_yAccel << ":" << input.yVelocity << std::endl;
    }

    if (PLAYLAYER->m_player1->m_xAccel != input.xVelocity) {
        std::cout << "MISMATCH X VELOCITY " << PLAYLAYER->m_player1->m_xAccel << ":" << input.xVelocity << std::endl;
    }

    if (PLAYLAYER) {
        if (input.pressingDown) {
            if (click_both_players) {
                Hooks::PlayLayer::pushButton(PLAYLAYER, 0, !PLAYLAYER->m_player1 ^ gamevar);
                Hooks::PlayLayer::pushButton(PLAYLAYER, 0, !PLAYLAYER->m_player2 ^ gamevar);
            } else
            if (input.isPlayer2) {
                Hooks::PlayLayer::pushButton(PLAYLAYER, 0, !PLAYLAYER->m_player2 ^ gamevar);
            }
            else {
                Hooks::PlayLayer::pushButton(PLAYLAYER, 0, !PLAYLAYER->m_player1 ^ gamevar);
            }
        } else {
            if (click_both_players) {
                Hooks::PlayLayer::releaseButton(PLAYLAYER, 0, !PLAYLAYER->m_player1 ^ gamevar);
                Hooks::PlayLayer::releaseButton(PLAYLAYER, 0, !PLAYLAYER->m_player2 ^ gamevar);
            }
            else
                if (input.isPlayer2) {
                    Hooks::PlayLayer::releaseButton(PLAYLAYER, 0, !PLAYLAYER->m_player2 ^ gamevar);
                }
                else {
                    Hooks::PlayLayer::releaseButton(PLAYLAYER, 0, !PLAYLAYER->m_player1 ^ gamevar);
                }
        }
    }
}

unsigned Logic::count_presses_in_last_second() {
    unsigned frame_limit = round((((PLAYLAYER->m_time) * get_fps()) - get_removed()) - get_fps());
    if (frame_limit > ((((PLAYLAYER->m_time) * get_fps()) - get_removed()) + 2))
        frame_limit = 0;

    unsigned press_count = 0;
    for (auto it = inputs.rbegin(); it != inputs.rend(); ++it) {
        if (it->number > frame_limit && it->pressingDown) {
            press_count++;
        }
    }

    if (press_count > max_cps)
        return max_cps;

    return press_count;
}

// A cpu trojan
std::string Logic::highest_cps() {
    int highest_cps = 0;
    std::string to_return = "0";

    for (Frame frames : inputs) {
        int frame_limit = frames.number - get_fps() > 0 ? frames.number - get_fps() : 0;

        unsigned press_count = 0;
        for (Frame frame : inputs) {
            if (frame.number >= frame_limit && frame.number <= frames.number && frame.pressingDown) {
                press_count++;
            }
        }

        if (press_count > highest_cps) {
            highest_cps = press_count;
            to_return = std::to_string(highest_cps) + " (" + std::to_string(frame_limit) + " to " + std::to_string(frames.number) + ")";
        }
    }

    return to_return;
}

void Logic::write_osu_file(const std::string& filename) {
    std::string dir = ".echo\\osu\\" + filename + "\\";
    std::filesystem::create_directory(dir);
    std::string ext = ".osu";
    std::string full_filename = dir + filename + ext;

    std::ofstream osu_file(full_filename);
    if (!osu_file.is_open()) {
        error = "Error writing file '" + filename + "'!";
        return;
    }
    error = "";

    // osu file header
    osu_file << "osu file format v14\n\n";

    // General section
    osu_file << "[General]\n";
    osu_file << "StackLeniency: 0.7\n";
    osu_file << "AudioLeadIn: 0\n";
    osu_file << "Mode: 1\n"; // Mode 1 is for Taiko
    osu_file << "Countdown: 0\n"; // No countdown before the song starts
    osu_file << "\n";

    // Metadata section
    osu_file << "[Metadata]\n";
    osu_file << "Title:" + filename + "\n";
    osu_file << "Artist:EchoBot\n";
    osu_file << "Creator:EchoBot\n";
    osu_file << "Version:1.0\n";
    osu_file << "\n";

    // Difficulty section
    osu_file << "[Difficulty]\n";
    osu_file << "HPDrainRate:5\n";
    osu_file << "CircleSize:5\n";
    osu_file << "OverallDifficulty:5\n";
    osu_file << "\n";

    // TimingPoints section
    osu_file << "[TimingPoints]\n";
    osu_file << "0,500,4,1,0,100,1,0\n";
    osu_file << "\n";

    // HitObjects section
    osu_file << "[HitObjects]\n";

    for (auto& input : inputs) {

        if (input.pressingDown) {
            // Calculate the time in milliseconds
            int time = round(input.number / get_fps() * 1000 * (2 - speedhack));

            // For Taiko mode, there's no positional data needed.
            // The format for a single hit object is: time,x,y,type,hitSound,objectParams,hitSample
            // type: 1 for a regular hit, 2 for a drumroll (slider in standard osu), 8 for a spinner.
            // hitSound: 0 for a normal hit sound, 2 for whistle, 4 for finish, 8 for clap.
            osu_file << "256,192," << time << ",1,0,0:0:0:0:\n";
        }
    }

    osu_file.close();
}

    int Logic::find_closest_input() {
        if (inputs.empty()) {
            // Return -1 or throw an exception if there are no inputs.
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

    for (auto& input : inputs) {
        w_b(input.number);
        w_b(input.pressingDown);

        w_b(input.yPosition);
        w_b(input.xPosition);
        w_b(input.rotation);
        w_b(input.yVelocity);
        w_b(input.xVelocity);
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

        r_b(input.yPosition);
        r_b(input.xPosition);
        r_b(input.rotation);
        r_b(input.yVelocity);
        r_b(input.xVelocity);

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