#include "core/Game.h"
#include "frontend/ncurses/NcursesFrontend.h"

#include <chrono>

int main() {
    cretris::core::Game game;
    cretris::frontend::NcursesFrontend frontend;
    frontend.initialize(game.state());

    using clock = std::chrono::steady_clock;
    auto last_tick = clock::now();
    auto gravity = game.gravity_interval();

    bool running = true;
    while (running) {
        auto action = frontend.poll_input();
        if (action == cretris::core::InputAction::Quit) {
            break;
        }
        game.apply_action(action);

        auto now = clock::now();
        gravity = game.gravity_interval();
        if (now - last_tick >= gravity) {
            if (!game.tick()) {
                // allow player to quit after game over
            }
            last_tick = now;
            gravity = game.gravity_interval();
        }

        frontend.render(game.state());
        frontend.sleep_for(std::chrono::milliseconds{16});
    }

    frontend.shutdown();
    return 0;
}
