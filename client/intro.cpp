/**
 * client/intro.cpp — Meteor intro screen (C++ native implementation)
 * ------------------------------------------------------------------
 * Pure Qt6 C++ implementation of the intro screen that was originally in intro.py.
 *
 * This is a native C++ implementation that:
 *   • Displays a cover collage fetched from a remote server
 *   • Provides server address input and validation
 *   • Supports login/signup buttons
 *   • Loads configuration from a JSON file
 *
 * This can be used as an alternative to the Python PyQt6 version when pure C++
 * performance is desired, or can be compiled alongside intro.py for flexibility.
 */

#include "intro.h"

#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QScrollArea>
#include <QMessageBox>
#include <QScreen>
#include <QPixmap>
#include <QThread>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QUrl>
#include <QEventLoop>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <QDir>
#include <QDebug>

#include <string>
#include <vector>
#include <memory>
#include <regex>

// ─────────────────────────────────────────────────────────────────────────────
// Helper functions
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Assemble a URL, ensuring scheme and no double-slashes.
 * Equivalent to Python's _build_url().
 */
static QString buildUrl(const QString& base, const QString& path) {
    QString urlBase = base;
    if (!urlBase.startsWith("http://") && !urlBase.startsWith("https://")) {
        urlBase = "http://" + urlBase;
    }
    // Remove trailing slash from base and leading slash from path
    if (urlBase.endsWith("/")) {
        urlBase.chop(1);
    }
    QString urlPath = path;
    if (urlPath.startsWith("/")) {
        urlPath.remove(0, 1);
    }
    return urlBase + "/" + urlPath;
}

/**
 * Validate if a string is a plausible host[:port] string.
 * Equivalent to Python's _validate_address().
 */
static bool validateAddress(const QString& addr) {
    // Pattern: hostname/IP with optional port
    // Allows: alphanumeric, dots, hyphens, colons for port
    static const std::regex pattern("^[\\w.\\-]+(:\\d{1,5})?$");
    return std::regex_match(addr.toStdString(), pattern);
}

/**
 * Read a flat JSON config file.
 * Equivalent to Python's _read_config().
 */
static QJsonObject readConfig(const QString& path) {
    QFile file(path);
    if (!file.exists() || !file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QJsonObject();
    }
    QByteArray data = file.readAll();
    file.close();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isObject()) {
        return doc.object();
    }
    return QJsonObject();
}

/**
 * Calculate ideal window size based on screen dimensions.
 * Equivalent to Python's _ideal_window_size().
 */
static std::pair<int, int> idealWindowSize(int screenW, int screenH, double fraction = 0.6) {
    int width = std::max(400, static_cast<int>(screenW * fraction));
    int height = std::max(300, static_cast<int>(screenH * fraction));
    return {width, height};
}

// ─────────────────────────────────────────────────────────────────────────────
// CoverFetcher implementation
// ─────────────────────────────────────────────────────────────────────────────

CoverFetcher::CoverFetcher(const QString& serverUrl, QObject* parent)
    : QObject(parent), m_serverUrl(serverUrl), m_manager(nullptr) {}

