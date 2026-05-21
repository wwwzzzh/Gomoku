#include "game_controller.h"

#include <QMessageBox>
#include <QStringList>

#include "ai_engine.h"
#include "game_board_widget.h"
#include "network_client.h"

using namespace gobang;

namespace {
QString zh(const char *text)
{
    return QString::fromUtf8(text);
}

bool hasFiveFrom(const Board &board, int row, int col)
{
    const int cell = board[row][col];
    if (cell == 0) {
        return false;
    }

    const int directions[4][2] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};
    for (const auto &direction : directions) {
        int count = 1;
        for (int step = 1; step < 5; ++step) {
            const int r = row + direction[0] * step;
            const int c = col + direction[1] * step;
            if (r < 0 || r >= kBoardSize || c < 0 || c >= kBoardSize || board[r][c] != cell) {
                break;
            }
            ++count;
        }
        for (int step = 1; step < 5; ++step) {
            const int r = row - direction[0] * step;
            const int c = col - direction[1] * step;
            if (r < 0 || r >= kBoardSize || c < 0 || c >= kBoardSize || board[r][c] != cell) {
                break;
            }
            ++count;
        }
        if (count >= 5) {
            return true;
        }
    }
    return false;
}
} // namespace

GameController::GameController(NetworkClient *client, GameBoardWidget *board, QObject *parent)
    : QObject(parent)
    , m_client(client)
    , m_board(board)
    , m_ai(new AIEngine(this))
{
    connect(m_board, &GameBoardWidget::cellClicked, this, &GameController::onBoardCellClicked);
    connect(m_ai, &AIEngine::moveReady, this, &GameController::onAiMoveReady);
    connect(m_ai, &AIEngine::aiError, this, &GameController::onAiError);
    connect(m_client, &NetworkClient::gameStarted, this, &GameController::onGameStarted);
    connect(m_client, &NetworkClient::opponentMoved, this, &GameController::onOpponentMove);
    connect(m_client, &NetworkClient::gameOver, this, &GameController::onGameOver);
    connect(m_client, &NetworkClient::inviteReceived, this, &GameController::onInviteReceived);
}

void GameController::setLocalUser(const QString &user)
{
    m_localUser = user;
}

void GameController::startFriendMode()
{
    m_mode = Mode::Friend;
    m_friendActive = false;
    m_aiMyTurn = false;
    m_roomId.clear();
    m_opponent.clear();
    m_board->resetBoard();
    m_board->setLocalColor(StoneColor::Black);
    m_board->setCurrentTurn(StoneColor::Black);
    m_board->setMoveEnabled(false);
    m_board->clearCenterMessage();
    setStatusLabel(zh(u8"\u597d\u53cb\u5bf9\u6218\uff1a\u7b49\u5f85\u9080\u8bf7\u6216\u8f93\u5165\u597d\u53cb\u8d26\u53f7\u5f00\u59cb"));
}

void GameController::startAiMode(AiDifficulty difficulty, const QUrl &endpoint)
{
    m_mode = Mode::AI;
    m_aiDifficulty = difficulty;
    m_aiEndpoint = endpoint;
    m_roomId.clear();
    m_opponent = QStringLiteral("AI");
    m_friendActive = true;
    m_aiMyTurn = true;
    m_board->resetBoard();
    m_board->setLocalColor(StoneColor::Black);
    m_board->setCurrentTurn(StoneColor::Black);
    m_board->setMoveEnabled(true);
    m_board->clearCenterMessage();
    refreshBoardBanner(zh(u8"\u4eba\u673a\u5bf9\u6218"));
}

StoneColor GameController::myColor() const
{
    return m_friendMyColor;
}

void GameController::setStatusLabel(const QString &text)
{
    m_board->setGameLabel(text);
}

int GameController::currentRound() const
{
    int stoneCount = 0;
    const auto board = m_board->board();
    for (const auto &line : board) {
        for (const int cell : line) {
            if (cell != 0) {
                ++stoneCount;
            }
        }
    }
    return stoneCount / 2 + 1;
}

QString GameController::activeSideText() const
{
    if (m_mode == Mode::AI) {
        return m_aiMyTurn ? zh(u8"\u4f60\u7684\u56de\u5408") : zh(u8"\u5bf9\u65b9\u56de\u5408");
    }

    if (m_mode == Mode::Friend) {
        if (!m_friendActive) {
            return zh(u8"\u7b49\u5f85\u5f00\u59cb");
        }
        return m_friendTurn == myColor() ? zh(u8"\u4f60\u7684\u56de\u5408") : zh(u8"\u5bf9\u65b9\u56de\u5408");
    }

    return zh(u8"\u7b49\u5f85\u5f00\u59cb");
}

void GameController::refreshBoardBanner(const QString &prefix)
{
    QStringList parts;
    if (!prefix.isEmpty()) {
        parts << prefix;
    }
    parts << zh(u8"\u5f53\u524d\u56de\u5408\u6570\uff1a%1").arg(currentRound());
    parts << activeSideText();
    setStatusLabel(parts.join(QStringLiteral("   |   ")));
}

void GameController::afterMovePlaced(int row, int col, StoneColor color)
{
    m_board->placeStone(row, col, color);
    if (m_mode == Mode::Friend) {
        m_client->sendMove(m_roomId, row, col);
        m_friendTurn = opposite(color);
        m_board->setCurrentTurn(m_friendTurn);
        m_board->setMoveEnabled(false);
    }
}

