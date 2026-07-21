#pragma once

#include <QString>
#include <QWidget>

class QLabel;
class QLineEdit;
class QPushButton;
class NetworkClient;

class LoginWindow : public QWidget
{
public:
    explicit LoginWindow(NetworkClient *networkClient, QWidget *parent = nullptr);

private:
    enum class PendingAction
    {
        None,
        Login,
        Register
    };

    PendingAction pendingAction = PendingAction::None;

    NetworkClient *networkClient;
    QString pendingUsername;
    QString pendingPassword;
    QLineEdit *usernameEdit;
    QLineEdit *passwordEdit;
    QPushButton *registerButton;
    QLabel *statusLabel;
    QPushButton *loginButton;

    void handleLogin();
    void handleRegister();
    void sendPendingRequest();
};
