#ifndef MHR_H
#define MHR_H

class MHR : public Convertible {
public:
    void import(const std::string& filename) override {

    }

    void export_to(const std::string& filename) override {

    }

    std::string get_type_filter() const override {
        return ".json";
    }
};

#endif
