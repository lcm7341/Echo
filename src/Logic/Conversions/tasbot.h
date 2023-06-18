#include <nlohmann/json.hpp>

#ifndef TASBOT_H
#define TASBOT_H

using json = nlohmann::json;
class TASBot : public Convertible {
public:

    void import(const std::string& filename) override {
        auto& logic = Logic::get();

        std::ifstream file(filename);

        json state;
        file >> state;

        logic.fps = state["fps"].get<double>();

        logic.inputs.clear();
        for (auto& json_input : state["macro"]) {
            Frame input;
            input.number = json_input["frame"].get<unsigned>();
            input.pressingDown = json_input["player_1"]["click"].get<int>() == 1;
            input.isPlayer2 = json_input["player_2"]["click"].get<int>() == 1;
            input.xPosition = json_input["player_1"]["x_position"].get<float>();
            logic.inputs.push_back(input);
        }

        logic.conversion_message = ""; // Clearing
        file.close();
    }

    void export_to(const std::string& filename) override {
        auto& logic = Logic::get();
        std::string dir = ".echo\\";
        std::string ext = ".json";

        std::string full_filename = dir + filename + ext;

        std::ofstream file(full_filename);

        json state;

        state["fps"] = logic.fps;

        json json_macro = json::array();
        for (auto& input : logic.inputs) {
            json json_input;
            json_input["frame"] = input.number;
            json_input["player_1"]["click"] = input.pressingDown ? 1 : 2;
            json_input["player_1"]["x_position"] = input.xPosition;
            json_input["player_2"]["click"] = input.isPlayer2 ? 1 : 2;
            json_input["player_2"]["x_position"] = input.xPosition;
            json_macro.push_back(json_input);
        }

        state["macro"] = json_macro;

        file << state.dump(4);

        logic.conversion_message = ""; // Clearing
        file.close();
    }

    std::string get_type_filter() const override {
        return ".json";
    }
};

#endif
