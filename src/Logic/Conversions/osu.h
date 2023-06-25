#ifndef OSU_H
#define OSU_H

class Osu : public Convertible {
public:

    void import(const std::string& filename) override {

    }

    void export_to(const std::string& filename) override {
        auto& logic = Logic::get();

        std::string dir = ".echo\\osu\\" + filename + "\\";
        std::filesystem::create_directory(dir);
        std::string ext = ".osu";
        std::string full_filename = dir + filename + ext;

        std::ofstream osu_file(full_filename);

        // osu file header
        osu_file << "osu file format v14\n\n";

        // General section
        osu_file << "[General]\n";
        osu_file << "StackLeniency: 0.7\n";
        osu_file << "AudioLeadIn: 0\n";
        osu_file << "Mode: 1\n"; // Mode 1 is for Taiko
        osu_file << "Countdown: 0\n"; // No countdown before the song starts
        osu_file << "\n";

        // Metadata section
        osu_file << "[Metadata]\n";
        osu_file << "Title:" + filename + "\n";
        osu_file << "Artist:Echo\n"; // It's Echo not EchoBot you scum
        osu_file << "Creator:Echo\n";
        osu_file << "Version:1.0\n";
        osu_file << "\n";

        // Difficulty section
        osu_file << "[Difficulty]\n";
        osu_file << "HPDrainRate:5\n";
        osu_file << "CircleSize:5\n";
        osu_file << "OverallDifficulty:5\n";
        osu_file << "\n";

        // TimingPoints section
        osu_file << "[TimingPoints]\n";
        osu_file << "0,500,4,1,0,100,1,0\n";
        osu_file << "\n";

        // HitObjects section
        osu_file << "[HitObjects]\n";

        for (auto& input : logic.inputs) {

            if (input.pressingDown) {
                // Calculate the time in milliseconds
                int time = round(input.number / logic.get_fps() * 1000 * (2 - logic.speedhack));

                // For Taiko mode, there's no positional data needed.
                // The format for a single hit object is: time,x,y,type,hitSound,objectParams,hitSample
                // type: 1 for a regular hit, 2 for a drumroll (slider in standard osu), 8 for a spinner.
                // hitSound: 0 for a normal hit sound, 2 for whistle, 4 for finish, 8 for clap.
                osu_file << "256,192," << time << ",1,0,0:0:0:0:\n";
            }
        }

        logic.conversion_message = ""; // Clearing
        osu_file.close();
    }

    std::string get_type_filter() const override {
        return ".ocr";
    }

    std::string get_directory() const override {
        return ".echo/osu/";
    }

};

#endif
