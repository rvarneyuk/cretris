#pragma once

#include "Tetromino.h"

#include <array>
#include <chrono>
#include <deque>

namespace cretris::core {

constexpr int BOARD_WIDTH = 10;
constexpr int BOARD_HEIGHT = 20;
constexpr int QUEUE_SIZE = 5;
constexpr int LINES_PER_LEVEL = 20;
constexpr int MAX_LEVEL = 20;

struct GameState {
    std::array<std::array<int, BOARD_WIDTH>, BOARD_HEIGHT> board{}; // -1 empty, else TetrominoType
    Tetromino active_piece{};
    std::deque<TetrominoType> queue{};
    int score{0};
    int total_lines{0};
    int level{1};
    bool game_over{false};
};

enum class InputAction {
    None,
    MoveLeft,
    MoveRight,
    SoftDrop,
    HardDrop,
    RotateCW,
    RotateCCW,
    Quit
};

class Game {
public:
    Game();

    const GameState &state() const noexcept { return state_; }

    void apply_action(InputAction action);
    bool tick(); // gravity tick; returns false on game over
    std::chrono::milliseconds gravity_interval() const;

private:
    bool collides(const Tetromino &tet) const;
    void lock_piece();
    void spawn_piece();
    void refill_queue();
    void clear_lines();
    void place_active(Tetromino &target, Rotation new_rotation);
    void move_active(int dx, int dy);

    GameState state_{};
    BagRandomizer randomizer_{};
};

} // namespace cretris::core
