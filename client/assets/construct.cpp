#include "construct.h"
#include <QDir>
#include <QImage>
#include <QFileInfo>
#include <QDebug>

namespace MeteorClient {
namespace Construct {

QStringList listImages(const QString& directory) {
    QDir dir(directory);
    if (!dir.exists()) {
        return QStringList();
    }
    
    QStringList nameFilters;
    nameFilters << "*.png" << "*.jpg" << "*.jpeg" << "*.gif" << "*.webp" << "*.bmp";
    
    QStringList files = dir.entryList(nameFilters, QDir::Files, QDir::Name);
    return files;
}

bool thumbnail(const QString& srcPath, const QString& destPath, const QSize& size) {
    QImage image;
    if (!image.load(srcPath)) {
        qDebug() << "[construct] thumbnail() error: unable to load" << srcPath;
        return false;
    }
    
    QImage thumb = image.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    if (!thumb.save(destPath)) {
        qDebug() << "[construct] thumbnail() error: unable to save" << destPath;
        return false;
    }
    
    return true;
}

QStringList coversAsThumbnails(const QString& coversDir, const QString& outDir, const QSize& size) {
    QDir().mkpath(outDir);
    
    QStringList results;
    QStringList images = listImages(coversDir);
    
    for (const QString& name : images) {
        QString src = QDir(coversDir).filePath(name);
        QString dest = QDir(outDir).filePath(name);
        if (thumbnail(src, dest, size)) {
            results.append(dest);
        }
    }
    
    return results;
}

} // namespace Construct
} // namespace MeteorClient
