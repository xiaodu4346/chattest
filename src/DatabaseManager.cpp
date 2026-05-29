#include "DatabaseManager.h"

#include <QDebug>
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
            content TEXT NOT NULL
        )
    )";

    if (!query.exec(sql)) {
        qDebug() << "Failed to create messages table:" << query.lastError().text();
        return false;
    }

    return true;
}

bool DatabaseManager::saveMessage(const QString &friendName, const QString &sender, const QString &content)
{
    QSqlQuery query;
    query.prepare(R"(
        INSERT INTO messages (friend, sender, content)
        VALUES (:friend, :sender, :content)
    )");
    query.bindValue(":friend", friendName);
    query.bindValue(":sender", sender);
    query.bindValue(":content", content);

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
        SELECT sender, content
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
        history += sender + ": " + content + "\n";
    }

    return history;
}
