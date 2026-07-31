#pragma once

#include <QDialog>

class QLabel;
class QLineEdit;
class QListWidget;
class NetworkClient;
class QPushButton;

class AddFriendDialog : public QDialog
{
public:
    explicit AddFriendDialog(
        NetworkClient *networkClient,
        QWidget *parent = nullptr
    );

private:
    NetworkClient *networkClient;
    QLineEdit *searchEdit;
    QListWidget *resultList;
    QLabel *statusLabel;
    QPushButton *sendRequestButton;

    void searchUsers();
    void sendFriendRequest();
};
