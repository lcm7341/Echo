#include "macros.hpp"
#define PLAYLAYER gd::GameManager::sharedState()->getPlayLayer()

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

void Macro::remove_inputs(unsigned frame) {
    auto it = std::remove_if(inputs.begin(), inputs.end(),
        [frame](const Input& input) {
            return input.frame >= frame;
        });
    inputs.erase(it, inputs.end());
}

void Macro::handle_checkpoint_data() {
    if (PLAYLAYER) {
        if (checkpoints.size() > 0) {
            Checkpoint& data = checkpoints.back();

            PLAYLAYER->m_pPlayer1->setRotationX(data.player_1.rotation);
            PLAYLAYER->m_pPlayer2->setRotationX(data.player_2.rotation);

            PLAYLAYER->m_pPlayer1->m_yAccel = data.player_1.y_accel;
            PLAYLAYER->m_pPlayer2->m_yAccel = data.player_2.y_accel;

        }
    }
}