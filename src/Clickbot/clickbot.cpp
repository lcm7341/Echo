#include "Clickbot.hpp"
#include <vector>
#include <filesystem>
#include <random>
#include <iostream>
#include <sstream>
#include <fstream>
#include <unordered_map>
#include <chrono>

namespace fs = std::filesystem;

std::string convertToCamelCase(const std::string& input) {
    std::stringstream ss;
    bool capitalizeNext = false;

    for (char c : input) {
        if (c == '_') {
            capitalizeNext = true;
        }
        else {
            if (capitalizeNext) {
                ss << static_cast<char>(toupper(c));
                capitalizeNext = false;
            }
            else {
                ss << c;
            }
        }
    }

    return ss.str();
}

namespace Clickbot
{
    FMOD::Sound* cachedSound(FMOD::System* system, const std::string& path)
    {
        static std::unordered_map<std::string, FMOD::Sound*> sounds;
        if (!system) return nullptr;

        auto it = sounds.find(path);
        if (it != sounds.end()) return it->second;

        FMOD::Sound* sound = nullptr;
        if (system->createSound(path.c_str(), FMOD_DEFAULT, nullptr, &sound) != FMOD_OK)
            sound = nullptr;
        if (sound) sounds.emplace(path, sound);
        return sound;
    }

    std::string pickRandomFile(std::string path, std::string folder)
    {
        struct Listing {
            std::vector<std::string> files;
            std::chrono::steady_clock::time_point scanned;
        };
        static std::unordered_map<std::string, Listing> listings;
        static std::mt19937 rng{ std::random_device{}() };
        constexpr auto rescan_after = std::chrono::seconds(2);

        const std::string key = path + '\\' + folder;
        const auto now = std::chrono::steady_clock::now();
        auto cached = listings.find(key);
        if (cached != listings.end() && now - cached->second.scanned < rescan_after) {
            const auto& files = cached->second.files;
            if (files.empty()) return "";
            return files[std::uniform_int_distribution<size_t>(0, files.size() - 1)(rng)];
        }

        std::vector<std::string> clicks;
        std::string path2 = ".echo\\clickpacks\\" + path + "\\" + folder;
        if (!fs::is_directory(path2)) path2 = ".echo\\clickpacks\\" + path + "\\" + convertToCamelCase(folder);
        if (!fs::is_directory(path2)) {
            listings[key] = { {}, now };
            return "";
        }
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

        auto& listing = listings[key];
        listing.files = std::move(clicks);
        listing.scanned = now;

        if (listing.files.empty()) return "";
        return listing.files[std::uniform_int_distribution<size_t>(0, listing.files.size() - 1)(rng)];
    }
}