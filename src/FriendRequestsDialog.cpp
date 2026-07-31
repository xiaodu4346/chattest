#include "FriendRequestsDialog.h"

#include "NetworkClient.h"

#include <QHBoxLayout>
#include <QJsonObject>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QVBoxLayout>
#include <QVariant>

FriendRequestsDialog::FriendRequestsDialog(
    NetworkClient *networkClient,
    QWidget *parent
)
    : QDialog(parent),
      networkClient(networkClient)
{
    resize(380, 280);
    setWindowTitle("Friend Requests");

    requestList = new QListWidget(this);
    statusLabel = new QLabel(this);
    statusLabel->setWordWrap(true);
    acceptButton = new QPushButton("Accept", this);
    rejectButton = new QPushButton("Reject", this);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(acceptButton);
    buttonLayout->addWidget(rejectButton);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(requestList);
    layout->addWidget(statusLabel);
    layout->addLayout(buttonLayout);

    connect(requestList, &QListWidget::currentItemChanged,
            this, &FriendRequestsDialog::updateButtons);
    connect(acceptButton, &QPushButton::clicked,
            this, [this]() { respondToCurrentRequest(true); });
    connect(rejectButton, &QPushButton::clicked,
            this, [this]() { respondToCurrentRequest(false); });

    connect(
        networkClient,
        &NetworkClient::friendRequestListReceived,
        this,
        [this](
            const QString &result,
            const QList<ClientFriendRequest> &requests
        ) {
            requestList->clear();

            if (result != "success") {
                statusLabel->setText(
                    "Could not load friend requests: " + result
                );
                updateButtons();
                return;
            }

            for (const ClientFriendRequest &request : requests) {
                addRequest(request.id, request.requesterUsername);
            }

            statusLabel->setText(
                requests.isEmpty()
                    ? "No pending friend requests"
                    : QString("%1 pending request(s)").arg(requests.size())
            );

            if (!requests.isEmpty()) {
                show();
                raise();
                activateWindow();
            }

            updateButtons();
        }
    );

    connect(
        networkClient,
        &NetworkClient::friendRequestReceived,
        this,
        [this](qint64 requestId, const QString &requesterUsername) {
            addRequest(requestId, requesterUsername);
            statusLabel->setText(
                requesterUsername + " wants to add you as a friend"
            );
            show();
            raise();
            activateWindow();
        }
    );

    connect(
        networkClient,
        &NetworkClient::respondFriendRequestResult,
        this,
        [this](const QString &result, qint64 requestId, bool accepted) {
            if (result == "success") {
                removeRequest(requestId);
                statusLabel->setText(
                    accepted
                        ? "Friend request accepted"
                        : "Friend request rejected"
                );
            } else {
                statusLabel->setText(
                    "Could not respond to request: " + result
                );
            }

            updateButtons();
        }
    );

    updateButtons();
}

void FriendRequestsDialog::refresh()
{
    QJsonObject request;
    request["type"] = "get_friend_requests";
    networkClient->sendJson(request);
    statusLabel->setText("Loading requests...");
}

void FriendRequestsDialog::addRequest(
    qint64 requestId,
    const QString &requesterUsername
)
{
    for (int index = 0; index < requestList->count(); ++index) {
        QListWidgetItem *item = requestList->item(index);

        if (item->data(Qt::UserRole).toLongLong() == requestId) {
            return;
        }
    }

    QListWidgetItem *item = new QListWidgetItem(
        requesterUsername + " wants to add you",
        requestList
    );
    item->setData(Qt::UserRole, requestId);

    if (requestList->currentItem() == nullptr) {
        requestList->setCurrentItem(item);
    }

    updateButtons();
}

void FriendRequestsDialog::respondToCurrentRequest(bool accepted)
{
    QListWidgetItem *item = requestList->currentItem();

    if (item == nullptr) {
        return;
    }

    const qint64 requestId = item->data(Qt::UserRole).toLongLong();

    QJsonObject request;
    request["type"] = "respond_friend_request";
    request["request_id"] = static_cast<double>(requestId);
    request["accepted"] = accepted;
    networkClient->sendJson(request);

    acceptButton->setEnabled(false);
    rejectButton->setEnabled(false);
    statusLabel->setText("Sending response...");
}

void FriendRequestsDialog::removeRequest(qint64 requestId)
{
    for (int index = 0; index < requestList->count(); ++index) {
        QListWidgetItem *item = requestList->item(index);

        if (item->data(Qt::UserRole).toLongLong() == requestId) {
            delete requestList->takeItem(index);
            return;
        }
    }
}

void FriendRequestsDialog::updateButtons()
{
    const bool hasSelection = requestList->currentItem() != nullptr;
    acceptButton->setEnabled(hasSelection);
    rejectButton->setEnabled(hasSelection);
}
