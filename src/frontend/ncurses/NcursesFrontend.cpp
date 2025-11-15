#include "NcursesFrontend.h"

#include <ncurses.h>

#include <algorithm>
#include <array>
#include <thread>

namespace cretris::frontend {

namespace {
using Footprint = std::array<bool, core::BOARD_WIDTH>;

short color_for(core::TetrominoType type) {
    switch (type) {
    case core::TetrominoType::I:
        return 1;
    case core::TetrominoType::O:
        return 2;
    case core::TetrominoType::T:
        return 3;
    case core::TetrominoType::S:
        return 4;
    case core::TetrominoType::Z:
        return 5;
    case core::TetrominoType::J:
        return 6;
    case core::TetrominoType::L:
        return 7;
    default:
        return 1;
    }
}

Footprint landing_footprint(const core::GameState &state) {
    Footprint footprint{};
    footprint.fill(false);

    const auto &shape = core::tetromino_shape(state.active_piece.type);
    const auto &cells = shape[static_cast<std::size_t>(state.active_piece.rotation)];

    core::Tetromino projected = state.active_piece;
    while (true) {
        bool collision = false;
        for (const auto &cell : cells) {
            int x = projected.position.x + cell.x;
            int y = projected.position.y + cell.y + 1;
            if (x < 0 || x >= core::BOARD_WIDTH) {
                collision = true;
                break;
            }
            if (y >= core::BOARD_HEIGHT) {
                collision = true;
                break;
            }
            if (y >= 0 && state.board[y][x] != -1) {
                collision = true;
                break;
            }
        }
        if (collision) {
            break;
        }
        projected.position.y += 1;
    }

    for (const auto &cell : cells) {
        int x = projected.position.x + cell.x;
        if (x >= 0 && x < core::BOARD_WIDTH) {
            footprint[static_cast<std::size_t>(x)] = true;
        }
    }

    return footprint;
}

} // namespace

void NcursesFrontend::initialize(const core::GameState &state) {
    if (initialized_) {
        return;
    }
    (void)state;
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, true);
    nodelay(stdscr, true);
    curs_set(0);
    if (has_colors()) {
        start_color();
        use_default_colors();
        init_pair(1, COLOR_CYAN, -1);
        init_pair(2, COLOR_YELLOW, -1);
        init_pair(3, COLOR_MAGENTA, -1);
        init_pair(4, COLOR_GREEN, -1);
        init_pair(5, COLOR_RED, -1);
        init_pair(6, COLOR_BLUE, -1);
        init_pair(7, COLOR_WHITE, -1);
    }
    initialized_ = true;
}

void NcursesFrontend::render(const core::GameState &state) {
    if (!initialized_) {
        return;
    }
    erase();
    draw_board(state);
    draw_next_preview(state);
    draw_stats(state);
    refresh();
}

core::InputAction NcursesFrontend::poll_input() {
    int ch = getch();
    switch (ch) {
    case KEY_LEFT:
    case 'a':
        return core::InputAction::MoveLeft;
    case KEY_RIGHT:
    case 'd':
        return core::InputAction::MoveRight;
    case KEY_DOWN:
    case 's':
        return core::InputAction::SoftDrop;
    case ' ':
        return core::InputAction::HardDrop;
    case 'w':
    case KEY_UP:
        return core::InputAction::RotateCW;
    case 'q':
        return core::InputAction::RotateCCW;
    case 'x':
    case 'Q':
        return core::InputAction::Quit;
    default:
        return core::InputAction::None;
    }
}

void NcursesFrontend::shutdown() {
    if (initialized_) {
        endwin();
        initialized_ = false;
    }
}

void NcursesFrontend::sleep_for(std::chrono::milliseconds duration) {
    std::this_thread::sleep_for(duration);
}

