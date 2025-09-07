﻿#include "logic.hpp"
#include "../Hooks/hooks.hpp"
#include <stdio.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <algorithm>  // for std::sort and min_element
#include <cmath>      // for std::abs
#include <random>
#include <cctype>
#include <regex>
#include <ShObjIdl.h>

using json = nlohmann::json;

#define PLAYLAYER gd::GameManager::sharedState()->getPlayLayer()

int Logic::get_frame() {
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
        //auto twoplayer = PLAYLAYER->m_pLevelSettings->m_twoPlayerMode;
        player1 ^= 1 && gd::GameManager::sharedState()->getGameVariable("0010"); // what the fuck ?
        if (player1) {
            add_input({ get_frame(), down, !player1, PLAYLAYER->m_pPlayer1->getPositionY(), PLAYLAYER->m_pPlayer1->getPositionX(), PLAYLAYER->m_pPlayer1->getRotation(), PLAYLAYER->m_pPlayer1->m_yAccel, PLAYLAYER->m_pPlayer1->m_xAccel });
        }
        else {
            add_input({ get_frame(), down, !player1, PLAYLAYER->m_pPlayer2->getPositionY(), PLAYLAYER->m_pPlayer2->getPositionX(), PLAYLAYER->m_pPlayer2->getRotation(), PLAYLAYER->m_pPlayer2->m_yAccel, PLAYLAYER->m_pPlayer2->m_xAccel });
        }
    }
}

void Logic::play_input(Frame& input) {
    auto gamevar = gd::GameManager::sharedState()->getGameVariable("0010"); // game var again 
    auto editor = gd::GameManager::sharedState()->getEditorLayer();

    // PLAYLAYER->m_player1->setPositionY(input.yPosition);
    // PLAYLAYER->m_player1->setPositionX(input.xPosition);


    if (PLAYLAYER->m_pPlayer1->m_yAccel != input.yVelocity) {
        printf("MISMATCH Y VELOCITY %f:%f\n", PLAYLAYER->m_pPlayer1->m_yAccel, input.yVelocity);
    }

    if (PLAYLAYER->m_pPlayer1->m_xAccel != input.xVelocity) {
        printf("MISMATCH X VELOCITY %f:%f\n", PLAYLAYER->m_pPlayer1->m_xAccel, input.xVelocity);
    }

    if (!input.isPlayer2 ^ gamevar && !play_player_1)
        return;
    if (!(!input.isPlayer2 ^ gamevar) && !play_player_2)
        return;

    if (PLAYLAYER) {
        live_inputs.push_back(input);

        if (input.pressingDown) {

            currently_pressing = true;
            playback_clicking = true;
            try {
                //Hooks::PlayLayer::pushButton(PLAYLAYER, 0, !input.isPlayer2 ^ gamevar);
                Hooks::PlayLayer::pushButton_h(PLAYLAYER, 0, 0, !input.isPlayer2 ^ gamevar);
            }
            catch (std::exception& ex) {
                printf("Clickbot error: %s\n", ex.what());

                std::ofstream logfile("error.log");
                if (logfile.is_open())
                {
                    logfile << "Clickbot error: " << ex.what() << std::endl;
                    logfile.close();
                }
            }
            playback_clicking = false;
        }
        else {
            playback_releasing = true;
            //Hooks::PlayLayer::releaseButton(PLAYLAYER, 0, !input.isPlayer2 ^ gamevar);
            Hooks::PlayLayer::releaseButton_h(PLAYLAYER, 0, 0, !input.isPlayer2 ^ gamevar);
            playback_releasing = false;
        }
    }
}

