#include <functional>
#include <string>

class Keybinds {
public:
    using Func = std::function<void()>;

    void add(const std::string& key, Func function);

    void execute(const std::string& key);

private:
    std::unordered_map<std::string, Func> binds;
};