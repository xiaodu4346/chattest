#include <QCoreApplication>
#include <QDebug>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QTcpServer>
#include <QTcpSocket>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QTcpServer server;

    QObject::connect(&server, &QTcpServer::newConnection, [&server](){
        QTcpSocket *clientSocket = server.nextPendingConnection();
        qDebug() << "New client connected:" << clientSocket;

        QObject::connect(clientSocket, &QTcpSocket::readyRead, [clientSocket]() {
            QByteArray data = clientSocket->readAll();

            QJsonParseError error;
            QJsonDocument document = QJsonDocument::fromJson(data, &error);

            if (error.error != QJsonParseError::NoError || !document.isObject()) {
                qDebug() << "Invalid JSON from client:" << QString::fromUtf8(data);
                return;
            }

            QJsonObject json = document.object();

            QString sender = json["sender"].toString();
            QString receiver = json["receiver"].toString();
            QString content = json["content"].toString();

            qDebug() << "sender:" << sender;
            qDebug() << "receiver:" << receiver;
            qDebug() << "content:" << content;
        });
    });

    if (!server.listen(QHostAddress::Any,12345)) {
        qDebug() << "Server listen failed:" << server.errorString();
        return 1;
    }

    qDebug() << "Server is listening on port 12345";
    
    return app.exec();

}
