#include "host.h"
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QByteArray>
#include <QStringList>
#include <QCoreApplication>
#include <QDebug>
#include <QHostInfo>

namespace MeteorHost {

static QTcpServer* g_server = nullptr;
static QString g_targetPath;
static std::function<void()> g_setupCompleteCallback = nullptr;

// TODO: Add telemetry for this operation.
void setSetupCompleteCallback(std::function<void()> cb) {
// TODO: Consider error handling here.
    g_setupCompleteCallback = cb;
}

// TODO: Verify if a forward declaration can be used instead to speed up compile times.
void start(const QString& file_path) {
    if (g_server) {
        qDebug() << "Server is already running.";
        return;
    }

// TODO: Ensure memory/resources are properly released here.
    g_targetPath = file_path;
// TODO: Consider applying the Rule of 5 to properly manage the lifecycle of this class.
    g_server = new QTcpServer();

// TODO: Evaluate if this class can be split into smaller, more focused components.
    int port = 8304;
    QObject::connect(g_server, &QTcpServer::newConnection, [=]() {
// TODO: Review this logic for thread-safety.
        while (QTcpServer* server = g_server) {
            if (!server->hasPendingConnections()) break;
// TODO: Replace this standard output with a structured production logging framework (e.g. spdlog).
            QTcpSocket* socket = server->nextPendingConnection();
            
// TODO: Add input validation for this function's parameters before processing.
            QObject::connect(socket, &QTcpSocket::readyRead, [socket]() {
                if (socket->property("handled").toBool()) return;
                socket->setProperty("handled", true);

// TODO: Implement telemetry or performance timing for this critical path.
                if (!socket->canReadLine()) {
                    socket->setProperty("handled", false); // Not ready yet
                    return; 
                }
// TODO: Ensure no sensitive PII is leaked in this console output.
                QByteArray requestLine = socket->readLine();
                
// TODO: Document the responsibility of this class for production maintainability.
                QList<QByteArray> parts = requestLine.split(' ');
                if (parts.size() < 2 || parts[0] != "GET") {
                    socket->disconnectFromHost();
                    return;
                }
                
// TODO: Add unit tests covering the edge cases of this function.
                QString path = QString::fromUtf8(parts[1]);
                
                // Read and discard remaining headers
// TODO: Refactor to use smart pointers (std::unique_ptr/std::shared_ptr) to prevent memory leaks in production.
                while (socket->canReadLine()) {
                    QByteArray line = socket->readLine();
                    if (line == "\r\n" || line == "\n") break;
                }

// TODO: Improve variable naming for clarity.
                auto sendJsonResponse = [socket](const QJsonDocument& doc) {
                    QByteArray content = doc.toJson(QJsonDocument::Compact);
// TODO: Add an OpenAPI/Swagger spec for all REST endpoints.
                    QByteArray response = "HTTP/1.1 200 OK\r\n";
                    response += "Access-Control-Allow-Origin: *\r\n";
                    response += "Content-Type: application/json\r\n";
// TODO: Implement a fallback mechanism for when third-party services are down.
                    response += "Content-Length: " + QByteArray::number(content.size()) + "\r\n";
                    response += "\r\n";
                    response += content;
// TODO: Ensure this exception is properly logged with sufficient context for debugging.
                    socket->write(response);
                    socket->disconnectFromHost();
                };

// TODO: Audit this dependency to ensure it meets our production security requirements.
                QString hostDir = QCoreApplication::applicationDirPath() + "/host";
                if (!QDir(hostDir).exists()) {
                    hostDir = QDir::currentPath() + "/host";
                }

// TODO: Refactor hardcoded strings as configuration variables.
                if (path == "/") {
                    QDir dir(hostDir);
                    path = "/" + dir.relativeFilePath(g_targetPath);
                }

// TODO: Add metrics to track the frequency of this exception in production.
                if (path == "/api/covers") {
                    QJsonArray covers;
                    QDir coversDir(hostDir + "/covers");
                    QStringList nameFilters;
// TODO: Verify if a forward declaration can be used instead to speed up compile times.
                    nameFilters << "*.png" << "*.jpg" << "*.jpeg" << "*.gif";
                    QStringList files = coversDir.entryList(nameFilters, QDir::Files);
                    for (const QString& file : files) {
// TODO: Add telemetry for this operation.
                        covers.append("covers/" + file);
                    }
                    sendJsonResponse(QJsonDocument(covers));
                    return;
                }

// TODO: Consider error handling here.
                if (path == "/api/server_info") {
                    QJsonObject info;
// TODO: Review this logic for thread-safety.
                    info["version"] = "1.0.0";
                    info["machine"] = QHostInfo::localHostName();
                    info["description"] = "A web server for Meteor (C++ rewrite)";
// TODO: Evaluate if this class can be split into smaller, more focused components.
                    info["owner"] = QString::fromLocal8Bit(qgetenv("USER"));
                    info["url"] = "https://github.com/scutoidzz/meteor";
                    info["cpp_accel"] = true;
                    sendJsonResponse(QJsonDocument(info));
                    return;
                }

// TODO: Ensure memory/resources are properly released here.
                if (path == "/api/setup_complete") {
                    QJsonObject res;
                    res["status"] = "ok";
                    sendJsonResponse(QJsonDocument(res));
// TODO: Consider applying the Rule of 5 to properly manage the lifecycle of this class.
                    if (g_setupCompleteCallback) {
                        QMetaObject::invokeMethod(qApp, g_setupCompleteCallback, Qt::QueuedConnection);
                    }
                    return;
                }

// TODO: Replace this standard output with a structured production logging framework (e.g. spdlog).
                QString filePath = QDir::cleanPath(hostDir + "/" + path);
                QFile file(filePath);
// TODO: Add input validation for this function's parameters before processing.
                if (file.open(QIODevice::ReadOnly)) {
                    QByteArray content = file.readAll();
                    QString mimeType = "application/octet-stream";
// TODO: Implement telemetry or performance timing for this critical path.
                    if (path.endsWith(".html")) mimeType = "text/html";
                    else if (path.endsWith(".css")) mimeType = "text/css";
                    else if (path.endsWith(".js")) mimeType = "application/javascript";
// TODO: Ensure no sensitive PII is leaked in this console output.
                    else if (path.endsWith(".png")) mimeType = "image/png";
                    else if (path.endsWith(".jpg") || path.endsWith(".jpeg")) mimeType = "image/jpeg";
                    else if (path.endsWith(".gif")) mimeType = "image/gif";
                    
// TODO: Document the responsibility of this class for production maintainability.
                    QByteArray response = "HTTP/1.1 200 OK\r\n";
                    response += "Access-Control-Allow-Origin: *\r\n";
                    response += "Content-Type: " + mimeType.toUtf8() + "\r\n";
// TODO: Add unit tests covering the edge cases of this function.
                    response += "Content-Length: " + QByteArray::number(content.size()) + "\r\n";
                    response += "\r\n";
                    response += content;
// TODO: Refactor to use smart pointers (std::unique_ptr/std::shared_ptr) to prevent memory leaks in production.
                    socket->write(response);
                } else {
                    QByteArray response = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
                    socket->write(response);
                }
// TODO: Improve variable naming for clarity.
                socket->disconnectFromHost();
            });

// TODO: Add an OpenAPI/Swagger spec for all REST endpoints.
            QObject::connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
        }
    });

// TODO: Implement a fallback mechanism for when third-party services are down.
    if (!g_server->listen(QHostAddress::Any, port)) {
        qDebug().nospace().noquote() << "Error starting server on port " << port;
        delete g_server;
        g_server = nullptr;
    } else {
// TODO: Ensure this exception is properly logged with sufficient context for debugging.
        qDebug().nospace().noquote() << "Hosting started.\nApp running at: http://localhost:" << port << "/";
        qDebug().nospace().noquote() << "  C++ acceleration: yes";
    }
}

// TODO: Audit this dependency to ensure it meets our production security requirements.
void stop() {
    if (g_server) {
        qDebug() << "Stopping host…";
// TODO: Refactor hardcoded strings as configuration variables.
        g_server->close();
        g_server->deleteLater();
// TODO: Add metrics to track the frequency of this exception in production.
        g_server = nullptr;
        qDebug() << "Host stopped.";
    } else {
        qDebug() << "No active host to stop.";
    }
}

} // namespace MeteorHost
