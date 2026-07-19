#include "ServerDatabase.h"

#include <QDebug>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

bool ServerDatabase::openDatabase()
{
    QSqlDatabase database = QSqlDatabase::addDatabase("QSQLITE");
    database.setDatabaseName("server.db");


    if (!database.open()) {
        qDebug() << "Failed to open database:" << database.lastError().text();
        return false;
    }

    return true;
}

bool ServerDatabase::createTables()
{
    QSqlQuery query;

    const QString createUsersTable = R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT NOT NULL UNIQUE,
            password TEXT NOT NULL,
            created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
        )
    )";

    if (!query.exec(createUsersTable)) {
        qDebug() << "Failed to create users table:" << query.lastError().text();
        return false;
    }

    return true;
}

ServerDatabase::RegisterResult ServerDatabase::registerUser(const QString &username, const QString &password)
{
    QSqlQuery checkQuery;

    checkQuery.prepare("SELECT 1 FROM users WHERE username = :username LIMIT 1");
    checkQuery.bindValue(":username", username);

    if (!checkQuery.exec()) {
        qDebug() << "Failed to check username:" << checkQuery.lastError().text();
        return RegisterResult::DatabaseError;
    }

    if (checkQuery.next()) {
        return RegisterResult::UsernameExists;
    }

    QSqlQuery insertQuery;

    insertQuery.prepare(
        "INSERT INTO users (username, password) "
        "VALUES (:username, :password)"
    );
    insertQuery.bindValue(":username", username);
    insertQuery.bindValue(":password", password);

    if (!insertQuery.exec()) {
        qDebug() << "Failed to insert user:" << insertQuery.lastError().text();
        return RegisterResult::DatabaseError;
    }

    return RegisterResult::Success;
}
