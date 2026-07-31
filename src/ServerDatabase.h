#pragma once

#include <QList>
#include <QString>
#include <QStringList>
#include <QtTypes>

class ServerDatabase
{
public:
    enum class RegisterResult {
        Success,
        UsernameExists,
        InvalidInput,
        DatabaseError
    };

    enum class LoginResult {
        Success,
        UserNotFound,
        WrongPassword,
        InvalidInput,
        DatabaseError
    };

    enum class SendFriendRequestResult {
        Success,
        UserNotFound,
        CannotAddSelf,
        AlreadyFriends,
        RequestPending,
        InvalidInput,
        DatabaseError
    };

    enum class RespondFriendRequestResult {
        Success,
        RequestNotFound,
        AlreadyHandled,
        DatabaseError
    };

    struct FriendRequestInfo {
        qint64 id;
        QString requesterUsername;
    };

    struct FriendRequestUpdate {
        qint64 id;
        QString responderUsername;
        bool accepted;
    };

    bool openDatabase();
    bool createTables();

    RegisterResult registerUser(const QString &username, const QString &password);
    LoginResult loginUser(const QString &username, const QString &password);

    bool searchUsers(
        const QString &username,
        const QString &keyword,
        QStringList &usernames
    );

    SendFriendRequestResult sendFriendRequest(
        const QString &requesterUsername,
        const QString &receiverUsername,
        qint64 &requestId
    );

    bool getPendingFriendRequests(
        const QString &receiverUsername,
        QList<FriendRequestInfo> &requests
    );

    RespondFriendRequestResult respondToFriendRequest(
        const QString &receiverUsername,
        qint64 requestId,
        bool accepted,
        QString &requesterUsername
    );

    bool getFriendRequestUpdates(
        const QString &requesterUsername,
        QList<FriendRequestUpdate> &updates
    );

    bool markFriendRequestUpdatesNotified(const QList<qint64> &requestIds);

    bool getFriends(const QString &username, QStringList &friendUsernames);
    bool areFriends(
        const QString &username,
        const QString &friendUsername,
        bool &friends
    );
};
