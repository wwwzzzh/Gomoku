#include "gobang_server.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

#include "protocol.h"
#include "gobang_types.h"

using namespace gobang;
using namespace gobang::protocol;

GobangServer::GobangServer(QObject *parent)
    : QObject(parent)
    , m_accounts(QDir(QCoreApplication::applicationDirPath()).filePath("gobang_accounts.db"))
{
}

bool GobangServer::start(quint16 port)
{
    QString error;
    if (!m_accounts.initialize(&error)) {
        qWarning() << "Database init failed:" << error;
        return false;
    }

    connect(&m_server, &QTcpServer::newConnection, this, &GobangServer::onNewConnection);
    if (!m_server.listen(QHostAddress::Any, port)) {
        qWarning() << "Listen failed:" << m_server.errorString();
        return false;
    }
    qInfo() << "Gobang server listening on port" << port;
    return true;
}

void GobangServer::onNewConnection()
{
    while (auto *socket = m_server.nextPendingConnection()) {
        connect(socket, &QTcpSocket::readyRead, this, &GobangServer::onReadyRead);
        connect(socket, &QTcpSocket::disconnected, this, &GobangServer::onDisconnected);
        qInfo() << "Client connected from" << socket->peerAddress().toString() << socket->peerPort();
    }
}

QString GobangServer::socketUser(QTcpSocket *socket) const
{
    return m_socketToUser.value(socket);
}

QStringList GobangServer::onlineUsers() const
{
    return m_userToSocket.keys();
}

void GobangServer::sendJson(QTcpSocket *socket, const QJsonObject &message)
{
    if (!socket || socket->state() != QAbstractSocket::ConnectedState) {
        return;
    }
    socket->write(serialize(message));
    socket->flush();
}

void GobangServer::broadcastOnlineUsers()
{
    QJsonObject payload = makeMessage("online_list");
    QJsonArray users;
    for (const auto &user : onlineUsers()) {
        users.append(user);
    }
    payload["users"] = users;
    for (auto *socket : m_socketToUser.keys()) {
        sendJson(socket, payload);
    }
}

QString GobangServer::createRoomId(const QString &a, const QString &b) const
{
    QStringList ordered{a, b};
    ordered.sort();
    return ordered.join("__") + "_" + QString::number(QDateTime::currentMSecsSinceEpoch());
}

void GobangServer::clearUserFromRooms(const QString &username)
{
    const auto roomIds = m_rooms.keys();
    for (const auto &roomId : roomIds) {
        auto room = m_rooms.value(roomId);
        if (room.black == username || room.white == username) {
            const auto opponent = room.black == username ? room.white : room.black;
            if (m_userToSocket.contains(opponent)) {
                QJsonObject msg = makeMessage("game_over");
                msg["room_id"] = room.id;
                msg["winner"] = opponent;
                msg["reason"] = "opponent_disconnected";
                sendJson(m_userToSocket.value(opponent), msg);
            }
            m_rooms.remove(roomId);
        }
    }
}

bool GobangServer::placeStone(Room &room, const QString &username, int row, int col, QString *errorMessage)
{
    if (!room.active) {
        if (errorMessage) *errorMessage = "对局尚未开始";
        return false;
    }
    if (row < 0 || row >= kBoardSize || col < 0 || col >= kBoardSize) {
        if (errorMessage) *errorMessage = "坐标越界";
        return false;
    }
    if (room.board[row][col] != 0) {
        if (errorMessage) *errorMessage = "该位置已被占用";
        return false;
    }
    if (room.currentTurn != username) {
        if (errorMessage) *errorMessage = "还没轮到你";
        return false;
    }
    const auto cell = room.black == username ? 1 : 2;
    room.board[row][col] = cell;
    room.currentTurn = (cell == 1 ? room.white : room.black);
    return true;
}

bool GobangServer::hasFive(const QVector<QVector<int>> &board, int row, int col) const
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

void GobangServer::finishRoom(const QString &roomId, const QString &winner)
{
    if (!m_rooms.contains(roomId)) {
        return;
    }
    const auto room = m_rooms.value(roomId);
    for (const auto &player : {room.black, room.white}) {
        if (m_userToSocket.contains(player)) {
            QJsonObject msg = makeMessage("game_over");
            msg["room_id"] = room.id;
            msg["winner"] = winner;
            msg["reason"] = "win";
            sendJson(m_userToSocket.value(player), msg);
        }
    }
    m_rooms.remove(roomId);
}

