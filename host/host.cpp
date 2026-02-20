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

void setSetupCompleteCallback(std::function<void()> cb) {
    g_setupCompleteCallback = cb;
}

void start(const QString& file_path) {
    if (g_server) {
        qDebug() << "Server is already running.";
        return;
    }

    g_targetPath = file_path;
    g_server = new QTcpServer();

    int port = 8304;
    QObject::connect(g_server, &QTcpServer::newConnection, [=]() {
        while (QTcpServer* server = g_server) {
            if (!server->hasPendingConnections()) break;
            QTcpSocket* socket = server->nextPendingConnection();
            
            QObject::connect(socket, &QTcpSocket::readyRead, [socket]() {
                if (socket->property("handled").toBool()) return;
                socket->setProperty("handled", true);

                if (!socket->canReadLine()) {
                    socket->setProperty("handled", false); // Not ready yet
                    return; 
                }
                QByteArray requestLine = socket->readLine();
                
                QList<QByteArray> parts = requestLine.split(' ');
                if (parts.size() < 2 || parts[0] != "GET") {
                    socket->disconnectFromHost();
                    return;
                }
                
                QString path = QString::fromUtf8(parts[1]);
                
                // Read and discard remaining headers
                while (socket->canReadLine()) {
                    QByteArray line = socket->readLine();
                    if (line == "\r\n" || line == "\n") break;
                }

                auto sendJsonResponse = [socket](const QJsonDocument& doc) {
                    QByteArray content = doc.toJson(QJsonDocument::Compact);
                    QByteArray response = "HTTP/1.1 200 OK\r\n";
                    response += "Access-Control-Allow-Origin: *\r\n";
                    response += "Content-Type: application/json\r\n";
                    response += "Content-Length: " + QByteArray::number(content.size()) + "\r\n";
                    response += "\r\n";
                    response += content;
                    socket->write(response);
                    socket->disconnectFromHost();
                };

                QString hostDir = QCoreApplication::applicationDirPath() + "/host";
                if (!QDir(hostDir).exists()) {
                    hostDir = QDir::currentPath() + "/host";
                }

                if (path == "/") {
                    QDir dir(hostDir);
                    path = "/" + dir.relativeFilePath(g_targetPath);
                }

                if (path == "/api/covers") {
                    QJsonArray covers;
                    QDir coversDir(hostDir + "/covers");
                    QStringList nameFilters;
                    nameFilters << "*.png" << "*.jpg" << "*.jpeg" << "*.gif";
                    QStringList files = coversDir.entryList(nameFilters, QDir::Files);
                    for (const QString& file : files) {
                        covers.append("covers/" + file);
                    }
                    sendJsonResponse(QJsonDocument(covers));
                    return;
                }

                if (path == "/api/server_info") {
                    QJsonObject info;
                    info["version"] = "1.0.0";
                    info["machine"] = QHostInfo::localHostName();
                    info["description"] = "A web server for Meteor (C++ rewrite)";
                    info["owner"] = QString::fromLocal8Bit(qgetenv("USER"));
                    info["url"] = "https://github.com/scutoidzz/meteor";
                    info["cpp_accel"] = true;
                    sendJsonResponse(QJsonDocument(info));
                    return;
                }

                if (path == "/api/setup_complete") {
                    QJsonObject res;
                    res["status"] = "ok";
                    sendJsonResponse(QJsonDocument(res));
                    if (g_setupCompleteCallback) {
                        QMetaObject::invokeMethod(qApp, g_setupCompleteCallback, Qt::QueuedConnection);
                    }
                    return;
                }

                QString filePath = QDir::cleanPath(hostDir + "/" + path);
                QFile file(filePath);
                if (file.open(QIODevice::ReadOnly)) {
                    QByteArray content = file.readAll();
                    QString mimeType = "application/octet-stream";
                    if (path.endsWith(".html")) mimeType = "text/html";
                    else if (path.endsWith(".css")) mimeType = "text/css";
                    else if (path.endsWith(".js")) mimeType = "application/javascript";
                    else if (path.endsWith(".png")) mimeType = "image/png";
                    else if (path.endsWith(".jpg") || path.endsWith(".jpeg")) mimeType = "image/jpeg";
                    else if (path.endsWith(".gif")) mimeType = "image/gif";
                    
                    QByteArray response = "HTTP/1.1 200 OK\r\n";
                    response += "Access-Control-Allow-Origin: *\r\n";
                    response += "Content-Type: " + mimeType.toUtf8() + "\r\n";
                    response += "Content-Length: " + QByteArray::number(content.size()) + "\r\n";
                    response += "\r\n";
                    response += content;
                    socket->write(response);
                } else {
                    QByteArray response = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
                    socket->write(response);
                }
                socket->disconnectFromHost();
            });

            QObject::connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
        }
    });

    if (!g_server->listen(QHostAddress::Any, port)) {
        qDebug().nospace().noquote() << "Error starting server on port " << port;
        delete g_server;
        g_server = nullptr;
    } else {
        qDebug().nospace().noquote() << "Hosting started.\nApp running at: http://localhost:" << port << "/";
        qDebug().nospace().noquote() << "  C++ acceleration: yes";
    }
}

void stop() {
    if (g_server) {
        qDebug() << "Stopping host…";
        g_server->close();
        g_server->deleteLater();
        g_server = nullptr;
        qDebug() << "Host stopped.";
    } else {
        qDebug() << "No active host to stop.";
    }
}

} // namespace MeteorHost