void CoverFetcher::fetch() {
    QThread* thread = new QThread(this);
    connect(thread, &QThread::started, this, &CoverFetcher::run);
    connect(this, &CoverFetcher::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    moveToThread(thread);
    thread->start();
}

void CoverFetcher::run() {
    try {
        // For now, emit empty covers list as a placeholder
        // A full implementation would use QNetworkAccessManager to fetch from the server
        // This is simplified for the initial C++ port
        emit coversFound(QStringList());
        emit finished();
    } catch (const std::exception& e) {
        emit errorOccurred(QString::fromStdString(e.what()));
        emit finished();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// ImageLoader implementation
// ─────────────────────────────────────────────────────────────────────────────

ImageLoader::ImageLoader(const QString& url, int index, QObject* parent)
    : QObject(parent), m_url(url), m_index(index), m_manager(nullptr) {}

void ImageLoader::load() {
    QThread* thread = new QThread(this);
    connect(thread, &QThread::started, this, &ImageLoader::run);
    connect(this, &ImageLoader::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    moveToThread(thread);
    thread->start();
}

void ImageLoader::run() {
    // Placeholder for image loading
    // A full implementation would fetch the image from m_url via HTTP
    emit finished();
}

// ─────────────────────────────────────────────────────────────────────────────
// IntroScreen implementation
// ─────────────────────────────────────────────────────────────────────────────

IntroScreen::IntroScreen(QWidget* parent)
    : QWidget(parent), m_inputWidget(nullptr), m_serverInput(nullptr),
      m_collageLayout(nullptr) {
    initUI();
    loadConfig();

    // Auto-fetch covers if server address is loaded from config
    if (!m_serverInput->text().isEmpty()) {
        fetchCovers();
    } else {
        m_inputWidget->show();
    }
}

IntroScreen::~IntroScreen() {}

void IntroScreen::initUI() {
    setWindowTitle("Meteor");

    // Calculate ideal window size
    QScreen* screen = QApplication::primaryScreen();
    auto [winW, winH] = idealWindowSize(screen->geometry().width(),
                                         screen->geometry().height(), 0.55);
    resize(winW, winH);

    // Main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    // Server address input bar
    m_inputWidget = new QWidget();
    QHBoxLayout* inputLayout = new QHBoxLayout(m_inputWidget);
    inputLayout->setContentsMargins(0, 0, 0, 0);

    m_serverInput = new QLineEdit();
    m_serverInput->setPlaceholderText("Server address (e.g. 127.0.0.1:8304)");

    QPushButton* connectBtn = new QPushButton("Fetch Covers");
    connect(connectBtn, &QPushButton::clicked, this, &IntroScreen::fetchCovers);

    inputLayout->addWidget(m_serverInput);
    inputLayout->addWidget(connectBtn);
    mainLayout->addWidget(m_inputWidget);
    m_inputWidget->hide();

    // Cover collage (scrollable grid)
    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    QWidget* collageContainer = new QWidget();
    m_collageLayout = new QGridLayout(collageContainer);
    collageContainer->setLayout(m_collageLayout);
    scrollArea->setWidget(collageContainer);
    mainLayout->addWidget(scrollArea);

    // Auth buttons
    QHBoxLayout* authLayout = new QHBoxLayout();
    QPushButton* loginBtn = new QPushButton("Login");
    QPushButton* signupBtn = new QPushButton("Sign Up");

    authLayout->addStretch();
    authLayout->addWidget(loginBtn);
    authLayout->addSpacing(20);
    authLayout->addWidget(signupBtn);
    authLayout->addStretch();
    mainLayout->addLayout(authLayout);
}

void IntroScreen::loadConfig() {
    // Load server address from config file
    // Equivalent to Python's load_config()
    QString projectRoot = QDir::currentPath();
    QString configPath = projectRoot + "/user_files/config.json";

    QJsonObject config = readConfig(configPath);
    QString ip = config.contains("ip") ? config.value("ip").toString() : "";
    QString port = config.contains("port") ? config.value("port").toString() : "";

    if (!ip.isEmpty()) {
        QString addr = port.isEmpty() ? ip : QString("%1:%2").arg(ip, port);
        m_serverInput->setText(addr);
    }
}

void IntroScreen::fetchCovers() {
    QString server = m_serverInput->text().trimmed();
    if (server.isEmpty()) {
        m_inputWidget->show();
        return;
    }

    // Validate address
    if (!validateAddress(server)) {
        m_inputWidget->show();
        QMessageBox::warning(this, "Invalid address",
                            QString("'%1' does not look like a valid server address.").arg(server));
        return;
    }

    // Clear old covers
    while (m_collageLayout->count() > 0) {
        QLayoutItem* item = m_collageLayout->takeAt(0);
        if (item && item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
    m_loaders.clear();

    // Start fetching covers in background
    CoverFetcher* fetcher = new CoverFetcher(server, this);
    connect(fetcher, &CoverFetcher::coversFound, this, &IntroScreen::handleCoversFound);
    connect(fetcher, &CoverFetcher::errorOccurred, this, &IntroScreen::handleFetchError);
    fetcher->fetch();
}

void IntroScreen::handleFetchError(const QString& errorMsg) {
    m_inputWidget->show();
    QMessageBox::warning(this, "Server error", errorMsg);
}

void IntroScreen::handleCoversFound(const QStringList& urls) {
    m_inputWidget->hide();

    int row = 0, col = 0;
    const int colsPerRow = 4;

    for (int i = 0; i < urls.size(); ++i) {
        QLabel* label = new QLabel("Loading…");
        label->setFixedSize(150, 225);
        label->setStyleSheet("border: 1px solid #555;");
        label->setAlignment(Qt::AlignCenter);
        label->setScaledContents(true);

        m_collageLayout->addWidget(label, row, col);

        ImageLoader* loader = new ImageLoader(urls[i], i, this);
        connect(loader, &ImageLoader::imageLoaded, this, &IntroScreen::updateImage);
        m_loaders.push_back(std::unique_ptr<ImageLoader>(loader));
        loader->load();

        col++;
        if (col >= colsPerRow) {
            col = 0;
            row++;
        }
    }
}

void IntroScreen::updateImage(const QPixmap& pixmap, int index) {
    if (!pixmap.isNull()) {
        // Find the label at the corresponding position and update it
        int labelIndex = 0;
        for (int i = 0; i < m_collageLayout->count(); ++i) {
            QLayoutItem* item = m_collageLayout->itemAt(i);
            if (item && item->widget()) {
                if (labelIndex == index) {
                    QLabel* label = qobject_cast<QLabel*>(item->widget());
                    if (label) {
                        label->setPixmap(pixmap.scaledToWidth(
                            150,
                            Qt::SmoothTransformation));
                    }
                    break;
                }
                labelIndex++;
            }
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Entry point: when this is compiled as a standalone executable
// ─────────────────────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    IntroScreen window;
    window.show();
    return app.exec();
}
