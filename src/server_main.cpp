#include "ServerDatabase.h"

#include <QCoreApplication>
#include <QDebug>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QMap>
#include <QTcpServer>
#include <QTcpSocket>

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
