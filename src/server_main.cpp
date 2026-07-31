#include "ServerDatabase.h"

#include <QCoreApplication>
#include <QDebug>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QMap>
#include <QTcpServer>
#include <QTcpSocket>

namespace {

void sendJson(QTcpSocket *socket, const QJsonObject &json)
{
    QByteArray data = QJsonDocument(json).toJson(QJsonDocument::Compact);
    data.append('\n');
    socket->write(data);
}

}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    ServerDatabase database;

    if (!database.openDatabase()) {
        qDebug() << "Failed to open database";
        return 1;
    }

    if (!database.createTables()) {
        qDebug() << "Failed to create tables";
        return 1;
    }

    QTcpServer server;

    QMap<QString, QTcpSocket *> onlineUsers;
    QMap<QTcpSocket *, QString> authenticatedUsers;

    QObject::connect(
        &server,
        &QTcpServer::newConnection,
        [&server, &onlineUsers, &authenticatedUsers, &database]() {
        QTcpSocket *clientSocket = server.nextPendingConnection();
        qDebug() << "New client connected:" << clientSocket;
        QByteArray *buffer = new QByteArray();


        QObject::connect(
            clientSocket,
            &QTcpSocket::readyRead,
            [clientSocket, buffer, &onlineUsers, &authenticatedUsers, &database]() {
            buffer->append(clientSocket->readAll());

            while (true) {
                int newlineIndex = buffer->indexOf('\n');

                if (newlineIndex == -1) {
                    break;
                }

                QByteArray line = buffer->left(newlineIndex);
                buffer->remove(0, newlineIndex + 1);

                QJsonParseError error;
                QJsonDocument document = QJsonDocument::fromJson(line, &error);

                if (error.error != QJsonParseError::NoError || !document.isObject()) {
                    qDebug() << "Invalid JSON from client:" << QString::fromUtf8(line);
                    continue;
                }
                QJsonObject json = document.object();
                QString type = json["type"].toString();

                if (type == "register") {
                    QString username = json["username"].toString();
                    QString password = json["password"].toString();

                    ServerDatabase::RegisterResult result =
                        database.registerUser(username, password);

                    QJsonObject response;
                    response["type"] = "register_result";

                    if (result == ServerDatabase::RegisterResult::Success) {
                        response["result"] = "success";
                    } else if (
                        result == ServerDatabase::RegisterResult::UsernameExists
                    ) {
                        response["result"] = "username_exists";
                    } else if (
                        result == ServerDatabase::RegisterResult::InvalidInput
                    ) {
                        response["result"] = "invalid_input";
                    } else {
                        response["result"] = "database_error";
                    }

                    QByteArray responseData =
                        QJsonDocument(response).toJson(QJsonDocument::Compact);

                    responseData.append('\n');
                    clientSocket->write(responseData);
                } else if (type == "login") {
                    const QString username = json["username"].toString();
                    const QString password = json["password"].toString();
                    const ServerDatabase::LoginResult result =
                        database.loginUser(username, password);
                    QJsonObject response;
                    response["type"] = "login_result";

                    if (result == ServerDatabase::LoginResult::Success) {
                        const QString previousUsername =
                            authenticatedUsers.value(clientSocket);

                        if (!previousUsername.isEmpty()
                            && previousUsername != username
                            && onlineUsers.value(previousUsername) == clientSocket) {
                            onlineUsers.remove(previousUsername);
                        }

                        QTcpSocket *previousSocket =
                            onlineUsers.value(username, nullptr);

                        if (previousSocket != nullptr
                            && previousSocket != clientSocket) {
                            authenticatedUsers.remove(previousSocket);
                        }

                        onlineUsers[username] = clientSocket;
                        authenticatedUsers[clientSocket] = username;
                        response["result"] = "success";

                        qDebug() << "user online:" << username;
                        qDebug() << "online users count:" << onlineUsers.size();
                    } else if (result == ServerDatabase::LoginResult::UserNotFound) {
                        response["result"] = "user_not_found";
                    } else if (result == ServerDatabase::LoginResult::WrongPassword) {
                        response["result"] = "wrong_password";
                    } else if (result == ServerDatabase::LoginResult::InvalidInput) {
                        response["result"] = "invalid_input";
                    } else {
                        response["result"] = "database_error";
                    }

                    QByteArray responseData =
                        QJsonDocument(response).toJson(QJsonDocument::Compact);
                    responseData.append('\n');
                    clientSocket->write(responseData);
                } else if (type == "search_users") {
                    const auto authenticatedUser =
                        authenticatedUsers.constFind(clientSocket);

                    QJsonObject response;
                    response["type"] = "search_results";
                    QJsonArray users;

                    if (authenticatedUser == authenticatedUsers.cend()) {
                        response["result"] = "not_authenticated";
                    } else {
                        const QString keyword = json["keyword"].toString();
                        QStringList usernames;

                        if (keyword.trimmed().isEmpty()) {
                            response["result"] = "invalid_input";
                        } else if (database.searchUsers(
                                       authenticatedUser.value(),
                                       keyword,
                                       usernames
                                   )) {
                            response["result"] = "success";

                            for (const QString &username : usernames) {
                                users.append(username);
                            }
                        } else {
                            response["result"] = "database_error";
                        }
                    }

                    response["users"] = users;
                    sendJson(clientSocket, response);
                } else if (type == "send_friend_request") {
                    const auto authenticatedUser =
                        authenticatedUsers.constFind(clientSocket);

                    QJsonObject response;
                    response["type"] = "send_friend_request_result";

                    const QString targetUsername =
                        json["target_username"].toString();
                    response["target_username"] = targetUsername;

                    if (authenticatedUser == authenticatedUsers.cend()) {
                        response["result"] = "not_authenticated";
                    } else {
                        qint64 requestId = 0;
                        const QString requesterUsername =
                            authenticatedUser.value();
                        const ServerDatabase::SendFriendRequestResult result =
                            database.sendFriendRequest(
                                requesterUsername,
                                targetUsername,
                                requestId
                            );

                        if (result
                            == ServerDatabase::SendFriendRequestResult::Success) {
                            response["result"] = "success";
                            response["request_id"] =
                                static_cast<double>(requestId);

                            QTcpSocket *receiverSocket =
                                onlineUsers.value(targetUsername, nullptr);

                            if (receiverSocket != nullptr) {
                                QJsonObject notification;
                                notification["type"] =
                                    "friend_request_received";
                                notification["request_id"] =
                                    static_cast<double>(requestId);
                                notification["requester_username"] =
                                    requesterUsername;
                                sendJson(receiverSocket, notification);
                            }
                        } else if (
                            result
                            == ServerDatabase::SendFriendRequestResult::UserNotFound
                        ) {
                            response["result"] = "user_not_found";
                        } else if (
                            result
                            == ServerDatabase::SendFriendRequestResult::CannotAddSelf
                        ) {
                            response["result"] = "cannot_add_self";
                        } else if (
                            result
                            == ServerDatabase::SendFriendRequestResult::AlreadyFriends
                        ) {
                            response["result"] = "already_friends";
                        } else if (
                            result
                            == ServerDatabase::SendFriendRequestResult::RequestPending
                        ) {
                            response["result"] = "request_pending";
                        } else if (
                            result
                            == ServerDatabase::SendFriendRequestResult::InvalidInput
                        ) {
                            response["result"] = "invalid_input";
                        } else {
                            response["result"] = "database_error";
                        }
                    }

                    sendJson(clientSocket, response);
                } else if (type == "get_friend_requests") {
                    const auto authenticatedUser =
                        authenticatedUsers.constFind(clientSocket);

                    QJsonObject response;
                    response["type"] = "friend_request_list";
                    QJsonArray requestsJson;

                    if (authenticatedUser == authenticatedUsers.cend()) {
                        response["result"] = "not_authenticated";
                    } else {
                        QList<ServerDatabase::FriendRequestInfo> requests;

                        if (database.getPendingFriendRequests(
                                authenticatedUser.value(),
                                requests
                            )) {
                            response["result"] = "success";

                            for (const auto &request : requests) {
                                QJsonObject requestJson;
                                requestJson["request_id"] =
                                    static_cast<double>(request.id);
                                requestJson["requester_username"] =
                                    request.requesterUsername;
                                requestsJson.append(requestJson);
                            }
                        } else {
                            response["result"] = "database_error";
                        }
                    }

                    response["requests"] = requestsJson;
                    sendJson(clientSocket, response);
                } else if (type == "respond_friend_request") {
                    const auto authenticatedUser =
                        authenticatedUsers.constFind(clientSocket);

                    QJsonObject response;
                    response["type"] = "respond_friend_request_result";
                    const qint64 requestId =
                        static_cast<qint64>(json["request_id"].toDouble());
                    const bool accepted = json["accepted"].toBool();
                    response["request_id"] = static_cast<double>(requestId);
                    response["accepted"] = accepted;

                    if (authenticatedUser == authenticatedUsers.cend()) {
                        response["result"] = "not_authenticated";
                    } else {
                        QString requesterUsername;
                        const QString receiverUsername =
                            authenticatedUser.value();
                        const auto result = database.respondToFriendRequest(
                            receiverUsername,
                            requestId,
                            accepted,
                            requesterUsername
                        );

                        if (result
                            == ServerDatabase::RespondFriendRequestResult::Success) {
                            response["result"] = "success";

                            QTcpSocket *requesterSocket =
                                onlineUsers.value(requesterUsername, nullptr);

                            if (requesterSocket != nullptr) {
                                QJsonObject notification;
                                notification["type"] =
                                    "friend_request_resolved";
                                notification["request_id"] =
                                    static_cast<double>(requestId);
                                notification["responder_username"] =
                                    receiverUsername;
                                notification["accepted"] = accepted;
                                sendJson(requesterSocket, notification);
                                database.markFriendRequestUpdatesNotified(
                                    {requestId}
                                );
                            }
                        } else if (
                            result
                            == ServerDatabase::RespondFriendRequestResult::RequestNotFound
                        ) {
                            response["result"] = "request_not_found";
                        } else if (
                            result
                            == ServerDatabase::RespondFriendRequestResult::AlreadyHandled
                        ) {
                            response["result"] = "already_handled";
                        } else {
                            response["result"] = "database_error";
                        }
                    }

                    sendJson(clientSocket, response);
                } else if (type == "get_friend_request_updates") {
                    const auto authenticatedUser =
                        authenticatedUsers.constFind(clientSocket);

                    QJsonObject response;
                    response["type"] = "friend_request_updates";
                    QJsonArray updatesJson;
                    QList<qint64> deliveredRequestIds;

                    if (authenticatedUser == authenticatedUsers.cend()) {
                        response["result"] = "not_authenticated";
                    } else {
                        QList<ServerDatabase::FriendRequestUpdate> updates;

                        if (database.getFriendRequestUpdates(
                                authenticatedUser.value(),
                                updates
                            )) {
                            response["result"] = "success";

                            for (const auto &update : updates) {
                                QJsonObject updateJson;
                                updateJson["request_id"] =
                                    static_cast<double>(update.id);
                                updateJson["responder_username"] =
                                    update.responderUsername;
                                updateJson["accepted"] = update.accepted;
                                updatesJson.append(updateJson);
                                deliveredRequestIds.append(update.id);
                            }
                        } else {
                            response["result"] = "database_error";
                        }
                    }

                    response["updates"] = updatesJson;
                    sendJson(clientSocket, response);

                    if (!deliveredRequestIds.isEmpty()) {
                        database.markFriendRequestUpdatesNotified(
                            deliveredRequestIds
                        );
                    }
                } else if (type == "get_friends") {
                    const auto authenticatedUser =
                        authenticatedUsers.constFind(clientSocket);

                    QJsonObject response;
                    response["type"] = "friend_list";

                    QJsonArray friends;

                    if (authenticatedUser == authenticatedUsers.cend()) {
                        response["result"] = "not_authenticated";
                    } else {
                        const QString username = authenticatedUser.value();
                        QStringList friendUsernames;

                        if (database.getFriends(username, friendUsernames)) {
                            response["result"] = "success";

                            for (const QString &friendUsername : friendUsernames) {
                                friends.append(friendUsername);
                            }
                        } else {
                            response["result"] = "database_error";
                        }
                    }

                    response["friends"] = friends;

                    QByteArray responseData =
                        QJsonDocument(response).toJson(QJsonDocument::Compact);
                    responseData.append('\n');
                    clientSocket->write(responseData);
                } else if (type == "chat") {
                    const auto authenticatedUser =
                        authenticatedUsers.constFind(clientSocket);

                    if (authenticatedUser == authenticatedUsers.cend()) {
                        qDebug() << "Ignored chat from unauthenticated client:"
                                 << clientSocket;
                        continue;
                    }

                    const QString sender = authenticatedUser.value();
                    const QString receiver = json["receiver"].toString();
                    const QString content = json["content"].toString();

                    bool friends = false;

                    if (!database.areFriends(sender, receiver, friends)) {
                        qDebug() << "Failed to verify friendship for chat:"
                                 << sender << receiver;
                        continue;
                    }

                    if (!friends) {
                        qDebug() << "Ignored chat between non-friends:"
                                 << sender << receiver;
                        continue;
                    }

                    qDebug() << "type:" << type;
                    qDebug() << "sender:" << sender;
                    qDebug() << "receiver:" << receiver;
                    qDebug() << "content:" << content;

                    if (onlineUsers.contains(receiver)) {
                        QTcpSocket *receiverSocket = onlineUsers.value(receiver);

                        QJsonObject forwardedMessage;
                        forwardedMessage["type"] = "chat";
                        forwardedMessage["sender"] = sender;
                        forwardedMessage["receiver"] = receiver;
                        forwardedMessage["content"] = content;

                        QByteArray forwardedData =
                            QJsonDocument(forwardedMessage)
                                .toJson(QJsonDocument::Compact);
                        forwardedData.append('\n');
                        receiverSocket->write(forwardedData);

                        qDebug() << "message forwarded to:" << receiver;
                    } else {
                        qDebug() << "receiver offline:" << receiver;
                    }
                } else {
                    qDebug() << "Unknown message type:" << type;
                }
            }
        });

        QObject::connect(
            clientSocket,
            &QTcpSocket::disconnected,
            [clientSocket, buffer, &onlineUsers, &authenticatedUsers]() {
            qDebug() << "Client disconnected:" << clientSocket;

            const QString username = authenticatedUsers.take(clientSocket);

            if (!username.isEmpty()
                && onlineUsers.value(username) == clientSocket) {
                onlineUsers.remove(username);
                qDebug() << "user offline:" << username;
            }

            qDebug() << "online users count:" << onlineUsers.size();

            delete buffer;
            clientSocket->deleteLater();
        });
    });

    if (!server.listen(QHostAddress::Any,12345)) {
        qDebug() << "Server listen failed:" << server.errorString();
        return 1;
    }

    qDebug() << "Server is listening on port 12345";

    return app.exec();

}
