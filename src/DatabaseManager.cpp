#include "DatabaseManager.h"

#include <QDebug>
#include <QDateTime>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

bool DatabaseManager::openDatabase()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("chattest.db");
    if(!db.open())
    {
        qDebug() << "Failed to open database:" << db.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::createTables()
{
    QSqlQuery query;

    const QString sql = R"(
        CREATE TABLE IF NOT EXISTS messages (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            friend TEXT NOT NULL,
            sender TEXT NOT NULL,
            content TEXT NOT NULL,
            created_at TEXT NOT NULL DEFAULT ''
        )
    )";

    if (!query.exec(sql)) {
        qDebug() << "Failed to create messages table:" << query.lastError().text();
        return false;
    }

    QSqlQuery alterQuery;
    if (!alterQuery.exec("ALTER TABLE messages ADD COLUMN created_at TEXT NOT NULL DEFAULT ''")) {
        const QString errorText = alterQuery.lastError().text();
        if (!errorText.contains("duplicate column name")) {
            qDebug() << "Failed to add created_at column:" << errorText;
            return false;
        }
    }

    return true;
}

bool DatabaseManager::saveMessage(const QString &friendName, const QString &sender, const QString &content)
{
    QSqlQuery query;
    query.prepare(R"(
        INSERT INTO messages (friend, sender, content, created_at)
        VALUES (:friend, :sender, :content, :created_at)
    )");
    query.bindValue(":friend", friendName);
    query.bindValue(":sender", sender);
    query.bindValue(":content", content);
    const QString createdAt = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    query.bindValue(":created_at", createdAt);

    if (!query.exec()) {
        qDebug() << "Failed to save message:" << query.lastError().text();
        return false;
    }

    return true;
}

QString DatabaseManager::loadMessages(const QString &friendName)
{
    QSqlQuery query;
    query.prepare(R"(
        SELECT sender, content, created_at
        FROM messages
        WHERE friend = :friend
        ORDER BY id
    )");
    query.bindValue(":friend", friendName);

    if (!query.exec()) {
        qDebug() << "Failed to load messages:" << query.lastError().text();
        return QString();
    }

    QString history;

    while (query.next()) {
        const QString sender = query.value(0).toString();
        const QString content = query.value(1).toString();
        const QString createdAt = query.value(2).toString();
        if (createdAt.isEmpty()) {
            history += sender + ": " + content + "\n";
        } else {
            history += "[" + createdAt + "] " + sender + ": " + content + "\n";
        }
    }

    return history;
}
