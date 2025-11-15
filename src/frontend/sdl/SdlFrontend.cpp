#include "SdlFrontend.h"

#include <SDL2/SDL.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <cmath>
#include <initializer_list>
#include <numbers>
#include <string>
#include <unordered_map>
#include <vector>

namespace cretris::frontend {

namespace {

constexpr int WINDOW_WIDTH = 1280;
constexpr int WINDOW_HEIGHT = 720;
constexpr int TILE_SIZE = 30;
constexpr int BOARD_WIDTH_PX = core::BOARD_WIDTH * TILE_SIZE;
constexpr int BOARD_HEIGHT_PX = core::BOARD_HEIGHT * TILE_SIZE;
constexpr int BOARD_ORIGIN_X = 140;
constexpr int BOARD_ORIGIN_Y = 50;
constexpr int FONT_WIDTH = 5;
constexpr int FONT_HEIGHT = 5;

struct Glyph {
    std::array<uint8_t, FONT_HEIGHT> rows{};
};

Glyph glyph_from_strings(std::initializer_list<const char *> pattern) {
    Glyph glyph{};
    int row = 0;
    for (const char *line : pattern) {
        uint8_t bits = 0;
        for (int col = 0; col < FONT_WIDTH && line[col] != '\0'; ++col) {
            if (line[col] != ' ') {
                bits |= static_cast<uint8_t>(1u << (FONT_WIDTH - 1 - col));
            }
        }
        if (row < FONT_HEIGHT) {
            glyph.rows[static_cast<std::size_t>(row)] = bits;
        }
        ++row;
    }
    return glyph;
}

const std::unordered_map<char, Glyph> &glyph_map() {
    static const std::unordered_map<char, Glyph> map = {
        {'A', glyph_from_strings({"  #  ", " # # ", "#####", "#   #", "#   #"})},
        {'B', glyph_from_strings({"#### ", "#   #", "#### ", "#   #", "#### "})},
        {'C', glyph_from_strings({" ####", "#    ", "#    ", "#    ", " ####"})},
        {'D', glyph_from_strings({"###  ", "#  # ", "#   #", "#  # ", "###  "})},
        {'E', glyph_from_strings({"#####", "#    ", "#### ", "#    ", "#####"})},
        {'F', glyph_from_strings({"#####", "#    ", "#### ", "#    ", "#    "})},
        {'G', glyph_from_strings({" ####", "#    ", "# ###", "#   #", " ####"})},
        {'H', glyph_from_strings({"#   #", "#   #", "#####", "#   #", "#   #"})},
        {'I', glyph_from_strings({"#####", "  #  ", "  #  ", "  #  ", "#####"})},
        {'J', glyph_from_strings({"  ###", "   # ", "   # ", "#  # ", " ##  "})},
        {'K', glyph_from_strings({"#   #", "#  # ", "###  ", "#  # ", "#   #"})},
        {'L', glyph_from_strings({"#    ", "#    ", "#    ", "#    ", "#####"})},
        {'M', glyph_from_strings({"#   #", "## ##", "# # #", "#   #", "#   #"})},
        {'N', glyph_from_strings({"#   #", "##  #", "# # #", "#  ##", "#   #"})},
        {'O', glyph_from_strings({" ### ", "#   #", "#   #", "#   #", " ### "})},
        {'P', glyph_from_strings({"#### ", "#   #", "#### ", "#    ", "#    "})},
        {'Q', glyph_from_strings({" ### ", "#   #", "#   #", "#  ##", " ####"})},
        {'R', glyph_from_strings({"#### ", "#   #", "#### ", "#  # ", "#   #"})},
        {'S', glyph_from_strings({" ####", "#    ", " ### ", "    #", "#### "})},
        {'T', glyph_from_strings({"#####", "  #  ", "  #  ", "  #  ", "  #  "})},
        {'U', glyph_from_strings({"#   #", "#   #", "#   #", "#   #", " ### "})},
        {'V', glyph_from_strings({"#   #", "#   #", "#   #", " # # ", "  #  "})},
        {'W', glyph_from_strings({"#   #", "#   #", "# # #", "## ##", "#   #"})},
        {'X', glyph_from_strings({"#   #", " # # ", "  #  ", " # # ", "#   #"})},
        {'Y', glyph_from_strings({"#   #", " # # ", "  #  ", "  #  ", "  #  "})},
        {'Z', glyph_from_strings({"#####", "   # ", "  #  ", " #   ", "#####"})},
        {'0', glyph_from_strings({" ### ", "#  ##", "# # #", "##  #", " ### "})},
        {'1', glyph_from_strings({"  #  ", " ##  ", "  #  ", "  #  ", " ### "})},
        {'2', glyph_from_strings({" ### ", "#   #", "   # ", "  #  ", "#####"})},
        {'3', glyph_from_strings({" ### ", "    #", " ### ", "    #", " ### "})},
        {'4', glyph_from_strings({"#   #", "#   #", "#####", "    #", "    #"})},
        {'5', glyph_from_strings({"#####", "#    ", "#### ", "    #", "#### "})},
        {'6', glyph_from_strings({" ####", "#    ", "#### ", "#   #", " ### "})},
        {'7', glyph_from_strings({"#####", "    #", "   # ", "  #  ", "  #  "})},
        {'8', glyph_from_strings({" ### ", "#   #", " ### ", "#   #", " ### "})},
        {'9', glyph_from_strings({" ### ", "#   #", " ####", "    #", " ### "})},
        {':', glyph_from_strings({"     ", "  #  ", "     ", "  #  ", "     "})},
        {'-', glyph_from_strings({"     ", "     ", "#####", "     ", "     "})}
    };
    return map;
}

const Glyph *glyph_for(char c) {
    if (c == ' ') {
        return nullptr;
    }
    char upper = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    const auto &map = glyph_map();
    auto it = map.find(upper);
    if (it == map.end()) {
        return nullptr;
    }
    return &it->second;
}

std::array<SDL_Color, static_cast<std::size_t>(core::TetrominoType::Count)> palette() {
    return {SDL_Color{0, 230, 255, 255}, SDL_Color{255, 221, 0, 255}, SDL_Color{220, 0, 255, 255},
            SDL_Color{0, 232, 125, 255}, SDL_Color{255, 70, 90, 255}, SDL_Color{70, 100, 255, 255},
            SDL_Color{255, 150, 40, 255}};
}

struct PieceCells {
    std::array<SDL_Point, 4> cells{};
};

PieceCells compute_cells(const core::Tetromino &piece) {
    PieceCells result{};
    const auto &shape = core::tetromino_shape(piece.type);
    const auto &mask = shape[static_cast<std::size_t>(piece.rotation)];
    for (std::size_t i = 0; i < mask.size(); ++i) {
        result.cells[i] = SDL_Point{piece.position.x + mask[i].x, piece.position.y + mask[i].y};
    }
    return result;
}

bool collides_with_board(const core::GameState &state, const core::Tetromino &piece) {
    const auto &shape = core::tetromino_shape(piece.type);
    const auto &mask = shape[static_cast<std::size_t>(piece.rotation)];
    for (const auto &cell : mask) {
        int x = piece.position.x + cell.x;
        int y = piece.position.y + cell.y;
        if (x < 0 || x >= core::BOARD_WIDTH || y >= core::BOARD_HEIGHT) {
            return true;
        }
        if (y >= 0 && state.board[y][x] != -1) {
            return true;
        }
    }
    return false;
}

std::vector<SDL_Point> compute_ghost(const core::GameState &state) {
    auto ghost = state.active_piece;
    while (true) {
        ghost.position.y += 1;
        if (collides_with_board(state, ghost)) {
            ghost.position.y -= 1;
            break;
        }
    }
    std::vector<SDL_Point> cells;
    const auto footprint = compute_cells(ghost);
    for (const auto &cell : footprint.cells) {
        if (cell.y >= 0 && cell.y < core::BOARD_HEIGHT) {
            cells.push_back(cell);
        }
    }
    return cells;
}

} // namespace

struct AudioEngine {
    void initialize() {
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

    void shutdown() {
        if (device_ != 0) {
            SDL_CloseAudioDevice(device_);
            device_ = 0;
        }
    }

    void trigger_line_clear() { line_pulse_.store(1.0f); }
    void trigger_hard_drop() { drop_pulse_.store(0.8f); }
    void set_level_progress(float progress) { tempo_mod_.store(std::clamp(progress, 0.0f, 1.0f)); }

private:
    static void audio_callback(void *userdata, Uint8 *stream, int len) {
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

    SDL_AudioDeviceID device_{0};
    SDL_AudioSpec obtained_{};
    double song_time_{0.0};
    double bass_phase_{0.0};
    double shimmer_phase_{0.0};
    double drop_phase_{0.0};
    double line_phase_{0.0};
    std::atomic<float> line_pulse_{0.0f};
    std::atomic<float> drop_pulse_{0.0f};
    std::atomic<float> tempo_mod_{0.0f};
};

void SdlFrontend::initialize(const core::GameState &state) {
    if (initialized_) {
        last_state_ = state;
        last_state_initialized_ = true;
        return;
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0) {
        SDL_Log("SDL init failed: %s", SDL_GetError());
        return;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
    window_ = SDL_CreateWindow("Cretris", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT,
                               SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window_) {
        SDL_Log("Window creation failed: %s", SDL_GetError());
        SDL_Quit();
        return;
    }

    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer_) {
        SDL_Log("Renderer creation failed: %s", SDL_GetError());
        SDL_DestroyWindow(window_);
        window_ = nullptr;
        SDL_Quit();
        return;
    }
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);

    last_state_ = state;
    last_state_initialized_ = true;
    initialized_ = true;

    audio_ = std::make_unique<AudioEngine>();
    audio_->initialize();
}

void SdlFrontend::render(const core::GameState &state) {
    if (!initialized_ || !renderer_) {
        return;
    }

    draw_background();
    draw_board(state);
    draw_next_queue(state);
    draw_stats(state);
    if (state.game_over) {
        draw_game_over();
    }
    SDL_RenderPresent(renderer_);

    if (audio_) {
        float normalized = 0.0f;
        if (core::MAX_LEVEL > 1) {
            normalized = static_cast<float>(state.level - 1) / static_cast<float>(core::MAX_LEVEL - 1);
        }
        normalized = std::clamp(normalized, 0.0f, 1.0f);
        audio_->set_level_progress(normalized);
        if (last_state_initialized_ && state.total_lines > last_state_.total_lines) {
            audio_->trigger_line_clear();
        }
    }
    last_state_ = state;
    last_state_initialized_ = true;
}

core::InputAction SdlFrontend::poll_input() {
    if (!initialized_) {
        return core::InputAction::None;
    }

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            return core::InputAction::Quit;
        }
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE) {
            return core::InputAction::Quit;
        }
        if (event.type == SDL_KEYDOWN && !event.key.repeat) {
            switch (event.key.keysym.sym) {
            case SDLK_LEFT:
            case SDLK_a:
                return core::InputAction::MoveLeft;
            case SDLK_RIGHT:
            case SDLK_d:
                return core::InputAction::MoveRight;
            case SDLK_DOWN:
            case SDLK_s:
                return core::InputAction::SoftDrop;
            case SDLK_SPACE:
                if (audio_) {
                    audio_->trigger_hard_drop();
                }
                return core::InputAction::HardDrop;
            case SDLK_UP:
            case SDLK_w:
                return core::InputAction::RotateCW;
            case SDLK_q:
                return core::InputAction::RotateCCW;
            case SDLK_ESCAPE:
            case SDLK_x:
                return core::InputAction::Quit;
            default:
                break;
            }
        }
    }
    return core::InputAction::None;
}

