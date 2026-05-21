#pragma once

#include <QObject>
#include <QString>
#include <QUrl>

#include "gobang_types.h"

class AIEngine;
class GameBoardWidget;
class NetworkClient;

class GameController : public QObject
{
    Q_OBJECT

public:
    enum class Mode {
        None,
        Friend,
        AI
    };

    explicit GameController(NetworkClient *client, GameBoardWidget *board, QObject *parent = nullptr);

    void startFriendMode();
    void startAiMode(gobang::AiDifficulty difficulty, const QUrl &endpoint);
    void setLocalUser(const QString &user);

private:
    NetworkClient *m_client;
    GameBoardWidget *m_board;
    AIEngine *m_ai = nullptr;
    Mode m_mode = Mode::None;
    QString m_localUser;
    QString m_roomId;
    QString m_opponent;
    gobang::StoneColor m_friendMyColor = gobang::StoneColor::Black;
    gobang::StoneColor m_friendTurn = gobang::StoneColor::Black;
    bool m_friendActive = false;
    bool m_aiMyTurn = true;
    gobang::AiDifficulty m_aiDifficulty = gobang::AiDifficulty::Normal;
    QUrl m_aiEndpoint;

    void setStatusLabel(const QString &text);
    gobang::StoneColor myColor() const;
    void afterMovePlaced(int row, int col, gobang::StoneColor color);
    void handleWin(const QString &winner);
    bool hasFiveAt(const gobang::Board &board, int row, int col) const;
    int currentRound() const;
    QString activeSideText() const;
    void refreshBoardBanner(const QString &prefix = QString());

private slots:
    void onBoardCellClicked(int row, int col);
    void onAiMoveReady(int row, int col);
    void onAiError(const QString &message);
    void onGameStarted(const QString &roomId, const QString &black, const QString &white, const QString &youColor);
    void onOpponentMove(const QString &roomId, int row, int col, const QString &by);
    void onGameOver(const QString &roomId, const QString &winner, const QString &reason);
    void onInviteReceived(const QString &from);
};
