#pragma once

#include <QWidget>

class QLabel;
class QLineEdit;
class QPushButton;

class LoginWindow : public QWidget
{
public:
    explicit LoginWindow(QWidget *parent = nullptr);

private:
    QLineEdit *usernameEdit;
    QLineEdit *passwordEdit;
    QLabel *statusLabel;
    QPushButton *loginButton;

    void handleLogin();
};
