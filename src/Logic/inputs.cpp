#include "inputs.hpp"

void Macro::remove_inputs(unsigned frame) {
    auto it = std::remove_if(inputs.begin(), inputs.end(),
        [frame](const Input& input) { return input.frame >= frame; });
    inputs.erase(it, inputs.end());
}

void Macro::write_file(const std::string& filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error opening file " << filename << "!.\n";
        return;
    }
    file.write(reinterpret_cast<const char*>(inputs.data()), static_cast<std::streamsize>(inputs.size()) * sizeof(Input));
    file.close();
}

void Macro::read_file(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error opening file " << filename << "!\n";
        return;
    }
    file.seekg(0, std::ios::end);
    std::streampos fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    inputs.resize(fileSize / sizeof(Input));

    file.read(reinterpret_cast<char*>(inputs.data()), fileSize);

    file.close();
}