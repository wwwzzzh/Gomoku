#pragma once

#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <QByteArray>

namespace gobang::protocol {

inline QJsonObject makeMessage(const QString &type)
{
    QJsonObject obj;
    obj["type"] = type;
    return obj;
}

inline QByteArray serialize(const QJsonObject &obj)
{
    return QJsonDocument(obj).toJson(QJsonDocument::Compact) + '\n';
}

inline bool parseLine(const QByteArray &line, QJsonObject *out)
{
    const auto doc = QJsonDocument::fromJson(line);
    if (!doc.isObject()) {
        return false;
    }
    *out = doc.object();
    return true;
}

inline QString typeOf(const QJsonObject &obj)
{
    return obj.value("type").toString();
}

} // namespace gobang::protocol
