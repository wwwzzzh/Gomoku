#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QByteArray>
#include <QJsonObject>
#include <QString>
#include <QStringList>

class NetworkClient : public QObject
{
    Q_OBJECT

public:
    explicit NetworkClient(QObject *parent = nullptr);

    void connectToServer(const QString &host, quint16 port);
    void disconnectFromServer();
    bool isConnected() const;

    void login(const QString &username, const QString &password);
    void registerAccount(const QString &username, const QString &password);
    void invitePlayer(const QString &target);
    void replyInvite(const QString &from, bool accept);
    void sendMove(const QString &roomId, int row, int col);
    void sendGameOver(const QString &roomId, const QString &winner);

    QString currentUser() const { return m_currentUser; }

signals:
    void connectedChanged(bool connected);
    void loginResult(bool ok, const QString &message, const QString &username, const QStringList &onlineUsers);
    void registerResult(bool ok, const QString &message);
    void onlineUsersChanged(const QStringList &users);
    void inviteReceived(const QString &from);
    void inviteResult(bool ok, const QString &message);
    void gameStarted(const QString &roomId, const QString &black, const QString &white, const QString &youColor);
    void opponentMoved(const QString &roomId, int row, int col, const QString &by);
    void moveResult(bool ok, const QString &message);
    void gameOver(const QString &roomId, const QString &winner, const QString &reason);
    void serverMessage(const QString &message);

private slots:
    void onReadyRead();

private:
    QTcpSocket m_socket;
    QByteArray m_buffer;
    QString m_currentUser;

    void sendJson(const QJsonObject &obj);
    void handleJson(const QJsonObject &obj);
};
