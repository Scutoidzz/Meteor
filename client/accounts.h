#ifndef METEOR_ACCOUNTS_H
#define METEOR_ACCOUNTS_H

#include <QString>
#include <QJsonObject>
#include <QStringList>

namespace MeteorClient {
namespace Accounts {

QJsonObject getServerInfo();
QStringList getCovers();
QByteArray callServer(const QString& endpoint = "api/server_info");

} // namespace Accounts
} // namespace MeteorClient

#endif // METEOR_ACCOUNTS_H