void GobangServer::handleMessage(QTcpSocket *socket, const QJsonObject &message)
{
    const QString type = typeOf(message);
    const QString username = socketUser(socket);

    if (type == "register") {
        QString error;
        const bool ok = m_accounts.registerUser(message.value("username").toString(),
                                                message.value("password").toString(),
                                                &error);
        QJsonObject reply = makeMessage("register_reply");
        reply["ok"] = ok;
        reply["message"] = ok ? "注册成功" : error;
        sendJson(socket, reply);
        return;
    }

    if (type == "login") {
        QString error;
        const auto user = message.value("username").toString();
        const auto password = message.value("password").toString();
        const bool ok = m_accounts.validateLogin(user, password, &error);
        QJsonObject reply = makeMessage("login_reply");
        reply["ok"] = ok;
        reply["message"] = ok ? "登录成功" : error;
        if (ok) {
            m_socketToUser[socket] = user;
            m_userToSocket[user] = socket;
            reply["username"] = user;
            QJsonArray users;
            for (const auto &u : onlineUsers()) users.append(u);
            reply["users"] = users;
            broadcastOnlineUsers();
        }
        sendJson(socket, reply);
        return;
    }

    if (username.isEmpty()) {
        QJsonObject reply = makeMessage("error");
        reply["message"] = "请先登录";
        sendJson(socket, reply);
        return;
    }

    if (type == "invite") {
        const QString target = message.value("target").toString();
        if (target.isEmpty() || target == username || !m_userToSocket.contains(target)) {
            QJsonObject reply = makeMessage("invite_result");
            reply["ok"] = false;
            reply["message"] = "目标用户不可用";
            sendJson(socket, reply);
            return;
        }
        m_pendingInvites[target] = username;
        QJsonObject invite = makeMessage("invite");
        invite["from"] = username;
        sendJson(m_userToSocket.value(target), invite);
        return;
    }

    if (type == "invite_reply") {
        const QString from = message.value("from").toString();
        const bool accept = message.value("accept").toBool();
        if (!m_pendingInvites.contains(username) || m_pendingInvites.value(username) != from) {
            return;
        }
        m_pendingInvites.remove(username);
        QJsonObject reply = makeMessage("invite_result");
        reply["ok"] = accept;
        reply["message"] = accept ? "对局已开始" : "对方拒绝了邀请";
        if (!m_userToSocket.contains(from)) {
            return;
        }
        sendJson(m_userToSocket.value(from), reply);
        if (!accept) {
            return;
        }
        Room room;
        room.black = from;
        room.white = username;
        room.currentTurn = room.black;
        room.active = true;
        room.id = createRoomId(room.black, room.white);
        room.board = makeEmptyBoard();
        m_rooms.insert(room.id, room);

        QJsonObject start = makeMessage("game_start");
        start["room_id"] = room.id;
        start["black"] = room.black;
        start["white"] = room.white;
        start["current_turn"] = room.currentTurn;
        start["you_color"] = "black";
        sendJson(m_userToSocket.value(room.black), start);
        start["you_color"] = "white";
        sendJson(m_userToSocket.value(room.white), start);
        return;
    }

    if (type == "move") {
        const QString roomId = message.value("room_id").toString();
        if (!m_rooms.contains(roomId)) {
            return;
        }
        auto room = m_rooms.value(roomId);
        QString error;
        const int row = message.value("row").toInt();
        const int col = message.value("col").toInt();
        if (!placeStone(room, username, row, col, &error)) {
            QJsonObject reply = makeMessage("move_result");
            reply["ok"] = false;
            reply["message"] = error;
            sendJson(socket, reply);
            return;
        }

        m_rooms[roomId] = room;

        QJsonObject relay = makeMessage("opponent_move");
        relay["room_id"] = roomId;
        relay["row"] = row;
        relay["col"] = col;
        relay["by"] = username;
        const auto opponent = room.black == username ? room.white : room.black;
        if (m_userToSocket.contains(opponent)) {
            sendJson(m_userToSocket.value(opponent), relay);
        }

        QJsonObject ack = makeMessage("move_result");
        ack["ok"] = true;
        sendJson(socket, ack);

        if (hasFive(room.board, row, col)) {
            finishRoom(roomId, username);
        }
        return;
    }

    if (type == "game_over") {
        const QString roomId = message.value("room_id").toString();
        const QString winner = message.value("winner").toString();
        finishRoom(roomId, winner);
        return;
    }
}

void GobangServer::onReadyRead()
{
    auto *socket = qobject_cast<QTcpSocket *>(sender());
    if (!socket) {
        return;
    }

    while (socket->canReadLine()) {
        const QByteArray line = socket->readLine().trimmed();
        if (line.isEmpty()) {
            continue;
        }
        QJsonObject message;
        if (!parseLine(line, &message)) {
            continue;
        }
        handleMessage(socket, message);
    }
}

void GobangServer::onDisconnected()
{
    auto *socket = qobject_cast<QTcpSocket *>(sender());
    if (!socket) {
        return;
    }
    const auto user = m_socketToUser.take(socket);
    if (!user.isEmpty()) {
        m_userToSocket.remove(user);
        m_pendingInvites.remove(user);
        clearUserFromRooms(user);
        broadcastOnlineUsers();
    }
    socket->deleteLater();
}
