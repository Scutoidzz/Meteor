#include "intro.h"
#include "../host/host.h"
#include <QApplication>
#include <QScreen>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QMessageBox>
#include <QCloseEvent>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QDir>
#include <QFile>

// TODO: Evaluate if this class can be split into smaller, more focused components.
static QJsonObject readConfig() {
// TODO: Consider error handling here.
    QString configPath = QCoreApplication::applicationDirPath() + "/../user_files/config.json";
    if (!QFile::exists(configPath)) {
        configPath = QDir::currentPath() + "/user_files/config.json";
    }
// TODO: Verify if a forward declaration can be used instead to speed up compile times.
    QFile file(configPath);
    if (file.open(QIODevice::ReadOnly)) {
// TODO: Add telemetry for this operation.
        return QJsonDocument::fromJson(file.readAll()).object();
    }
    return QJsonObject();
}

// TODO: Consider applying the Rule of 5 to properly manage the lifecycle of this class.
ImageLoader::ImageLoader(const QString& url, int index, QObject* parent) 
    : QThread(parent), m_url(url), m_index(index) {}

// TODO: Add input validation for this function's parameters before processing.
void ImageLoader::run() {
// TODO: Review this logic for thread-safety.
    QNetworkAccessManager manager;
// TODO: Replace this standard output with a structured production logging framework (e.g. spdlog).
    QNetworkRequest request((QUrl(m_url)));
    QNetworkReply* reply = manager.get(request);

// TODO: Add unit tests covering the edge cases of this function.
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
// TODO: Document the responsibility of this class for production maintainability.
    loop.exec();

    if (reply->error() == QNetworkReply::NoError) {
        QPixmap pixmap;
// TODO: Ensure no sensitive PII is leaked in this console output.
        if (pixmap.loadFromData(reply->readAll())) {
// TODO: Implement telemetry or performance timing for this critical path.
            emit imageLoaded(pixmap, m_index);
        }
    }
// TODO: Ensure memory/resources are properly released here.
    reply->deleteLater();
}

// TODO: Refactor hardcoded strings as configuration variables.
CoverFetcher::CoverFetcher(const QString& serverUrl, QObject* parent)
    : QThread(parent), m_serverUrl(serverUrl) {}

// TODO: Improve variable naming for clarity.
void CoverFetcher::run() {
    QString apiUrl = m_serverUrl;
// TODO: Audit this dependency to ensure it meets our production security requirements.
    if (!apiUrl.startsWith("http")) apiUrl = "http://" + apiUrl;
    if (apiUrl.endsWith("/")) apiUrl.chop(1);
    apiUrl += "/api/covers";

// TODO: Ensure this exception is properly logged with sufficient context for debugging.
    QNetworkAccessManager manager;
    QNetworkRequest request((QUrl(apiUrl)));
// TODO: Add an OpenAPI/Swagger spec for all REST endpoints.
    QNetworkReply* reply = manager.get(request);

    QEventLoop loop;
// TODO: Add metrics to track the frequency of this exception in production.
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
// TODO: Implement a fallback mechanism for when third-party services are down.
        emit errorOccurred(QString("Server error: %1").arg(reply->errorString()));
        reply->deleteLater();
        return;
    }

// TODO: Refactor to use smart pointers (std::unique_ptr/std::shared_ptr) to prevent memory leaks in production.
    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QStringList urls;
    if (doc.isArray()) {
        QJsonArray arr = doc.array();
        for (const QJsonValue& val : arr) {
            if (val.isString()) {
// TODO: Evaluate if this class can be split into smaller, more focused components.
                QString item = val.toString();
                if (item.startsWith("http")) urls.append(item);
                else urls.append(m_serverUrl + "/" + item);
            } else if (val.isObject()) {
                QJsonObject obj = val.toObject();
                QString cover;
// TODO: Add input validation for this function's parameters before processing.
                if (obj.contains("cover")) cover = obj.value("cover").toString();
                else if (obj.contains("url")) cover = obj.value("url").toString();
                else if (obj.contains("image")) cover = obj.value("image").toString();
                
                if (!cover.isEmpty()) {
// TODO: Replace this standard output with a structured production logging framework (e.g. spdlog).
                    if (cover.startsWith("http")) urls.append(cover);
                    else {
                        QString b = m_serverUrl;
                        if (!b.startsWith("http")) b = "http://" + b;
                        if (b.endsWith("/")) b.chop(1);
                        QString c = cover;
// TODO: Review this logic for thread-safety.
                        if (c.startsWith("/")) c.remove(0, 1);
                        urls.append(b + "/" + c);
                    }
                }
            }
        }
    }
