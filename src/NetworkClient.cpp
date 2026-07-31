#include "NetworkClient.h"

#include <QAbstractSocket>
#include <QDebug>
#include <QJsonArray>
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

bool NetworkClient::isConnected() const
{
    return socket->state() == QAbstractSocket::ConnectedState;
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
        } else if (type == "search_results") {
            QStringList usernames;

            for (const QJsonValue &value : json["users"].toArray()) {
                if (value.isString()) {
                    usernames.append(value.toString());
                }
            }

            emit searchResults(json["result"].toString(), usernames);
        } else if (type == "send_friend_request_result") {
            emit sendFriendRequestResult(
                json["result"].toString(),
                json["target_username"].toString()
            );
        } else if (type == "friend_request_list") {
            QList<ClientFriendRequest> requests;

            for (const QJsonValue &value : json["requests"].toArray()) {
                if (!value.isObject()) {
                    continue;
                }

                const QJsonObject request = value.toObject();
                requests.append({
                    static_cast<qint64>(request["request_id"].toDouble()),
                    request["requester_username"].toString()
                });
            }

            emit friendRequestListReceived(
                json["result"].toString(),
                requests
            );
        } else if (type == "friend_request_received") {
            emit friendRequestReceived(
                static_cast<qint64>(json["request_id"].toDouble()),
                json["requester_username"].toString()
            );
        } else if (type == "respond_friend_request_result") {
            emit respondFriendRequestResult(
                json["result"].toString(),
                static_cast<qint64>(json["request_id"].toDouble()),
                json["accepted"].toBool()
            );
        } else if (type == "friend_request_resolved") {
            emit friendRequestResolved(
                static_cast<qint64>(json["request_id"].toDouble()),
                json["responder_username"].toString(),
                json["accepted"].toBool()
            );
        } else if (type == "friend_request_updates") {
            QList<ClientFriendRequestUpdate> updates;

            for (const QJsonValue &value : json["updates"].toArray()) {
                if (!value.isObject()) {
                    continue;
                }

                const QJsonObject update = value.toObject();
                updates.append({
                    static_cast<qint64>(update["request_id"].toDouble()),
                    update["responder_username"].toString(),
                    update["accepted"].toBool()
                });
            }

            emit friendRequestUpdatesReceived(
                json["result"].toString(),
                updates
            );
        } else if (type == "friend_list") {
            QStringList friends;

            const QJsonArray friendArray = json["friends"].toArray();

            for (const QJsonValue &friendValue : friendArray) {
                if (friendValue.isString()) {
                    friends.append(friendValue.toString());
                }
            }

            emit friendListReceived(
                json["result"].toString(),
                friends
            );
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
