#include <QApplication>
#include <QDebug>

#include "DatabaseManager.h"
#include "LoginWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    DatabaseManager databaseManager;

    if (!databaseManager.openDatabase()) {
        qDebug() << "Database open failed.";
    }

    if (!databaseManager.createTables()) {
        qDebug() << "Database table creation failed.";
    }

    LoginWindow window;
    window.show();

    return app.exec();
}
