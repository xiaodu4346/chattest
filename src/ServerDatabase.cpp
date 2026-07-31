#include "ServerDatabase.h"

#include <QCoreApplication>
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>


bool ServerDatabase::openDatabase()
{
    QSqlDatabase database = QSqlDatabase::addDatabase("QSQLITE");
    database.setDatabaseName(QCoreApplication::applicationDirPath() + "/server.db");

    if (!database.open()) {
        qDebug() << "Failed to open database:" << database.lastError().text();
        return false;
    }

    QSqlQuery query(database);

    if (!query.exec("PRAGMA foreign_keys = ON")) {
        qDebug() << "Failed to enable foreign keys:"
                 << query.lastError().text();
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

    const QString createFriendshipsTable = R"(
        CREATE TABLE IF NOT EXISTS friendships (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user1_id INTEGER NOT NULL,
            user2_id INTEGER NOT NULL,
            created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
            UNIQUE(user1_id, user2_id),
            CHECK(user1_id < user2_id),
            FOREIGN KEY(user1_id) REFERENCES users(id) ON DELETE CASCADE,
            FOREIGN KEY(user2_id) REFERENCES users(id) ON DELETE CASCADE
        )
    )";

    if (!query.exec(createFriendshipsTable)) {
        qDebug() << "Failed to create friendships table:" << query.lastError().text();
        return false;
    }

    return true;
}

ServerDatabase::RegisterResult ServerDatabase::registerUser(const QString &username, const QString &password)
{
    if (username.trimmed().isEmpty() || password.isEmpty()) {
        return RegisterResult::InvalidInput;
    }

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

ServerDatabase::LoginResult ServerDatabase::loginUser(const QString &username, const QString &password)
{
    if (username.trimmed().isEmpty() || password.isEmpty()) {
        return LoginResult::InvalidInput;
    }

    QSqlQuery query;

    query.prepare("SELECT password FROM users WHERE username = :username");
    query.bindValue(":username", username);

    if (!query.exec()) {
        qDebug() << "Failed to query user:" << query.lastError().text();
        return LoginResult::DatabaseError;
    }

    if (!query.next()) {
        return LoginResult::UserNotFound;
    }

    const QString storedPassword = query.value("password").toString();

    if (storedPassword != password) {
        return LoginResult::WrongPassword;
    }

    return LoginResult::Success;
}

ServerDatabase::AddFriendResult ServerDatabase::addFriend(
    const QString &username,
    const QString &friendUsername
)
{
    if (username.trimmed().isEmpty()
        || friendUsername.trimmed().isEmpty()) {
        return AddFriendResult::InvalidInput;
    }

    if (username == friendUsername) {
        return AddFriendResult::CannotAddSelf;
    }

    QSqlQuery currentUserQuery;

    currentUserQuery.prepare(
        "SELECT id FROM users "
        "WHERE username = :username "
        "LIMIT 1"
    );
    currentUserQuery.bindValue(":username", username);

    if (!currentUserQuery.exec()) {
        qDebug() << "Failed to query current user:"
                 << currentUserQuery.lastError().text();
        return AddFriendResult::DatabaseError;
    }

    if (!currentUserQuery.next()) {
        qDebug() << "Authenticated user not found:" << username;
        return AddFriendResult::DatabaseError;
    }

    const int userId = currentUserQuery.value("id").toInt();

    QSqlQuery friendUserQuery;

    friendUserQuery.prepare(
        "SELECT id FROM users "
        "WHERE username = :friendUsername "
        "LIMIT 1"
    );
    friendUserQuery.bindValue(":friendUsername", friendUsername);

    if (!friendUserQuery.exec()) {
        qDebug() << "Failed to query friend user:"
                 << friendUserQuery.lastError().text();
        return AddFriendResult::DatabaseError;
    }

    if (!friendUserQuery.next()) {
        return AddFriendResult::UserNotFound;
    }

    const int friendId = friendUserQuery.value("id").toInt();
    const int user1Id = userId < friendId ? userId : friendId;
    const int user2Id = userId < friendId ? friendId : userId;

    QSqlQuery friendshipQuery;

    friendshipQuery.prepare(
        "SELECT 1 FROM friendships "
        "WHERE user1_id = :user1Id "
        "AND user2_id = :user2Id "
        "LIMIT 1"
    );
    friendshipQuery.bindValue(":user1Id", user1Id);
    friendshipQuery.bindValue(":user2Id", user2Id);

    if (!friendshipQuery.exec()) {
        qDebug() << "Failed to check friendship:"
                 << friendshipQuery.lastError().text();
        return AddFriendResult::DatabaseError;
    }

    if (friendshipQuery.next()) {
        return AddFriendResult::AlreadyFriends;
    }

    QSqlQuery insertFriendshipQuery;

    insertFriendshipQuery.prepare(
        "INSERT INTO friendships (user1_id, user2_id) "
        "VALUES (:user1Id, :user2Id)"
    );
    insertFriendshipQuery.bindValue(":user1Id", user1Id);
    insertFriendshipQuery.bindValue(":user2Id", user2Id);

    if (!insertFriendshipQuery.exec()) {
        qDebug() << "Failed to insert friendship:"
                 << insertFriendshipQuery.lastError().text();
        return AddFriendResult::DatabaseError;
    }

    return AddFriendResult::Success;
}

bool ServerDatabase::getFriends(
    const QString &username,
    QStringList &friendUsernames
)
{
    friendUsernames.clear();

    if (username.trimmed().isEmpty()) {
        return false;
    }

    QSqlQuery userQuery;

    userQuery.prepare(
        "SELECT id FROM users "
        "WHERE username = :username "
        "LIMIT 1"
    );
    userQuery.bindValue(":username", username);

    if (!userQuery.exec()) {
        qDebug() << "Failed to query user for friend list:"
                 << userQuery.lastError().text();
        return false;
    }

    if (!userQuery.next()) {
        qDebug() << "User not found while querying friend list:"
                 << username;
        return false;
    }

    const int userId = userQuery.value("id").toInt();

    QSqlQuery friendsQuery;

    friendsQuery.prepare(
        "SELECT friend_users.username "
        "FROM friendships "
        "JOIN users AS friend_users "
        "ON friend_users.id = CASE "
        "WHEN friendships.user1_id = :caseUserId "
        "THEN friendships.user2_id "
        "ELSE friendships.user1_id "
        "END "
        "WHERE friendships.user1_id = :user1Id "
        "OR friendships.user2_id = :user2Id "
        "ORDER BY friend_users.username"
    );

    friendsQuery.bindValue(":caseUserId", userId);
    friendsQuery.bindValue(":user1Id", userId);
    friendsQuery.bindValue(":user2Id", userId);

    if (!friendsQuery.exec()) {
        qDebug() << "Failed to query friend list:"
                 << friendsQuery.lastError().text();
        return false;
    }

    while (friendsQuery.next()) {
        friendUsernames.append(
            friendsQuery.value("username").toString()
        );
    }

    return true;
}
