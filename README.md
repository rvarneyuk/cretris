# cretris

Cretris is a terminal-first Tetris clone written in modern C++20. The gameplay core is independent from any specific presentation layer, enabling multiple front ends. The initial implementation ships with an ncurses renderer, and future front ends (for example SDL) can plug into the same interface.

## Features
- Seven-bag randomizer, scoring, level tracking, and line clearing implemented in the core engine
- Every 20 cleared lines increase the level (capped at level 20) and speed up gravity ticks
- Abstract `Frontend` interface that decouples rendering/input from the game loop
- Ncurses front end with keyboard controls, a rendered next-piece preview, and score/level display

## Building
The project uses CMake and depends on ncurses.

```bash
sudo apt install cmake libncurses5-dev # if not already installed
cmake -S . -B build
cmake --build build
```

## Running
Run the executable from the build directory:

```bash
./build/cretris
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

When adding a new renderer (e.g., SDL), implement the `Frontend` methods and plug it into `main.cpp` in place of the ncurses version.
