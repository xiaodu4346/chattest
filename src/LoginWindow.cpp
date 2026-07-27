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
    registerButton = new QPushButton("Register", this);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(titleLabel);
    layout->addWidget(usernameEdit);
    layout->addWidget(passwordEdit);
    layout->addWidget(statusLabel);
    layout->addWidget(loginButton);
    layout->addWidget(registerButton);

    connect(loginButton, &QPushButton::clicked, this, &LoginWindow::handleLogin);
    connect(registerButton, &QPushButton::clicked,
            this, &LoginWindow::handleRegister);

    connect(networkClient, &NetworkClient::connected,
            this, &LoginWindow::sendPendingRequest);

    connect(networkClient, &NetworkClient::connectionError,
        this, [this](const QString &message) {
            statusLabel->setText("Connection error: " + message);
        });

    connect(networkClient, &NetworkClient::registerResult,
        this, [this](const QString &result) {
            if (result == "success") {
                statusLabel->setText("Registration successful. You can now log in.");
            } else if (result == "username_exists") {
                statusLabel->setText("Username already exists");
            } else if (result == "invalid_input") {
                statusLabel->setText("Username and password must not be empty");
            } else {
                statusLabel->setText("Server database error");
            }
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
            } else if (result == "invalid_input") {
                statusLabel->setText("Username and password must not be empty");
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
    pendingAction = PendingAction::Login;

    if (networkClient->isConnected()) {
        sendPendingRequest();
    } else {
        statusLabel->setText("Connecting to server...");
        networkClient->connectToServer();
    }
}

void LoginWindow::handleRegister()
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
    pendingAction = PendingAction::Register;

    if (networkClient->isConnected()) {
        sendPendingRequest();
    } else {
        statusLabel->setText("Connecting to server...");
        networkClient->connectToServer();
    }
}

void LoginWindow::sendPendingRequest()
{
    QJsonObject json;

    if (pendingAction == PendingAction::Login) {
        statusLabel->setText("Verifying account...");
        json["type"] = "login";
    } else if (pendingAction == PendingAction::Register) {
        statusLabel->setText("Creating account...");
        json["type"] = "register";
    } else {
        return;
    }

    json["username"] = pendingUsername;
    json["password"] = pendingPassword;

    networkClient->sendJson(json);
    pendingAction = PendingAction::None;
}
