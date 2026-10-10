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
        if (!file.is_open())
            throw std::runtime_error("Could not open MHR file");

        // Read the header
        char header[8] = {};
        file.read(header, 8);

        // Read the meta size
        int meta_size = 0;
        file.read(reinterpret_cast<char*>(&meta_size), 4);
        if (!file || meta_size < 4 || meta_size > 1024 * 1024)
            throw std::runtime_error("Invalid MHR file");

        // Read the meta
        std::vector<char> meta(meta_size);
        file.read(meta.data(), meta_size);

        // Read the reserved
        char reserved[8] = {};
        file.read(reserved, 8);

        // Read the event size
        int event_size = 0;
        file.read(reinterpret_cast<char*>(&event_size), 4);

        // Read the event count
        int event_count = 0;
        file.read(reinterpret_cast<char*>(&event_count), 4);

        if (!file || event_size != sizeof(Event) || event_count < 0 || event_count > 50000000)
            throw std::runtime_error("Invalid MHR file");

        // Read the events
        std::vector<Event> events(event_count);
        file.read(reinterpret_cast<char*>(events.data()), static_cast<std::streamsize>(event_size) * event_count);
        events.resize(static_cast<size_t>(file.gcount()) / sizeof(Event));

        // Close the file
        file.close();

        int fps = 0;

        for (int i = 3; i >= 0; i--) {
            fps = (fps << 8) | static_cast<unsigned char>(meta[i]);
        }

        if (fps <= 0)
            throw std::runtime_error("Invalid MHR file");

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
        std::string dir = ".echo\\";
        std::string ext = ".mhr";

        if (logic.export_to_bot_location) dir = "macros\\";

        std::string full_filename = dir + filename + ext;

        std::ofstream file(full_filename, std::ios::binary);

        unsigned char bytes[] = { 0x48, 0x41, 0x43, 0x4B, 0x50, 0x52, 0x4F, 0x07 };
        file.write(reinterpret_cast<const char*>(&bytes), 8);

        int meta_size = 4;
        file.write(reinterpret_cast<const char*>(&meta_size), 4);
        int fps = (int)logic.fps;
        file.write(reinterpret_cast<const char*>(&fps), 4);

        const char reserved[8] = { 'A', 'B', 'S', 'O', 'L', 'U', 'T', 'E' };
        file.write(reserved, 8);

        int event_size = 32;
        file.write(reinterpret_cast<const char*>(&event_size), 4);

        int event_count = logic.inputs.size();
        file.write(reinterpret_cast<const char*>(&event_count), 4);

        for (const auto& input : logic.inputs) {
            wchar_t type = 2;
            file.write(reinterpret_cast<const char*>(&type), 2);
            file.write(reinterpret_cast<const char*>(&input.pressingDown), 1);
            file.write(reinterpret_cast<const char*>(&input.isPlayer2), 1);
            file.write(reinterpret_cast<const char*>(&input.number), 4);
            file.write(reinterpret_cast<const char*>(&input.xPosition), 4);
            file.write(reinterpret_cast<const char*>(&input.yPosition), 4);
            file.write(reinterpret_cast<const char*>(&input.yVelocity), 8);
            double rotation = input.rotation;
            file.write(reinterpret_cast<const char*>(&rotation), 8);
        }

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
