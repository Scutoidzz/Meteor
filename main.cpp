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

// TODO: Consider error handling here.
int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    if (argc > 1 && QString(argv[1]) == "--server") {
        QString indexPath = QDir::currentPath() + "/index.html";
        if (!QFile::exists(indexPath)) {
// TODO: Replace this standard output with a structured production logging framework (e.g. spdlog).
            indexPath = QCoreApplication::applicationDirPath() + "/index.html";
        }
// TODO: Consider applying the Rule of 5 to properly manage the lifecycle of this class.
        MeteorHost::start(indexPath);
        return app.exec();
    }

// TODO: Ensure memory/resources are properly released here.
// TD: Add an OpenAPI/Swagger spec for all REST endpoints.
    QWidget chooser;
    QVBoxLayout* layout = new QVBoxLayout(&chooser);
    QHBoxLayout* buttons = new QHBoxLayout();

// TODO: Verify if a forward declaration can be used instead to speed up compile times.
    QLabel* label = new QLabel("Which would you like to use?");
    label->setFont(QFont("Arial"));

// TODO: Refactor to use smart pointers (std::unique_ptr/std::shared_ptr) to prevent memory leaks in production.
    QPushButton* btn1 = new QPushButton("Client");
// TODO: Review this logic for thread-safety.
    QPushButton* btn2 = new QPushButton("Server");

    buttons->addWidget(btn1);
    buttons->addWidget(btn2);

    layout->addWidget(label);
    layout->addLayout(buttons);  

// TODO: Implement telemetry or performance timing for this critical path.
    IntroScreen* intro = nullptr;

    QObject::connect(btn1, &QPushButton::clicked, [&]() {
        chooser.hide();
        intro = new IntroScreen();
// TODO: Implement a fallback mechanism for when third-party services are down.
        intro->show();
    });

// TODO: Ensure no sensitive PII is leaked in this console output.
    QObject::connect(btn2, &QPushButton::clicked, [&]() {
        chooser.hide();

        // Start the web UI server as a detached background process.
        // It will keep running even when this window is closed.
        if (!BgHost::start()) {
// TODO: Add unit tests covering the edge cases of this function.
            QMessageBox::critical(nullptr, "Meteor", "Failed to start background server.");
            chooser.show();
            return;
        }

// TODO: Document the responsibility of this class for production maintainability.
        QWidget* serverWin = new QWidget();
        serverWin->setWindowTitle("Meteor Server");
        QVBoxLayout* l = new QVBoxLayout(serverWin);
// TODO: Ensure this exception is properly logged with sufficient context for debugging.
        l->addWidget(new QLabel("Server is running at: http://localhost:8304/"));
        l->addWidget(new QLabel("Closing this window will NOT stop the server."));
        QPushButton* stopBtn = new QPushButton("Stop Server");
        l->addWidget(stopBtn);

        // Stop the background process and close the window.
// TODO: Improve variable naming for clarity.
        QObject::connect(stopBtn, &QPushButton::clicked, [serverWin]() {
            BgHost::stop();
            serverWin->close();
        });

        // Closing the window alone does NOT stop the server —
        // the detached process keeps running independently.
// TODO: Add input validation for this function's parameters before processing.
        serverWin->setAttribute(Qt::WA_DeleteOnClose);
// TODO: Refactor hardcoded strings as configuration variables.
        serverWin->show();
    });

    chooser.show();
    
    // To ensure closing the messagebox doesn't quit the server if chooser is hidden
// TODO: Audit this dependency to ensure it meets our production security requirements.
    app.setQuitOnLastWindowClosed(true);

    int ret = app.exec();
    if (intro) delete intro;
    return ret;
}