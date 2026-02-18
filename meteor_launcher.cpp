/**
 * meteor_launcher.cpp — Unified Meteor application launcher
 * ===========================================================
 * Main entry point that provides a tabbed interface to switch between:
 *   • Client intro screen (cover collage)
 *   • Accounts manager (login, sign-up, dashboard)
 *   • Server launcher
 *
 * This is the single executable that users run to access all Meteor features.
 */

#include "meteor_launcher.h"

#include <QApplication>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QWidget>
#include <QScreen>
#include <QMessageBox>
#include <QDir>
#include <QCoreApplication>
#include <QStatusBar>
#include <QFont>

// ─────────────────────────────────────────────────────────────────────────────
// Helper functions
// ─────────────────────────────────────────────────────────────────────────────

static std::pair<int, int> idealWindowSize(int screenW, int screenH, double fraction = 0.65) {
    int width = std::max(800, static_cast<int>(screenW * fraction));
    int height = std::max(600, static_cast<int>(screenH * fraction));
    return {width, height};
}

// ─────────────────────────────────────────────────────────────────────────────
// ServerLauncher implementation
// ─────────────────────────────────────────────────────────────────────────────

ServerLauncher::ServerLauncher(const QString& projectRoot, QWidget* parent)
    : QWidget(parent), m_projectRoot(projectRoot) {
    initUI();
}

void ServerLauncher::initUI() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(40, 40, 40, 40);
    layout->setSpacing(20);

    QLabel* title = new QLabel("Server Launcher");
    QFont titleFont = title->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    title->setFont(titleFont);
    layout->addWidget(title);

    QLabel* desc = new QLabel("Start and manage the Meteor server.");
    desc->setStyleSheet("color: #666;");
    layout->addWidget(desc);

    layout->addSpacing(20);

    // Server controls
    QPushButton* startBtn = new QPushButton("Start Server");
    startBtn->setMinimumHeight(44);
    connect(startBtn, &QPushButton::clicked, this, &ServerLauncher::onStartServer);
    layout->addWidget(startBtn);

    QPushButton* stopBtn = new QPushButton("Stop Server");
    stopBtn->setMinimumHeight(44);
    connect(stopBtn, &QPushButton::clicked, this, &ServerLauncher::onStopServer);
    layout->addWidget(stopBtn);

    layout->addSpacing(20);

    QLabel* statusLabel = new QLabel("Status: Not running");
    statusLabel->setStyleSheet("color: #FF6B6B;");
    layout->addWidget(statusLabel);

    layout->addStretch();
}

void ServerLauncher::onStartServer() {
    QMessageBox::information(this, "Server",
        "Would start the server at host/host.py\n(Server management would happen here)");
}

void ServerLauncher::onStopServer() {
    QMessageBox::information(this, "Server",
        "Would stop the running server\n(Server management would happen here)");
}

// ─────────────────────────────────────────────────────────────────────────────
// MeteorMainWindow implementation
// ─────────────────────────────────────────────────────────────────────────────

MeteorMainWindow::MeteorMainWindow(QWidget* parent)
    : QMainWindow(parent) {
    initUI();
}

void MeteorMainWindow::initUI() {
    setWindowTitle("Meteor");
    setWindowFlags(Qt::Window);

    // Calculate ideal window size
    QScreen* screen = QApplication::primaryScreen();
    auto [winW, winH] = idealWindowSize(screen->geometry().width(),
                                         screen->geometry().height(), 0.65);
    resize(winW, winH);

    // Apply stylesheet
    setStyleSheet(R"css(
        QMainWindow {
            background-color: #0d0d0d;
            color: #ffffff;
        }
        QTabWidget::pane {
            border: none;
            background-color: #1a1a1a;
        }
        QTabBar::tab {
            background-color: #1a1a1a;
            color: #888;
            padding: 12px 24px;
            margin-right: 4px;
            border-bottom: 2px solid #1a1a1a;
        }
        QTabBar::tab:hover {
            background-color: #252525;
        }
        QTabBar::tab:selected {
            background-color: #0d0d0d;
            color: #ffffff;
            border-bottom: 2px solid #007AFF;
        }
        QWidget {
            background-color: #0d0d0d;
            color: #ffffff;
            font-family: 'Inter', 'Segoe UI', system-ui, sans-serif;
        }
        QPushButton {
            background-color: #1a1a1a;
            color: #ffffff;
            border: 1px solid #333333;
            border-radius: 4px;
            padding: 10px 20px;
            font-weight: 500;
        }
        QPushButton:hover {
            background-color: #2a2a2a;
            border-color: #555;
        }
        QPushButton:pressed {
            background-color: #111111;
        }
    )css");

    // Create central widget with tab interface
    QTabWidget* tabWidget = new QTabWidget();
    setCentralWidget(tabWidget);

    // Get project root
    QString projectRoot = QDir(QCoreApplication::applicationDirPath()).absolutePath();

    // Create tabs
    QWidget* introStub = createIntroStub();
    QWidget* accountsStub = createAccountsStub();
    QWidget* serverStub = new ServerLauncher(projectRoot);

    tabWidget->addTab(introStub, "Intro");
    tabWidget->addTab(accountsStub, "Accounts");
    tabWidget->addTab(serverStub, "Server");

    statusBar()->showMessage("Meteor - Ready");
}

QWidget* MeteorMainWindow::createIntroStub() {
    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(40, 40, 40, 40);
    layout->setSpacing(20);

    QLabel* title = new QLabel("Client - Intro Screen");
    QFont titleFont = title->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    title->setFont(titleFont);
    layout->addWidget(title);

    QLabel* desc = new QLabel("Browse covers and connect to a Meteor server.");
    desc->setStyleSheet("color: #666;");
    layout->addWidget(desc);

    layout->addSpacing(20);

    QPushButton* launchBtn = new QPushButton("Launch Intro Screen");
    launchBtn->setMinimumHeight(44);
    connect(launchBtn, &QPushButton::clicked, this, &MeteorMainWindow::onLaunchIntro);
    layout->addWidget(launchBtn);

    layout->addStretch();
    return widget;
}

QWidget* MeteorMainWindow::createAccountsStub() {
    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(40, 40, 40, 40);
    layout->setSpacing(20);

    QLabel* title = new QLabel("Accounts");
    QFont titleFont = title->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    title->setFont(titleFont);
    layout->addWidget(title);

    QLabel* desc = new QLabel("Log in, sign up, or manage your Meteor account.");
    desc->setStyleSheet("color: #666;");
    layout->addWidget(desc);

    layout->addSpacing(20);

    QPushButton* launchBtn = new QPushButton("Launch Accounts Manager");
    launchBtn->setMinimumHeight(44);
    connect(launchBtn, &QPushButton::clicked, this, &MeteorMainWindow::onLaunchAccounts);
    layout->addWidget(launchBtn);

    layout->addStretch();
    return widget;
}

void MeteorMainWindow::onLaunchIntro() {
    QMessageBox::information(this, "Intro Screen",
        "Would launch meteorui-intro in a separate window.\n"
        "(Full integration would happen here)");
}

void MeteorMainWindow::onLaunchAccounts() {
    QMessageBox::information(this, "Accounts Manager",
        "Would launch meteorui-accounts in a separate window.\n"
        "(Full integration would happen here)");
}

// ─────────────────────────────────────────────────────────────────────────────
// Entry point
// ─────────────────────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Meteor");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Meteor");

    MeteorMainWindow window;
    window.show();

    return app.exec();
}
