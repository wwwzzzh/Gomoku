#pragma once

#include <QDialog>
#include <QString>

class QLineEdit;
class QLabel;
class QPushButton;

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);

    QString host() const;
    quint16 port() const;
    QString username() const;
    QString password() const;

    void setBusy(bool busy);
    void setStatusText(const QString &text);

signals:
    void loginRequested(const QString &username, const QString &password);
    void registerRequested(const QString &username, const QString &password);
    void connectRequested(const QString &host, quint16 port);

private:
    QLineEdit *m_hostEdit = nullptr;
    QLineEdit *m_portEdit = nullptr;
    QLineEdit *m_userEdit = nullptr;
    QLineEdit *m_passwordEdit = nullptr;
    QLabel *m_statusLabel = nullptr;
    QPushButton *m_loginButton = nullptr;
    QPushButton *m_registerButton = nullptr;
};
