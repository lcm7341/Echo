#ifndef KEYBINDABLEBASE_HPP
#define KEYBINDABLEBASE_HPP

class KeybindableBase {
public:
    virtual ~KeybindableBase() = default;
    virtual void ran() = 0;
};

#endif // KEYBINDABLEBASE_HPP
