#include <QApplication>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QWidget window;
    window.resize(480, 240);
    window.setWindowTitle("ChatTest Login");

    QLabel titleLabel("Welcome to ChatTest");

    QLineEdit usernameEdit;
    usernameEdit.setPlaceholderText("Username");

    QLineEdit passwordEdit;
    passwordEdit.setPlaceholderText("Password");
    passwordEdit.setEchoMode(QLineEdit::Password);

    QPushButton loginButton("Login");

    QVBoxLayout layout;
    layout.addWidget(&titleLabel);
    layout.addWidget(&usernameEdit);
    layout.addWidget(&passwordEdit);
    layout.addWidget(&loginButton);

    window.setLayout(&layout);

    QObject::connect(&loginButton, &QPushButton::clicked, [&window, &usernameEdit]() {
        const QString username = usernameEdit.text();

        if (username.isEmpty()) {
            window.setWindowTitle("Please enter a username");
            return;
        }

        window.setWindowTitle("Hello, " + username);
    });

    window.show();

    return app.exec();
}
