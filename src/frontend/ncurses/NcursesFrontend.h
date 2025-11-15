#pragma once

#include "../Frontend.h"

#include <array>

namespace cretris::frontend {

class NcursesFrontend : public Frontend {
public:
    void initialize(const core::GameState &state) override;
    void render(const core::GameState &state) override;
    core::InputAction poll_input() override;
    void shutdown() override;
    void sleep_for(std::chrono::milliseconds duration) override;

private:
    void draw_board(const core::GameState &state);
    void draw_queue(const core::GameState &state);
    void draw_stats(const core::GameState &state);

    bool initialized_{false};
};

} // namespace cretris::frontend
