/**
 * client/accounts.h — Server communication helpers (header)
 * =========================================================
 * Public interface for the accounts module providing API calls to the server.
 */

#ifndef METEOR_ACCOUNTS_H
#define METEOR_ACCOUNTS_H

#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <map>
#include <string>

/**
 * Make a GET request to the specified endpoint on the configured server.
 *
 * Reads server configuration from user_files/config.json.
 * Falls back to 127.0.0.1:8304 if not configured.
 *
 * @param endpoint The API endpoint (e.g., "api/server_info")
 * @param projectRoot Path to the project root (for finding config.json)
 * @return The HTTP response as a QString (JSON or empty on error)
 */
QString callServer(const QString& endpoint, const QString& projectRoot);

/**
 * Return the server's /api/server_info dict as a JSON object.
 * Returns empty object on error.
 *
 * @param projectRoot Path to the project root
 * @return QJsonObject containing server information
 */
QJsonObject getServerInfo(const QString& projectRoot);

/**
 * Return the list of cover URLs from /api/covers.
 * Returns empty list on error.
 *
 * @param projectRoot Path to the project root
 * @return QStringList of cover image URLs
 */
QStringList getCovers(const QString& projectRoot);

/**
 * Get server info as a plain C++ interface (easier integration).
 * Returns a map of key-value pairs from the server info endpoint.
 *
 * @param projectRoot Path to the project root
 * @return std::map of server information
 */
std::map<std::string, std::string> getServerInfoMap(const QString& projectRoot);

#endif // METEOR_ACCOUNTS_H
