#include "macros.hpp"

void Macro::remove_inputs(unsigned frame) {
    auto it = std::remove_if(inputs.begin(), inputs.end(), 
        [frame](const Input& input) { 
            return input.frame >= frame;
        });
    inputs.erase(it, inputs.end());
}

#define w_b(var) file.write(reinterpret_cast<char*>(&var), sizeof(var));
#define r_b(var) file.read(reinterpret_cast<char*>(&var), sizeof(var));

void Macro::write_file(const std::string& filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        error = "Error writing file " + filename + "!";
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
}

void Macro::read_file(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        error = "Error reading file " + filename + "!";
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
