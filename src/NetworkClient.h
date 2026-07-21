#pragma once

#include <QByteArray>
#include <QObject>
#include <QString>

class QJsonObject;
class QTcpSocket;

class NetworkClient : public QObject
{
    Q_OBJECT

public:
    explicit NetworkClient(QObject *parent = nullptr);

    void connectToServer();
    void sendJson(const QJsonObject &json);

signals:
    void connected();
    void connectionError(const QString &message);

    void registerResult(const QString &result);
    void loginResult(const QString &result);

    void chatReceived(const QString &sender, const QString &content);

private:
    QTcpSocket *socket;
    QByteArray receiveBuffer;

    void handleReadyRead();
};