static constexpr int   kMaxPerSecond = 16;  // Rule 1
static constexpr int   kMaxPerFrame = 3;   // Rule 2
static constexpr float kMaxBurstRate = 48.f;// Rule 3
static constexpr int   kMinBurstClicks = 5;   // "stint of 5 or more clicks"
// -----------------------------
// 1) Count inputs in the last second (per player)
// -----------------------------
// NOTE: “regardless of type” ⇒ count every input event for that player.
// We do NOT filter on pressingDown anymore.
unsigned Logic::count_presses_in_last_second(bool player2) {
    const int fps = get_fps();
    const int nowFrame = get_frame();
    const int frame_limit = max(0, nowFrame - fps); // exactly last 1 second window [now-fps, now]

    // Gather relevant inputs (recorded vs live) for this player only.
    std::vector<Frame> inputs_for_player;
    if (is_recording()) {
        for (const auto& f : inputs)
            if (f.isPlayer2 == player2) inputs_for_player.push_back(f);
    }
    else {
        for (const auto& f : live_inputs)
            if (f.isPlayer2 == player2) inputs_for_player.push_back(f);
    }

    // Count inputs in the last second window (inclusive of 'now').
    unsigned count = 0;
    for (auto it = inputs_for_player.rbegin(); it != inputs_for_player.rend(); ++it) {
        if (it->number < frame_limit) break; // past the window
        if (it->number <= nowFrame) ++count;
    }

    // Track state for UI/flags.
    current_cps = count;
    over_max_cps = (count > static_cast<unsigned>(kMaxPerSecond));

    return count;
}

// -----------------------------
// 2) Highest CPS (per player) + collect violations for all rules
// -----------------------------
// Returns a string like "P1: 14 (1200 to 1260)" OR "P2: 15 (…)"
// Also populates:
//   - cps_over_percents      : vector<{frame, "Rule … (P1)"}>
//   - cps_over_percents_p2   : vector<{frame, "Rule … (P2)"}>
// Where 'frame' is the frame number of the end of the offending window.
std::string Logic::highest_cps() {
    std::string best_str_p1 = "P1: 0";
    std::string best_str_p2 = "P2: 0";
    int best_cps_p1 = 0, best_cps_p2 = 0;

    cps_over_percents.clear();
    cps_over_percents_p2.clear();

    // Split recorded inputs by player (rule 2: handled independently).
    std::vector<Frame> p1, p2;
    p1.reserve(inputs.size());
    p2.reserve(inputs.size());
    for (const auto& f : inputs) {
        if (f.pressingDown)
        (f.isPlayer2 ? p2 : p1).push_back(f);
    }

    const float fpsf = static_cast<float>(get_fps());
    const int   fps = get_fps();

    auto eval_player = [&](const std::vector<Frame>& vec, bool isP2) {
        if (vec.empty()) return;

        // ---------- Rule 2: no more than 3 in the same frame ----------
        {
            int i = 0;
            while (i < static_cast<int>(vec.size())) {
                const int frameNum = vec[i].number;
                int j = i;
                while (j < static_cast<int>(vec.size()) && vec[j].number == frameNum && vec[j].pressingDown) ++j;
                const int countThisFrame = j - i;
                if (countThisFrame > kMaxPerFrame) {
                    (isP2 ? cps_over_percents_p2 : cps_over_percents)
                        .push_back({ static_cast<float>(frameNum), isP2 ? "Rule 2 (P2)" : "Rule 2 (P1)" });
                }
                i = j;
            }
        }

        // ---------- Highest CPS within any 1-second window + Rule 1 ----------
        {
            int left = 0;
            for (int right = 0; right < static_cast<int>(vec.size()); ++right) {
                const int startFrame = vec[right].number - fps; // window: (right - 1s, right]
                while (left <= right && vec[left].number < startFrame) ++left;
                const int windowCount = right - left + 1;

                // Track per-player best CPS (as "count in 1s window")
                if (!isP2 && windowCount > best_cps_p1) {
                    best_cps_p1 = windowCount;
                    best_str_p1 = "P1: " + std::to_string(windowCount) + " (" +
                        std::to_string(vec[left].number) + " to " +
                        std::to_string(vec[right].number) + ")";
                }
                else if (isP2 && windowCount > best_cps_p2) {
                    best_cps_p2 = windowCount;
                    best_str_p2 = "P2: " + std::to_string(windowCount) + " (" +
                        std::to_string(vec[left].number) + " to " +
                        std::to_string(vec[right].number) + ")";
                }

                // Rule 1 violation: >16 inputs within any 1-second window
                if (windowCount > kMaxPerSecond) {
                    (isP2 ? cps_over_percents_p2 : cps_over_percents)
                        .push_back({ static_cast<float>(vec[right].number),
                                     isP2 ? "Rule 1 (P2)" : "Rule 1 (P1)" });
                }
            }
        }

        // ---------- Rule 3: >48/sec over any stint of 5+ clicks ----------
        // Sliding “start” index; expand “end” and check any window with >=5 clicks.
        {
            int n = static_cast<int>(vec.size());
            for (int i = 0; i < n; ++i) {
                // We only need to check windows that end at or after i+4 (5 clicks).
                for (int j = i + (kMinBurstClicks - 1); j < n; ++j) {
                    const int frames = vec[j].number - vec[i].number;
                    if (frames <= 0) {
                        // All 5+ in same/earlier frame -> already a 2 violation,
                        // but it also implies infinite rate; count as 3 too to be strict.
                        (isP2 ? cps_over_percents_p2 : cps_over_percents)
                            .push_back({ static_cast<float>(vec[j].number),
                                         isP2 ? "Rule 3 (P2)" : "Rule 3 (P1)" });
                        continue;
                    }
                    const float seconds = static_cast<float>(frames) / fpsf;
                    const int clicks = j - i + 1;
                    const float rate = static_cast<float>(clicks) / seconds; // clicks per second
                    if (rate > kMaxBurstRate) {
                        (isP2 ? cps_over_percents_p2 : cps_over_percents)
                            .push_back({ static_cast<float>(vec[j].number),
                                         isP2 ? "Rule 3 (P2)" : "Rule 3 (P1)" });
                    }
                    else {
                        // Because seconds increases and rate drops as j grows,
                        // once it's under the limit for this i, extending j will stay under.
                        break;
                    }
                }
            }
        }
    };

    eval_player(p1, /*isP2=*/false);
    eval_player(p2, /*isP2=*/true);

    // De-duplicate nearby violations (within ~0.4 frames) like your original code
    auto dedupe = [](std::vector<std::pair<float, std::string>>& v) {
        std::vector<std::pair<float, std::string>> out;
        for (auto& e : v) {
            bool keep = true;
            for (auto& u : out) {
                if (std::fabs(e.first - u.first) <= 0.4f && e.second == u.second) {
                    keep = false; break;
                }
            }
            if (keep) out.push_back(e);
        }
        v.swap(out);
    };
    dedupe(cps_over_percents);
    dedupe(cps_over_percents_p2);

    // Return the single highest across both players, formatted like before.
    // (They’re enforced independently; this string is just a display convenience.)
    if (best_cps_p2 > best_cps_p1) return best_str_p2;
    return best_str_p1;
}

