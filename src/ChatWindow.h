#pragma once

#include <QMap>
#include <QString>
#include <QWidget>

class QLineEdit;
class QListWidget;
class QLabel;
class QPlainTextEdit;
class QPushButton;
class AddFriendDialog;
class FriendRequestsDialog;
class NetworkClient;

class ChatWindow : public QWidget
{
public:
    explicit ChatWindow(const QString &username, NetworkClient *networkClient,
                        QWidget *parent = nullptr);

    void initialize();

private:
    QString username;
    NetworkClient *networkClient;
    QPlainTextEdit *messageView;
    QLineEdit *messageEdit;
    QListWidget *friendList;
    QLabel *chatTargetLabel;
    QMap<QString, QString> chatHistory;
    QPushButton *sendButton;
    QPushButton *addFriendButton;
    QPushButton *friendRequestsButton;
    AddFriendDialog *addFriendDialog;
    FriendRequestsDialog *friendRequestsDialog;

    void requestFriendList();
    void handleSendMessage();
    void handleFriendChanged(const QString &friendName);
    void appendMessage(const QString &friendName, const QString &sender, const QString &message);
};
