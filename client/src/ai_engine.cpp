#include "ai_engine.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

using namespace gobang;

AIEngine::AIEngine(QObject *parent)
    : QObject(parent)
{
}

void AIEngine::requestMove(const Board &board, AiDifficulty difficulty, const QUrl &endpoint)
{
    if (endpoint.isValid() && !endpoint.isEmpty()) {
        auto *manager = new QNetworkAccessManager(this);
        QNetworkRequest request(endpoint);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QJsonObject payload;
        payload["difficulty"] = static_cast<int>(difficulty);
        QJsonArray rows;
        for (const auto &row : board) {
            QJsonArray arr;
            for (int cell : row) {
                arr.append(cell);
            }
            rows.append(arr);
        }
        payload["board"] = rows;

        auto *reply = manager->post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
        connect(reply, &QNetworkReply::finished, this, [this, reply, manager]() {
            const auto data = reply->readAll();
            reply->deleteLater();
            manager->deleteLater();
            QJsonParseError error;
            const auto doc = QJsonDocument::fromJson(data, &error);
            if (error.error != QJsonParseError::NoError || !doc.isObject()) {
                emit aiError(QStringLiteral("AI接口返回格式错误"));
                return;
            }
            const auto obj = doc.object();
            const int row = obj.value("row").toInt(-1);
            const int col = obj.value("col").toInt(-1);
            if (row < 0 || col < 0) {
                emit aiError(QStringLiteral("AI接口未返回有效坐标"));
                return;
            }
            emit moveReady(row, col);
        });
        return;
    }

    QTimer::singleShot(300, this, [this, board, difficulty]() {
        const auto move = localSearch(board, difficulty);
        emit moveReady(move.first, move.second);
    });
}

QPair<int, int> AIEngine::bestPointAroundCenter(const Board &board) const
{
    const int center = kBoardSize / 2;
    if (board[center][center] == 0) {
        return {center, center};
    }
    for (int radius = 1; radius < kBoardSize; ++radius) {
        for (int row = center - radius; row <= center + radius; ++row) {
            for (int col = center - radius; col <= center + radius; ++col) {
                if (row >= 0 && row < kBoardSize && col >= 0 && col < kBoardSize && board[row][col] == 0) {
                    return {row, col};
                }
            }
        }
    }
    return {center, center};
}

int AIEngine::scorePoint(const Board &board, int row, int col, int cell) const
{
    if (board[row][col] != 0) {
        return -1;
    }

    const int directions[4][2] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};
    int score = 0;
    for (const auto &direction : directions) {
        int own = 1;
        int block = 0;
        for (int step = 1; step < 5; ++step) {
            const int r = row + direction[0] * step;
            const int c = col + direction[1] * step;
            if (r < 0 || r >= kBoardSize || c < 0 || c >= kBoardSize) {
                ++block;
                break;
            }
            if (board[r][c] == cell) {
                ++own;
            } else if (board[r][c] != 0) {
                ++block;
                break;
            } else {
                break;
            }
        }
        for (int step = 1; step < 5; ++step) {
            const int r = row - direction[0] * step;
            const int c = col - direction[1] * step;
            if (r < 0 || r >= kBoardSize || c < 0 || c >= kBoardSize) {
                ++block;
                break;
            }
            if (board[r][c] == cell) {
                ++own;
            } else if (board[r][c] != 0) {
                ++block;
                break;
            } else {
                break;
            }
        }

        if (own >= 5) {
            score += 100000;
        } else if (own == 4 && block < 2) {
            score += 10000;
        } else if (own == 3 && block < 2) {
            score += 2000;
        } else if (own == 2 && block < 2) {
            score += 200;
        } else {
            score += own * 10 - block * 3;
        }
    }
    return score;
}

QPair<int, int> AIEngine::localSearch(const Board &board, AiDifficulty difficulty) const
{
    const int aiCell = 2;
    const int humanCell = 1;

    if (difficulty == AiDifficulty::Easy) {
        auto point = bestPointAroundCenter(board);
        if (board[point.first][point.second] == 0) {
            return point;
        }
    }

    int bestScore = -1;
    QPair<int, int> bestMove = bestPointAroundCenter(board);

    for (int row = 0; row < kBoardSize; ++row) {
        for (int col = 0; col < kBoardSize; ++col) {
            if (board[row][col] != 0) {
                continue;
            }
            int score = 0;
            score += scorePoint(board, row, col, aiCell);
            if (difficulty != AiDifficulty::Easy) {
                score += scorePoint(board, row, col, humanCell) * 2 / 3;
            }
            if (difficulty == AiDifficulty::Hard) {
                const int distance = qAbs(row - kBoardSize / 2) + qAbs(col - kBoardSize / 2);
                score += qMax(0, 50 - distance * 4);
            }
            if (score > bestScore) {
                bestScore = score;
                bestMove = {row, col};
            }
        }
    }

    return bestMove;
}
