#ifndef KEYBINDS_H
#define KEYBINDS_H

#include <unordered_map>
#include <string>
#include <optional>

class Keybind {
public:
    Keybind();
    void SetKey(int key);
    void UnsetKey();
    std::optional<int> GetKey() const;
private:
    std::optional<int> key;
};

class Keybinds {
public:
    Keybinds();
    Keybind& GetKeybind(const std::string& action);
    bool GetKeybindMode(const std::string& pane) const;
    void SetKeybindMode(const std::string& pane, bool mode);

private:
    std::unordered_map<std::string, Keybind> bindings;
    std::unordered_map<std::string, bool> keybindModes;
};
#endif // KEYBINDS_H