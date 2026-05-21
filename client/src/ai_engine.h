#pragma once

#include <QObject>
#include <QPair>
#include <QString>
#include <QUrl>

#include "gobang_types.h"

class AIEngine : public QObject
{
    Q_OBJECT

public:
    explicit AIEngine(QObject *parent = nullptr);

    void requestMove(const gobang::Board &board, gobang::AiDifficulty difficulty, const QUrl &endpoint);

signals:
    void moveReady(int row, int col);
    void aiError(const QString &message);

private:
    QPair<int, int> localSearch(const gobang::Board &board, gobang::AiDifficulty difficulty) const;
    QPair<int, int> bestPointAroundCenter(const gobang::Board &board) const;
    int scorePoint(const gobang::Board &board, int row, int col, int cell) const;
};
