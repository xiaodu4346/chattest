#include "ChatWindow.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

ChatWindow::ChatWindow(const QString &username, QWidget *parent)
    : QWidget(parent),
      username(username)
{
    resize(640, 480);
    setWindowTitle("ChatTest");

    QLabel *titleLabel = new QLabel("ChatTest", this);
    QLabel *userLabel = new QLabel("Logged in as: " + username, this);

    messageView = new QPlainTextEdit(this);
    messageView->setReadOnly(true);
    messageView->appendPlainText("System: Welcome to ChatTest.");

    messageEdit = new QLineEdit(this);
    messageEdit->setPlaceholderText("Type a message");

    sendButton = new QPushButton("Send", this);

    QHBoxLayout *inputLayout = new QHBoxLayout;
    inputLayout->addWidget(messageEdit);
    inputLayout->addWidget(sendButton);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(userLabel);
    mainLayout->addWidget(messageView);
    mainLayout->addLayout(inputLayout);

    connect(sendButton, &QPushButton::clicked, this, &ChatWindow::handleSendMessage);
    connect(messageEdit, &QLineEdit::returnPressed, this, &ChatWindow::handleSendMessage);
}

void ChatWindow::handleSendMessage()
{
    const QString message = messageEdit->text().trimmed();

    if (message.isEmpty()) {
        return;
    }

    messageView->appendPlainText(username + ": " + message);
    messageEdit->clear();
}
