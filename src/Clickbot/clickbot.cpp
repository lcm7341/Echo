#include "Clickbot.hpp"
#include <vector>
#include <filesystem>
#include <random>
#include <iostream>
#include <sstream>
#include <fstream>

namespace fs = std::filesystem;

namespace Clickbot
{
    std::string pickRandomFile(std::string path, std::string folder)
    {
        std::vector<std::string> clicks;
        std::vector<std::string> out;
        std::string path2 = ".echo\\clickpacks\\" + path + "\\" + folder;
        try
        {
            for (const auto& entry : std::filesystem::directory_iterator(path2))
            {
                clicks.push_back(entry.path().string());
            }
        }
        catch (const std::filesystem::filesystem_error& ex)
        {
            // Log the error message using printf
            printf("Filesystem error: %s\n", ex.what());

            std::ofstream logfile("error.log");
            if (logfile.is_open())
            {
                logfile << "Filesystem error: " << ex.what() << std::endl;
                logfile.close();
            }

            // Return an empty string to indicate an error
            return "";
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