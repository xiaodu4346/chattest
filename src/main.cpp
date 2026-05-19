#include <QApplication>
#include <QLabel>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QLabel window("Hello ChatTest! This is our first Qt window.");
    window.resize(480, 240);
    window.setWindowTitle("ChatTest");
    window.show();

    return app.exec();
}
