#include <QApplication>
#include <QMessageBox>
#include "client/intro.h"
#include "host/host.h"
#include "host/bghost.h"
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFont>
#include <QDir>
#include <QCoreApplication>
#include <QFile>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    if (argc > 1 && QString(argv[1]) == "--server") {
        QString indexPath = QDir::currentPath() + "/index.html";
        if (!QFile::exists(indexPath)) {
            indexPath = QCoreApplication::applicationDirPath() + "/index.html";
        }
        MeteorHost::start(indexPath);
        return app.exec();
    }

    QWidget chooser;
    QVBoxLayout* layout = new QVBoxLayout(&chooser);
    QHBoxLayout* buttons = new QHBoxLayout();

    QLabel* label = new QLabel("Which would you like to use?");
    label->setFont(QFont("Arial"));

    QPushButton* btn1 = new QPushButton("Client");
    QPushButton* btn2 = new QPushButton("Server");

    buttons->addWidget(btn1);
    buttons->addWidget(btn2);

    layout->addWidget(label);
    layout->addLayout(buttons);  

    IntroScreen* intro = nullptr;

    QObject::connect(btn1, &QPushButton::clicked, [&]() {
        chooser.hide();
        intro = new IntroScreen();
        intro->show();
    });

    QObject::connect(btn2, &QPushButton::clicked, [&]() {
        chooser.hide();

        // Start the web UI server as a detached background process.
        // It will keep running even when this window is closed.
        if (!BgHost::start()) {
            QMessageBox::critical(nullptr, "Meteor", "Failed to start background server.");
            chooser.show();
            return;
        }

        QWidget* serverWin = new QWidget();
        serverWin->setWindowTitle("Meteor Server");
        QVBoxLayout* l = new QVBoxLayout(serverWin);
        l->addWidget(new QLabel("Server is running at: http://localhost:8304/"));
        l->addWidget(new QLabel("Closing this window will NOT stop the server."));
        QPushButton* stopBtn = new QPushButton("Stop Server");
        l->addWidget(stopBtn);

        // Stop the background process and close the window.
        QObject::connect(stopBtn, &QPushButton::clicked, [serverWin]() {
            BgHost::stop();
            serverWin->close();
        });

        // Closing the window alone does NOT stop the server —
        // the detached process keeps running independently.
        serverWin->setAttribute(Qt::WA_DeleteOnClose);
        serverWin->show();
    });

    chooser.show();
    
    // To ensure closing the messagebox doesn't quit the server if chooser is hidden
    app.setQuitOnLastWindowClosed(true);

    int ret = app.exec();
    if (intro) delete intro;
    return ret;
}