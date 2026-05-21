#pragma once

#include <QMainWindow>
#include <QListWidget>

class QComboBox;
class QLineEdit;
class QLabel;
class QPushButton;
class QTabWidget;

class GameBoardWidget;
class GameController;
class NetworkClient;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(NetworkClient *client, QWidget *parent = nullptr);

private:
    NetworkClient *m_client;
    GameBoardWidget *m_board = nullptr;
    GameController *m_controller = nullptr;
    QListWidget *m_onlineList = nullptr;
    QLabel *m_userLabel = nullptr;
    QLabel *m_statusLabel = nullptr;
    QLineEdit *m_inviteEdit = nullptr;
    QComboBox *m_difficultyCombo = nullptr;
    QLineEdit *m_aiEndpointEdit = nullptr;
    QPushButton *m_startAiButton = nullptr;
    QPushButton *m_inviteButton = nullptr;

    void buildUi();
    void applyStyle();
    void bindNetworkSignals();
};
