#include "ChatWindow.h"
#include "NetworkClient.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QJsonObject>
#include <QVBoxLayout>

ChatWindow::ChatWindow(const QString &username, NetworkClient *networkClient,
                       QWidget *parent)
    : QWidget(parent),
      username(username),
      networkClient(networkClient)
{
    resize(640, 480);
    setWindowTitle("ChatTest");

    QLabel *titleLabel = new QLabel("ChatTest", this);
    QLabel *userLabel = new QLabel("Logged in as: " + username, this);
    chatTargetLabel = new QLabel(this);

    friendList = new QListWidget(this);
    friendList->addItem("friendA");
    friendList->addItem("friendB");
    friendList->addItem("friendC");
    friendList->addItem("friendD");

    messageView = new QPlainTextEdit(this);
    messageView->setReadOnly(true);

    messageEdit = new QLineEdit(this);
    messageEdit->setPlaceholderText("Type a message");

    sendButton = new QPushButton("Send", this);

    QHBoxLayout *inputLayout = new QHBoxLayout;
    inputLayout->addWidget(messageEdit);
    inputLayout->addWidget(sendButton);

    QVBoxLayout *chatLayout = new QVBoxLayout;
    chatLayout->addWidget(titleLabel);
    chatLayout->addWidget(userLabel);
    chatLayout->addWidget(chatTargetLabel);
    chatLayout->addWidget(messageView);
    chatLayout->addLayout(inputLayout);

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->addWidget(friendList);
    mainLayout->addLayout(chatLayout);

    connect(sendButton, &QPushButton::clicked, this, &ChatWindow::handleSendMessage);
    connect(messageEdit, &QLineEdit::returnPressed, this, &ChatWindow::handleSendMessage);
    connect(friendList, &QListWidget::currentTextChanged, this, &ChatWindow::handleFriendChanged);
    friendList->setCurrentRow(0);

    connect(networkClient, &NetworkClient::chatReceived,
            this, [this](const QString &sender, const QString &content) {
                appendMessage(sender, sender, content);
            });
}

void ChatWindow::handleSendMessage()
{
    if (friendList->currentItem() == nullptr) {
        return;
    }

    const QString message = messageEdit->text().trimmed();

    if (message.isEmpty()) {
        return;
    }

    const QString friendName = friendList->currentItem()->text();

    appendMessage(friendName, username, message);

    QJsonObject json;
    json["type"] = "chat";
    json["receiver"] = friendName;
    json["content"] = message;

    networkClient->sendJson(json);
    messageEdit->clear();
}

void ChatWindow::handleFriendChanged(const QString &friendName)
{
    chatTargetLabel->setText("chatting with: " + friendName);
    messageView->setPlainText(chatHistory[friendName]);
}

void ChatWindow::appendMessage(const QString &friendName, const QString &sender, const QString &message)
{
    const QString chatLine = sender + ": " + message;
    chatHistory[friendName] += chatLine + "\n";

    if (friendList->currentItem() != nullptr
        && friendList->currentItem()->text() == friendName) {
        messageView->appendPlainText(chatLine);
    }
}
