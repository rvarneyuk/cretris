#include "NcursesFrontend.h"

#include <ncurses.h>

#include <string>
#include <thread>

namespace cretris::frontend {

namespace {
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

std::string name_for(core::TetrominoType type) {
    switch (type) {
    case core::TetrominoType::I:
        return "I";
    case core::TetrominoType::O:
        return "O";
    case core::TetrominoType::T:
        return "T";
    case core::TetrominoType::S:
        return "S";
    case core::TetrominoType::Z:
        return "Z";
    case core::TetrominoType::J:
        return "J";
    case core::TetrominoType::L:
        return "L";
    default:
        return "?";
    }
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
    draw_queue(state);
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
}

void NcursesFrontend::draw_queue(const core::GameState &state) {
    int start_y = 2;
    int start_x = core::BOARD_WIDTH * 2 + 6;
    mvprintw(start_y - 1, start_x, "Next:");
    int idx = 0;
    for (const auto &type : state.queue) {
        if (idx >= core::QUEUE_SIZE) {
            break;
        }
        mvprintw(start_y + idx, start_x, "%s", name_for(type).c_str());
        ++idx;
    }
}

void NcursesFrontend::draw_stats(const core::GameState &state) {
    int start_x = core::BOARD_WIDTH * 2 + 6;
    int y = core::QUEUE_SIZE + 4;
    mvprintw(y++, start_x, "Score: %d", state.score);
    mvprintw(y++, start_x, "Lines: %d", state.total_lines);
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
