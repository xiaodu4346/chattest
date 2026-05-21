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

    QLabel statusLabel;
    QPushButton loginButton("Login");

    QVBoxLayout layout;
    layout.addWidget(&titleLabel);
    layout.addWidget(&usernameEdit);
    layout.addWidget(&passwordEdit);
    layout.addWidget(&statusLabel);
    layout.addWidget(&loginButton);
   
    window.setLayout(&layout);

    QObject::connect(&loginButton, &QPushButton::clicked, [&usernameEdit, &passwordEdit, &statusLabel]() {
        const QString username = usernameEdit.text();
        const QString password = passwordEdit.text();
      
        if (username.isEmpty()) {
            statusLabel.setText("Please enter a username");
            return;
        }

        if (password.isEmpty()) {
            statusLabel.setText("Please enter a password");
            return;
        }

        statusLabel.setText("Login success. Hello, " + username);
    });

    window.show();

    return app.exec();
}
