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

    QMap<QString,QTcpSocket*> onlineUsers;

    QObject::connect(&server, &QTcpServer::newConnection, [&server, &onlineUsers, &database](){
        QTcpSocket *clientSocket = server.nextPendingConnection();
        qDebug() << "New client connected:" << clientSocket;
        QByteArray *buffer = new QByteArray();


        QObject::connect(clientSocket, &QTcpSocket::readyRead, [clientSocket, buffer, &onlineUsers, &database]() {
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
                    } else {
                        response["result"] = "database_error";
                    }

                    QByteArray responseData =
                        QJsonDocument(response).toJson(QJsonDocument::Compact);

                    responseData.append('\n');
                    clientSocket->write(responseData);

                } else if (type == "login") {
                    QString username = json["username"].toString();

                    onlineUsers[username] = clientSocket;

                    qDebug() << "type:" << type;
                    qDebug() << "user online:" << username;
                    qDebug() << "online users count:" << onlineUsers.size();
                } else if (type == "chat") {
                    QString sender = json["sender"].toString();
                    QString receiver = json["receiver"].toString();
                    QString content = json["content"].toString();

                    qDebug() << "type:" << type;
                    qDebug() << "sender:" << sender;
                    qDebug() << "receiver:" << receiver;
                    qDebug() << "content:" << content;

                    if (onlineUsers.contains(receiver)) {
                        QTcpSocket *receiverSocket = onlineUsers.value(receiver);

                        line.append('\n');
                        receiverSocket->write(line);

                        qDebug() << "message forwarded to:" << receiver;
                    } else {
                        qDebug() << "receiver offline:" << receiver;
                    }
                } else {
                    qDebug() << "Unknown message type:" << type;
                }
            }
        });

        QObject::connect(clientSocket, &QTcpSocket::disconnected, [clientSocket, buffer, &onlineUsers]() {
            qDebug() << "Client disconnected:" << clientSocket;
            for (auto it = onlineUsers.begin(); it != onlineUsers.end(); ) {
                if (it.value() == clientSocket) {
                    qDebug() << "user offline:" << it.key();
                    it = onlineUsers.erase(it);
                } else {
                    ++it;
                }
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
