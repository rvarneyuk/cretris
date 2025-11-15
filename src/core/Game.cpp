#include "Game.h"

#include <algorithm>
#include <array>
#include <chrono>

namespace cretris::core {

namespace {
constexpr int spawn_x() { return BOARD_WIDTH / 2 - 1; }
constexpr int spawn_y() { return 0; }

constexpr std::array<int, 5> LINE_CLEAR_SCORES = {0, 100, 300, 500, 800};
constexpr int BASE_GRAVITY_MS = 500;
constexpr int GRAVITY_STEP_MS = 20;
constexpr int MIN_GRAVITY_MS = 100;

int lines_to_score(int lines) {
    if (lines < 0 || static_cast<std::size_t>(lines) >= LINE_CLEAR_SCORES.size()) {
        return 0;
    }
    return LINE_CLEAR_SCORES[static_cast<std::size_t>(lines)];
}

} // namespace

Game::Game() {
    for (auto &row : state_.board) {
        row.fill(-1);
    }
    state_.active_piece.position = {spawn_x(), spawn_y()};
    refill_queue();
    spawn_piece();
}

void Game::apply_action(InputAction action) {
    if (state_.game_over) {
        return;
    }

    switch (action) {
    case InputAction::MoveLeft:
        move_active(-1, 0);
        break;
    case InputAction::MoveRight:
        move_active(1, 0);
        break;
    case InputAction::SoftDrop:
        move_active(0, 1);
        state_.score += 1;
        break;
    case InputAction::HardDrop: {
        int drop = 0;
        Tetromino test = state_.active_piece;
        while (!collides(test)) {
            state_.active_piece = test;
            ++drop;
            test.position.y += 1;
        }
        state_.score += drop * 2;
        lock_piece();
        break;
    }
    case InputAction::RotateCW:
        place_active(state_.active_piece, static_cast<Rotation>((static_cast<std::size_t>(state_.active_piece.rotation) + 1) % static_cast<std::size_t>(Rotation::Count)));
        break;
    case InputAction::RotateCCW:
        place_active(state_.active_piece, static_cast<Rotation>((static_cast<std::size_t>(state_.active_piece.rotation) + static_cast<std::size_t>(Rotation::Count) - 1) % static_cast<std::size_t>(Rotation::Count)));
        break;
    case InputAction::Quit:
    case InputAction::None:
        break;
    }
}

bool Game::tick() {
    if (state_.game_over) {
        return false;
    }

    Tetromino next = state_.active_piece;
    next.position.y += 1;
    if (collides(next)) {
        lock_piece();
    } else {
        state_.active_piece = next;
    }

    return !state_.game_over;
}

bool Game::collides(const Tetromino &tet) const {
    const auto &table = tetromino_shape(tet.type);
    const auto &cells = table[static_cast<std::size_t>(tet.rotation)];
    for (const auto &cell : cells) {
        int x = tet.position.x + cell.x;
        int y = tet.position.y + cell.y;
        if (x < 0 || x >= BOARD_WIDTH || y < 0 || y >= BOARD_HEIGHT) {
            return true;
        }
        if (state_.board[y][x] != -1) {
            return true;
        }
    }
    return false;
}

void Game::lock_piece() {
    const auto &table = tetromino_shape(state_.active_piece.type);
    const auto &cells = table[static_cast<std::size_t>(state_.active_piece.rotation)];
    for (const auto &cell : cells) {
        int x = state_.active_piece.position.x + cell.x;
        int y = state_.active_piece.position.y + cell.y;
        if (y >= 0 && y < BOARD_HEIGHT && x >= 0 && x < BOARD_WIDTH) {
            state_.board[y][x] = static_cast<int>(state_.active_piece.type);
        }
    }
    clear_lines();
    spawn_piece();
}

void Game::spawn_piece() {
    if (state_.queue.empty()) {
        refill_queue();
    }
    state_.active_piece = Tetromino{state_.queue.front(), Rotation::R0, {spawn_x(), spawn_y()}};
    state_.queue.pop_front();
    while (state_.queue.size() < QUEUE_SIZE) {
        state_.queue.push_back(randomizer_.next());
    }

    if (collides(state_.active_piece)) {
        state_.game_over = true;
    }
}

void Game::refill_queue() {
    while (state_.queue.size() < QUEUE_SIZE) {
        state_.queue.push_back(randomizer_.next());
    }
}

void Game::clear_lines() {
    int lines_cleared = 0;
    for (int y = BOARD_HEIGHT - 1; y >= 0; --y) {
        bool full = std::all_of(state_.board[y].begin(), state_.board[y].end(), [](int value) { return value != -1; });
        if (full) {
            ++lines_cleared;
            for (int row = y; row > 0; --row) {
                state_.board[row] = state_.board[row - 1];
            }
            state_.board[0].fill(-1);
            ++y; // re-check same row after collapsing
        }
    }

    if (lines_cleared > 0) {
        state_.total_lines += lines_cleared;
        state_.score += lines_to_score(lines_cleared);

        int computed_level = state_.total_lines / LINES_PER_LEVEL + 1;
        state_.level = std::min(MAX_LEVEL, computed_level);
    }
}

void Game::place_active(Tetromino &target, Rotation new_rotation) {
    Tetromino rotated = target;
    rotated.rotation = new_rotation;
    if (!collides(rotated)) {
        target = rotated;
    }
}

void Game::move_active(int dx, int dy) {
    Tetromino moved = state_.active_piece;
    moved.position.x += dx;
    moved.position.y += dy;
    if (!collides(moved)) {
        state_.active_piece = moved;
    }
}

std::chrono::milliseconds Game::gravity_interval() const {
    int level_offset = std::max(0, state_.level - 1);
    int ms = BASE_GRAVITY_MS - level_offset * GRAVITY_STEP_MS;
    ms = std::max(MIN_GRAVITY_MS, ms);
    return std::chrono::milliseconds{ms};
}

} // namespace cretris::core
