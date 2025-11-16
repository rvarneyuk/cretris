#pragma once

#include "../Frontend.h"
#include "AudioEngine.h"

#include <SDL2/SDL.h>

#include <array>
#include <chrono>
#include <memory>
#include <string>
#include <vector>

namespace cretris::frontend {

class SdlFrontend : public Frontend {
public:
    SdlFrontend() = default;
    ~SdlFrontend() override = default;

    void initialize(const core::GameState &state) override;
    void render(const core::GameState &state) override;
    core::InputAction poll_input() override;
    void shutdown() override;
    void sleep_for(std::chrono::milliseconds duration) override;

private:
    void draw_background();
    void draw_board(const core::GameState &state);
    void draw_next_queue(const core::GameState &state);
    void draw_stats(const core::GameState &state);
    void draw_game_over();
    void render_text(const std::string &text, int x, int y, int scale, SDL_Color color);

    core::GameState last_state_{};
    bool last_state_initialized_{false};
    std::chrono::steady_clock::time_point line_flash_start_{};
    bool line_flash_active_{false};
    int line_flash_count_{0};
    std::vector<int> line_flash_rows_{};

    SDL_Window *window_{nullptr};
    SDL_Renderer *renderer_{nullptr};
    bool initialized_{false};

    std::unique_ptr<AudioEngine> audio_;
};

} // namespace cretris::frontend

