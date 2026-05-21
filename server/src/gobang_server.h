#pragma once

#include <QObject>
#include <QJsonObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHash>
#include <QStringList>

#include "account_store.h"
#include "gobang_types.h"

class GobangServer : public QObject
{
    Q_OBJECT

public:
    explicit GobangServer(QObject *parent = nullptr);
    bool start(quint16 port);

private slots:
    void onNewConnection();
    void onReadyRead();
    void onDisconnected();

private:
    struct Room {
        QString id;
        QString black;
        QString white;
        QString currentTurn;
        bool active = false;
        QVector<QVector<int>> board;
    };

    QTcpServer m_server;
    AccountStore m_accounts;
    QHash<QTcpSocket *, QString> m_socketToUser;
    QHash<QString, QTcpSocket *> m_userToSocket;
    QHash<QString, Room> m_rooms;
    QHash<QString, QString> m_pendingInvites;

    void handleMessage(QTcpSocket *socket, const QJsonObject &message);
    void sendJson(QTcpSocket *socket, const QJsonObject &message);
    void broadcastOnlineUsers();
    QStringList onlineUsers() const;
    QString socketUser(QTcpSocket *socket) const;
    void clearUserFromRooms(const QString &username);
    QString createRoomId(const QString &a, const QString &b) const;
    void finishRoom(const QString &roomId, const QString &winner);
    bool placeStone(Room &room, const QString &username, int row, int col, QString *errorMessage);
    bool hasFive(const QVector<QVector<int>> &board, int row, int col) const;
};