void SdlFrontend::shutdown() {
    if (audio_) {
        audio_->shutdown();
        audio_.reset();
    }
    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    initialized_ = false;
    last_state_initialized_ = false;
    SDL_Quit();
}

void SdlFrontend::sleep_for(std::chrono::milliseconds duration) { SDL_Delay(static_cast<Uint32>(duration.count())); }

void SdlFrontend::draw_background() {
    SDL_Rect viewport;
    SDL_RenderGetViewport(renderer_, &viewport);
    for (int y = 0; y < viewport.h; ++y) {
        float t = static_cast<float>(y) / static_cast<float>(viewport.h);
        SDL_Color color{static_cast<Uint8>(5 + 20 * t), static_cast<Uint8>(10 + 40 * t), static_cast<Uint8>(35 + 120 * t),
                        255};
        SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
        SDL_RenderDrawLine(renderer_, 0, y, viewport.w, y);
    }

    SDL_SetRenderDrawColor(renderer_, 255, 255, 255, 12);
    for (int x = 0; x < viewport.w; x += 20) {
        SDL_RenderDrawLine(renderer_, x, 0, x, viewport.h);
    }
    SDL_SetRenderDrawColor(renderer_, 255, 255, 255, 22);
    for (int y = 0; y < viewport.h; y += 20) {
        SDL_RenderDrawLine(renderer_, 0, y, viewport.w, y);
    }
}

