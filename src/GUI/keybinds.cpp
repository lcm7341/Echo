#include <functional>
#include <string>

class Keybinds {
public:
    using Func = std::function<void()>;

    void add(const std::string& key, Func function) {
        binds[key] = function;
    }

    void execute(const std::string& key) {
        if (binds.count(key) > 0) {
            binds[key]();
        }
    }

private:
    std::unordered_map<std::string, Func> binds;
};
