/**
 * client/accounts.cpp — Server communication helpers
 * --------------------------------------------------
 * C++ library providing functions to call the Meteor host server's API endpoints.
 *
 * This is a direct port of the Python accounts.py module, providing:
 *   • Configuration reading from user_files/config.json
 *   • URL assembly with proper schemes and slashes
 *   • API endpoint calls to the server
 *   • Helper functions for getting server info and cover lists
 */

#include <QString>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QTimer>
#include <QDebug>

#include <string>
#include <map>
#include <vector>
#include <memory>

// ─────────────────────────────────────────────────────────────────────────────
// Configuration and defaults
// ─────────────────────────────────────────────────────────────────────────────

// Default server configuration
const char* DEFAULT_IP   = "127.0.0.1";
const char* DEFAULT_PORT = "8304";

// ─────────────────────────────────────────────────────────────────────────────
// Helper functions (C++ equivalents of Python helpers)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Read a flat JSON config file.
 * Returns a map of key-value pairs.
 * Returns empty map if file doesn't exist or can't be parsed.
 */
static std::map<std::string, std::string> readConfig(const QString& path) {
    std::map<std::string, std::string> config;

    QFile file(path);
    if (!file.exists() || !file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return config;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        return config;
    }

    QJsonObject obj = doc.object();
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        // Convert JSON values to strings
        QString key = it.key();
        QJsonValue value = it.value();

        if (value.isString()) {
            config[key.toStdString()] = value.toString().toStdString();
        } else if (value.isDouble()) {
            config[key.toStdString()] = QString::number(value.toInt()).toStdString();
        }
    }

    return config;
}

/**
 * Assemble a URL with correct scheme and slashes.
 * Equivalent to Python's _build_url().
 */
static QString buildUrl(const QString& base, const QString& path) {
    QString urlBase = base;

    // Add http:// if no scheme present
    if (!urlBase.startsWith("http://") && !urlBase.startsWith("https://")) {
        urlBase = "http://" + urlBase;
    }

    // Remove trailing slash from base
    if (urlBase.endsWith("/")) {
        urlBase.chop(1);
    }

    // Remove leading slash from path
    QString urlPath = path;
    if (urlPath.startsWith("/")) {
        urlPath.remove(0, 1);
    }

    return urlBase + "/" + urlPath;
}

// ─────────────────────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Make a GET request to the specified endpoint on the configured server.
 *
 * Reads server configuration from user_files/config.json.
 * Falls back to DEFAULT_IP:DEFAULT_PORT if not configured.
 *
 * @param endpoint The API endpoint (e.g., "api/server_info")
 * @param projectRoot Path to the project root (for finding config.json)
 * @return The HTTP response as a QString (JSON or empty on error)
 */
QString callServer(const QString& endpoint, const QString& projectRoot) {
    // Build config path
    QString configPath = projectRoot + "/user_files/config.json";

    // Read configuration
    auto config = readConfig(configPath);

    std::string ip   = config.count("ip")   ? config["ip"]   : DEFAULT_IP;
    std::string port = config.count("port") ? config["port"] : DEFAULT_PORT;

    // Build the full URL
    QString baseUrl = QString::fromStdString(ip + ":" + port);
    QString url = buildUrl(baseUrl, endpoint);

    // Make the HTTP request
    QNetworkAccessManager manager;
    QNetworkRequest request{QUrl(url)};

    // Set a timeout
    QNetworkReply* reply = manager.get(request);

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);

    // Signal connections for async waiting
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);

    timer.start(10000);  // 10 second timeout
    loop.exec();

    if (timer.isActive()) {
        timer.stop();
    } else {
        // Timeout occurred
        reply->abort();
        delete reply;
        return "";
    }

    QString result = "";
    if (reply->error() == QNetworkReply::NoError) {
        result = QString::fromUtf8(reply->readAll());
    }

    delete reply;
    return result;
}

/**
 * Return the server's /api/server_info dict as a JSON object.
 * Returns empty object on error.
 */
QJsonObject getServerInfo(const QString& projectRoot) {
    QString response = callServer("api/server_info", projectRoot);
    if (response.isEmpty()) {
        return QJsonObject();
    }

    QJsonDocument doc = QJsonDocument::fromJson(response.toUtf8());
    if (doc.isObject()) {
        return doc.object();
    }

    return QJsonObject();
}

/**
 * Return the list of cover URLs from /api/covers.
 * Returns empty list on error.
 */
QStringList getCovers(const QString& projectRoot) {
    QStringList urls;

    // Read config to get base URL
    QString configPath = projectRoot + "/user_files/config.json";
    auto config = readConfig(configPath);

    std::string ip   = config.count("ip")   ? config["ip"]   : DEFAULT_IP;
    std::string port = config.count("port") ? config["port"] : DEFAULT_PORT;
    QString baseUrl = QString::fromStdString(ip + ":" + port);

    // Fetch covers from API
    QString response = callServer("api/covers", projectRoot);
    if (response.isEmpty()) {
        return urls;
    }

    QJsonDocument doc = QJsonDocument::fromJson(response.toUtf8());
    if (!doc.isArray()) {
        return urls;
    }

    QJsonArray array = doc.array();
    for (const QJsonValue& item : array) {
        if (item.isString()) {
            QString urlStr = item.toString();
            if (urlStr.startsWith("http://") || urlStr.startsWith("https://")) {
                urls.append(urlStr);
            } else {
                urls.append(buildUrl(baseUrl, urlStr));
            }
        } else if (item.isObject()) {
            QJsonObject obj = item.toObject();
            QString cover = obj.value("cover").toString(
                obj.value("url").toString(
                    obj.value("image").toString()
                )
            );
            if (!cover.isEmpty()) {
                if (cover.startsWith("http://") || cover.startsWith("https://")) {
                    urls.append(cover);
                } else {
                    urls.append(buildUrl(baseUrl, cover));
                }
            }
        }
    }

    return urls;
}

/**
 * Get server info as a plain C++ interface (for easier integration).
 * Returns a map of key-value pairs from the server info endpoint.
 */
std::map<std::string, std::string> getServerInfoMap(const QString& projectRoot) {
    std::map<std::string, std::string> result;

    QJsonObject info = getServerInfo(projectRoot);
    for (auto it = info.begin(); it != info.end(); ++it) {
        QString key = it.key();
        QJsonValue value = it.value();

        if (value.isString()) {
            result[key.toStdString()] = value.toString().toStdString();
        } else if (value.isDouble()) {
            result[key.toStdString()] = QString::number(value.toInt()).toStdString();
        }
    }

    return result;
}
