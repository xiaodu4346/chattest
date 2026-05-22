#pragma once

#include <QString>
#include <QWidget>

class QLineEdit;
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
    QPushButton *sendButton;

    void handleSendMessage();
};