int Logic::find_closest_input() {
    if (inputs.empty()) {
        return 0;
    }

    unsigned current_frame = get_frame();
    auto closest_it = inputs.end();
    for (auto it = inputs.begin(); it != inputs.end(); ++it) {
        if (it->number > current_frame) {
            closest_it = it;
            break;
        }
    }

    if (closest_it == inputs.end()) {
        // No input frame found above the current frame
        // Select the last input frame
        closest_it = std::prev(inputs.end());
    }
    else if (closest_it != inputs.begin()) {
        // If the closest input is not the first frame,
        // move the iterator back to select the frame right before it
        --closest_it;
    }

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

bool Logic::play_macro() {
    if (is_playing()) {
        auto& inputs = get_inputs();
        unsigned current_frame = get_frame()/* - offset*/;
        bool ret = false;

        while (inputs[replay_pos].number <= current_frame && replay_pos < inputs.size()) {
            auto& input = inputs[replay_pos];

            auto player = input.isPlayer2 ? PLAYLAYER->m_pPlayer2 : PLAYLAYER->m_pPlayer1;
            auto gamemode = CheckpointData::GetGamemode(player);
            bool passes = gamemode != gd::kGamemodeBall && gamemode != gd::kGamemodeSpider && gamemode != gd::kGamemodeUfo && gamemode != gd::kGamemodeCube;

            if (input.number == current_frame) {
                if (input.number == current_frame) {
                    play_input(input);
                }
            }
            ++replay_pos;
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

int extractLastNumber(const std::string& str) {
    std::regex regex("\\d+");
    std::sregex_iterator iter(str.begin(), str.end(), regex);
    std::sregex_iterator end;

    int lastNumber = 0;
    for (; iter != end; ++iter) {
        lastNumber = std::stoi(iter->str());
    }

    return lastNumber;
}

std::string generateNewString(const std::string& input, int newNumber) {
    std::string newString = input;
    int lastNumber = extractLastNumber(newString);

    if (lastNumber > 0) {
        size_t pos = newString.find_last_of(std::to_string(lastNumber));
        newString.replace(pos, std::to_string(lastNumber).length(), std::to_string(newNumber));
    }

    size_t hashPos = newString.find('#');
    if (hashPos != std::string::npos) {
        newString.replace(hashPos, 1, std::to_string(newNumber));
    }

    return newString;
}

std::pair<std::string, std::string> generateNewFileName(const std::string& fileName, std::string ext, bool json = false) {
    auto& logic = Logic::get();
    std::string rename_format = logic.rename_format; // _#

    std::string dir = json ? ".echo\\converted\\" : ".echo\\replays\\";
    std::string baseName = fs::path(fileName).stem().string();
    if (json) baseName = fs::path(fileName).stem().stem().string();
    std::string extension = fs::path(fileName).extension().string();

    if (rename_format.empty()) {
        std::string place = dir;
        place += baseName;
        place += ext;
        return { place, fs::path(place).stem().string() };;
    }

    if (rename_format.find("#") == std::string::npos) {
        rename_format += "#";
    }

    std::string newFileName = dir + baseName + ext;
    int count = 1;

    while (fs::exists(newFileName)) {
        int lastNumber = extractLastNumber(newFileName);
        if (lastNumber >= 1) {
            int newNumber = lastNumber + 1;
            newFileName = generateNewString(newFileName, newNumber);
        }
        else {
            newFileName = dir + baseName + rename_format + ext;
            newFileName = generateNewString(newFileName, 1);
        }
    }

    if (json)
        return { newFileName, fs::path(newFileName).stem().stem().string() };

    return { newFileName, fs::path(newFileName).stem().string() };
}

void Logic::write_file(const std::string& filename) {
    std::string ext = ".echo";
    std::string base = filename;
    auto newFileName = generateNewFileName(base, ext);

    std::string full_filename = newFileName.first;
    std::string newBase = newFileName.second;
    strcpy(macro_name, newBase.c_str());

    std::ofstream file(full_filename, std::ios::binary);
    if (!file.is_open()) {
        error = "Error writing file '" + filename + "'!";
        return;
    }
    error = "";

    std::string file_format = "";
    if (format == SIMPLE)
        file_format = "SIMPLE";
    else if (format == DEBUG)
        file_format = "DEBUG";
    else if (format == META)
        file_format = "META";
    else if (format == META_DBG)
        file_format = "METADBG";

    w_b(file_format);

    w_b(fps);
    w_b(end_portal_position);

    double total_seconds = total_recording_time.count();
    w_b(total_seconds);

    int clicks_amt = 0;
    for (auto& input : inputs)  if (input.pressingDown) clicks_amt++;
    w_b(clicks_amt);
    w_b(total_attempt_count);

    for (auto& input : inputs) {
        w_b(input.number);
        w_b(input.pressingDown);
        w_b(input.isPlayer2);

        if (format == DEBUG || format == META_DBG) {
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
    std::string dir = ".echo\\replays\\";
    std::string ext = ".echo";

    std::string full_filename = is_path ? filename : dir + filename + ext;

    std::ifstream temp_file(full_filename, std::ios::binary);
    if (!temp_file.is_open()) {
        error = "Error reading file '" + filename + "'!";
        return;
    }

    std::string file_format;
    char ch;
    while (temp_file.get(ch)) {
        if (!std::isalpha(ch)) {
            break;
        }
        file_format += ch;
    }

    std::ifstream file(full_filename, std::ios::binary);
    if (!file.is_open()) {
        error = "Error reading file '" + filename + "'!";
        return;
    }
    error = "";

    if (is_recording()) toggle_recording();
    if (!is_playing()) toggle_playing();

    inputs.clear();


    if (file_format == "DEBUG") {
        r_b(file_format);
        format = DEBUG;
    }
    else if (file_format == "SIMPLE") {
        r_b(file_format);
        format = SIMPLE;
    }
    else if (file_format == "META") {
        r_b(file_format);
        format = META;
    }
    else if (file_format == "METADBG") {
        r_b(file_format);
        format = META_DBG;
    }

    r_b(fps);
    r_b(end_portal_position);

    if (format == META || format == META_DBG) {
        double total_seconds = 0.f;
        r_b(total_seconds);

        // Assign the read value to total_time
        total_recording_time = std::chrono::duration<double>(total_seconds);

        int total_clicks = 0;
        r_b(total_clicks);
        r_b(total_attempt_count);
    }

    while (true) {
        Frame input;
        r_b(input.number);
        r_b(input.pressingDown);
        r_b(input.isPlayer2);

        if (format == DEBUG || format == META_DBG) {
            r_b(input.xPosition);
            r_b(input.yVelocity);
            r_b(input.xVelocity);
            r_b(input.yPosition);
            r_b(input.rotation);
        }
        else {
            input.xPosition = 0;
            input.yVelocity = 0;
            input.xVelocity = 0;
            input.yPosition = 0;
            input.rotation = 0;
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
    std::string ext = ".echo.json";
    std::string base = filename;

    auto newFileName = generateNewFileName(base, ext, true);

    std::string full_filename = newFileName.first;
    std::string newBase = newFileName.second;
    strcpy(macro_name, newBase.c_str());

    std::ofstream file(full_filename);
    if (!file.is_open()) {
        error = "Error writing file '" + filename + "'!";
        return;
    }
    error = "";

    // Create a JSON object to store the state data
    json state;

    if (format == SIMPLE)
        state["version"] = "SIMPLE";
    else if (format == DEBUG)
        state["version"] = "DEBUG";
    else if (format == META)
        state["version"] = "META";
    else if (format == META_DBG)
        state["version"] = "METADBG";

    state["fps"] = fps;
    state["end_xpos"] = end_portal_position;

    double total_seconds = total_recording_time.count();

    int hours = static_cast<int>(total_seconds / 3600);
    int minutes = static_cast<int>((total_seconds - hours * 3600) / 60);
    int seconds = static_cast<int>(total_seconds - hours * 3600 - minutes * 60);

    // Format the time as HH:MM:SS
    std::ostringstream oss;
    oss << std::setfill('0');
    oss << std::setw(2) << hours << ":"
        << std::setw(2) << minutes << ":"
        << std::setw(2) << seconds;

    std::string time_str = oss.str();

    state["metadata"]["recording_time_formatted"] = time_str;
    state["metadata"]["recording_time"] = total_seconds;
    state["metadata"]["total_attempts"] = total_attempt_count;
    int clicks_amt = 0;
    for (auto& input : inputs)  if (input.pressingDown) clicks_amt++;
    state["metadata"]["clicks_count"] = clicks_amt;


    // Create a JSON array to store the input data
    json json_inputs = json::array();
    for (auto& input : inputs) {
        // Each input is a JSON object
        json json_input;
        json_input["frame"] = input.number;
        json_input["holding"] = input.pressingDown;

        if (input.isPlayer2)
            json_input["player_2"] = input.isPlayer2;

        if (format == DEBUG || format == META_DBG) {
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
    std::string dir = ".echo\\replays\\";
    std::string ext = ".echo.json";

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
        if (version == "SIMPLE") format = SIMPLE;
        if (version == "DEBUG") format = DEBUG;
        if (version == "META") format = META;
        if (version == "METADBG") format = META_DBG;
    }
    else {
        format = SIMPLE;
    }

    if (is_recording()) toggle_recording();
    if (!is_playing()) toggle_playing();

    // Extract the state data from the JSON object
    fps = state["fps"].get<double>();

    end_portal_position = state["end_xpos"].get<double>();

    if (format == META || format == META_DBG) {
        double total_seconds = 0.f;
        total_seconds = state["metadata"]["recording_time"];
        total_attempt_count = state["metadata"]["total_attempts"];

        total_recording_time = std::chrono::duration<double>(total_seconds);
    }

    inputs.clear();
    // Extract the input data from the JSON object
    for (auto& json_input : state["inputs"]) {
        Frame input;
        input.number = json_input["frame"].get<unsigned>();
        input.pressingDown = json_input["holding"].get<bool>();

        if (json_input.contains("player_2"))
            input.isPlayer2 = json_input["player_2"].get<bool>();
        else
            input.isPlayer2 = false;

        if (format == DEBUG || format == META_DBG) {
            input.xPosition = json_input["x_position"].get<float>();
            input.yVelocity = json_input["y_vel"].get<double>();
            input.xVelocity = json_input["x_vel"].get<double>();
            input.rotation = json_input["rotation"].get<float>();
        }
        inputs.push_back(input);
    }

    format = META;
    file.close();
}

void Logic::sort_inputs() {
    std::map<unsigned, std::vector<Frame>> frameMap;

    std::stable_sort(inputs.begin(), inputs.end(), [](const Frame& a, const Frame& b) {
        if (a.number == b.number) {
            return false;  // Skip inputs with the same frame number
        }
        return a.number < b.number;
        });
}

typedef std::function<void* (void*)> cast_function;

template <typename To, typename From>
cast_function make_cast() {
    std::unique_ptr<cast_function> caster(new cast_function([](void* from)->void* {
        return static_cast<void*>(static_cast<To*>(static_cast<From*>(from)));
        }));

    auto result = std::bind(*caster, std::placeholders::_1);
    return result;
}

void Logic::handle_checkpoint_data() {
    if (PLAYLAYER) {
        if (checkpoints.size() > 0) {
            Checkpoint& data = checkpoints.back();


            PLAYLAYER->stopAllActions();
            for (auto& action : data.actions) {
                //PLAYLAYER->runAction(action);
            }

            // PLAYLAYER->m_cameraPos = data.camera;

            if (is_playing()) {
                if (play_player_1) {
                    PLAYLAYER->m_pPlayer1->m_isHolding = data.player_1_data.isHolding;
                    PLAYLAYER->m_pPlayer1->m_isHolding2 = data.player_1_data.isHolding2;
                }
                if (play_player_2) {
                    PLAYLAYER->m_pPlayer2->m_isHolding = data.player_2_data.isHolding;
                    PLAYLAYER->m_pPlayer2->m_isHolding2 = data.player_2_data.isHolding2;
                }
            }

            if (click_inverse_p1) {
                PLAYLAYER->m_pPlayer1->m_isHolding = data.player_1_data.isHolding;
                PLAYLAYER->m_pPlayer1->m_isHolding2 = data.player_1_data.isHolding2;
            }
            if (click_inverse_p2) {
                PLAYLAYER->m_pPlayer2->m_isHolding = data.player_2_data.isHolding;
                PLAYLAYER->m_pPlayer2->m_isHolding2 = data.player_2_data.isHolding2;
            }


            data.player_1_data.apply(PLAYLAYER->m_pPlayer1);
            data.player_2_data.apply(PLAYLAYER->m_pPlayer2);
            return;

            if (PLAYLAYER->m_pObjectLayer) {
                auto layer = static_cast<CCLayer*>(PLAYLAYER->getChildren()->objectAtIndex(2));
                float xp = -layer->getPositionX() / layer->getScale();
                cast_function caster = make_cast<gd::GameObject, CCObject>();

                for (int s = PLAYLAYER->sectionForPos(xp) - 5; s < PLAYLAYER->sectionForPos(xp) + 6; ++s) {
                    if (s < 0)
                        continue;
                    if (s >= PLAYLAYER->m_sectionObjects->count())
                        break;
                    auto section = static_cast<CCArray*>(PLAYLAYER->m_sectionObjects->objectAtIndex(s));
                    for (size_t i = 0; i < section->count(); ++i)
                    {
                        auto obj = static_cast<gd::GameObject*>(section->objectAtIndex(i));
                        if (obj) {
                            if (obj->m_nObjectType == gd::kGameObjectTypeDecoration) continue;
                            for (const auto& pair : data.objects) {
                                const ObjectData& nodeData = pair.second;
                                obj->setPositionX(nodeData.posX);
                                obj->setPositionY(nodeData.posY);
                                //obj->setRotation(nodeData.rotation);
                                /*obj->setRotationX(nodeData.rotX);
                                obj->setRotationY(nodeData.rotY);*/
                                /* obj->setSkewX(nodeData.velX);
                                 obj->setSkewY(nodeData.velY);*/

                                 /*obj->m_unk2F4 = nodeData.speed1;
                                 obj->m_unk2F8 = nodeData.speed2;
                                 obj->m_unk33C = nodeData.speed3;
                                 obj->m_unk340 = nodeData.speed4;
                                 obj->m_unk390 = nodeData.speed5;*/

                                 // I HATE GEOMETRY DASH
                                obj->m_bUnk3 = nodeData.m_bUnk3;
                                obj->m_bIsBlueMaybe = nodeData.m_bIsBlueMaybe;
                                obj->m_fUnk2 = nodeData.m_fUnk2;
                                obj->m_fUnk = nodeData.m_fUnk;
                                obj->m_fUnk3 = nodeData.m_fUnk3;
                                obj->m_fUnk4 = nodeData.m_fUnk4;
                                obj->m_bUnk = nodeData.m_bUnk;
                                obj->m_fAnimSpeed2 = nodeData.m_fAnimSpeed2;
                                obj->m_bIsEffectObject = nodeData.m_bIsEffectObject;
                                obj->m_bRandomisedAnimStart = nodeData.m_bRandomisedAnimStart;
                                obj->m_fAnimSpeed = nodeData.m_fAnimSpeed;
                                //obj->m_bBlackChild = nodeData.m_bBlackChild;
                                //obj->m_bUnkOutlineMaybe = nodeData.m_bUnkOutlineMaybe;
                                //obj->m_fBlackChildOpacity = nodeData.m_fBlackChildOpacity;
                                /*obj->field_23 = nodeData.field_23;
                                obj->m_bEditor = nodeData.m_bEditor;
                                obj->m_bGroupDisabled = nodeData.m_bGroupDisabled;
                                obj->m_bColourOnTop = nodeData.m_bColourOnTop;
                                obj->m_pMainColourMode = nodeData.m_pMainColourMode;
                                obj->m_pSecondaryColourMode = nodeData.m_pSecondaryColourMode;
                                obj->m_bCol1 = nodeData.m_bCol1;
                                obj->m_bCol2 = nodeData.m_bCol2;
                                obj->m_obStartPosOffset = nodeData.m_obStartPosOffset;*/
                                //obj->m_fUnkRotationField = nodeData.m_fUnkRotationField;
                                obj->m_bTintTrigger = nodeData.m_bTintTrigger;
                                //obj->m_bIsFlippedX = nodeData.m_bIsFlippedX;
                                //obj->m_bIsFlippedY = nodeData.m_bIsFlippedY;
                                //obj->m_obBoxOffset = nodeData.m_obBoxOffset;
                                obj->m_bIsOriented = nodeData.m_bIsOriented;
                                //obj->m_obBoxOffset2 = nodeData.m_obBoxOffset2;
                                obj->m_pObjectOBB2D = nodeData.m_pObjectOBB2D;
                                obj->m_bOriented = nodeData.m_bOriented;
                                //obj->m_pGlowSprite = nodeData.m_pGlowSprite;
                                obj->m_bNotEditor = nodeData.m_bNotEditor;
                                obj->m_pMyAction = nodeData.m_pMyAction;
                                obj->setMyAction(nodeData.m_pMyAction);
                                obj->m_bRunActionWithTag = nodeData.m_bRunActionWithTag;
                                //obj->m_bUnk1 = nodeData.m_bUnk1;
                                /*obj->m_bRunActionWithTag = nodeData.m_bRunActionWithTag;
                                obj->m_bObjectPoweredOn = nodeData.m_bObjectPoweredOn;
                                obj->m_obObjectSize = nodeData.m_obObjectSize;
                                obj->m_bTrigger = nodeData.m_bTrigger;
                                obj->m_bAnimationFinished = nodeData.m_bAnimationFinished;*/

                                obj->m_pParticleSystem = nodeData.m_pParticleSystem;
                                obj->m_sEffectPlistName = nodeData.m_sEffectPlistName;
                                obj->m_bParticleAdded = nodeData.m_bParticleAdded;
                                obj->m_bHasParticles = nodeData.m_bHasParticles;
                                obj->m_bUnkCustomRing = nodeData.m_bUnkCustomRing;
                                obj->m_obPortalPosition = nodeData.m_obPortalPosition;
                                /*obj->m_bUnkParticleSystem = nodeData.m_bUnkParticleSystem;
                                obj->m_obObjectTextureRect = nodeData.m_obObjectTextureRect;
                                obj->m_bTextureRectDirty = nodeData.m_bTextureRectDirty;
                                obj->m_fRectXCenterMaybe = nodeData.m_fRectXCenterMaybe;
                                obj->m_obObjectRect2 = nodeData.m_obObjectRect2;
                                obj->m_bIsObjectRectDirty = nodeData.m_bIsObjectRectDirty;*/

                                /*obj->m_bIsOrientedRectDirty = nodeData.m_bIsOrientedRectDirty;
                                obj->m_objectRadius = nodeData.m_objectRadius;
                                obj->m_bIsRotatedSide = nodeData.m_bIsRotatedSide;

                                obj->m_bTouchTriggered = nodeData.m_bTouchTriggered;
                                obj->m_bSpawnTriggered = nodeData.m_bSpawnTriggered;
                                obj->m_sTextureName = nodeData.m_sTextureName;
                                obj->m_unk32C = nodeData.m_unk32C;*/

                                /*obj->m_unk32D = nodeData.m_unk32D;
                                obj->m_unk33C = nodeData.m_unk33C;
                                obj->m_unk340 = nodeData.m_unk340;
                                obj->m_bIsGlowDisabled = nodeData.m_bIsGlowDisabled;
                                obj->m_nTargetColorID = nodeData.m_nTargetColorID;
                                obj->m_fScale = nodeData.m_fScale;
                                obj->m_nObjectID = nodeData.m_nObjectID;
                                obj->m_unk368 = nodeData.m_unk368;
                                obj->m_unk369 = nodeData.m_unk369;
                                obj->m_unk36A = nodeData.m_unk36A;*/
                                obj->m_bIsDontEnter = nodeData.m_bIsDontEnter;
                                obj->m_bIsDontFade = nodeData.m_bIsDontFade;
                                obj->m_nDefaultZOrder = nodeData.m_nDefaultZOrder;
                                obj->m_unk38C = nodeData.m_unk38C;
                                obj->m_unk38D = nodeData.m_unk38D;
                                obj->m_unk38E = nodeData.m_unk38E;
                                obj->m_unk390 = nodeData.m_unk390;
                                obj->m_bShowGamemodeBorders = nodeData.m_bShowGamemodeBorders;
                                obj->m_unk3D9 = nodeData.m_unk3D9;
                                obj->m_bIsSelected = nodeData.m_bIsSelected;

                                /*obj->m_nGlobalClickCounter = nodeData.m_nGlobalClickCounter;
                                obj->m_bUnknownLayerRelated = nodeData.m_bUnknownLayerRelated;
                                obj->m_fMultiScaleMultiplier = nodeData.m_fMultiScaleMultiplier;
                                obj->m_bIsGroupParent = nodeData.m_bIsGroupParent;
                                obj->m_pGroups = nodeData.m_pGroups;
                                obj->m_nGroupCount = nodeData.m_nGroupCount;
                                obj->m_nEditorLayer = nodeData.m_nEditorLayer;
                                obj->m_nEditorLayer2 = nodeData.m_nEditorLayer2;
                                obj->m_unk414 = nodeData.m_unk414;
                                obj->m_obFirstPosition = nodeData.m_obFirstPosition;*/
                                section->replaceObjectAtIndex(i, obj);
                            }
                        }
                    }
                }
            }

            data.player_1_data.apply(PLAYLAYER->m_pPlayer1);
            data.player_2_data.apply(PLAYLAYER->m_pPlayer2);

        }
    }
}