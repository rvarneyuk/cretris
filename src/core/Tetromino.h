#pragma once

#include <array>
#include <cstddef>
#include <random>

namespace cretris::core {

struct Position {
    int x{};
    int y{};
};

enum class TetrominoType : std::size_t {
    I,
    O,
    T,
    S,
    Z,
    J,
    L,
    Count
};

enum class Rotation : std::size_t {
    R0 = 0,
    R90,
    R180,
    R270,
    Count
};

struct Tetromino {
    TetrominoType type{};
    Rotation rotation{Rotation::R0};
    Position position{0, 0};
};

using RotationTable = std::array<std::array<Position, 4>, static_cast<std::size_t>(Rotation::Count)>;

const RotationTable &tetromino_shape(TetrominoType type);

class BagRandomizer {
public:
    explicit BagRandomizer(unsigned seed = std::random_device{}());
    TetrominoType next();

private:
    void refill();

    std::mt19937 rng_;
    std::array<TetrominoType, static_cast<std::size_t>(TetrominoType::Count)> bag_{};
    std::size_t index_{bag_.size()};
};

} // namespace cretris::core
