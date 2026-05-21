#pragma once

#include <QString>
#include <QStringList>
#include <QtSql/QSqlDatabase>

class AccountStore
{
public:
    explicit AccountStore(const QString &databasePath);

    bool initialize(QString *errorMessage);
    bool registerUser(const QString &username, const QString &password, QString *errorMessage);
    bool validateLogin(const QString &username, const QString &password, QString *errorMessage);

private:
    QString m_databasePath;
    QSqlDatabase m_database;

    QString hashPassword(const QString &password) const;
};
