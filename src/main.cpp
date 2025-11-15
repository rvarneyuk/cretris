#include "core/Game.h"
#include "frontend/ncurses/NcursesFrontend.h"
#include "frontend/sdl/SdlFrontend.h"

#include <chrono>
#include <iostream>
#include <memory>
#include <string>

int main(int argc, char **argv) {
    std::string frontend_name = "sdl";
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--ncurses") {
            frontend_name = "ncurses";
        } else if (arg == "--sdl") {
            frontend_name = "sdl";
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [--sdl|--ncurses]\n";
            return 0;
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            std::cerr << "Use --help for usage information.\n";
            return 1;
        }
    }

    std::unique_ptr<cretris::frontend::Frontend> frontend;
    if (frontend_name == "ncurses") {
        frontend = std::make_unique<cretris::frontend::NcursesFrontend>();
    } else {
        frontend = std::make_unique<cretris::frontend::SdlFrontend>();
    }

    cretris::core::Game game;
    frontend->initialize(game.state());

    using clock = std::chrono::steady_clock;
    auto last_tick = clock::now();
    auto gravity = game.gravity_interval();

    bool running = true;
    while (running) {
        auto action = frontend->poll_input();
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

        frontend->render(game.state());
        frontend->sleep_for(std::chrono::milliseconds{16});
    }

    frontend->shutdown();
    return 0;
}
