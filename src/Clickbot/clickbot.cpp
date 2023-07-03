#include "Clickbot.hpp"
#include <vector>
#include <filesystem>
#include <random>

namespace fs = std::filesystem;

namespace Clickbot
{
    std::string pickRandomFile(std::string folder, bool player1)
    {
        std::vector<std::string> clicks;
        std::vector<std::string> out;
        std::string player = player1 ? "player_1\\" : "player_2\\";
        std::string path = ".echo\\clickbot\\" + player + folder;
        for (const auto& entry : fs::directory_iterator(path))
        {
            clicks.push_back(entry.path().string());
        }
        if (clicks.empty()) return "";
        std::sample(
            clicks.begin(),
            clicks.end(),
            std::back_inserter(out),
            1,
            std::mt19937{ std::random_device{}() }
        );
        return out[0];
    }
}