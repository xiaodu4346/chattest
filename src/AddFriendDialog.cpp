#include "AddFriendDialog.h"

#include "NetworkClient.h"

#include <QHBoxLayout>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

AddFriendDialog::AddFriendDialog(
    NetworkClient *networkClient,
    QWidget *parent
)
    : QDialog(parent),
      networkClient(networkClient)
{
    resize(420, 320);
    setWindowTitle("Add Friend");

    searchEdit = new QLineEdit(this);
    searchEdit->setPlaceholderText("Search by username");

    QPushButton *searchButton = new QPushButton("Search", this);
    resultList = new QListWidget(this);
    statusLabel = new QLabel(this);
    statusLabel->setWordWrap(true);
    sendRequestButton = new QPushButton("Send Friend Request", this);
    sendRequestButton->setEnabled(false);

    QHBoxLayout *searchLayout = new QHBoxLayout;
    searchLayout->addWidget(searchEdit);
    searchLayout->addWidget(searchButton);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addLayout(searchLayout);
    layout->addWidget(resultList);
    layout->addWidget(statusLabel);
    layout->addWidget(sendRequestButton);

    connect(searchButton, &QPushButton::clicked,
            this, &AddFriendDialog::searchUsers);
    connect(searchEdit, &QLineEdit::returnPressed,
            this, &AddFriendDialog::searchUsers);
    connect(sendRequestButton, &QPushButton::clicked,
            this, &AddFriendDialog::sendFriendRequest);
    connect(resultList, &QListWidget::currentItemChanged,
            this, [this]() {
                sendRequestButton->setEnabled(
                    resultList->currentItem() != nullptr
                );
            });

    connect(
        networkClient,
        &NetworkClient::searchResults,
        this,
        [this](const QString &result, const QStringList &usernames) {
            resultList->clear();

            if (result != "success") {
                statusLabel->setText(
                    result == "invalid_input"
                        ? "Enter a username to search"
                        : "Could not search users: " + result
                );
                return;
            }

            resultList->addItems(usernames);

            if (usernames.isEmpty()) {
                statusLabel->setText("No matching users found");
            } else {
                resultList->setCurrentRow(0);
                statusLabel->setText(
                    QString("Found %1 user(s)").arg(usernames.size())
                );
            }
        }
    );

    connect(
        networkClient,
        &NetworkClient::sendFriendRequestResult,
        this,
        [this](const QString &result, const QString &targetUsername) {
            if (result == "success") {
                statusLabel->setText(
                    "Friend request sent to " + targetUsername
                );
            } else if (result == "request_pending") {
                statusLabel->setText(
                    "A friend request is already pending between you and "
                    + targetUsername
                );
            } else if (result == "already_friends") {
                statusLabel->setText(
                    targetUsername + " is already your friend"
                );
            } else if (result == "user_not_found") {
                statusLabel->setText("User no longer exists");
            } else if (result == "cannot_add_self") {
                statusLabel->setText("You cannot add yourself");
            } else {
                statusLabel->setText(
                    "Could not send friend request: " + result
                );
            }
        }
    );
}

void AddFriendDialog::searchUsers()
{
    const QString keyword = searchEdit->text().trimmed();

    if (keyword.isEmpty()) {
        statusLabel->setText("Enter a username to search");
        return;
    }

    QJsonObject request;
    request["type"] = "search_users";
    request["keyword"] = keyword;
    networkClient->sendJson(request);
    statusLabel->setText("Searching...");
}

void AddFriendDialog::sendFriendRequest()
{
    if (resultList->currentItem() == nullptr) {
        return;
    }

    const QString targetUsername = resultList->currentItem()->text();

    QJsonObject request;
    request["type"] = "send_friend_request";
    request["target_username"] = targetUsername;
    networkClient->sendJson(request);
    statusLabel->setText("Sending request...");
}