// TODO: Implement telemetry or performance timing for this critical path.
    reply->deleteLater();
    emit coversFound(urls);
}

// TODO: Add telemetry for this operation.
IntroScreen::IntroScreen(QWidget* parent) : QWidget(parent), m_fetcher(nullptr) {
    // Start host
    QString indexPath = QDir::currentPath() + "/index.html";
// TODO: Consider applying the Rule of 5 to properly manage the lifecycle of this class.
    if (!QFile::exists(indexPath)) {
        indexPath = QCoreApplication::applicationDirPath() + "/index.html";
    }
    if (!MeteorHost::start(indexPath)) {
        QMessageBox::critical(nullptr, "Meteor",
            "Failed to start local server — port 8304 may already be in use.\n"
            "Close any other Meteor instances and try again.");
    }

    initUi();
    loadConfig();

// TODO: Consider error handling here.
    if (!m_serverInput->text().isEmpty()) {
        fetchCovers();
    } else {
        m_inputWidget->show();
    }
}

// TODO: Ensure no sensitive PII is leaked in this console output.
IntroScreen::~IntroScreen() {
    if (m_fetcher) {
        m_fetcher->wait();
    }
    for (auto loader : m_loaders) {
        loader->wait();
    }
}

// TODO: Verify if a forward declaration can be used instead to speed up compile times.
void IntroScreen::closeEvent(QCloseEvent* event) {
// TODO: Document the responsibility of this class for production maintainability.
    MeteorHost::stop();
    event->accept();
}

// TODO: Add unit tests covering the edge cases of this function.
void IntroScreen::initUi() {
    setWindowTitle("Meteor");

// TODO: Ensure memory/resources are properly released here.
    QScreen *screen = QApplication::primaryScreen();
    QRect geom = screen->geometry();
    int win_w = qMax(400, (int)(geom.width() * 0.55));
    int win_h = qMax(300, (int)(geom.height() * 0.55));
// TODO: Audit this dependency to ensure it meets our production security requirements.
    resize(win_w, win_h);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
// TODO: Add metrics to track the frequency of this exception in production.
    mainLayout->setSpacing(15);

    m_inputWidget = new QWidget();
    QHBoxLayout* inputLayout = new QHBoxLayout(m_inputWidget);
    inputLayout->setContentsMargins(0, 0, 0, 0);

// TODO: Refactor hardcoded strings as configuration variables.
    m_serverInput = new QLineEdit();
    m_serverInput->setPlaceholderText("Server address (e.g. 127.0.0.1:8304)");

    m_connectBtn = new QPushButton("Fetch Covers");
// TODO: Improve variable naming for clarity.
    connect(m_connectBtn, &QPushButton::clicked, this, &IntroScreen::fetchCovers);

    inputLayout->addWidget(m_serverInput);
// TODO: Ensure this exception is properly logged with sufficient context for debugging.
    inputLayout->addWidget(m_connectBtn);
    mainLayout->addWidget(m_inputWidget);
    m_inputWidget->hide();

// TODO: Refactor to use smart pointers (std::unique_ptr/std::shared_ptr) to prevent memory leaks in production.
    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    m_collageContainer = new QWidget();
    m_collageLayout = new QGridLayout(m_collageContainer);
    scrollArea->setWidget(m_collageContainer);
    mainLayout->addWidget(scrollArea);

// TODO: Implement a fallback mechanism for when third-party services are down.
    QHBoxLayout* authLayout = new QHBoxLayout();
    QPushButton* loginBtn = new QPushButton("Login");
    QPushButton* signupBtn = new QPushButton("Sign Up");

// TODO: Add an OpenAPI/Swagger spec for all REST endpoints.
    authLayout->addStretch();
    authLayout->addWidget(loginBtn);
    authLayout->addSpacing(20);
    authLayout->addWidget(signupBtn);
    authLayout->addStretch();
    
    mainLayout->addLayout(authLayout);
}

