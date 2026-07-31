#pragma once

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QtTypes>

class QJsonObject;
class QTcpSocket;

struct ClientFriendRequest
{
    qint64 id;
    QString requesterUsername;
};

struct ClientFriendRequestUpdate
{
    qint64 id;
    QString responderUsername;
    bool accepted;
};

class NetworkClient : public QObject
{
    Q_OBJECT

public:
    explicit NetworkClient(QObject *parent = nullptr);

    bool isConnected() const;
    void connectToServer();
    void sendJson(const QJsonObject &json);

signals:
    void connected();
    void connectionError(const QString &message);

    void registerResult(const QString &result);
    void loginResult(const QString &result);

    void searchResults(
        const QString &result,
        const QStringList &usernames
    );

    void sendFriendRequestResult(
        const QString &result,
        const QString &targetUsername
    );

    void friendRequestListReceived(
        const QString &result,
        const QList<ClientFriendRequest> &requests
    );

    void friendRequestReceived(
        qint64 requestId,
        const QString &requesterUsername
    );

    void respondFriendRequestResult(
        const QString &result,
        qint64 requestId,
        bool accepted
    );

    void friendRequestResolved(
        qint64 requestId,
        const QString &responderUsername,
        bool accepted
    );

    void friendRequestUpdatesReceived(
        const QString &result,
        const QList<ClientFriendRequestUpdate> &updates
    );

    void friendListReceived(
        const QString &result,
        const QStringList &friends
    );

    void chatReceived(const QString &sender, const QString &content);

private:
    QTcpSocket *socket;
    QByteArray receiveBuffer;

    void handleReadyRead();
};
