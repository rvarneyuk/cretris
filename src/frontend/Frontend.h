#pragma once

#include "../core/Game.h"

#include <chrono>

namespace cretris::frontend {

class Frontend {
public:
    virtual ~Frontend() = default;

    virtual void initialize(const core::GameState &state) = 0;
    virtual void render(const core::GameState &state) = 0;
    virtual core::InputAction poll_input() = 0;
    virtual void shutdown() = 0;
    virtual void sleep_for(std::chrono::milliseconds duration) = 0;
};

} // namespace cretris::frontend
