#ifndef JSON_H
#define JSON_H

class JSON : public Convertible {
public:
    void import(const std::string& filename) override {
        auto& logic = Logic::get();

        logic.read_file_json(filename, true);
    }

    void export_to(const std::string& filename) override {
        auto& logic = Logic::get();
        logic.write_file_json(filename);
    }

    std::string get_type_filter() const override {
        return ".json";
    }

    std::string get_directory() const override {
        return ".echo/";
    }
};

#endif
