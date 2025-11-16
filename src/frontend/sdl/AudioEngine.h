#pragma once

#include <SDL2/SDL.h>

#include <atomic>

namespace cretris::frontend {

class AudioEngine {
public:
    void initialize();
    void shutdown();
    void trigger_line_clear();
    void trigger_hard_drop();
    void set_level_progress(float progress);

private:
    static void audio_callback(void *userdata, Uint8 *stream, int len);

    SDL_AudioDeviceID device_{0};
    SDL_AudioSpec obtained_{};
    double song_time_{0.0};
    double bass_phase_{0.0};
    double bass_phase_sub_{0.0};
    double shimmer_phase_{0.0};
    double lead_phase_{0.0};
    double lead_phase_b_{0.0};
    double vibrato_phase_{0.0};
    double pad_phase_{0.0};
    double pad_lfo_phase_{0.0};
    double drop_phase_{0.0};
    double line_phase_{0.0};
    double noise_state_{0.33};
    double lead_env_{0.0};
    double bass_env_{0.0};
    double pad_env_{0.0};
    int last_lead_note_{-1};
    int last_bass_note_{-1};
    int last_pad_note_{-1};
    std::atomic<float> line_pulse_{0.0f};
    std::atomic<float> drop_pulse_{0.0f};
    std::atomic<float> tempo_mod_{0.0f};
};

} // namespace cretris::frontend
