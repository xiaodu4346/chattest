#pragma once

#include <QString>
#include <QStringList>

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

    enum class AddFriendResult {
        Success,
        UserNotFound,
        CannotAddSelf,
        AlreadyFriends,
        InvalidInput,
        DatabaseError
    };

    bool openDatabase();
    bool createTables();

    RegisterResult registerUser(const QString &username, const QString &password);
    LoginResult loginUser(const QString &username, const QString &password);
    AddFriendResult addFriend(const QString &username, const QString &friendUsername);
    bool getFriends(const QString &username, QStringList &friendUsernames);
};
