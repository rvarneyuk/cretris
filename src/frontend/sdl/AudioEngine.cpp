#include "AudioEngine.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace {

constexpr std::array<int, 16> kBassLine = {38, 38, 45, 45, 41, 41, 48, 48, 38, 38, 45, 45, 43, 43, 50, 50};
constexpr std::array<int, 8> kChordRoots = {50, 50, 53, 53, 57, 57, 48, 48};
constexpr std::array<int, 32> kLeadLine = {79, 81, 79, 76, 74, 76, 72, 74, 76, 79, 81, 83, 81, 79, 76, 72,
                                           74, 76, 79, 76, 74, 72, 71, 72, 74, 79, 76, 74, 72, 69, 67, 69};
constexpr std::array<int, 4> kChordIntervals = {0, 3, 7, 10};

double midi_to_freq(int midi) {
    return 440.0 * std::pow(2.0, (static_cast<double>(midi) - 69.0) / 12.0);
}

double wrap_phase(double phase) {
    phase = std::fmod(phase, 1.0);
    if (phase < 0.0) {
        phase += 1.0;
    }
    return phase;
}

double saw_wave(double phase) {
    double wrapped = wrap_phase(phase);
    return 2.0 * wrapped - 1.0;
}

double triangle_wave(double phase) {
    double wrapped = wrap_phase(phase);
    return 1.0 - 4.0 * std::abs(wrapped - 0.5);
}

double softstep(double value, double steepness) {
    return std::exp(-value * steepness);
}

} // namespace

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
    float tempo_mod = self->tempo_mod_.load();
    double tempo = 1.15 + 0.75 * tempo_mod;
    double sample_rate = static_cast<double>(self->obtained_.freq);

    for (int i = 0; i < frames; ++i) {
        double t = self->song_time_;
        double beats = t * tempo;
        double sixteenth = beats * 4.0;
        int int_sixteenth = static_cast<int>(sixteenth);
        double step_fraction = sixteenth - static_cast<double>(int_sixteenth);
        int bass_index = (int_sixteenth / 2) % static_cast<int>(kBassLine.size());
        int lead_index = int_sixteenth % static_cast<int>(kLeadLine.size());
        int chord_index = static_cast<int>(beats) % static_cast<int>(kChordRoots.size());

        double bass_freq = midi_to_freq(kBassLine[static_cast<std::size_t>(bass_index)]);
        self->bass_phase_ = wrap_phase(self->bass_phase_ + bass_freq / sample_rate);
        double bass = triangle_wave(self->bass_phase_) * 0.35 * softstep(step_fraction, 2.6);

        double lead_freq = midi_to_freq(kLeadLine[static_cast<std::size_t>(lead_index)]);
        self->shimmer_phase_ = wrap_phase(self->shimmer_phase_ + lead_freq / sample_rate);
        double lead = std::sin(2.0 * std::numbers::pi_v<double> * self->shimmer_phase_);
        lead *= 0.22 * (0.35 + softstep(step_fraction, 3.4));

        int arp_index = (int_sixteenth / 4) % static_cast<int>(kChordRoots.size());
        double arp_freq = midi_to_freq(kChordRoots[static_cast<std::size_t>(arp_index)] + 12 + (int_sixteenth % 4) * 2);
        self->lead_phase_ = wrap_phase(self->lead_phase_ + arp_freq / sample_rate);
        double arp = saw_wave(self->lead_phase_) * 0.12;

        double pad_freq = midi_to_freq(kChordRoots[static_cast<std::size_t>(chord_index)]);
        self->pad_phase_ = wrap_phase(self->pad_phase_ + pad_freq / sample_rate * 0.25);
        double pad = 0.0;
        for (int interval : kChordIntervals) {
            double freq = midi_to_freq(kChordRoots[static_cast<std::size_t>(chord_index)] + interval);
            double phase = wrap_phase(self->pad_phase_ * freq / pad_freq);
            pad += saw_wave(phase);
        }
        pad = (pad / static_cast<double>(kChordIntervals.size())) * 0.18;

        double beat_fraction = beats - std::floor(beats);
        double kick_carrier = std::sin(2.0 * std::numbers::pi_v<double> * (beat_fraction * (1.0 + 2.0 * (1.0 - beat_fraction))));
        double kick = kick_carrier * 0.55 * softstep(beat_fraction, 7.0);

        self->noise_state_ = std::fmod(self->noise_state_ * 987.654321 + 0.12345, 1.0);
        double white = self->noise_state_ * 2.0 - 1.0;
        int hat_step = int_sixteenth % 16;
        double hat_env = softstep(step_fraction, 48.0);
        double hat = white * hat_env * ((hat_step % 2 == 0) ? 0.25 : 0.55);
        double snare = 0.0;
        if (hat_step == 4 || hat_step == 12) {
            snare = white * 0.5 * softstep(step_fraction, 20.0);
        }

        double ambience = std::sin(2.0 * std::numbers::pi_v<double> * (t * 0.2)) * 0.08;
        double sample = bass + pad + lead + arp + hat + snare + kick + ambience;

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