void SdlFrontend::draw_board(const core::GameState &state) {
    auto buffer = state.board;
    const auto &shape = core::tetromino_shape(state.active_piece.type);
    const auto &mask = shape[static_cast<std::size_t>(state.active_piece.rotation)];
    for (const auto &cell : mask) {
        int x = state.active_piece.position.x + cell.x;
        int y = state.active_piece.position.y + cell.y;
        if (x >= 0 && x < core::BOARD_WIDTH && y >= 0 && y < core::BOARD_HEIGHT) {
            buffer[y][x] = static_cast<int>(state.active_piece.type);
        }
    }

    SDL_Rect panel{BOARD_ORIGIN_X - 35, BOARD_ORIGIN_Y - 35, BOARD_WIDTH_PX + 70, BOARD_HEIGHT_PX + 70};
    SDL_SetRenderDrawColor(renderer_, 10, 10, 22, 220);
    SDL_RenderFillRect(renderer_, &panel);
    SDL_SetRenderDrawColor(renderer_, 255, 255, 255, 60);
    SDL_RenderDrawRect(renderer_, &panel);

    auto colors = palette();
    for (int y = 0; y < core::BOARD_HEIGHT; ++y) {
        for (int x = 0; x < core::BOARD_WIDTH; ++x) {
            SDL_Rect shadow{BOARD_ORIGIN_X + x * TILE_SIZE + 4, BOARD_ORIGIN_Y + y * TILE_SIZE + 4, TILE_SIZE - 2,
                            TILE_SIZE - 2};
            SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 70);
            SDL_RenderFillRect(renderer_, &shadow);

            int cell = buffer[y][x];
            SDL_Rect rect{BOARD_ORIGIN_X + x * TILE_SIZE, BOARD_ORIGIN_Y + y * TILE_SIZE, TILE_SIZE - 4, TILE_SIZE - 4};
            if (cell == -1) {
                SDL_SetRenderDrawColor(renderer_, 255, 255, 255, 12);
                SDL_RenderDrawRect(renderer_, &rect);
            } else {
                const auto &color = colors[static_cast<std::size_t>(cell)];
                SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, 255);
                SDL_RenderFillRect(renderer_, &rect);
                SDL_SetRenderDrawColor(renderer_, 255, 255, 255, 60);
                SDL_Rect highlight = rect;
                highlight.h /= 3;
                SDL_RenderFillRect(renderer_, &highlight);
            }
        }
    }

    auto ghost = compute_ghost(state);
    SDL_Color ghost_color = colors[static_cast<std::size_t>(state.active_piece.type)];
    SDL_SetRenderDrawColor(renderer_, ghost_color.r, ghost_color.g, ghost_color.b, 80);
    for (const auto &cell : ghost) {
        SDL_Rect rect{BOARD_ORIGIN_X + cell.x * TILE_SIZE, BOARD_ORIGIN_Y + cell.y * TILE_SIZE, TILE_SIZE - 4,
                      TILE_SIZE - 4};
        SDL_RenderDrawRect(renderer_, &rect);
    }
}

