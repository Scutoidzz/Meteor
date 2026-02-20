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

static QJsonObject readConfig() {
    QString configPath = QCoreApplication::applicationDirPath() + "/../user_files/config.json";
    if (!QFile::exists(configPath)) {
        configPath = QDir::currentPath() + "/user_files/config.json";
    }
    QFile file(configPath);
    if (file.open(QIODevice::ReadOnly)) {
        return QJsonDocument::fromJson(file.readAll()).object();
    }
    return QJsonObject();
}

ImageLoader::ImageLoader(const QString& url, int index, QObject* parent) 
    : QThread(parent), m_url(url), m_index(index) {}

void ImageLoader::run() {
    QNetworkAccessManager manager;
    QNetworkRequest request((QUrl(m_url)));
    QNetworkReply* reply = manager.get(request);

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() == QNetworkReply::NoError) {
        QPixmap pixmap;
        if (pixmap.loadFromData(reply->readAll())) {
            emit imageLoaded(pixmap, m_index);
        }
    }
    reply->deleteLater();
}

CoverFetcher::CoverFetcher(const QString& serverUrl, QObject* parent)
    : QThread(parent), m_serverUrl(serverUrl) {}

void CoverFetcher::run() {
    QString apiUrl = m_serverUrl;
    if (!apiUrl.startsWith("http")) apiUrl = "http://" + apiUrl;
    if (apiUrl.endsWith("/")) apiUrl.chop(1);
    apiUrl += "/api/covers";

    QNetworkAccessManager manager;
    QNetworkRequest request((QUrl(apiUrl)));
    QNetworkReply* reply = manager.get(request);

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred(QString("Server error: %1").arg(reply->errorString()));
        reply->deleteLater();
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QStringList urls;
    if (doc.isArray()) {
        QJsonArray arr = doc.array();
        for (const QJsonValue& val : arr) {
            if (val.isString()) {
                QString item = val.toString();
                if (item.startsWith("http")) urls.append(item);
                else urls.append(m_serverUrl + "/" + item);
            } else if (val.isObject()) {
                QJsonObject obj = val.toObject();
                QString cover;
                if (obj.contains("cover")) cover = obj.value("cover").toString();
                else if (obj.contains("url")) cover = obj.value("url").toString();
                else if (obj.contains("image")) cover = obj.value("image").toString();
                
                if (!cover.isEmpty()) {
                    if (cover.startsWith("http")) urls.append(cover);
                    else {
                        QString b = m_serverUrl;
                        if (!b.startsWith("http")) b = "http://" + b;
                        if (b.endsWith("/")) b.chop(1);
                        QString c = cover;
                        if (c.startsWith("/")) c.remove(0, 1);
                        urls.append(b + "/" + c);
                    }
                }
            }
        }
    }
    reply->deleteLater();
    emit coversFound(urls);
}

IntroScreen::IntroScreen(QWidget* parent) : QWidget(parent), m_fetcher(nullptr) {
    // Start host
    QString indexPath = QDir::currentPath() + "/index.html";
    if (!QFile::exists(indexPath)) {
        indexPath = QCoreApplication::applicationDirPath() + "/index.html";
    }
    MeteorHost::start(indexPath);

    initUi();
    loadConfig();

    if (!m_serverInput->text().isEmpty()) {
        fetchCovers();
    } else {
        m_inputWidget->show();
    }
}

IntroScreen::~IntroScreen() {
    if (m_fetcher) {
        m_fetcher->wait();
    }
    for (auto loader : m_loaders) {
        loader->wait();
    }
}

void IntroScreen::closeEvent(QCloseEvent* event) {
    MeteorHost::stop();
    event->accept();
}

void IntroScreen::initUi() {
    setWindowTitle("Meteor");

    QScreen *screen = QApplication::primaryScreen();
    QRect geom = screen->geometry();
    int win_w = qMax(400, (int)(geom.width() * 0.55));
    int win_h = qMax(300, (int)(geom.height() * 0.55));
    resize(win_w, win_h);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    m_inputWidget = new QWidget();
    QHBoxLayout* inputLayout = new QHBoxLayout(m_inputWidget);
    inputLayout->setContentsMargins(0, 0, 0, 0);

    m_serverInput = new QLineEdit();
    m_serverInput->setPlaceholderText("Server address (e.g. 127.0.0.1:8304)");

    m_connectBtn = new QPushButton("Fetch Covers");
    connect(m_connectBtn, &QPushButton::clicked, this, &IntroScreen::fetchCovers);

    inputLayout->addWidget(m_serverInput);
    inputLayout->addWidget(m_connectBtn);
    mainLayout->addWidget(m_inputWidget);
    m_inputWidget->hide();

    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    m_collageContainer = new QWidget();
    m_collageLayout = new QGridLayout(m_collageContainer);
    scrollArea->setWidget(m_collageContainer);
    mainLayout->addWidget(scrollArea);

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
    QJsonObject config = readConfig();
    QString ip = config.value("ip").toString();
    QString port = config.value("port").toString();
    if (!ip.isEmpty()) {
        QString addr = port.isEmpty() ? ip : ip + ":" + port;
        m_serverInput->setText(addr);
    }
}

void IntroScreen::fetchCovers() {
    QString server = m_serverInput->text().trimmed();
    if (server.isEmpty()) {
        m_inputWidget->show();
        return;
    }

    QLayoutItem *child;
    while ((child = m_collageLayout->takeAt(0)) != nullptr) {
        if (child->widget()) delete child->widget();
        delete child;
    }
    
    for (auto loader : m_loaders) {
        loader->wait();
        loader->deleteLater();
    }
    m_loaders.clear();

    if (m_fetcher) {
        m_fetcher->wait();
        m_fetcher->deleteLater();
    }

    m_fetcher = new CoverFetcher(server, this);
    connect(m_fetcher, &CoverFetcher::coversFound, this, &IntroScreen::handleCoversFound);
    connect(m_fetcher, &CoverFetcher::errorOccurred, this, &IntroScreen::handleFetchError);
    m_fetcher->start();
}

void IntroScreen::handleFetchError(const QString& errorMsg) {
    m_inputWidget->show();
}

void IntroScreen::handleCoversFound(const QStringList& urls) {
    m_inputWidget->hide();

    int row = 0, col = 0;
    int cols_per_row = 4;

    for (int i = 0; i < urls.size(); ++i) {
        QLabel* label = new QLabel("Loading…");
        label->setFixedSize(150, 225);
        label->setStyleSheet("border: 1px solid #555;");
        label->setAlignment(Qt::AlignCenter);
        label->setScaledContents(true);

        m_collageLayout->addWidget(label, row, col);

        ImageLoader* loader = new ImageLoader(urls[i], i, this);
        connect(loader, &ImageLoader::imageLoaded, this, [label](const QPixmap& pixmap, int) {
            if (!pixmap.isNull()) {
                label->setPixmap(pixmap.scaled(label->size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
            }
        });
        m_loaders.append(loader);
        loader->start();

        col++;
        if (col >= cols_per_row) {
            col = 0;
            row++;
        }
    }
}
