class AudiopitchHack {
public:
    static auto& getInstance() {
        static AudiopitchHack instance;
        return instance;
    }

    AudiopitchHack(AudiopitchHack const&) = delete;
    void operator=(AudiopitchHack const&) = delete;
    void initialize();

    bool isEnabled() const;
    void setEnabled(bool enabled);
    void setPitch(float freq);

    bool enabled;
    void* channel;
    float speed;

private:
    AudiopitchHack();
};