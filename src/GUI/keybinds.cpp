// Keybinds.cpp
#include "Keybinds.h"

Keybind::Keybind() : key(std::nullopt) {}

void Keybind::SetKey(int newKey) {
    key = newKey;
}

void Keybind::UnsetKey() {
    key = std::nullopt;
}

std::optional<int> Keybind::GetKey() const {
    return key;
}

Keybinds::Keybinds() {
    // Initialize keybinds with no keys by default
    bindings["audioHack"] = Keybind();
    bindings["Play"] = Keybind();
    // ... add more keybinds as needed

    keybindModes["Editor"] = false;
    keybindModes["Main"] = false;
}

Keybind& Keybinds::GetKeybind(const std::string& action) {
    return bindings.at(action);
}

bool Keybinds::GetKeybindMode(const std::string& pane) const {
    return keybindModes.at(pane);
}

void Keybinds::SetKeybindMode(const std::string& pane, bool mode) {
    keybindModes[pane] = mode;
}