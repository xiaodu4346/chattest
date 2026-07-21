#include <QApplication>
#include "LoginWindow.h"
#include "NetworkClient.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);


    NetworkClient networkClient;

    LoginWindow window(&networkClient);
    window.show();

    return app.exec();
}
