#ifndef KEYBINDABLE_HPP
#define KEYBINDABLE_HPP

#include <tuple>
#include <functional>
#include "keybindablebase.h"

template<typename Fn, typename... Args>
class Keybindable : public KeybindableBase {
public:
    Keybindable(Fn&& fn, Args&&... args)
        : func(std::forward<Fn>(fn))
        , args(std::make_tuple(std::forward<Args>(args)...)) {}

    void ran() override {
        if constexpr (sizeof...(Args) == 0) {
            func();
        }
        else {
            std::apply(func, args);
        }
    }

private:
    std::function<void(Args...)> func;
    std::tuple<Args...> args;
};


template<typename Fn>
class Keybindable<Fn, std::enable_if_t<std::is_invocable_v<Fn>>>
    : public KeybindableBase {
public:
    Keybindable(Fn&& fn)
        : func(std::forward<Fn>(fn)) {}

    void ran() override {
        func();
    }

private:
    std::function<void()> func;
};

#endif // KEYBINDABLE_HPP