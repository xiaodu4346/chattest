#include "ChatWindow.h"

#include "AddFriendDialog.h"
#include "FriendRequestsDialog.h"
#include "NetworkClient.h"

#include <QHBoxLayout>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
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

    messageView = new QPlainTextEdit(this);
    messageView->setReadOnly(true);

    messageEdit = new QLineEdit(this);
    messageEdit->setPlaceholderText("Type a message");

    sendButton = new QPushButton("Send", this);
    addFriendButton = new QPushButton("Add Friend", this);
    friendRequestsButton = new QPushButton("Friend Requests", this);

    addFriendDialog = new AddFriendDialog(networkClient, this);
    friendRequestsDialog = new FriendRequestsDialog(networkClient, this);

    QHBoxLayout *inputLayout = new QHBoxLayout;
    inputLayout->addWidget(messageEdit);
    inputLayout->addWidget(sendButton);

    QVBoxLayout *chatLayout = new QVBoxLayout;
    chatLayout->addWidget(titleLabel);
    chatLayout->addWidget(userLabel);
    chatLayout->addWidget(chatTargetLabel);
    chatLayout->addWidget(messageView);
    chatLayout->addLayout(inputLayout);

    QVBoxLayout *friendsLayout = new QVBoxLayout;
    friendsLayout->addWidget(addFriendButton);
    friendsLayout->addWidget(friendRequestsButton);
    friendsLayout->addWidget(friendList);

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->addLayout(friendsLayout);
    mainLayout->addLayout(chatLayout);

    connect(sendButton, &QPushButton::clicked, this, &ChatWindow::handleSendMessage);
    connect(messageEdit, &QLineEdit::returnPressed, this, &ChatWindow::handleSendMessage);
    connect(friendList, &QListWidget::currentTextChanged, this, &ChatWindow::handleFriendChanged);
    connect(addFriendButton, &QPushButton::clicked,
            this, [this]() {
                addFriendDialog->show();
                addFriendDialog->raise();
                addFriendDialog->activateWindow();
            });
    connect(friendRequestsButton, &QPushButton::clicked,
            this, [this]() {
                friendRequestsDialog->refresh();
                friendRequestsDialog->show();
                friendRequestsDialog->raise();
                friendRequestsDialog->activateWindow();
            });

    connect(networkClient, &NetworkClient::chatReceived,
            this, [this](const QString &sender, const QString &content) {
                appendMessage(sender, sender, content);
            });

    connect(
        networkClient,
        &NetworkClient::friendListReceived,
        this,
        [this](const QString &result, const QStringList &friends) {
            if (result != "success") {
                chatTargetLabel->setText(
                    "Failed to load friends: " + result
                );
                return;
            }

            const QString currentFriend =
                friendList->currentItem() == nullptr
                    ? QString()
                    : friendList->currentItem()->text();

            friendList->clear();
            friendList->addItems(friends);

            if (friends.isEmpty()) {
                chatTargetLabel->setText("No friends yet");
                messageView->clear();
                return;
            }

            const int previousIndex = friends.indexOf(currentFriend);
            friendList->setCurrentRow(previousIndex >= 0 ? previousIndex : 0);
        }
    );

    connect(
        networkClient,
        &NetworkClient::respondFriendRequestResult,
        this,
        [this](const QString &result, qint64, bool accepted) {
            if (result == "success" && accepted) {
                requestFriendList();
            }
        }
    );

    connect(
        networkClient,
        &NetworkClient::friendRequestResolved,
        this,
        [this](qint64, const QString &responderUsername, bool accepted) {
            QMessageBox::information(
                this,
                "Friend Request",
                responderUsername
                    + (accepted
                           ? " accepted your friend request"
                           : " rejected your friend request")
            );

            if (accepted) {
                requestFriendList();
            }
        }
    );

    connect(
        networkClient,
        &NetworkClient::friendRequestUpdatesReceived,
        this,
        [this](
            const QString &result,
            const QList<ClientFriendRequestUpdate> &updates
        ) {
            if (result != "success") {
                return;
            }

            bool friendListChanged = false;

            for (const ClientFriendRequestUpdate &update : updates) {
                QMessageBox::information(
                    this,
                    "Friend Request",
                    update.responderUsername
                        + (update.accepted
                               ? " accepted your friend request"
                               : " rejected your friend request")
                );
                friendListChanged = friendListChanged || update.accepted;
            }

            if (friendListChanged) {
                requestFriendList();
            }
        }
    );
}

void ChatWindow::initialize()
{
    requestFriendList();
    friendRequestsDialog->refresh();

    QJsonObject request;
    request["type"] = "get_friend_request_updates";
    networkClient->sendJson(request);
}

void ChatWindow::requestFriendList()
{
    QJsonObject request;
    request["type"] = "get_friends";
    networkClient->sendJson(request);
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
