#pragma once

#include <QPoint>
#include <QPointF>
#include <QRectF>
#include <QSize>
#include <QString>
#include <QWidget>

class QMouseEvent;
class QPaintEvent;
class QPainter;

#include "gobang_types.h"

class GameBoardWidget : public QWidget
{
    Q_OBJECT

public:
    explicit GameBoardWidget(QWidget *parent = nullptr);

    void resetBoard();
    void setBoard(const gobang::Board &board);
    void setMoveEnabled(bool enabled);
    void setCurrentTurn(gobang::StoneColor color);
    void setLocalColor(gobang::StoneColor color);
    void setGameLabel(const QString &label);
    void setCenterMessage(const QString &message);
    void clearCenterMessage();
    void placeStone(int row, int col, gobang::StoneColor color);
    gobang::Board board() const { return m_board; }
    bool isCellEmpty(int row, int col) const;
    QString gameLabel() const { return m_gameLabel; }

signals:
    void cellClicked(int row, int col);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    QSize minimumSizeHint() const override;

private:
    gobang::Board m_board;
    bool m_moveEnabled = false;
    gobang::StoneColor m_currentTurn = gobang::StoneColor::Black;
    gobang::StoneColor m_localColor = gobang::StoneColor::Black;
    QString m_gameLabel = QString::fromUtf8(u8"\u672a\u5f00\u59cb\u5bf9\u5c40");
    QString m_centerMessage;
    QPoint m_lastMove = {-1, -1};

    QPointF boardOrigin() const;
    qreal spacing() const;
    QRectF boardRect() const;
    bool pointToCell(const QPoint &pos, int *row, int *col) const;
    void drawStone(QPainter &painter, int row, int col, gobang::StoneColor color, bool isLastMove);
};
