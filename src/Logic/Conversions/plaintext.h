#ifndef PLAINTEXT_H
#define PLAINTEXT_H

class PlainText : public Convertible {
public:

    void import(const std::string& filename) override {
        auto& logic = Logic::get();
        std::ifstream file(filename);
        logic.inputs.clear();

        // Read fps from the first line
        file >> logic.fps;

        while (!file.eof()) {
            Frame input;
            int pressingDown, isPlayer2;

            file >> input.number
                >> pressingDown
                >> isPlayer2;

            input.pressingDown = pressingDown != 0;
            input.isPlayer2 = isPlayer2 != 0;

            logic.inputs.push_back(input);
        }

        logic.conversion_message = ""; // Clearing
        file.close();
    }

    void export_to(const std::string& filename) override {
        auto& logic = Logic::get();

        std::string dir = ".echo\\";
        std::string ext = ".txt";

        std::string full_filename = dir + filename + ext;

        std::ofstream file(full_filename);

        file << logic.fps << "\n";

        for (auto& input : logic.inputs) {
            file << input.number << " "
                << (input.pressingDown ? 1 : 0) << " "
                << (input.isPlayer2 ? 1 : 0) << "\n";
        }

        logic.conversion_message = ""; // Clearing
        file.close();
    }

    std::string get_type_filter() const override {
        return ".txt";
    }
};

#endif