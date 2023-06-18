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
        if ((current_frame % frames_between_presses == 0) && !is_pressed) {
            is_pressed = true;
            return true; // it's time to press
        }
        return false; // no need to press this frame
    }

    // Call this method every frame
    bool shouldRelease() {
        if (is_pressed && (current_frame % (frames_between_presses + frames_between_releases) == 0)) {
            is_pressed = false;
            return true; // it's time to release
        }
        return false; // no need to release this frame
    }

    // Call this method every frame to update internal counter
    void update(int frame) {
        current_frame = frame;
    }

private:
    Autoclicker()
        : frames_between_presses(0),
        frames_between_releases(0),
        current_frame(0),
        is_pressed(false)
    {}

    // prevent copying and assignment
    Autoclicker(const Autoclicker&) = delete;
    Autoclicker& operator=(const Autoclicker&) = delete;

    int frames_between_presses;
    int frames_between_releases;
    int current_frame;
    bool is_pressed;
};
