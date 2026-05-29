#pragma once

#include <QString>

class DatabaseManager
{
public:
    bool openDatabase();
    bool createTables();
    bool saveMessage(const QString &friendName, const QString &sender, const QString &content);
    QString loadMessages(const QString &friendName);
};
