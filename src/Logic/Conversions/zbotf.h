#ifndef ZBF_H
#define ZBF_H


class ZBF : public Convertible {
    struct Action {
        int32_t x;
        bool hold;
        bool player1;
    };
public:
    void import(const std::string& filename) override {
        auto& logic = Logic::get();

        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            // Handle error: Unable to open the file
            return;
        }

        logic.inputs.clear();

        float delta = 0.f, speedhack = 1.0;
        file.read(reinterpret_cast<char*>(&delta), sizeof(delta));
        file.read(reinterpret_cast<char*>(&speedhack), sizeof(speedhack));

        float fps = 1.0 / (delta * speedhack);
        std::vector<Action> actions;

        while (!file.eof()) {
            int x = 0;
            bool hold = false, player1 = true;

            file.read(reinterpret_cast<char*>(&x), sizeof(x));
            file.read(reinterpret_cast<char*>(&hold), sizeof(hold));
            file.read(reinterpret_cast<char*>(&player1), sizeof(player1));

            actions.push_back({ x, hold == 0x31, player1 == 0x31 });
        }

        logic.fps = fps;
        
        for (auto& action : actions) {
            logic.inputs.push_back({ action.x + 1, action.hold, action.player1, 0, 0, 0, 0, 0 });
        }

        file.close();
    }

    void export_to(const std::string& filename) override {
        auto& logic = Logic::get();
        std::string dir = ".echo\\";
        std::string ext = ".zbf";

        if (logic.export_to_bot_location) dir = "replays\\";

        std::string full_filename = dir + filename + ext;

        std::ofstream file(full_filename, std::ios::binary);
        if (!file.is_open()) {
            // Handle error: Unable to open the file
            return;
        }

        float delta = 1.0 / logic.fps;
        float speedhack = 1.0;

        file.write(reinterpret_cast<const char*>(&delta), sizeof(delta));
        file.write(reinterpret_cast<const char*>(&speedhack), sizeof(speedhack));

        for (const auto& action : logic.get_inputs()) {
            int x = action.number - 1;
            int hold = action.pressingDown ? 0x31 : 0x30;
            int player1 = !action.isPlayer2 ? 0x31 : 0x30;

            file.write(reinterpret_cast<const char*>(&x), sizeof(x));
            file.write(reinterpret_cast<const char*>(&hold), 1);
            file.write(reinterpret_cast<const char*>(&player1), 1);
        }

        file.close();
    }

    std::string get_type_filter() const override {
        return ".zbf";
    }

    std::string get_directory() const override {
        return "replays/";
    }
};

#endif