void SdlFrontend::draw_next_queue(const core::GameState &state) {
    auto colors = palette();
    int block_size = TILE_SIZE - 6;
    int box_x = BOARD_ORIGIN_X + BOARD_WIDTH_PX + 60;
    int box_y = BOARD_ORIGIN_Y;
    SDL_Rect backdrop{box_x - 20, box_y - 20, 260, BOARD_HEIGHT_PX};
    SDL_SetRenderDrawColor(renderer_, 8, 8, 25, 200);
    SDL_RenderFillRect(renderer_, &backdrop);
    SDL_SetRenderDrawColor(renderer_, 255, 255, 255, 50);
    SDL_RenderDrawRect(renderer_, &backdrop);

    render_text("NEXT", box_x, box_y - 10, 3, SDL_Color{255, 255, 255, 255});

    int offset_y = box_y + 50;
    int preview_count = std::min(static_cast<int>(state.queue.size()), 3);
    for (int i = 0; i < preview_count; ++i) {
        auto type = state.queue[static_cast<std::size_t>(i)];
        const auto &shape = core::tetromino_shape(type);
        const auto &mask = shape[static_cast<std::size_t>(core::Rotation::R0)];
        int min_x = mask[0].x, max_x = mask[0].x, min_y = mask[0].y, max_y = mask[0].y;
        for (const auto &cell : mask) {
            min_x = std::min(min_x, cell.x);
            max_x = std::max(max_x, cell.x);
            min_y = std::min(min_y, cell.y);
            max_y = std::max(max_y, cell.y);
        }
        int width = max_x - min_x + 1;
        int height = max_y - min_y + 1;
        int local_origin_x = box_x + (180 - width * block_size) / 2;
        int local_origin_y = offset_y + (120 - height * block_size) / 2;
        SDL_SetRenderDrawColor(renderer_, 255, 255, 255, 18);
        SDL_Rect frame{box_x, offset_y, 180, 120};
        SDL_RenderDrawRect(renderer_, &frame);

        const auto &color = colors[static_cast<std::size_t>(type)];
        for (const auto &cell : mask) {
            int px = (cell.x - min_x) * block_size;
            int py = (cell.y - min_y) * block_size;
            SDL_Rect rect{local_origin_x + px, local_origin_y + py, block_size - 4, block_size - 4};
            SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, 255);
            SDL_RenderFillRect(renderer_, &rect);
        }
        offset_y += 140;
    }
}

