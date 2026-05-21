#include "account_store.h"

#include <QCryptographicHash>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace {
QString zh(const char *text)
{
    return QString::fromUtf8(text);
}
}

AccountStore::AccountStore(const QString &databasePath)
    : m_databasePath(databasePath)
{
}

bool AccountStore::initialize(QString *errorMessage)
{
    if (QSqlDatabase::contains("gobang_accounts")) {
        m_database = QSqlDatabase::database("gobang_accounts");
    } else {
        m_database = QSqlDatabase::addDatabase("QSQLITE", "gobang_accounts");
        m_database.setDatabaseName(m_databasePath);
    }

    if (!m_database.open()) {
        if (errorMessage) {
            *errorMessage = m_database.lastError().text();
        }
        return false;
    }

    QSqlQuery query(m_database);
    if (!query.exec(R"(
        CREATE TABLE IF NOT EXISTS accounts (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT NOT NULL UNIQUE,
            password_hash TEXT NOT NULL,
            created_at TEXT DEFAULT CURRENT_TIMESTAMP
        )
    )")) {
        if (errorMessage) {
            *errorMessage = query.lastError().text();
        }
        return false;
    }

    return true;
}

QString AccountStore::hashPassword(const QString &password) const
{
    return QString::fromUtf8(QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex());
}

bool AccountStore::registerUser(const QString &username, const QString &password, QString *errorMessage)
{
    const QString normalizedUsername = username.trimmed();
    if (normalizedUsername.isEmpty() || password.isEmpty()) {
        if (errorMessage) {
            *errorMessage = zh(u8"\u7528\u6237\u540d\u548c\u5bc6\u7801\u4e0d\u80fd\u4e3a\u7a7a");
        }
        return false;
    }
    if (normalizedUsername.size() > 32) {
        if (errorMessage) {
            *errorMessage = zh(u8"\u7528\u6237\u540d\u957f\u5ea6\u4e0d\u80fd\u8d85\u8fc7 32 \u4e2a\u5b57\u7b26");
        }
        return false;
    }
    if (password.size() < 6) {
        if (errorMessage) {
            *errorMessage = zh(u8"\u5bc6\u7801\u957f\u5ea6\u4e0d\u80fd\u5c11\u4e8e 6 \u4f4d");
        }
        return false;
    }

    QSqlQuery check(m_database);
    check.prepare("SELECT id FROM accounts WHERE username = :username");
    check.bindValue(":username", QVariant(normalizedUsername));
    if (!check.exec()) {
        if (errorMessage) {
            *errorMessage = check.lastError().text();
        }
        return false;
    }
    if (check.next()) {
        if (errorMessage) {
            *errorMessage = zh(u8"\u7528\u6237\u540d\u5df2\u5b58\u5728");
        }
        return false;
    }

    QSqlQuery insert(m_database);
    insert.prepare("INSERT INTO accounts(username, password_hash) VALUES(:username, :password_hash)");
    insert.bindValue(":username", QVariant(normalizedUsername));
    insert.bindValue(":password_hash", QVariant(hashPassword(password)));
    if (!insert.exec()) {
        if (errorMessage) {
            *errorMessage = insert.lastError().text();
        }
        return false;
    }
    return true;
}

bool AccountStore::validateLogin(const QString &username, const QString &password, QString *errorMessage)
{
    const QString normalizedUsername = username.trimmed();

    QSqlQuery query(m_database);
    query.prepare("SELECT password_hash FROM accounts WHERE username = :username");
    query.bindValue(":username", QVariant(normalizedUsername));
    if (!query.exec()) {
        if (errorMessage) {
            *errorMessage = query.lastError().text();
        }
        return false;
    }
    if (!query.next()) {
        if (errorMessage) {
            *errorMessage = zh(u8"\u7528\u6237\u4e0d\u5b58\u5728");
        }
        return false;
    }

    const auto savedHash = query.value(0).toString();
    if (savedHash != hashPassword(password)) {
        if (errorMessage) {
            *errorMessage = zh(u8"\u5bc6\u7801\u9519\u8bef");
        }
        return false;
    }
    return true;
}
