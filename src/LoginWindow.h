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
    NetworkClient *networkClient;
    QString pendingUsername;
    QString pendingPassword;
    QLineEdit *usernameEdit;
    QLineEdit *passwordEdit;
    QLabel *statusLabel;
    QPushButton *loginButton;

    void handleLogin();
};
