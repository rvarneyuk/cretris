#include "AudioEngine.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace {

struct NoteEvent {
    int midi;
    int duration; // in sixteenth notes
};

template <std::size_t N>
constexpr int total_duration(const std::array<NoteEvent, N> &sequence) {
    int total = 0;
    for (const auto &event : sequence) {
        total += event.duration;
    }
    return total;
}

template <std::size_t N>
int note_at_step(const std::array<NoteEvent, N> &sequence, int step, int period) {
    int position = period > 0 ? step % period : 0;
    for (const auto &event : sequence) {
        if (position < event.duration) {
            return event.midi;
        }
        position -= event.duration;
    }
    return sequence.back().midi;
}

// Lead melody adapted from the public-domain "Ode to Joy" by Ludwig van Beethoven.
constexpr std::array<NoteEvent, 48> kMelody = {{{64, 4}, {64, 4}, {65, 4}, {67, 4}, {67, 4}, {65, 4}, {64, 4}, {62, 4},
                                                {60, 4}, {60, 4}, {62, 4}, {64, 4}, {62, 4}, {60, 8},
                                                {62, 4}, {62, 4}, {64, 4}, {65, 4}, {65, 4}, {64, 4}, {62, 4}, {60, 4},
                                                {60, 4}, {62, 4}, {64, 4}, {62, 4}, {60, 8},
                                                {64, 4}, {64, 4}, {60, 4}, {62, 4}, {64, 4}, {65, 4}, {67, 6}, {65, 2},
                                                {64, 4}, {64, 4}, {60, 4}, {62, 4}, {64, 4}, {65, 4}, {67, 6}, {65, 2},
                                                {64, 4}, {62, 4}, {60, 4}, {62, 4}, {60, 8}}};

constexpr std::array<NoteEvent, 16> kBassSequence = {{{48, 8}, {48, 8}, {43, 8}, {45, 8}, {41, 8}, {45, 8}, {43, 8}, {48, 8},
                                                      {48, 8}, {48, 8}, {43, 8}, {45, 8}, {41, 8}, {45, 8}, {43, 8}, {48, 8}}};

constexpr std::array<NoteEvent, 8> kPadSequence = {{{48, 16}, {43, 16}, {45, 16}, {41, 16}, {48, 16}, {43, 16}, {45, 16}, {48, 16}}};

constexpr int kMelodyPeriod = total_duration(kMelody);
constexpr int kBassPeriod = total_duration(kBassSequence);
constexpr int kPadPeriod = total_duration(kPadSequence);
constexpr std::array<int, 4> kChordIntervals = {0, 4, 7, 11};

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

double square_wave(double phase) {
    double wrapped = wrap_phase(phase);
    return wrapped < 0.5 ? 1.0 : -1.0;
}

double softstep(double value, double steepness) {
    return std::exp(-value * steepness);
}

double approach(double current, double target, double coeff) {
    return current + (target - current) * coeff;
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
        int melody_note = note_at_step(kMelody, int_sixteenth, kMelodyPeriod);
        int bass_note = note_at_step(kBassSequence, int_sixteenth, kBassPeriod);
        int pad_root = note_at_step(kPadSequence, int_sixteenth, kPadPeriod);

        if (melody_note != self->last_lead_note_) {
            self->lead_env_ = 1.0;
            self->lead_phase_ = 0.0;
            self->lead_phase_b_ = 0.25;
            self->last_lead_note_ = melody_note;
        }
        if (bass_note != self->last_bass_note_) {
            self->bass_env_ = 1.0;
            self->bass_phase_ = 0.0;
            self->bass_phase_sub_ = 0.0;
            self->last_bass_note_ = bass_note;
        }
        if (pad_root != self->last_pad_note_) {
            self->pad_env_ = 1.0;
            self->pad_phase_ = 0.0;
            self->last_pad_note_ = pad_root;
        }

        self->lead_env_ = approach(self->lead_env_, 0.68, 0.00035);
        self->bass_env_ = approach(self->bass_env_, 0.55, 0.0006);
        self->pad_env_ = approach(self->pad_env_, 0.9, 0.00012);

        double lead_freq = midi_to_freq(melody_note);
        self->vibrato_phase_ = wrap_phase(self->vibrato_phase_ + 5.2 / sample_rate);
        double vibrato = std::sin(2.0 * std::numbers::pi_v<double> * self->vibrato_phase_) * 0.006;
        self->lead_phase_ = wrap_phase(self->lead_phase_ + (lead_freq * (1.0 + vibrato * 0.75)) / sample_rate);
        self->lead_phase_b_ = wrap_phase(self->lead_phase_b_ + (lead_freq * 0.997) / sample_rate);
        double lead_a = saw_wave(self->lead_phase_);
        double lead_b = square_wave(self->lead_phase_b_);
        double lead = (lead_a * 0.65 + lead_b * 0.35) * self->lead_env_ * (0.25 + softstep(step_fraction, 3.8) * 0.45);

        double bass_freq = midi_to_freq(bass_note);
        self->bass_phase_ = wrap_phase(self->bass_phase_ + bass_freq / sample_rate);
        self->bass_phase_sub_ = wrap_phase(self->bass_phase_sub_ + (bass_freq * 0.5) / sample_rate);
        double bass_carrier = triangle_wave(self->bass_phase_) * 0.55 + std::sin(2.0 * std::numbers::pi_v<double> * self->bass_phase_sub_) * 0.45;
        double bass = bass_carrier * self->bass_env_ * (0.4 + softstep(step_fraction, 2.2) * 0.4);

        double pad_freq = midi_to_freq(pad_root);
        self->pad_phase_ = wrap_phase(self->pad_phase_ + pad_freq / sample_rate * 0.35);
        self->pad_lfo_phase_ = wrap_phase(self->pad_lfo_phase_ + 0.12 / sample_rate);
        double pad_lfo = (std::sin(2.0 * std::numbers::pi_v<double> * self->pad_lfo_phase_) + 1.0) * 0.5;
        double pad = 0.0;
        for (int interval : kChordIntervals) {
            double freq = midi_to_freq(pad_root + interval - 12);
            double phase = wrap_phase(self->pad_phase_ * freq / pad_freq);
            pad += triangle_wave(phase) * (0.75 + pad_lfo * 0.25);
        }
        pad = (pad / static_cast<double>(kChordIntervals.size())) * self->pad_env_ * 0.22;

        double arp_freq = midi_to_freq(pad_root + 12 + (int_sixteenth % 4) * 2);
        self->shimmer_phase_ = wrap_phase(self->shimmer_phase_ + arp_freq / sample_rate);
        double arp = saw_wave(self->shimmer_phase_) * 0.13 * softstep(step_fraction, 5.5);

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
            snare = white * 0.45 * softstep(step_fraction, 20.0);
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
