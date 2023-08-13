#ifndef MHR_JSON_H
#define MHR_JSON_H

template<typename T>
T getOrDefault(const json& j, const std::string& key, const T& defaultValue) {
    if (j.contains(key) && !j[key].is_null()) {
        return j[key].get<T>();
    }
    return defaultValue;
}

class MHRJSON : public Convertible {
public:
    void import(const std::string& filename) override {
        auto& logic = Logic::get();

        std::ifstream file(filename);

        json state;
        file >> state;


        if (!state.contains("_")) {
            file.close();
            logic.conversion_message = "Not an MHR file!";
            return;
        }


        logic.fps = state["meta"]["fps"].get<float>();

        logic.inputs.clear();
        for (auto& json_input : state["events"]) {
            if (json_input.find("down") != json_input.end()) {
                Frame input;
                input.number = json_input["frame"].get<unsigned>();
                input.pressingDown = json_input["down"].get<int>();
                input.xPosition = json_input["x"].get<float>();
                input.yPosition = json_input["y"].get<float>();
                input.rotation = json_input["r"].get<float>();
                input.yVelocity = json_input["a"].get<double>();
                input.isPlayer2 = getOrDefault(json_input, "p2", false);
                logic.inputs.push_back(input);
            }
        }

        logic.conversion_message = ""; // Clearing
        file.close();
    }

    void export_to(const std::string& filename) override {
        auto& logic = Logic::get();
        std::string dir = ".echo/";
        std::string ext = ".mhr.json";

        if (logic.export_to_bot_location) dir = "macros\\";

        std::string full_filename = dir + filename + ext;

        std::ofstream file(full_filename);

        json state;
        state["_"] = "Mega Hack v7.1.1-GM1 Replay";
        state["meta"]["fps"] = logic.fps;

        json json_events = json::array();
        for (auto& input : logic.inputs) {
            json json_input;
            json_input["frame"] = input.number;
            json_input["down"] = input.pressingDown;
            json_input["x"] = input.xPosition;
            json_input["a"] = input.yVelocity;
            json_input["r"] = input.rotation;
            json_input["y"] = input.yPosition;
            json_input["p2"] = input.isPlayer2;
            json_events.push_back(json_input);
        }

        state["events"] = json_events;

        file << state.dump(4);

        logic.conversion_message = ""; // Clearing
        file.close();
    }

    std::string get_type_filter() const override {
        return ".json";
    }

    std::string get_directory() const override {
        return "macros/";
    }
};

#endif
