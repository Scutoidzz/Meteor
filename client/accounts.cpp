#include "accounts.h"
#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QCoreApplication>
#include <QUrl>

namespace MeteorClient {
namespace Accounts {

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

static QString buildUrl(const QString& base, const QString& path) {
    QString url = base;
    if (!url.startsWith("http")) {
        url = "http://" + url;
    }
    if (url.endsWith("/")) url.chop(1);
    
    QString p = path;
    if (p.startsWith("/")) p.remove(0, 1);
    
    return url + "/" + p;
}

QByteArray callServer(const QString& endpoint) {
    QJsonObject config = readConfig();
    QString ip = config.value("ip").toString("127.0.0.1");
    QString port = config.value("port").toString("8304");

    QString baseUrl = ip + ":" + port;
    QString urlStr = buildUrl(baseUrl, endpoint);

    QNetworkAccessManager manager;
    QNetworkRequest request((QUrl(urlStr)));
    QNetworkReply* reply = manager.get(request);

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    QByteArray data;
    if (reply->error() == QNetworkReply::NoError) {
        data = reply->readAll();
    }
    reply->deleteLater();

    return data;
}

QJsonObject getServerInfo() {
    QByteArray data = callServer("api/server_info");
    if (data.isEmpty()) return QJsonObject();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    return doc.object();
}

QStringList getCovers() {
    QJsonObject config = readConfig();
    QString ip = config.value("ip").toString("127.0.0.1");
    QString port = config.value("port").toString("8304");
    QString baseUrl = ip + ":" + port;

    QByteArray data = callServer("api/covers");
    if (data.isEmpty()) return QStringList();
    
    QStringList urls;
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isArray()) {
        QJsonArray arr = doc.array();
        for (const QJsonValue& val : arr) {
            if (val.isString()) {
                QString item = val.toString();
                if (item.startsWith("http")) urls.append(item);
                else urls.append(buildUrl(baseUrl, item));
            } else if (val.isObject()) {
                QJsonObject obj = val.toObject();
                QString cover;
                if (obj.contains("cover")) cover = obj.value("cover").toString();
                else if (obj.contains("url")) cover = obj.value("url").toString();
                else if (obj.contains("image")) cover = obj.value("image").toString();
                
                if (!cover.isEmpty()) {
                    if (cover.startsWith("http")) urls.append(cover);
                    else urls.append(buildUrl(baseUrl, cover));
                }
            }
        }
    }
    return urls;
}

} // namespace Accounts
} // namespace MeteorClient
