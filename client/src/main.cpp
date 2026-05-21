#include <QApplication>

#include "login_dialog.h"
#include "main_window.h"
#include "network_client.h"

enum class PendingAuthMode {
    None,
    Login,
    Register
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("GobangClient");

    NetworkClient client;
    LoginDialog loginDialog;
    MainWindow *window = new MainWindow(&client);
    window->hide();

    PendingAuthMode pendingAuthMode = PendingAuthMode::None;
    QString pendingUser;
    QString pendingPassword;

    QObject::connect(&loginDialog, &LoginDialog::connectRequested, &client, [&client](const QString &host, quint16 port) {
        if (!client.isConnected()) {
            client.connectToServer(host, port);
        }
    });

    QObject::connect(&loginDialog, &LoginDialog::loginRequested, &client,
                     [&loginDialog, &client, &pendingAuthMode, &pendingUser, &pendingPassword](const QString &username, const QString &password) {
                         pendingAuthMode = PendingAuthMode::Login;
                         pendingUser = username;
                         pendingPassword = password;
                         loginDialog.setBusy(true);
                         loginDialog.setStatusText(QStringLiteral("正在登录..."));
                         if (client.isConnected()) {
                             client.login(username, password);
                         }
                     });

    QObject::connect(&loginDialog, &LoginDialog::registerRequested, &client,
                     [&loginDialog, &client, &pendingAuthMode, &pendingUser, &pendingPassword](const QString &username, const QString &password) {
                         pendingAuthMode = PendingAuthMode::Register;
                         pendingUser = username;
                         pendingPassword = password;
                         loginDialog.setBusy(true);
                         loginDialog.setStatusText(QStringLiteral("正在注册..."));
                         if (client.isConnected()) {
                             client.registerAccount(username, password);
                         }
                     });

    QObject::connect(&client, &NetworkClient::connectedChanged, &loginDialog, [&loginDialog](bool connected) {
        loginDialog.setStatusText(connected ? QStringLiteral("服务器连接成功，正在认证...") : QStringLiteral("连接已断开"));
    });

    QObject::connect(&client, &NetworkClient::connectedChanged, &client,
                     [&client, &pendingAuthMode, &pendingUser, &pendingPassword](bool connected) {
                         if (!connected) {
                             pendingAuthMode = PendingAuthMode::None;
                             return;
                         }
                         if (pendingAuthMode == PendingAuthMode::Login) {
                             client.login(pendingUser, pendingPassword);
                         } else if (pendingAuthMode == PendingAuthMode::Register) {
                             client.registerAccount(pendingUser, pendingPassword);
                         }
                     });

    QObject::connect(&client, &NetworkClient::loginResult, &loginDialog,
                     [&loginDialog, &window, &client, &pendingAuthMode](bool ok, const QString &message, const QString &, const QStringList &) {
                         if (!ok) {
                             pendingAuthMode = PendingAuthMode::None;
                             loginDialog.setBusy(false);
                             loginDialog.setStatusText(QStringLiteral("登录失败：%1").arg(message));
                             return;
                         }
                         pendingAuthMode = PendingAuthMode::None;
                         loginDialog.accept();
                         window->show();
                     });

    QObject::connect(&client, &NetworkClient::registerResult, &loginDialog,
                     [&loginDialog, &pendingAuthMode](bool ok, const QString &message) {
                         pendingAuthMode = PendingAuthMode::None;
                         loginDialog.setBusy(false);
                         loginDialog.setStatusText(ok ? QStringLiteral("注册成功，请继续登录") : QStringLiteral("注册失败：%1").arg(message));
                     });

    QObject::connect(&client, &NetworkClient::loginResult, &loginDialog,
                     [&loginDialog](bool ok, const QString &, const QString &, const QStringList &) {
                         if (ok) {
                             loginDialog.setBusy(false);
                         }
                     });

    loginDialog.show();
    return app.exec();
}