void SdlFrontend::draw_stats(const core::GameState &state) {
    int text_x = BOARD_ORIGIN_X;
    int text_y = BOARD_ORIGIN_Y + BOARD_HEIGHT_PX + 20;
    SDL_Color label{255, 255, 255, 255};
    SDL_Color accent{255, 180, 40, 255};
    render_text("SCORE", text_x, text_y, 3, label);
    render_text(std::to_string(state.score), text_x, text_y + 32, 4, accent);

    render_text("LINES", text_x + 280, text_y, 3, label);
    render_text(std::to_string(state.total_lines), text_x + 280, text_y + 32, 4, accent);

    render_text("LEVEL", text_x + 520, text_y, 3, label);
    render_text(std::to_string(state.level), text_x + 520, text_y + 32, 4, accent);

    int controls_x = BOARD_ORIGIN_X + BOARD_WIDTH_PX + 60;
    int controls_y = BOARD_ORIGIN_Y + BOARD_HEIGHT_PX - 120;
    render_text("CONTROLS", controls_x, controls_y, 3, label);
    controls_y += 34;
    render_text("ARROWS MOVE", controls_x, controls_y, 2, accent);
    controls_y += 28;
    render_text("S OR DOWN DROP", controls_x, controls_y, 2, accent);
    controls_y += 28;
    render_text("SPACE HARD", controls_x, controls_y, 2, accent);
    controls_y += 28;
    render_text("W OR UP ROT", controls_x, controls_y, 2, accent);
    controls_y += 28;
    render_text("Q CCW", controls_x, controls_y, 2, accent);
    controls_y += 28;
    render_text("X OR ESC QUIT", controls_x, controls_y, 2, accent);

    int progress_x = BOARD_ORIGIN_X + BOARD_WIDTH_PX + 60;
    int progress_y = BOARD_ORIGIN_Y + BOARD_HEIGHT_PX - 200;
    render_text("NEXT LVL", progress_x, progress_y, 3, label);
    float progress = 0.0f;
    if (state.level < core::MAX_LEVEL) {
        int remainder = state.total_lines % core::LINES_PER_LEVEL;
        progress = static_cast<float>(remainder) / static_cast<float>(core::LINES_PER_LEVEL);
    } else {
        progress = 1.0f;
    }
    SDL_Rect bar{progress_x, progress_y + 40, 180, 14};
    SDL_SetRenderDrawColor(renderer_, 255, 255, 255, 40);
    SDL_RenderDrawRect(renderer_, &bar);
    SDL_Rect fill = bar;
    fill.w = static_cast<int>(bar.w * progress);
    SDL_SetRenderDrawColor(renderer_, 0, 230, 180, 180);
    SDL_RenderFillRect(renderer_, &fill);
}

