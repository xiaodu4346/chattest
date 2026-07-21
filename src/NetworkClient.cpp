#include "NetworkClient.h"

#include <QAbstractSocket>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QTcpSocket>

NetworkClient::NetworkClient(QObject *parent)
    : QObject(parent),
      socket(new QTcpSocket(this))
{
    connect(socket, &QTcpSocket::connected, this, &NetworkClient::connected);

    connect(socket, &QTcpSocket::errorOccurred, this,
            [this](QAbstractSocket::SocketError) {
                emit connectionError(socket->errorString());
            });

    connect(socket, &QTcpSocket::readyRead, this, &NetworkClient::handleReadyRead);
}

void NetworkClient::connectToServer()
{
    socket->connectToHost("127.0.0.1", 12345);
}

void NetworkClient::sendJson(const QJsonObject &json)
{
    if (socket->state() != QAbstractSocket::ConnectedState) {
        emit connectionError("Not connected to server");
        return;
    }

    QByteArray data = QJsonDocument(json).toJson(QJsonDocument::Compact);
    data.append('\n');
    socket->write(data);
}

void NetworkClient::handleReadyRead()
{
    receiveBuffer.append(socket->readAll());

    while (true) {
        const int newlineIndex = receiveBuffer.indexOf('\n');

        if (newlineIndex == -1) {
            break;
        }

        const QByteArray line = receiveBuffer.left(newlineIndex);
        receiveBuffer.remove(0, newlineIndex + 1);

        QJsonParseError error;
        const QJsonDocument document = QJsonDocument::fromJson(line, &error);

        if (error.error != QJsonParseError::NoError || !document.isObject()) {
            qDebug() << "Invalid JSON from server:" << QString::fromUtf8(line);
            continue;
        }

        const QJsonObject json = document.object();
        const QString type = json["type"].toString();

        if (type == "register_result") {
            emit registerResult(json["result"].toString());
        } else if (type == "login_result") {
            emit loginResult(json["result"].toString());
        } else if (type == "chat") {
            emit chatReceived(
                json["sender"].toString(),
                json["content"].toString()
            );
        } else {
            qDebug() << "Unknown message type from server:" << type;
        }
    }
}
