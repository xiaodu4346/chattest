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
    


    bool openDatabase();
    bool createTables();

    RegisterResult registerUser(const QString &username, const QString &password);
};