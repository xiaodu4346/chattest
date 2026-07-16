#include "ChatWindow.h"

#include <QAbstractSocket>
#include <QDebug>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QVBoxLayout>
#include <QTcpSocket>

ChatWindow::ChatWindow(const QString &username, QWidget *parent)
    : QWidget(parent),
      username(username),
      socket(new QTcpSocket(this))
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

    connect(socket, &QTcpSocket::connected, this, [this]() {
        qDebug() << "connected to server";

        QJsonObject json;
        json["type"] = "login";
        json["username"] = this->username;

        QJsonDocument document(json);
        QByteArray data = document.toJson(QJsonDocument::Compact);
        data.append('\n');

        socket->write(data);
    });

    connect(socket, &QTcpSocket::errorOccurred, this, [](QAbstractSocket::SocketError) {
        qDebug() << "connect server error";
    });
    connect(socket, &QTcpSocket::readyRead, this, [this]() {
        receiveBuffer.append(socket->readAll());
    
        while (true) {
            int newlineIndex = receiveBuffer.indexOf('\n');
    
            if (newlineIndex == -1) {
                break;
            }
    
            QByteArray line = receiveBuffer.left(newlineIndex);
            receiveBuffer.remove(0, newlineIndex + 1);
    
            QJsonParseError error;
            QJsonDocument document = QJsonDocument::fromJson(line, &error);
    
            if (error.error != QJsonParseError::NoError || !document.isObject()) {
                qDebug() << "Invalid JSON from server:"
                         << QString::fromUtf8(line);
                continue;
            }
    
            QJsonObject json = document.object();
            QString type = json["type"].toString();
    
            if (type == "chat") {
                QString sender = json["sender"].toString();
                QString content = json["content"].toString();
    
                qDebug() << "received message from:" << sender;
                qDebug() << "content:" << content;
                appendMessage(sender, sender, content);
            }
        }
    });
    socket->connectToHost("127.0.0.1", 12345);
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
    if (socket->state() == QTcpSocket::ConnectedState) {
        QJsonObject json;
        json["type"] = "chat";
        json["sender"] = username;
        json["receiver"] = friendList->currentItem()->text();
        json["content"] = message;

        QJsonDocument document(json);
        QByteArray data = document.toJson(QJsonDocument::Compact);
        data.append('\n');

        socket->write(data);
    }
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
