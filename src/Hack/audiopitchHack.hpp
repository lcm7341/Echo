class AudiopitchHack {
public:
    static AudiopitchHack& getInstance();
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