void NcursesFrontend::draw_board(const core::GameState &state) {
    constexpr int offset_x = 2;
    constexpr int offset_y = 1;
    box(stdscr, 0, 0);

    auto buffer = state.board;
    const auto &shape = core::tetromino_shape(state.active_piece.type);
    const auto &cells = shape[static_cast<std::size_t>(state.active_piece.rotation)];
    for (const auto &cell : cells) {
        int x = state.active_piece.position.x + cell.x;
        int y = state.active_piece.position.y + cell.y;
        if (y >= 0 && y < core::BOARD_HEIGHT && x >= 0 && x < core::BOARD_WIDTH) {
            buffer[y][x] = static_cast<int>(state.active_piece.type);
        }
    }

    for (int y = 0; y < core::BOARD_HEIGHT; ++y) {
        for (int x = 0; x < core::BOARD_WIDTH; ++x) {
            int cell = buffer[y][x];
            if (cell == -1) {
                mvaddch(offset_y + y, offset_x + x * 2, '.');
                mvaddch(offset_y + y, offset_x + x * 2 + 1, '.');
            } else {
                short color = color_for(static_cast<core::TetrominoType>(cell));
                attron(COLOR_PAIR(color));
                mvaddch(offset_y + y, offset_x + x * 2, ' ' | A_REVERSE);
                mvaddch(offset_y + y, offset_x + x * 2 + 1, ' ' | A_REVERSE);
                attroff(COLOR_PAIR(color));
            }
        }
    }

    const auto footprint = landing_footprint(state);
    int indicator_y = offset_y + core::BOARD_HEIGHT + 1;
    for (int x = 0; x < core::BOARD_WIDTH; ++x) {
        int screen_x = offset_x + x * 2;
        mvaddch(indicator_y, screen_x, '.');
        mvaddch(indicator_y, screen_x + 1, '.');
    }
    short color = color_for(state.active_piece.type);
    attron(COLOR_PAIR(color));
    for (int x = 0; x < core::BOARD_WIDTH; ++x) {
        if (!footprint[static_cast<std::size_t>(x)]) {
            continue;
        }
        int screen_x = offset_x + x * 2;
        mvaddch(indicator_y, screen_x, ' ' | A_REVERSE);
        mvaddch(indicator_y, screen_x + 1, ' ' | A_REVERSE);
    }
    attroff(COLOR_PAIR(color));
}

void NcursesFrontend::draw_next_preview(const core::GameState &state) {
    int start_y = 2;
    int start_x = core::BOARD_WIDTH * 2 + 6;
    mvprintw(start_y - 1, start_x, "Next:");

    constexpr int preview_cells = 4;
    for (int y = 0; y < preview_cells; ++y) {
        for (int x = 0; x < preview_cells; ++x) {
            mvaddch(start_y + y, start_x + x * 2, '.');
            mvaddch(start_y + y, start_x + x * 2 + 1, '.');
        }
    }

    if (state.queue.empty()) {
        return;
    }

    auto type = state.queue.front();
    const auto &shape = core::tetromino_shape(type);
    const auto &cells = shape[static_cast<std::size_t>(core::Rotation::R0)];

    int min_x = cells[0].x;
    int max_x = cells[0].x;
    int min_y = cells[0].y;
    int max_y = cells[0].y;
    for (const auto &cell : cells) {
        min_x = std::min(min_x, cell.x);
        max_x = std::max(max_x, cell.x);
        min_y = std::min(min_y, cell.y);
        max_y = std::max(max_y, cell.y);
    }

    int width = max_x - min_x + 1;
    int height = max_y - min_y + 1;
    int offset_x = -min_x + (preview_cells - width) / 2;
    int offset_y = -min_y + (preview_cells - height) / 2;

    short color = color_for(type);
    attron(COLOR_PAIR(color));
    for (const auto &cell : cells) {
        int px = cell.x + offset_x;
        int py = cell.y + offset_y;
        if (px >= 0 && px < preview_cells && py >= 0 && py < preview_cells) {
            int screen_x = start_x + px * 2;
            int screen_y = start_y + py;
            mvaddch(screen_y, screen_x, ' ' | A_REVERSE);
            mvaddch(screen_y, screen_x + 1, ' ' | A_REVERSE);
        }
    }
    attroff(COLOR_PAIR(color));
}

void NcursesFrontend::draw_stats(const core::GameState &state) {
    int start_x = core::BOARD_WIDTH * 2 + 6;
    int y = core::QUEUE_SIZE + 4;
    mvprintw(y++, start_x, "Score: %d", state.score);
    mvprintw(y++, start_x, "Lines: %d", state.total_lines);
    mvprintw(y++, start_x, "Level: %d", state.level);
    if (state.level < core::MAX_LEVEL) {
        int remainder = state.total_lines % core::LINES_PER_LEVEL;
        int remaining = core::LINES_PER_LEVEL - remainder;
        mvprintw(y++, start_x, "Next lvl: %d", remaining);
    } else {
        mvprintw(y++, start_x, "Max level reached");
    }
    if (state.game_over) {
        mvprintw(y++, start_x, "GAME OVER (press x)");
    }
    mvprintw(y++, start_x, "Controls:");
    mvprintw(y++, start_x, "Left/Right or A/D");
    mvprintw(y++, start_x, "Down or S: soft drop");
    mvprintw(y++, start_x, "Space: hard drop");
    mvprintw(y++, start_x, "Up/W: rotate");
    mvprintw(y++, start_x, "Q: rotate CCW");
    mvprintw(y++, start_x, "X: quit");
}

} // namespace cretris::frontend
