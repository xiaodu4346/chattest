#include "LoginWindow.h"
#include "NetworkClient.h"

#include "ChatWindow.h"

#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QString>
#include <Qt>
#include <QVBoxLayout>

LoginWindow::LoginWindow(NetworkClient *networkClient, QWidget *parent)
    : QWidget(parent), networkClient(networkClient)
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
    connect(networkClient, &NetworkClient::connected,
        this, [this]() {
            statusLabel->setText("Verifying account...");

            QJsonObject json;
            json["type"] = "login";
            json["username"] = pendingUsername;
            json["password"] = pendingPassword;

            this->networkClient->sendJson(json);
        });

    connect(networkClient, &NetworkClient::connectionError,
        this, [this](const QString &message) {
            statusLabel->setText("Connection error: " + message);
        });

    connect(networkClient, &NetworkClient::loginResult,
        this, [this](const QString &result) {
            if (result == "success") {
                statusLabel->setText("Login successful");

                ChatWindow *chatWindow =
                    new ChatWindow(pendingUsername, this->networkClient);
                chatWindow->setAttribute(Qt::WA_DeleteOnClose);
                chatWindow->show();

                close();
            } else if (result == "user_not_found") {
                statusLabel->setText("User not found");
            } else if (result == "wrong_password") {
                statusLabel->setText("Wrong password");
            } else {
                statusLabel->setText("Server database error");
            }
        });
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

    pendingUsername = username;
    pendingPassword = password;

    statusLabel->setText("Connecting to server...");
    networkClient->connectToServer();
}
