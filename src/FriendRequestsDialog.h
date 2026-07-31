#pragma once

#include <QDialog>
#include <QtTypes>

class QLabel;
class QListWidget;
class NetworkClient;
class QPushButton;

class FriendRequestsDialog : public QDialog
{
public:
    explicit FriendRequestsDialog(
        NetworkClient *networkClient,
        QWidget *parent = nullptr
    );

    void refresh();

private:
    NetworkClient *networkClient;
    QListWidget *requestList;
    QLabel *statusLabel;
    QPushButton *acceptButton;
    QPushButton *rejectButton;

    void addRequest(qint64 requestId, const QString &requesterUsername);
    void respondToCurrentRequest(bool accepted);
    void removeRequest(qint64 requestId);
    void updateButtons();
};
