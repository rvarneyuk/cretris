#include "AudioEngine.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace cretris::frontend {

void AudioEngine::initialize() {
    SDL_AudioSpec desired{};
    desired.freq = 48000;
    desired.format = AUDIO_F32SYS;
    desired.channels = 2;
    desired.samples = 1024;
    desired.userdata = this;
    desired.callback = &AudioEngine::audio_callback;

    device_ = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained_, 0);
    if (device_ != 0) {
        SDL_PauseAudioDevice(device_, 0);
    }
}

void AudioEngine::shutdown() {
    if (device_ != 0) {
        SDL_CloseAudioDevice(device_);
        device_ = 0;
    }
}

void AudioEngine::trigger_line_clear() { line_pulse_.store(1.0f); }

void AudioEngine::trigger_hard_drop() { drop_pulse_.store(0.8f); }

void AudioEngine::set_level_progress(float progress) {
    tempo_mod_.store(std::clamp(progress, 0.0f, 1.0f));
}

void AudioEngine::audio_callback(void *userdata, Uint8 *stream, int len) {
    auto *self = static_cast<AudioEngine *>(userdata);
    if (!self || self->device_ == 0) {
        SDL_memset(stream, 0, len);
        return;
    }

    auto *buffer = reinterpret_cast<float *>(stream);
    int frames = len / (sizeof(float) * 2);
    float line = self->line_pulse_.load();
    float drop = self->drop_pulse_.load();
    float tempo = 0.75f + 0.45f * self->tempo_mod_.load();
    double sample_rate = static_cast<double>(self->obtained_.freq);

    for (int i = 0; i < frames; ++i) {
        double t = self->song_time_;
        double beat = std::fmod(t * tempo * 2.0, 4.0);
        double bass_freq = (beat < 2.0) ? 110.0 : 147.0;
        double melody_freqs[] = {440.0, 392.0, 523.25, 349.23};
        int melody_index = static_cast<int>(std::fmod(t * tempo * 1.5, 4.0));
        double melody_freq = melody_freqs[melody_index];

        double bass = std::sin(2.0 * std::numbers::pi_v<double> * self->bass_phase_);
        double shimmer = std::sin(2.0 * std::numbers::pi_v<double> * self->shimmer_phase_);
        self->bass_phase_ += bass_freq / sample_rate;
        self->shimmer_phase_ += melody_freq / sample_rate;
        if (self->bass_phase_ >= 1.0) {
            self->bass_phase_ -= 1.0;
        }
        if (self->shimmer_phase_ >= 1.0) {
            self->shimmer_phase_ -= 1.0;
        }

        double pad = std::sin(2.0 * std::numbers::pi_v<double> * (t * 0.5)) * 0.2;
        double sample = bass * 0.18 + shimmer * 0.12 + pad;

        if (line > 0.0f) {
            sample += std::sin(2.0 * std::numbers::pi_v<double> * self->line_phase_) * (0.3 * line);
            self->line_phase_ += 600.0 / sample_rate;
            if (self->line_phase_ >= 1.0) {
                self->line_phase_ -= 1.0;
            }
            line = std::max(0.0f, line - 0.0008f);
        }
        if (drop > 0.0f) {
            double drop_freq = 200.0 + 600.0 * drop;
            sample += std::sin(2.0 * std::numbers::pi_v<double> * self->drop_phase_) * (0.25 * drop);
            self->drop_phase_ += drop_freq / sample_rate;
            if (self->drop_phase_ >= 1.0) {
                self->drop_phase_ -= 1.0;
            }
            drop = std::max(0.0f, drop - 0.0006f);
        }

        self->song_time_ += 1.0 / sample_rate;
        float out_sample = static_cast<float>(sample * 0.8);
        buffer[i * 2] = out_sample;
        buffer[i * 2 + 1] = out_sample;
    }

    self->line_pulse_.store(line);
    self->drop_pulse_.store(drop);
}

} // namespace cretris::frontend