void GameController::handleWin(const QString &winner)
{
    const bool localWinner = winner == m_localUser;
    m_board->setCenterMessage(localWinner ? zh(u8"\u4f60\u8d62\u4e86") : zh(u8"\u4f60\u8f93\u4e86"));
    setStatusLabel(zh(u8"\u5f53\u524d\u56de\u5408\u6570\uff1a%1   |   \u5bf9\u5c40\u7ed3\u675f").arg(currentRound()));
    m_board->setMoveEnabled(false);
    m_friendActive = false;
    m_aiMyTurn = false;
    if (m_mode == Mode::Friend && localWinner) {
        m_client->sendGameOver(m_roomId, winner);
    }
}

bool GameController::hasFiveAt(const Board &board, int row, int col) const
{
    return hasFiveFrom(board, row, col);
}

void GameController::onBoardCellClicked(int row, int col)
{
    m_board->clearCenterMessage();

    if (m_mode == Mode::Friend) {
        if (!m_friendActive || m_friendTurn != myColor() || !m_board->isCellEmpty(row, col)) {
            return;
        }
        afterMovePlaced(row, col, myColor());
        if (hasFiveAt(m_board->board(), row, col)) {
            handleWin(m_localUser);
            return;
        }
        refreshBoardBanner(zh(u8"\u597d\u53cb\u5bf9\u6218"));
        return;
    }

    if (m_mode != Mode::AI || !m_board->isCellEmpty(row, col)) {
        return;
    }

    m_board->placeStone(row, col, StoneColor::Black);
    if (hasFiveAt(m_board->board(), row, col)) {
        handleWin(m_localUser);
        return;
    }
    m_aiMyTurn = false;
    m_board->setCurrentTurn(StoneColor::White);
    m_board->setMoveEnabled(false);
    setStatusLabel(zh(u8"\u5f53\u524d\u56de\u5408\u6570\uff1a%1   |   \u5bf9\u65b9\u56de\u5408\uff08AI\u601d\u8003\u4e2d\uff09").arg(currentRound()));
    m_ai->requestMove(m_board->board(), m_aiDifficulty, m_aiEndpoint);
}

void GameController::onAiMoveReady(int row, int col)
{
    if (m_mode != Mode::AI) {
        return;
    }
    if (!m_board->isCellEmpty(row, col)) {
        setStatusLabel(zh(u8"AI\u843d\u5b50\u65e0\u6548\uff0c\u8bf7\u91cd\u65b0\u64cd\u4f5c"));
        m_aiMyTurn = true;
        m_board->setMoveEnabled(true);
        return;
    }

    m_board->placeStone(row, col, StoneColor::White);
    if (hasFiveAt(m_board->board(), row, col)) {
        handleWin(QStringLiteral("AI"));
        return;
    }
    m_aiMyTurn = true;
    m_board->setCurrentTurn(StoneColor::Black);
    m_board->setMoveEnabled(true);
    refreshBoardBanner(zh(u8"\u4eba\u673a\u5bf9\u6218"));
}

void GameController::onAiError(const QString &message)
{
    if (m_mode != Mode::AI) {
        return;
    }
    m_aiMyTurn = true;
    setStatusLabel(zh(u8"AI \u51fa\u9519\uff1a%1").arg(message));
    m_board->setMoveEnabled(true);
}

void GameController::onGameStarted(const QString &roomId, const QString &black, const QString &white, const QString &youColor)
{
    m_mode = Mode::Friend;
    m_roomId = roomId;
    m_opponent = (black == m_localUser) ? white : black;
    m_friendMyColor = youColor == QStringLiteral("white") ? StoneColor::White : StoneColor::Black;
    m_friendTurn = StoneColor::Black;
    m_friendActive = true;
    m_aiMyTurn = false;
    m_board->resetBoard();
    m_board->setLocalColor(myColor());
    m_board->setCurrentTurn(m_friendTurn);
    m_board->setMoveEnabled(m_friendTurn == myColor());
    m_board->clearCenterMessage();
    refreshBoardBanner(zh(u8"\u597d\u53cb\u5bf9\u6218"));
}

void GameController::onOpponentMove(const QString &roomId, int row, int col, const QString &by)
{
    if (m_mode != Mode::Friend || roomId != m_roomId) {
        return;
    }
    const auto color = by == m_localUser ? myColor() : opposite(myColor());
    m_board->placeStone(row, col, color);
    if (hasFiveAt(m_board->board(), row, col)) {
        handleWin(by);
        return;
    }
    m_friendTurn = myColor();
    m_board->setCurrentTurn(myColor());
    m_board->setMoveEnabled(true);
    refreshBoardBanner(zh(u8"\u597d\u53cb\u5bf9\u6218"));
}

void GameController::onGameOver(const QString &roomId, const QString &winner, const QString &reason)
{
    if (m_mode == Mode::Friend && roomId != m_roomId) {
        return;
    }
    Q_UNUSED(reason);
    m_friendActive = false;
    handleWin(winner);
}

void GameController::onInviteReceived(const QString &from)
{
    const auto ret = QMessageBox::question(nullptr,
                                           zh(u8"\u597d\u53cb\u5bf9\u6218\u9080\u8bf7"),
                                           zh(u8"%1 \u9080\u8bf7\u4f60\u8fdb\u884c\u597d\u53cb\u5bf9\u6218\uff0c\u662f\u5426\u63a5\u53d7\uff1f").arg(from),
                                           QMessageBox::Yes | QMessageBox::No,
                                           QMessageBox::Yes);
    m_client->replyInvite(from, ret == QMessageBox::Yes);
}
