#include "LoginWindow.h"

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>

LoginWindow::LoginWindow(QWidget *parent)
    : QWidget(parent)
{
    resize(480, 240);
    setWindowTitle("ChatTest Login");

    QLabel *titleLabel = new QLabel("Welcome to ChatTest", this);

    usernameEdit = new QLineEdit(this);
    usernameEdit->setPlaceholderText("Username");

    passwordEdit = new QLineEdit(this);
    passwordEdit->setPlaceholderText("Password");
    passwordEdit->setEchoMode(QLineEdit::Password);

    statusLabel = new QLabel(this);
    loginButton = new QPushButton("Login", this);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(titleLabel);
    layout->addWidget(usernameEdit);
    layout->addWidget(passwordEdit);
    layout->addWidget(statusLabel);
    layout->addWidget(loginButton);

    connect(loginButton, &QPushButton::clicked, this, &LoginWindow::handleLogin);
}

void LoginWindow::handleLogin()
{
    const QString username = usernameEdit->text();
    const QString password = passwordEdit->text();

    if (username.isEmpty()) {
        statusLabel->setText("Please enter a username");
        return;
    }

    if (password.isEmpty()) {
        statusLabel->setText("Please enter a password");
        return;
    }

    statusLabel->setText("Login success. Hello, " + username);
}
