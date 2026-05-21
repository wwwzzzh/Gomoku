#include "game_board_widget.h"

#include <QColor>
#include <QFont>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QRadialGradient>

using namespace gobang;

GameBoardWidget::GameBoardWidget(QWidget *parent)
    : QWidget(parent)
    , m_board(makeEmptyBoard())
{
    setMinimumSize(720, 720);
    setMouseTracking(true);
}

void GameBoardWidget::resetBoard()
{
    m_board = makeEmptyBoard();
    m_centerMessage.clear();
    m_lastMove = {-1, -1};
    update();
}

void GameBoardWidget::setBoard(const Board &board)
{
    m_board = board;
    m_lastMove = {-1, -1};
    update();
}

void GameBoardWidget::setMoveEnabled(bool enabled)
{
    m_moveEnabled = enabled;
}

void GameBoardWidget::setCurrentTurn(StoneColor color)
{
    m_currentTurn = color;
    update();
}

void GameBoardWidget::setLocalColor(StoneColor color)
{
    m_localColor = color;
}

void GameBoardWidget::setGameLabel(const QString &label)
{
    m_gameLabel = label;
    update();
}

void GameBoardWidget::setCenterMessage(const QString &message)
{
    m_centerMessage = message;
    update();
}

void GameBoardWidget::clearCenterMessage()
{
    if (m_centerMessage.isEmpty()) {
        return;
    }
    m_centerMessage.clear();
    update();
}

void GameBoardWidget::placeStone(int row, int col, StoneColor color)
{
    if (row < 0 || row >= kBoardSize || col < 0 || col >= kBoardSize) {
        return;
    }
    m_board[row][col] = toCell(color);
    m_lastMove = {col, row};
    update();
}

bool GameBoardWidget::isCellEmpty(int row, int col) const
{
    return row >= 0 && row < kBoardSize && col >= 0 && col < kBoardSize && m_board[row][col] == 0;
}

QSize GameBoardWidget::minimumSizeHint() const
{
    return {720, 720};
}

QPointF GameBoardWidget::boardOrigin() const
{
    const auto rect = boardRect();
    return {rect.left() + 22, rect.top() + 22};
}

qreal GameBoardWidget::spacing() const
{
    return (qMin(width(), height()) - 64.0) / (kBoardSize - 1);
}

QRectF GameBoardWidget::boardRect() const
{
    const qreal side = spacing() * (kBoardSize - 1) + 44;
    const qreal x = (width() - side) / 2.0;
    const qreal y = (height() - side) / 2.0;
    return {x, y, side, side};
}

bool GameBoardWidget::pointToCell(const QPoint &pos, int *row, int *col) const
{
    const auto origin = boardOrigin();
    const qreal step = spacing();
    const qreal x = pos.x() - origin.x();
    const qreal y = pos.y() - origin.y();
    const int c = qRound(x / step);
    const int r = qRound(y / step);
    if (r < 0 || r >= kBoardSize || c < 0 || c >= kBoardSize) {
        return false;
    }
    const qreal dx = qAbs(x - c * step);
    const qreal dy = qAbs(y - r * step);
    if (dx > step * 0.45 || dy > step * 0.45) {
        return false;
    }
    *row = r;
    *col = c;
    return true;
}

void GameBoardWidget::drawStone(QPainter &painter, int row, int col, StoneColor color, bool isLastMove)
{
    const auto origin = boardOrigin();
    const qreal step = spacing();
    const QPointF center = origin + QPointF(col * step, row * step);
    const qreal radius = step * 0.42;

    QRadialGradient gradient(center - QPointF(radius * 0.25, radius * 0.25), radius * 1.2);
    if (color == StoneColor::Black) {
        gradient.setColorAt(0, QColor("#555555"));
        gradient.setColorAt(1, QColor("#101010"));
    } else {
        gradient.setColorAt(0, QColor("#ffffff"));
        gradient.setColorAt(1, QColor("#d7d7d7"));
    }

    painter.setBrush(gradient);
    painter.setPen(QPen(QColor(0, 0, 0, 90), 1.2));
    painter.drawEllipse(center, radius, radius);

    if (isLastMove) {
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(QColor("#ff7a00"), 3));
        painter.drawEllipse(center, radius + 6, radius + 6);
        painter.setPen(QPen(QColor("#ffe08a"), 1.5));
        painter.drawEllipse(center, radius + 10, radius + 10);
    }
}

void GameBoardWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QLinearGradient background(0, 0, 0, height());
    background.setColorAt(0, QColor("#2b2f3a"));
    background.setColorAt(1, QColor("#171a23"));
    painter.fillRect(rect(), background);

    const QRectF board = boardRect();
    QLinearGradient wood(board.topLeft(), board.bottomRight());
    wood.setColorAt(0, QColor("#f6ddb2"));
    wood.setColorAt(1, QColor("#d7a96a"));
    painter.setBrush(wood);
    painter.setPen(QPen(QColor("#9f6e2f"), 2));
    painter.drawRoundedRect(board, 18, 18);

    painter.setPen(QPen(QColor(80, 48, 18, 190), 1.4));
    const auto origin = boardOrigin();
    const qreal step = spacing();
    for (int i = 0; i < kBoardSize; ++i) {
        const qreal pos = origin.x() + i * step;
        painter.drawLine(QPointF(pos, origin.y()), QPointF(pos, origin.y() + step * (kBoardSize - 1)));
        painter.drawLine(QPointF(origin.x(), origin.y() + i * step), QPointF(origin.x() + step * (kBoardSize - 1), origin.y() + i * step));
    }

    const QVector<QPoint> stars = {
        {3, 3}, {3, 11}, {7, 7}, {11, 3}, {11, 11}
    };
    painter.setBrush(QColor(70, 45, 17));
    painter.setPen(Qt::NoPen);
    for (const auto &star : stars) {
        const QPointF center = origin + QPointF(star.x() * step, star.y() * step);
        painter.drawEllipse(center, 5, 5);
    }

    for (int row = 0; row < kBoardSize; ++row) {
        for (int col = 0; col < kBoardSize; ++col) {
            const auto cell = fromCell(m_board[row][col]);
            if (cell != StoneColor::Empty) {
                const bool isLastMove = (m_lastMove.x() == col && m_lastMove.y() == row);
                drawStone(painter, row, col, cell, isLastMove);
            }
        }
    }

    painter.setPen(QPen(QColor("#111111"), 2));
    painter.setBrush(Qt::NoBrush);
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 16, QFont::Bold));
    painter.drawText(board.adjusted(18, 18, -18, -18), Qt::AlignTop | Qt::AlignHCenter, m_gameLabel);

    if (!m_centerMessage.isEmpty()) {
        const QRectF messageRect = board.adjusted(56, board.height() * 0.3, -56, -board.height() * 0.3);
        QPainterPath path;
        path.addRoundedRect(messageRect, 20, 20);
        painter.fillPath(path, QColor(255, 248, 220, 210));
        painter.setPen(QPen(QColor("#111111"), 2));
        painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 28, QFont::Black));
        painter.drawText(messageRect, Qt::AlignCenter, m_centerMessage);
    }
}

void GameBoardWidget::mousePressEvent(QMouseEvent *event)
{
    if (!m_moveEnabled) {
        return;
    }
    int row = -1;
    int col = -1;
    if (pointToCell(event->pos(), &row, &col) && isCellEmpty(row, col)) {
        emit cellClicked(row, col);
    }
}
