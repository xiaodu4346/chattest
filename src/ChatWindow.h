#pragma once

#include <QMap>
#include <QString>
#include <QWidget>

class QLineEdit;
class QListWidget;
class QLabel;
class QPlainTextEdit;
class QPushButton;

class ChatWindow : public QWidget
{
public:
    explicit ChatWindow(const QString &username, QWidget *parent = nullptr);

private:
    QString username;
    QPlainTextEdit *messageView;
    QLineEdit *messageEdit;
    QListWidget *friendList;
    QLabel *chatTargetLabel;
    QMap<QString, QString> chatHistory;
    QPushButton *sendButton;

    void handleSendMessage();
    void handleFriendChanged(const QString &friendName);
    void appendMessage(const QString &sender, const QString &message);
    void appendAutoReply(const QString &message);
};
