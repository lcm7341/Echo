#include "keybinds.hpp"
#include <conio.h> // For _kbhit() and _getch() (Windows-specific)

void Keybinds::add(const int& key, Func function) {
    binds[key] = function;
}

void Keybinds::execute(const int& key) {
    if (binds.count(key) > 0) {
        binds[key]();
    }
}