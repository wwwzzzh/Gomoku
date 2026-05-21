#pragma once

#include <QVector>

namespace gobang {

constexpr int kBoardSize = 15;

enum class StoneColor {
    Empty = 0,
    Black = 1,
    White = 2
};

enum class AiDifficulty {
    Easy = 0,
    Normal = 1,
    Hard = 2
};

using Board = QVector<QVector<int>>;

inline Board makeEmptyBoard()
{
    return Board(kBoardSize, QVector<int>(kBoardSize, 0));
}

inline int toCell(StoneColor color)
{
    return static_cast<int>(color);
}

inline StoneColor fromCell(int cell)
{
    return static_cast<StoneColor>(cell);
}

inline StoneColor opposite(StoneColor color)
{
    return color == StoneColor::Black ? StoneColor::White : StoneColor::Black;
}

} // namespace gobang
