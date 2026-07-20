#pragma once

#include <QString>

class ServerDatabase
{
public:
    enum class RegisterResult {
        Success,
        UsernameExists,
        DatabaseError
    };

    enum class LoginResult {
        Success,
        UserNotFound,
        WrongPassword,
        DatabaseError
    };


    bool openDatabase();
    bool createTables();

    RegisterResult registerUser(const QString &username, const QString &password);
    LoginResult loginUser(const QString &username, const QString &password);
};
