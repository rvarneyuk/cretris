# cretris

Cretris is a modern C++20 Tetris clone with a presentation-agnostic gameplay core. It now ships with a feature-rich SDL2 front end that delivers a neon-inspired board, animated UI chrome, and procedural audio, while still supporting the classic ncurses renderer.

## Features
- Seven-bag randomizer, scoring, level tracking, and line clearing implemented in the core engine
- Every 20 cleared lines increase the level (capped at level 20) and speed up gravity ticks
- Abstract `Frontend` interface that decouples rendering/input from the game loop
- SDL2 front end with smooth animations, a custom pixel font HUD, landing indicator, and procedural music/effects
- Ncurses front end with keyboard controls, a rendered next-piece preview, a landing indicator, and score/level display

## Building
The project uses CMake and depends on SDL2 and ncurses.

```bash
sudo apt install cmake libncurses5-dev libsdl2-dev # if not already installed
cmake -S . -B build
cmake --build build
```

## Running
The SDL2 front end is used by default. Run the executable from the build directory:

```bash
./build/cretris
```

Pass `--ncurses` to opt into the terminal renderer, or `--sdl` to explicitly select the SDL2 mode:

```bash
./build/cretris --ncurses
```

Controls:
- Left/Right arrow or `A`/`D`: move
- Down arrow or `S`: soft drop
- Space: hard drop
- Up arrow or `W`: rotate clockwise
- `Q`: rotate counter-clockwise
- `X`: quit

## Architecture
The codebase is split into two layers:

- `src/core`: platform-independent game state, board handling, tetromino definitions, and scoring logic. `Game` consumes `InputAction` events and exposes the immutable `GameState` for rendering.
- `src/frontend`: the front-end abstraction. Each implementation satisfies the `Frontend` interface; for example `NcursesFrontend` renders text-mode graphics and translates keyboard events to core `InputAction`s.

When adding a new renderer (e.g., SDL), implement the `Frontend` interface and select it via the command-line option.