void SdlFrontend::draw_game_over() {
    SDL_Rect overlay{BOARD_ORIGIN_X, BOARD_ORIGIN_Y + BOARD_HEIGHT_PX / 2 - 80, BOARD_WIDTH_PX, 160};
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 180);
    SDL_RenderFillRect(renderer_, &overlay);
    render_text("GAME OVER", overlay.x + 30, overlay.y + 30, 4, SDL_Color{255, 90, 110, 255});
    render_text("PRESS X TO EXIT", overlay.x + 30, overlay.y + 90, 2, SDL_Color{255, 255, 255, 255});
}

void SdlFrontend::render_text(const std::string &text, int x, int y, int scale, SDL_Color color) {
    int cursor_x = x;
    int cursor_y = y;
    for (char ch : text) {
        if (ch == '\n') {
            cursor_y += (FONT_HEIGHT + 1) * scale;
            cursor_x = x;
            continue;
        }
        if (ch == ' ') {
            cursor_x += (FONT_WIDTH + 1) * scale;
            continue;
        }
        const Glyph *glyph = glyph_for(ch);
        if (!glyph) {
            cursor_x += (FONT_WIDTH + 1) * scale;
            continue;
        }
        SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
        for (int row = 0; row < FONT_HEIGHT; ++row) {
            uint8_t bits = glyph->rows[static_cast<std::size_t>(row)];
            for (int col = 0; col < FONT_WIDTH; ++col) {
                if ((bits >> (FONT_WIDTH - 1 - col)) & 0x1) {
                    SDL_Rect pixel{cursor_x + col * scale, cursor_y + row * scale, scale, scale};
                    SDL_RenderFillRect(renderer_, &pixel);
                }
            }
        }
        cursor_x += (FONT_WIDTH + 1) * scale;
    }
}

} // namespace cretris::frontend

