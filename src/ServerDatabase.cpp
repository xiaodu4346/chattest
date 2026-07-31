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

    const QString createFriendRequestsTable = R"(
        CREATE TABLE IF NOT EXISTS friend_requests (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            requester_id INTEGER NOT NULL,
            receiver_id INTEGER NOT NULL,
            status TEXT NOT NULL DEFAULT 'pending'
                CHECK(status IN ('pending', 'accepted', 'rejected')),
            requester_notified INTEGER NOT NULL DEFAULT 0
                CHECK(requester_notified IN (0, 1)),
            created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
            responded_at TEXT,
            CHECK(requester_id <> receiver_id),
            FOREIGN KEY(requester_id) REFERENCES users(id) ON DELETE CASCADE,
            FOREIGN KEY(receiver_id) REFERENCES users(id) ON DELETE CASCADE
        )
    )";

    if (!query.exec(createFriendRequestsTable)) {
        qDebug() << "Failed to create friend_requests table:"
                 << query.lastError().text();
        return false;
    }

    const QString createPendingRequestIndex = R"(
        CREATE UNIQUE INDEX IF NOT EXISTS idx_friend_requests_pending
        ON friend_requests(requester_id, receiver_id)
        WHERE status = 'pending'
    )";

    if (!query.exec(createPendingRequestIndex)) {
        qDebug() << "Failed to create pending friend request index:"
                 << query.lastError().text();
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

bool ServerDatabase::searchUsers(
    const QString &username,
    const QString &keyword,
    QStringList &usernames
)
{
    usernames.clear();

    const QString trimmedKeyword = keyword.trimmed();

    if (username.trimmed().isEmpty() || trimmedKeyword.isEmpty()) {
        return false;
    }

    QSqlQuery query;
    query.prepare(
        "SELECT candidate.username "
        "FROM users AS candidate "
        "JOIN users AS current_user "
        "ON current_user.username = :username "
        "WHERE candidate.id <> current_user.id "
        "AND instr(lower(candidate.username), lower(:keyword)) > 0 "
        "AND NOT EXISTS ("
        "    SELECT 1 FROM friendships "
        "    WHERE friendships.user1_id = "
        "        CASE WHEN candidate.id < current_user.id "
        "             THEN candidate.id ELSE current_user.id END "
        "    AND friendships.user2_id = "
        "        CASE WHEN candidate.id < current_user.id "
        "             THEN current_user.id ELSE candidate.id END"
        ") "
        "ORDER BY candidate.username "
        "LIMIT 50"
    );
    query.bindValue(":username", username);
    query.bindValue(":keyword", trimmedKeyword);

    if (!query.exec()) {
        qDebug() << "Failed to search users:" << query.lastError().text();
        return false;
    }

    while (query.next()) {
        usernames.append(query.value("username").toString());
    }

    return true;
}

ServerDatabase::SendFriendRequestResult ServerDatabase::sendFriendRequest(
    const QString &requesterUsername,
    const QString &receiverUsername,
    qint64 &requestId
)
{
    requestId = 0;

    if (requesterUsername.trimmed().isEmpty()
        || receiverUsername.trimmed().isEmpty()) {
        return SendFriendRequestResult::InvalidInput;
    }

    if (requesterUsername == receiverUsername) {
        return SendFriendRequestResult::CannotAddSelf;
    }

    QSqlQuery requesterQuery;
    requesterQuery.prepare(
        "SELECT id FROM users WHERE username = :username LIMIT 1"
    );
    requesterQuery.bindValue(":username", requesterUsername);

    if (!requesterQuery.exec() || !requesterQuery.next()) {
        qDebug() << "Failed to find authenticated requester:"
                 << requesterQuery.lastError().text();
        return SendFriendRequestResult::DatabaseError;
    }

    const qint64 requesterId = requesterQuery.value("id").toLongLong();

    QSqlQuery receiverQuery;
    receiverQuery.prepare(
        "SELECT id FROM users WHERE username = :username LIMIT 1"
    );
    receiverQuery.bindValue(":username", receiverUsername);

    if (!receiverQuery.exec()) {
        qDebug() << "Failed to find friend request receiver:"
                 << receiverQuery.lastError().text();
        return SendFriendRequestResult::DatabaseError;
    }

    if (!receiverQuery.next()) {
        return SendFriendRequestResult::UserNotFound;
    }

    const qint64 receiverId = receiverQuery.value("id").toLongLong();
    const qint64 user1Id = qMin(requesterId, receiverId);
    const qint64 user2Id = qMax(requesterId, receiverId);

    QSqlQuery friendshipQuery;
    friendshipQuery.prepare(
        "SELECT 1 FROM friendships "
        "WHERE user1_id = :user1Id AND user2_id = :user2Id LIMIT 1"
    );
    friendshipQuery.bindValue(":user1Id", user1Id);
    friendshipQuery.bindValue(":user2Id", user2Id);

    if (!friendshipQuery.exec()) {
        qDebug() << "Failed to check existing friendship:"
                 << friendshipQuery.lastError().text();
        return SendFriendRequestResult::DatabaseError;
    }

    if (friendshipQuery.next()) {
        return SendFriendRequestResult::AlreadyFriends;
    }

    QSqlQuery pendingQuery;
    pendingQuery.prepare(
        "SELECT 1 FROM friend_requests "
        "WHERE status = 'pending' "
        "AND ((requester_id = :requesterId AND receiver_id = :receiverId) "
        "OR (requester_id = :receiverId AND receiver_id = :requesterId)) "
        "LIMIT 1"
    );
    pendingQuery.bindValue(":requesterId", requesterId);
    pendingQuery.bindValue(":receiverId", receiverId);

    if (!pendingQuery.exec()) {
        qDebug() << "Failed to check pending friend request:"
                 << pendingQuery.lastError().text();
        return SendFriendRequestResult::DatabaseError;
    }

    if (pendingQuery.next()) {
        return SendFriendRequestResult::RequestPending;
    }

    QSqlQuery insertQuery;
    insertQuery.prepare(
        "INSERT INTO friend_requests (requester_id, receiver_id) "
        "VALUES (:requesterId, :receiverId)"
    );
    insertQuery.bindValue(":requesterId", requesterId);
    insertQuery.bindValue(":receiverId", receiverId);

    if (!insertQuery.exec()) {
        qDebug() << "Failed to create friend request:"
                 << insertQuery.lastError().text();
        return SendFriendRequestResult::DatabaseError;
    }

    requestId = insertQuery.lastInsertId().toLongLong();
    return SendFriendRequestResult::Success;
}

bool ServerDatabase::getPendingFriendRequests(
    const QString &receiverUsername,
    QList<FriendRequestInfo> &requests
)
{
    requests.clear();

    if (receiverUsername.trimmed().isEmpty()) {
        return false;
    }

    QSqlQuery query;
    query.prepare(
        "SELECT friend_requests.id, requester.username "
        "FROM friend_requests "
        "JOIN users AS receiver "
        "ON receiver.id = friend_requests.receiver_id "
        "JOIN users AS requester "
        "ON requester.id = friend_requests.requester_id "
        "WHERE receiver.username = :receiverUsername "
        "AND friend_requests.status = 'pending' "
        "ORDER BY friend_requests.created_at, friend_requests.id"
    );
    query.bindValue(":receiverUsername", receiverUsername);

    if (!query.exec()) {
        qDebug() << "Failed to query pending friend requests:"
                 << query.lastError().text();
        return false;
    }

    while (query.next()) {
        requests.append({
            query.value("id").toLongLong(),
            query.value("username").toString()
        });
    }

    return true;
}

ServerDatabase::RespondFriendRequestResult
ServerDatabase::respondToFriendRequest(
    const QString &receiverUsername,
    qint64 requestId,
    bool accepted,
    QString &requesterUsername
)
{
    requesterUsername.clear();

    if (receiverUsername.trimmed().isEmpty() || requestId <= 0) {
        return RespondFriendRequestResult::RequestNotFound;
    }

    QSqlDatabase database = QSqlDatabase::database();

    if (!database.transaction()) {
        qDebug() << "Failed to start friend request transaction:"
                 << database.lastError().text();
        return RespondFriendRequestResult::DatabaseError;
    }

    QSqlQuery requestQuery(database);
    requestQuery.prepare(
        "SELECT friend_requests.status, friend_requests.requester_id, "
        "friend_requests.receiver_id, requester.username "
        "FROM friend_requests "
        "JOIN users AS receiver "
        "ON receiver.id = friend_requests.receiver_id "
        "JOIN users AS requester "
        "ON requester.id = friend_requests.requester_id "
        "WHERE friend_requests.id = :requestId "
        "AND receiver.username = :receiverUsername "
        "LIMIT 1"
    );
    requestQuery.bindValue(":requestId", requestId);
    requestQuery.bindValue(":receiverUsername", receiverUsername);

    if (!requestQuery.exec()) {
        qDebug() << "Failed to query friend request:"
                 << requestQuery.lastError().text();
        database.rollback();
        return RespondFriendRequestResult::DatabaseError;
    }

    if (!requestQuery.next()) {
        database.rollback();
        return RespondFriendRequestResult::RequestNotFound;
    }

    if (requestQuery.value("status").toString() != "pending") {
        database.rollback();
        return RespondFriendRequestResult::AlreadyHandled;
    }

    const qint64 requesterId =
        requestQuery.value("requester_id").toLongLong();
    const qint64 receiverId =
        requestQuery.value("receiver_id").toLongLong();
    requesterUsername = requestQuery.value("username").toString();

    if (accepted) {
        const qint64 user1Id = qMin(requesterId, receiverId);
        const qint64 user2Id = qMax(requesterId, receiverId);

        QSqlQuery friendshipQuery(database);
        friendshipQuery.prepare(
            "INSERT OR IGNORE INTO friendships (user1_id, user2_id) "
            "VALUES (:user1Id, :user2Id)"
        );
        friendshipQuery.bindValue(":user1Id", user1Id);
        friendshipQuery.bindValue(":user2Id", user2Id);

        if (!friendshipQuery.exec()) {
            qDebug() << "Failed to create accepted friendship:"
                     << friendshipQuery.lastError().text();
            database.rollback();
            return RespondFriendRequestResult::DatabaseError;
        }
    }

    QSqlQuery updateQuery(database);
    updateQuery.prepare(
        "UPDATE friend_requests "
        "SET status = :status, responded_at = CURRENT_TIMESTAMP, "
        "requester_notified = 0 "
        "WHERE id = :requestId AND status = 'pending'"
    );
    updateQuery.bindValue(":status", accepted ? "accepted" : "rejected");
    updateQuery.bindValue(":requestId", requestId);

    if (!updateQuery.exec() || updateQuery.numRowsAffected() != 1) {
        qDebug() << "Failed to update friend request:"
                 << updateQuery.lastError().text();
        database.rollback();
        return RespondFriendRequestResult::DatabaseError;
    }

    if (!database.commit()) {
        qDebug() << "Failed to commit friend request response:"
                 << database.lastError().text();
        database.rollback();
        return RespondFriendRequestResult::DatabaseError;
    }

    return RespondFriendRequestResult::Success;
}

bool ServerDatabase::getFriendRequestUpdates(
    const QString &requesterUsername,
    QList<FriendRequestUpdate> &updates
)
{
    updates.clear();

    if (requesterUsername.trimmed().isEmpty()) {
        return false;
    }

    QSqlQuery query;
    query.prepare(
        "SELECT friend_requests.id, friend_requests.status, receiver.username "
        "FROM friend_requests "
        "JOIN users AS requester "
        "ON requester.id = friend_requests.requester_id "
        "JOIN users AS receiver "
        "ON receiver.id = friend_requests.receiver_id "
        "WHERE requester.username = :requesterUsername "
        "AND friend_requests.status IN ('accepted', 'rejected') "
        "AND friend_requests.requester_notified = 0 "
        "ORDER BY friend_requests.responded_at, friend_requests.id"
    );
    query.bindValue(":requesterUsername", requesterUsername);

    if (!query.exec()) {
        qDebug() << "Failed to query friend request updates:"
                 << query.lastError().text();
        return false;
    }

    while (query.next()) {
        updates.append({
            query.value("id").toLongLong(),
            query.value("username").toString(),
            query.value("status").toString() == "accepted"
        });
    }

    return true;
}

bool ServerDatabase::markFriendRequestUpdatesNotified(
    const QList<qint64> &requestIds
)
{
    QSqlQuery query;
    query.prepare(
        "UPDATE friend_requests SET requester_notified = 1 "
        "WHERE id = :requestId"
    );

    for (qint64 requestId : requestIds) {
        query.bindValue(":requestId", requestId);

        if (!query.exec()) {
            qDebug() << "Failed to mark friend request update as notified:"
                     << query.lastError().text();
            return false;
        }
    }

    return true;
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

bool ServerDatabase::areFriends(
    const QString &username,
    const QString &friendUsername,
    bool &friends
)
{
    friends = false;

    if (username.trimmed().isEmpty()
        || friendUsername.trimmed().isEmpty()) {
        return false;
    }

    QSqlQuery query;
    query.prepare(
        "SELECT 1 "
        "FROM friendships "
        "JOIN users AS first_user ON first_user.id = friendships.user1_id "
        "JOIN users AS second_user ON second_user.id = friendships.user2_id "
        "WHERE (first_user.username = :username "
        "       AND second_user.username = :friendUsername) "
        "OR (first_user.username = :friendUsername "
        "    AND second_user.username = :username) "
        "LIMIT 1"
    );
    query.bindValue(":username", username);
    query.bindValue(":friendUsername", friendUsername);

    if (!query.exec()) {
        qDebug() << "Failed to check friendship for chat:"
                 << query.lastError().text();
        return false;
    }

    friends = query.next();
    return true;
}
