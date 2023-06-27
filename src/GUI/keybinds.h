#ifndef KEYBINDS_H
#define KEYBINDS_H

#include <unordered_map>
#include <string>
#include <optional>
#include "keybindable.h"
#include <memory>

class Keybind {
public:
    Keybind();
    void SetKey(int key, bool ctrl = false, bool shift = false, bool alt = false);
    void UnsetKey();
    std::optional<int> GetKey() const;
    bool GetCtrl() const;
    bool GetShift() const;
    bool GetAlt() const;

    std::optional<int> key;
    bool ctrl;
    bool shift;
    bool alt;
};

class Keybinds {
public:
    Keybinds();
    Keybind& GetKeybind(const std::string& action);
    void SetAction(const std::string& action, std::unique_ptr<KeybindableBase> keybindable);
    std::unordered_map<std::string, std::pair<Keybind, std::unique_ptr<KeybindableBase>>> bindings;
};
#endif // KEYBINDS_H
