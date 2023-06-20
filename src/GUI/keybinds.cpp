#include "keybinds.hpp"

void Keybinds::add(const std::string& key, Func function) {
    binds[key] = function;
}

void Keybinds::execute(const std::string& key) {
    if (binds.count(key) > 0) {
        binds[key]();
    }
}
