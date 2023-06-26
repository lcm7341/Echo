#include <functional>
#include <string>

class Keybinds {
public:

    static auto& get() {
        static Keybinds instance;
        return instance;
    }

    using Func = std::function<void()>;

    void add(const int& key, Func function);

    void execute(const int& key);

private:
    std::unordered_map<int, Func> binds;
};