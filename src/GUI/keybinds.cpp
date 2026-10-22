#include "keybinds.h"
#include <iostream>
#include <imgui.h>
#include <Windows.h>

ImGuiKey ImGui_ImplWin32_KeyEventToImGuiKey(WPARAM wParam, LPARAM lParam);

static ImGuiKey ImGuiKeyFromVirtualKey(int key) {
    LPARAM scan_code = MapVirtualKey(key, MAPVK_VK_TO_VSC);
    return ImGui_ImplWin32_KeyEventToImGuiKey(key, scan_code << 16);
}

bool IsVirtualKeyPressed(int key) {
    ImGuiKey imgui_key = ImGuiKeyFromVirtualKey(key);
    return imgui_key != ImGuiKey_None && ImGui::IsKeyPressed(imgui_key, false);
}

bool IsVirtualKeyDown(int key) {
    ImGuiKey imgui_key = ImGuiKeyFromVirtualKey(key);
    return imgui_key != ImGuiKey_None && ImGui::IsKeyDown(imgui_key);
}

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
