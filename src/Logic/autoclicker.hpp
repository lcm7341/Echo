class Autoclicker {
public:
    static Autoclicker& get() {
        static Autoclicker instance;
        return instance;
    }

    // Setters for frames between presses and releases
    void setFramesBetweenPresses(int frames) {
        frames_between_presses = frames;
    }

    void setFramesBetweenReleases(int frames) {
        frames_between_releases = frames;
    }

    // Getters for frames between presses and releases
    int getFramesBetweenPresses() const {
        return frames_between_presses;
    }

    int getFramesBetweenReleases() const {
        return frames_between_releases;
    }

    // Call this method every frame
    bool shouldPress() {
        if (!is_pressed && frames_since_release >= frames_between_releases) {
            is_pressed = true;
            frames_since_release = 0;
            return true; // it's time to press
        }
        return false; // no need to press this frame
    }

    // Call this method every frame
    bool shouldRelease() {
        if (is_pressed && frames_since_press >= frames_between_presses) {
            is_pressed = false;
            frames_since_press = 0;
            return true; // it's time to release
        }
        return false; // no need to release this frame
    }

    // Call this method every frame to update internal counters
    void update() {
        if (is_pressed) {
            frames_since_press++;
        }
        else {
            frames_since_release++;
        }
    }

private:
    Autoclicker()
        : frames_between_presses(10),
        frames_between_releases(10),
        frames_since_press(0),
        frames_since_release(0),
        is_pressed(false)
    {}

    // prevent copying and assignment
    Autoclicker(const Autoclicker&) = delete;
    Autoclicker& operator=(const Autoclicker&) = delete;

    int frames_between_presses;
    int frames_between_releases;
    int frames_since_press;
    int frames_since_release;
    bool is_pressed;
};
