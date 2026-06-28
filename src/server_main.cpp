#include <QCoreApplication>
#include <QDebug>
#include <QHostAddress>
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
            qDebug() << "Received data from client:" << data;

        });
    });

    if (!server.listen(QHostAddress::Any,12345)) {
        qDebug() << "Server listen failed:" << server.errorString();
        return 1;
    }

    qDebug() << "Server is listening on port 12345";
    
    return app.exec();

}