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

    QTcpServer server;

    QMap<QString,QTcpSocket*> onlineUsers;

    QObject::connect(&server, &QTcpServer::newConnection, [&server, &onlineUsers](){
        QTcpSocket *clientSocket = server.nextPendingConnection();
        qDebug() << "New client connected:" << clientSocket;
        QByteArray *buffer = new QByteArray();


        QObject::connect(clientSocket, &QTcpSocket::readyRead, [clientSocket, buffer, &onlineUsers]() {
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

                if (type == "login") {
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
