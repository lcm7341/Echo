#include "keybinds.h"
#include <iostream>
#include <imgui.h>

Keybind::Keybind() : key(std::nullopt), ctrl(false), shift(false), alt(false) {}

void Keybind::SetKey(int key, bool ctrl, bool shift, bool alt) {
    this->key = key;
    this->ctrl = ctrl;
    this->shift = shift;
    this->alt = alt;
}

void Keybind::UnsetKey() {
    this->key = std::nullopt;
    this->ctrl = false;
    this->shift = false;
    this->alt = false;
}

std::optional<int> Keybind::GetKey() const {
    return this->key;
}

bool Keybind::GetCtrl() const {
    return this->ctrl;
}

bool Keybind::GetShift() const {
    return this->shift;
}

bool Keybind::GetAlt() const {
    return this->alt;
}

Keybinds::Keybinds() {}

Keybind& Keybinds::GetKeybind(const std::string& action) {
    return this->bindings[action].first;
}

void Keybinds::SetAction(const std::string& action, std::unique_ptr<KeybindableBase> keybindable) {
    // Update the bindings map with the new action
    bindings[action] = std::make_pair(Keybind(), std::move(keybindable));
    //std::cout << "Action " << action << " has been set." << std::endl;
    //std::cout << "Total bindings: " << bindings.size() << std::endl;
}
