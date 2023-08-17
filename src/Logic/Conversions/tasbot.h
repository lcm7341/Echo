#include <nlohmann/json.hpp>

#ifndef TASBOT_H
#define TASBOT_H

#include <simdjson.h>
using namespace simdjson;
using json = nlohmann::json;

class TASBot : public Convertible {
public:

    void import(const std::string& filename) override {
        auto& logic = Logic::get();

        // Step 1: Read the JSON file into memory
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << filename << std::endl;
            return;
        }

        std::string jsonString((std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>());
        file.close();

        // Step 2: Use simdjson API to parse the JSON data
        simdjson::dom::parser parser;
        simdjson::dom::element root;
        simdjson::error_code error = parser.parse(jsonString).get(root);
        if (error) {
            std::cerr << "Error parsing JSON: " << error << std::endl;
            return;
        }

        // Step 3: Access the data from the parsed JSON
        // Get the "fps" value
        double fps;
        if (root["fps"].is_double()) {
            fps = root["fps"].get_double().value();
            logic.fps = fps;
        }

        // Clear the existing inputs
        logic.inputs.clear();

        // Iterate over the "macro" array
        simdjson::dom::array macroArray = root["macro"].get_array();
        for (simdjson::dom::object macro : macroArray) {
            Frame input;    

            // Get the "frame" value
            unsigned frame;
            frame = static_cast<unsigned>(macro["frame"].get_uint64().value());
            input.number = frame;

            // Access "click" and "x_position" values for "player_1"
            int player1Click;
            float xPosition;
            player1Click = static_cast<int>(macro["player_1"]["click"].get_int64().value());
            xPosition = static_cast<float>(macro["player_1"]["x_position"].get_double().value());

            if (player1Click == 1) {
                input.pressingDown = true;
                input.isPlayer2 = false;
                input.xPosition = xPosition;
                logic.inputs.push_back(input);
            }
            else if (player1Click == 2) {
                input.pressingDown = false;
                input.isPlayer2 = false;
                input.xPosition = xPosition;
                logic.inputs.push_back(input);
            }

            int player2Click;
            player2Click = static_cast<int>(macro["player_2"]["click"].get_int64().value());
            xPosition = static_cast<float>(macro["player_2"]["x_position"].get_double().value());

            if (player2Click == 1) {
                input.pressingDown = true;
                input.isPlayer2 = true;
                input.xPosition = xPosition;
                logic.inputs.push_back(input);
            }
            else if (player2Click == 2) {
                input.pressingDown = false;
                input.isPlayer2 = true;
                input.xPosition = xPosition;
                logic.inputs.push_back(input);
            }
        }

        logic.offset_frames(1);

        logic.conversion_message = ""; // Clearing
    }

    void export_to(const std::string& filename) override {
        auto& logic = Logic::get();
        std::string dir = ".echo\\";
        std::string ext = ".json";

        if (logic.export_to_bot_location) dir = ".tasbot\\macro\\";

        std::string full_filename = dir + filename + ext;

        std::ofstream file(full_filename);

        json state;


        logic.offset_frames(-1);

        state["fps"] = logic.fps;

        json json_macro = json::array();
        for (auto& input : logic.inputs) {
            json json_input;
            json_input["frame"] = input.number;
            if (!input.isPlayer2) {
                json_input["player_1"]["click"] = input.pressingDown ? 1 : 2;
                json_input["player_2"]["click"] = 0;
            }
            else {
                json_input["player_2"]["click"] = input.pressingDown ? 1 : 2;
                json_input["player_1"]["click"] = 0;
            }
            json_input["player_1"]["x_position"] = input.xPosition;
            json_input["player_2"]["x_position"] = input.xPosition;
            json_macro.push_back(json_input);
        }

        state["macro"] = json_macro;

        file << state.dump(4);

        logic.offset_frames(1); // accounting for the previous -1 offset

        logic.conversion_message = ""; // Clearing
        file.close();
    }

    std::string get_type_filter() const override {
        return ".json";
    }

    std::string get_directory() const override {
        return ".tasbot/macro/";
    }
};

#endif
