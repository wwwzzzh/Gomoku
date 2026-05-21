#include "login_dialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Gobang 登录"));
    setModal(true);
    resize(460, 420);
    setObjectName("loginDialog");

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(28, 28, 28, 28);
    root->setSpacing(18);

    auto *title = new QLabel(QStringLiteral("五子棋联机对战"), this);
    title->setObjectName("loginTitle");
    title->setAlignment(Qt::AlignCenter);
    root->addWidget(title);

    auto *subtitle = new QLabel(QStringLiteral("支持登录注册、TCP/IP 联机与人机对战"), this);
    subtitle->setObjectName("loginSubtitle");
    subtitle->setAlignment(Qt::AlignCenter);
    root->addWidget(subtitle);

    auto *card = new QFrame(this);
    card->setObjectName("loginCard");
    auto *form = new QFormLayout(card);
    form->setLabelAlignment(Qt::AlignLeft);
    form->setVerticalSpacing(14);
    form->setHorizontalSpacing(12);

    m_hostEdit = new QLineEdit("127.0.0.1", card);
    m_portEdit = new QLineEdit("7777", card);
    m_userEdit = new QLineEdit(card);
    m_passwordEdit = new QLineEdit(card);
    m_passwordEdit->setEchoMode(QLineEdit::Password);

    form->addRow(QStringLiteral("服务器地址"), m_hostEdit);
    form->addRow(QStringLiteral("服务器端口"), m_portEdit);
    form->addRow(QStringLiteral("用户名"), m_userEdit);
    form->addRow(QStringLiteral("密码"), m_passwordEdit);
    root->addWidget(card);

    m_statusLabel = new QLabel(QStringLiteral("请输入服务器和账号信息"), this);
    m_statusLabel->setObjectName("loginStatus");
    m_statusLabel->setAlignment(Qt::AlignCenter);
    root->addWidget(m_statusLabel);

    auto *buttons = new QHBoxLayout();
    buttons->setSpacing(12);
    m_loginButton = new QPushButton(QStringLiteral("登录"), this);
    m_registerButton = new QPushButton(QStringLiteral("注册"), this);
    buttons->addWidget(m_loginButton);
    buttons->addWidget(m_registerButton);
    root->addLayout(buttons);

    connect(m_loginButton, &QPushButton::clicked, this, [this]() {
        emit connectRequested(host(), port());
        emit loginRequested(username(), password());
    });
    connect(m_registerButton, &QPushButton::clicked, this, [this]() {
        emit connectRequested(host(), port());
        emit registerRequested(username(), password());
    });

    setStyleSheet(R"(
        QDialog#loginDialog {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #1d2330, stop:1 #0e1118);
            color: #eef2ff;
        }
        QLabel#loginTitle {
            font-size: 30px;
            font-weight: 700;
            color: #ffffff;
        }
        QLabel#loginSubtitle, QLabel#loginStatus {
            color: #9ca3af;
            font-size: 12px;
        }
        QFrame#loginCard {
            background: rgba(255, 255, 255, 0.06);
            border: 1px solid rgba(255, 255, 255, 0.12);
            border-radius: 18px;
            padding: 18px;
        }
        QLineEdit {
            background: rgba(10, 14, 24, 0.72);
            border: 1px solid rgba(255, 255, 255, 0.12);
            border-radius: 10px;
            padding: 10px 12px;
            color: white;
        }
        QPushButton {
            background: #5b8def;
            color: white;
            border: none;
            border-radius: 12px;
            padding: 11px 18px;
            font-weight: 600;
        }
        QPushButton:hover { background: #6b99ff; }
        QPushButton:disabled { background: #3f4c6b; color: #a0aec0; }
    )");
}

QString LoginDialog::host() const { return m_hostEdit->text().trimmed(); }
quint16 LoginDialog::port() const { return static_cast<quint16>(m_portEdit->text().toUShort()); }
QString LoginDialog::username() const { return m_userEdit->text().trimmed(); }
QString LoginDialog::password() const { return m_passwordEdit->text(); }

void LoginDialog::setBusy(bool busy)
{
    m_loginButton->setDisabled(busy);
    m_registerButton->setDisabled(busy);
}

void LoginDialog::setStatusText(const QString &text)
{
    m_statusLabel->setText(text);
}
