#ifndef METEOR_CONSTRUCT_H
#define METEOR_CONSTRUCT_H

#include <QString>
#include <QStringList>
#include <QSize>

namespace MeteorClient {
namespace Construct {

QStringList listImages(const QString& directory);
bool thumbnail(const QString& srcPath, const QString& destPath, const QSize& size = QSize(150, 225));
QStringList coversAsThumbnails(const QString& coversDir, const QString& outDir, const QSize& size = QSize(150, 225));

} // namespace Construct
} // namespace MeteorClient

#endif // METEOR_CONSTRUCT_H
