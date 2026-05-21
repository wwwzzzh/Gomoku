#include "network_client.h"

#include <QJsonArray>
#include <QJsonDocument>

#include "protocol.h"

using namespace gobang::protocol;

NetworkClient::NetworkClient(QObject *parent)
    : QObject(parent)
{
    connect(&m_socket, &QTcpSocket::connected, this, [this]() {
        emit connectedChanged(true);
    });
    connect(&m_socket, &QTcpSocket::disconnected, this, [this]() {
        m_currentUser.clear();
        m_buffer.clear();
        emit connectedChanged(false);
    });
    connect(&m_socket, &QTcpSocket::readyRead, this, &NetworkClient::onReadyRead);
}

void NetworkClient::connectToServer(const QString &host, quint16 port)
{
    if (m_socket.state() != QAbstractSocket::UnconnectedState) {
        return;
    }
    m_socket.connectToHost(host, port);
}

void NetworkClient::disconnectFromServer()
{
    m_socket.disconnectFromHost();
}

bool NetworkClient::isConnected() const
{
    return m_socket.state() == QAbstractSocket::ConnectedState;
}

void NetworkClient::sendJson(const QJsonObject &obj)
{
    m_socket.write(serialize(obj));
    m_socket.flush();
}

void NetworkClient::login(const QString &username, const QString &password)
{
    QJsonObject obj = makeMessage("login");
    obj["username"] = username;
    obj["password"] = password;
    sendJson(obj);
}

void NetworkClient::registerAccount(const QString &username, const QString &password)
{
    QJsonObject obj = makeMessage("register");
    obj["username"] = username;
    obj["password"] = password;
    sendJson(obj);
}

void NetworkClient::invitePlayer(const QString &target)
{
    QJsonObject obj = makeMessage("invite");
    obj["target"] = target;
    sendJson(obj);
}

void NetworkClient::replyInvite(const QString &from, bool accept)
{
    QJsonObject obj = makeMessage("invite_reply");
    obj["from"] = from;
    obj["accept"] = accept;
    sendJson(obj);
}

void NetworkClient::sendMove(const QString &roomId, int row, int col)
{
    QJsonObject obj = makeMessage("move");
    obj["room_id"] = roomId;
    obj["row"] = row;
    obj["col"] = col;
    sendJson(obj);
}

void NetworkClient::sendGameOver(const QString &roomId, const QString &winner)
{
    QJsonObject obj = makeMessage("game_over");
    obj["room_id"] = roomId;
    obj["winner"] = winner;
    sendJson(obj);
}

void NetworkClient::handleJson(const QJsonObject &obj)
{
    const auto type = typeOf(obj);
    if (type == "login_reply") {
        const bool ok = obj.value("ok").toBool();
        const QString username = obj.value("username").toString();
        const QStringList users = [&]() {
            QStringList out;
            for (const auto &value : obj.value("users").toArray()) {
                out << value.toString();
            }
            return out;
        }();
        if (ok) {
            m_currentUser = username;
        }
        emit loginResult(ok, obj.value("message").toString(), username, users);
        return;
    }
    if (type == "register_reply") {
        emit registerResult(obj.value("ok").toBool(), obj.value("message").toString());
        return;
    }
    if (type == "online_list") {
        QStringList users;
        for (const auto &value : obj.value("users").toArray()) {
            users << value.toString();
        }
        emit onlineUsersChanged(users);
        return;
    }
    if (type == "invite") {
        emit inviteReceived(obj.value("from").toString());
        return;
    }
    if (type == "invite_result") {
        emit inviteResult(obj.value("ok").toBool(), obj.value("message").toString());
        return;
    }
    if (type == "game_start") {
        emit gameStarted(obj.value("room_id").toString(),
                         obj.value("black").toString(),
                         obj.value("white").toString(),
                         obj.value("you_color").toString());
        return;
    }
    if (type == "opponent_move") {
        emit opponentMoved(obj.value("room_id").toString(),
                           obj.value("row").toInt(),
                           obj.value("col").toInt(),
                           obj.value("by").toString());
        return;
    }
    if (type == "move_result") {
        emit moveResult(obj.value("ok").toBool(), obj.value("message").toString());
        return;
    }
    if (type == "game_over") {
        emit gameOver(obj.value("room_id").toString(),
                      obj.value("winner").toString(),
                      obj.value("reason").toString());
        return;
    }
    if (type == "error") {
        emit serverMessage(obj.value("message").toString());
    }
}

void NetworkClient::onReadyRead()
{
    m_buffer.append(m_socket.readAll());
    while (true) {
        const int index = m_buffer.indexOf('\n');
        if (index < 0) {
            break;
        }
        const QByteArray line = m_buffer.left(index).trimmed();
        m_buffer.remove(0, index + 1);
        if (line.isEmpty()) {
            continue;
        }
        QJsonObject obj;
        if (parseLine(line, &obj)) {
            handleJson(obj);
        }
    }
}
