#include "main_window.h"

#include <QComboBox>
#include <QAbstractItemView>
#include <QDockWidget>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
#include <QTabWidget>
#include <QUrl>
#include <QToolButton>
#include <QVBoxLayout>

#include "game_board_widget.h"
#include "game_controller.h"
#include "network_client.h"

MainWindow::MainWindow(NetworkClient *client, QWidget *parent)
    : QMainWindow(parent)
    , m_client(client)
{
    buildUi();
    applyStyle();
    bindNetworkSignals();
}

void MainWindow::buildUi()
{
    setWindowTitle(QStringLiteral("Gobang Client"));
    resize(1280, 860);

    auto *central = new QWidget(this);
    auto *root = new QHBoxLayout(central);
    root->setContentsMargins(18, 18, 18, 18);
    root->setSpacing(18);

    auto *leftPanel = new QFrame(central);
    leftPanel->setObjectName("sidePanel");
    leftPanel->setMinimumWidth(340);
    auto *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(18, 18, 18, 18);
    leftLayout->setSpacing(14);

    m_userLabel = new QLabel(QStringLiteral("未登录"), leftPanel);
    m_userLabel->setObjectName("userBadge");
    leftLayout->addWidget(m_userLabel);

    auto *tabs = new QTabWidget(leftPanel);
    tabs->setObjectName("modeTabs");

    auto *friendTab = new QWidget(tabs);
    auto *friendLayout = new QVBoxLayout(friendTab);
    friendLayout->setSpacing(12);
    friendLayout->setContentsMargins(8, 8, 8, 8);

    m_onlineList = new QListWidget(friendTab);
    m_onlineList->setSelectionMode(QAbstractItemView::SingleSelection);
    friendLayout->addWidget(new QLabel(QStringLiteral("在线好友"), friendTab));
    friendLayout->addWidget(m_onlineList, 1);

    m_inviteEdit = new QLineEdit(friendTab);
    m_inviteEdit->setPlaceholderText(QStringLiteral("输入好友用户名"));
    friendLayout->addWidget(m_inviteEdit);

    m_inviteButton = new QPushButton(QStringLiteral("邀请对战"), friendTab);
    friendLayout->addWidget(m_inviteButton);

    auto *aiTab = new QWidget(tabs);
    auto *aiLayout = new QVBoxLayout(aiTab);
    aiLayout->setSpacing(12);
    aiLayout->setContentsMargins(8, 8, 8, 8);

    m_difficultyCombo = new QComboBox(aiTab);
    m_difficultyCombo->addItems({QStringLiteral("简单"), QStringLiteral("普通"), QStringLiteral("困难")});
    m_aiEndpointEdit = new QLineEdit(aiTab);
    m_aiEndpointEdit->setPlaceholderText(QStringLiteral("可选：AI HTTP 接口地址，如 http://127.0.0.1:8000/move"));
    m_startAiButton = new QPushButton(QStringLiteral("开始人机对战"), aiTab);

    aiLayout->addWidget(new QLabel(QStringLiteral("AI 难度"), aiTab));
    aiLayout->addWidget(m_difficultyCombo);
    aiLayout->addWidget(new QLabel(QStringLiteral("AI 接口"), aiTab));
    aiLayout->addWidget(m_aiEndpointEdit);
    aiLayout->addStretch(1);
    aiLayout->addWidget(m_startAiButton);

    tabs->addTab(friendTab, QStringLiteral("好友对战"));
    tabs->addTab(aiTab, QStringLiteral("人机对战"));
    leftLayout->addWidget(tabs, 1);

    m_statusLabel = new QLabel(QStringLiteral("等待连接服务器"), leftPanel);
    m_statusLabel->setObjectName("statusLabel");
    leftLayout->addWidget(m_statusLabel);

    m_board = new GameBoardWidget(central);
    m_board->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    root->addWidget(leftPanel);
    root->addWidget(m_board, 1);
    setCentralWidget(central);

    m_controller = new GameController(m_client, m_board, this);

    connect(m_inviteButton, &QPushButton::clicked, this, [this]() {
        const auto target = m_inviteEdit->text().trimmed();
        if (target.isEmpty()) {
            QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请输入好友用户名"));
            return;
        }
        m_client->invitePlayer(target);
        m_statusLabel->setText(QStringLiteral("已发送邀请给 %1").arg(target));
    });

    connect(m_startAiButton, &QPushButton::clicked, this, [this]() {
        const auto difficulty = static_cast<gobang::AiDifficulty>(m_difficultyCombo->currentIndex());
        m_controller->startAiMode(difficulty, QUrl(m_aiEndpointEdit->text().trimmed()));
        m_statusLabel->setText(QStringLiteral("已切换到人机对战"));
    });
}

