#ifndef MHR_H
#define MHR_H


class MHR : public Convertible {
    struct Event {
        wchar_t type;
        bool mouse_down;
        bool player_2;
        int frame;
        float x;
        float y;
        double accel;
        double rot;
    };
public:
    void import(const std::string& filename) override {
        auto& logic = Logic::get();

        std::ifstream file(filename, std::ios::binary);

        // Read the header
        std::string header;
        file.read(reinterpret_cast<char*>(&header), 8);

        // Read the meta size
        int meta_size = 0;
        file.read(reinterpret_cast<char*>(&meta_size), 4);

        // Read the meta
        std::vector<char> meta(meta_size);
        file.read(meta.data(), meta_size);

        // Read the reserved
        std::string reserved;
        file.read(reinterpret_cast<char*>(&reserved), 8);

        // Read the event size
        int event_size = 0;
        file.read(reinterpret_cast<char*>(&event_size), 4);

        // Read the event count
        int event_count = 0;
        file.read(reinterpret_cast<char*>(&event_count), 4);

        // Read the events
        std::vector<Event> events(event_count);
        file.read(reinterpret_cast<char*>(events.data()), event_size * event_count);

        // Seek to the footer position
        file.seekg(0, std::ios::end);
        std::ifstream::pos_type fileSize = file.tellg();
        file.seekg(fileSize - static_cast<std::ifstream::pos_type>(sizeof(uint64_t)));

        // Read the footer
        uint64_t footer = 0;
        file.read(reinterpret_cast<char*>(&footer), sizeof(footer));

        // Close the file
        file.close();

        int fps = 0;

        for (int i = 3; i >= 0; i--) {
            fps = (fps << 8) | static_cast<unsigned char>(meta[i]);
        }

        std::ofstream outputFile("output.txt");
        if (!outputFile) {
            std::cerr << "Failed to create the output file." << std::endl;
        }

        for (const auto& event : events) {
            // Format the event data as a string
            std::string eventString = std::to_string(event.frame) + " " +
                std::to_string(event.mouse_down ? 1 : 0) + " " +
                std::to_string(event.player_2 ? 1 : 0) + "\n";

            // Write the event string to the output file
            outputFile << eventString;
        }

        outputFile.close();

        logic.inputs.clear();
        logic.fps = fps;

        for (int i = 0; i < events.size(); i++) {
            Event event = events[i];
            logic.add_input({ event.frame, event.mouse_down, event.player_2, event.y, event.x, float(event.rot), event.accel, 0.f });
        }

        logic.conversion_message = ""; // Clearing
        file.close();
    }

    void export_to(const std::string& filename) override {
        auto& logic = Logic::get();
        std::string dir = ".echo/";
        std::string ext = ".mhr";

        std::string full_filename = dir + filename + ext;

        std::ofstream file(full_filename, std::ios::binary);

        std::string header = "HACKPRO";
        file.write(reinterpret_cast<const char*>(&header), 8);

        int meta_size = 4;
        file.write(reinterpret_cast<const char*>(&meta_size), 4);
        file.write(reinterpret_cast<const char*>(&logic.fps), 4);

        std::string reserved = "ABSÌLUTE";
        file.write(reinterpret_cast<const char*>(&reserved), 8);

        int event_size = 32;
        file.write(reinterpret_cast<const char*>(&event_size), 4);

        int event_count = logic.inputs.size();
        file.write(reinterpret_cast<const char*>(&event_count), 4);

        std::vector<Event> events(event_count);
        for (const auto& input : logic.inputs) {
            events.push_back({ 2, input.pressingDown, input.isPlayer2, input.number, input.xPosition, input.yPosition, input.yVelocity, input.rotation });
        }

        file.write(reinterpret_cast<const char*>(&events), event_size * event_count);

        logic.conversion_message = ""; // Clearing
        file.close();
    }

    std::string get_type_filter() const override {
        return ".mhr";
    }

    std::string get_directory() const override {
        return "macros/";
    }
};

#endif
