#include "Tetromino.h"

#include <algorithm>

namespace cretris::core {

namespace {

constexpr RotationTable make_table(std::initializer_list<std::array<Position, 4>> rotations) {
    RotationTable table{};
    std::size_t idx = 0;
    for (const auto &rot : rotations) {
        table[idx++] = rot;
    }
    return table;
}

constexpr RotationTable I_TABLE = make_table({
    {{{-1, 0}, {0, 0}, {1, 0}, {2, 0}}},
    {{{1, -1}, {1, 0}, {1, 1}, {1, 2}}},
    {{{-1, 1}, {0, 1}, {1, 1}, {2, 1}}},
    {{{0, -1}, {0, 0}, {0, 1}, {0, 2}}},
});

constexpr RotationTable O_TABLE = make_table({
    {{{0, 0}, {1, 0}, {0, 1}, {1, 1}}},
    {{{0, 0}, {1, 0}, {0, 1}, {1, 1}}},
    {{{0, 0}, {1, 0}, {0, 1}, {1, 1}}},
    {{{0, 0}, {1, 0}, {0, 1}, {1, 1}}},
});

constexpr RotationTable T_TABLE = make_table({
    {{{-1, 0}, {0, 0}, {1, 0}, {0, 1}}},
    {{{0, -1}, {0, 0}, {0, 1}, {1, 0}}},
    {{{-1, 0}, {0, 0}, {1, 0}, {0, -1}}},
    {{{0, -1}, {0, 0}, {0, 1}, {-1, 0}}},
});

constexpr RotationTable S_TABLE = make_table({
    {{{0, 0}, {1, 0}, {-1, 1}, {0, 1}}},
    {{{0, -1}, {0, 0}, {1, 0}, {1, 1}}},
    {{{0, 0}, {1, 0}, {-1, 1}, {0, 1}}},
    {{{0, -1}, {0, 0}, {1, 0}, {1, 1}}},
});

constexpr RotationTable Z_TABLE = make_table({
    {{{-1, 0}, {0, 0}, {0, 1}, {1, 1}}},
    {{{0, 1}, {0, 0}, {1, 0}, {1, -1}}},
    {{{1, 0}, {0, 0}, {0, -1}, {-1, -1}}},
    {{{0, -1}, {0, 0}, {-1, 0}, {-1, 1}}},
});

constexpr RotationTable J_TABLE = make_table({
    {{{-1, 0}, {0, 0}, {1, 0}, {-1, 1}}},
    {{{0, -1}, {0, 0}, {0, 1}, {1, -1}}},
    {{{-1, 0}, {0, 0}, {1, 0}, {1, -1}}},
    {{{0, -1}, {0, 0}, {0, 1}, {-1, 1}}},
});

constexpr RotationTable L_TABLE = make_table({
    {{{-1, 0}, {0, 0}, {1, 0}, {1, 1}}},
    {{{0, 1}, {0, 0}, {0, -1}, {1, -1}}},
    {{{1, 0}, {0, 0}, {-1, 0}, {-1, -1}}},
    {{{0, -1}, {0, 0}, {0, 1}, {-1, 1}}},
});

constexpr std::array<RotationTable, static_cast<std::size_t>(TetrominoType::Count)> TABLES = {
    I_TABLE, O_TABLE, T_TABLE, S_TABLE, Z_TABLE, J_TABLE, L_TABLE};

} // namespace

const RotationTable &tetromino_shape(TetrominoType type) {
    return TABLES[static_cast<std::size_t>(type)];
}

BagRandomizer::BagRandomizer(unsigned seed) : rng_{seed} {
    refill();
}

TetrominoType BagRandomizer::next() {
    if (index_ >= bag_.size()) {
        refill();
    }
    return bag_[index_++];
}

void BagRandomizer::refill() {
    for (std::size_t i = 0; i < bag_.size(); ++i) {
        bag_[i] = static_cast<TetrominoType>(i);
    }
    std::shuffle(bag_.begin(), bag_.end(), rng_);
    index_ = 0;
}

} // namespace cretris::core