void MainWindow::applyStyle()
{
    setStyleSheet(R"(
        QMainWindow {
            background: #0f1220;
            color: #ecf2ff;
        }
        QFrame#sidePanel {
            background: rgba(19, 23, 36, 0.94);
            border-radius: 20px;
            border: 1px solid rgba(255, 255, 255, 0.08);
        }
        QLabel#userBadge {
            font-size: 18px;
            font-weight: 700;
            color: #ffffff;
            padding: 10px 12px;
            background: rgba(91, 141, 239, 0.18);
            border-radius: 14px;
        }
        QLabel#statusLabel {
            color: #9ca3af;
            padding-top: 4px;
        }
        QTabWidget::pane {
            border: none;
        }
        QTabBar::tab {
            background: rgba(255, 255, 255, 0.06);
            color: #d1d5db;
            padding: 10px 18px;
            margin-right: 8px;
            border-radius: 12px;
        }
        QTabBar::tab:selected {
            background: #5b8def;
            color: white;
        }
        QListWidget, QLineEdit, QComboBox {
            background: rgba(255, 255, 255, 0.05);
            border: 1px solid rgba(255, 255, 255, 0.1);
            border-radius: 10px;
            padding: 10px 12px;
            color: #f8fafc;
        }
        QPushButton {
            background: #5b8def;
            color: white;
            border: none;
            border-radius: 12px;
            padding: 11px 16px;
            font-weight: 600;
        }
        QPushButton:hover { background: #6b99ff; }
        QPushButton:pressed { background: #4f7dd9; }
    )");
}

void MainWindow::bindNetworkSignals()
{
    connect(m_client, &NetworkClient::onlineUsersChanged, this, [this](const QStringList &users) {
        m_onlineList->clear();
        m_onlineList->addItems(users);
    });
    connect(m_client, &NetworkClient::loginResult, this, [this](bool ok, const QString &message, const QString &username, const QStringList &users) {
        if (ok) {
            m_userLabel->setText(QStringLiteral("当前账号：%1").arg(username));
            m_statusLabel->setText(QStringLiteral("登录成功"));
            m_controller->setLocalUser(username);
            m_onlineList->clear();
            m_onlineList->addItems(users);
            m_controller->startFriendMode();
        } else {
            m_statusLabel->setText(QStringLiteral("登录失败：%1").arg(message));
        }
    });
    connect(m_client, &NetworkClient::registerResult, this, [this](bool ok, const QString &message) {
        m_statusLabel->setText(ok ? QStringLiteral("注册成功，请继续登录") : QStringLiteral("注册失败：%1").arg(message));
    });
    connect(m_client, &NetworkClient::inviteResult, this, [this](bool ok, const QString &message) {
        m_statusLabel->setText(ok ? QStringLiteral("好友对战已开始") : QStringLiteral("邀请处理：%1").arg(message));
    });
    connect(m_client, &NetworkClient::serverMessage, this, [this](const QString &message) {
        m_statusLabel->setText(message);
    });
    connect(m_client, &NetworkClient::connectedChanged, this, [this](bool connected) {
        m_statusLabel->setText(connected ? QStringLiteral("服务器已连接") : QStringLiteral("服务器已断开"));
    });
}