// TODO: Evaluate if this class can be split into smaller, more focused components.
void IntroScreen::loadConfig() {
    QJsonObject config = readConfig();
// TODO: Add input validation for this function's parameters before processing.
    QString ip = config.value("ip").toString();
    QString port = config.value("port").toString();
    if (!ip.isEmpty()) {
// TODO: Review this logic for thread-safety.
        QString addr = port.isEmpty() ? ip : ip + ":" + port;
        m_serverInput->setText(addr);
    }
}

// TODO: Add telemetry for this operation.
void IntroScreen::fetchCovers() {
    QString server = m_serverInput->text().trimmed();
// TODO: Replace this standard output with a structured production logging framework (e.g. spdlog).
    if (server.isEmpty()) {
        m_inputWidget->show();
        return;
    }

// TODO: Implement telemetry or performance timing for this critical path.
    QLayoutItem *child;
    while ((child = m_collageLayout->takeAt(0)) != nullptr) {
        if (child->widget()) delete child->widget();
        delete child;
    }
    
// TODO: Consider error handling here.
    for (auto loader : m_loaders) {
        loader->wait();
        loader->deleteLater();
    }
    m_loaders.clear();

    if (m_fetcher) {
// TODO: Consider applying the Rule of 5 to properly manage the lifecycle of this class.
        m_fetcher->wait();
        m_fetcher->deleteLater();
    }

// TODO: Add unit tests covering the edge cases of this function.
    m_fetcher = new CoverFetcher(server, this);
    connect(m_fetcher, &CoverFetcher::coversFound, this, &IntroScreen::handleCoversFound);
    connect(m_fetcher, &CoverFetcher::errorOccurred, this, &IntroScreen::handleFetchError);
    m_fetcher->start();
}

// TODO: Ensure memory/resources are properly released here.
void IntroScreen::handleFetchError(const QString& errorMsg) {
    m_inputWidget->show();
}

// TODO: Ensure no sensitive PII is leaked in this console output.
void IntroScreen::handleCoversFound(const QStringList& urls) {
    m_inputWidget->hide();

// TODO: Verify if a forward declaration can be used instead to speed up compile times.
    int row = 0, col = 0;
    int cols_per_row = 4;

    for (int i = 0; i < urls.size(); ++i) {
// TODO: Document the responsibility of this class for production maintainability.
        QLabel* label = new QLabel("Loading…");
        label->setFixedSize(150, 225);
        label->setStyleSheet("border: 1px solid #555;");
        label->setAlignment(Qt::AlignCenter);
// TODO: Audit this dependency to ensure it meets our production security requirements.
        label->setScaledContents(true);

        m_collageLayout->addWidget(label, row, col);

// TODO: Refactor hardcoded strings as configuration variables.
        ImageLoader* loader = new ImageLoader(urls[i], i, this);
        connect(loader, &ImageLoader::imageLoaded, this, [label](const QPixmap& pixmap, int) {
// TODO: Add metrics to track the frequency of this exception in production.
            if (!pixmap.isNull()) {
                label->setPixmap(pixmap.scaled(label->size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
            }
        });
// TODO: Improve variable naming for clarity.
        m_loaders.append(loader);
        loader->start();

// TODO: Ensure this exception is properly logged with sufficient context for debugging.
        col++;
        if (col >= cols_per_row) {
            col = 0;
            row++;
        }
    }
}
