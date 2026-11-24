/*
 * Photo Snap is a free, cross-platform desktop application for managing a domestic picture archive.
 * Copyright (C) 2026 Andrea Piovesan
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "imagecache.h"

#include <QCryptographicHash>
#include <QDataStream>
#include <QDir>
#include <QFileInfo>
#include <QLocalServer>
#include <QLocalSocket>
#include <QLockFile>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QUuid>
#include <QVariant>

#include <utility>

namespace {
constexpr quint8 ListImagesRequest = 1;
constexpr quint8 ListPathsRequest = 2;
constexpr quint8 AddImageRequest = 3;
constexpr quint8 HideImageRequest = 4;
constexpr quint8 DeleteImageRequest = 5;
constexpr quint8 UnhideImageRequest = 6;
constexpr quint8 UpdateImageDateRequest = 7;
constexpr quint8 ListImageYearsRequest = 8;
constexpr quint8 ListImageMonthsRequest = 9;
constexpr quint8 ListImagesByMonthRequest = 10;
constexpr quint8 FindHashRequest = 11;
constexpr quint8 RelinkImageRequest = 12;
constexpr quint8 UpdateHashRequest = 13;
constexpr quint8 ListUnhashedPathsRequest = 14;
constexpr quint32 MaximumResponseSize = 256u * 1024u * 1024u;

QByteArray encodeImageList(const QVector<ImageMetadata> &images)
{
    QByteArray payload;
    QDataStream stream(&payload, QIODevice::WriteOnly);
    stream << true << QString() << static_cast<quint32>(images.size());
    for (const ImageMetadata &image : images) {
        stream << image.path << image.sizeBytes << image.format << image.shootingDateTime
               << image.creationDateTime << image.updateDateTime
               << image.hasGpsPosition << image.gpsLatitude << image.gpsLongitude << image.thumbnailPng;
    }
    return payload;
}

QByteArray encodeError(const QString &error)
{
    QByteArray payload;
    QDataStream stream(&payload, QIODevice::WriteOnly);
    stream << false << error << quint32(0);
    return payload;
}

QByteArray encodePaths(const QStringList &paths)
{
    QByteArray payload;
    QDataStream stream(&payload, QIODevice::WriteOnly);
    stream << true << QString() << static_cast<quint32>(paths.size());
    for (const QString &path : paths)
        stream << path;
    return payload;
}

QByteArray encodeStrings(const QStringList &strings)
{
    QByteArray payload;
    QDataStream stream(&payload, QIODevice::WriteOnly);
    stream << true << QString() << static_cast<quint32>(strings.size());
    for (const QString &value : strings)
        stream << value;
    return payload;
}

QByteArray encodeAdded(bool inserted, const QString &error = QString())
{
    QByteArray payload;
    QDataStream stream(&payload, QIODevice::WriteOnly);
    stream << error.isEmpty() << error << inserted;
    return payload;
}

QByteArray framePayload(const QByteArray &payload)
{
    QByteArray frame;
    QDataStream stream(&frame, QIODevice::WriteOnly);
    stream << static_cast<quint32>(payload.size());
    frame.append(payload);
    return frame;
}

bool readAvailable(QLocalSocket &socket, QByteArray *bytes, qsizetype required, int timeoutMs)
{
    while (bytes->size() < required) {
        if (socket.bytesAvailable() > 0)
            bytes->append(socket.readAll());
        if (bytes->size() >= required)
            break;
        if (!socket.waitForReadyRead(timeoutMs) && socket.bytesAvailable() == 0)
            return false;
    }
    return true;
}
} // namespace

class ImageCache::QSqlDatabaseHolder
{
public:
    QSqlDatabase database;
};

ImageCache &ImageCache::instance()
{
    static ImageCache cache;
    return cache;
}

ImageCache::ImageCache()
    : QObject(nullptr),
      m_connectionName(QStringLiteral("photo-snap-cache-%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces))),
      m_databaseHolder(new QSqlDatabaseHolder)
{
    // Deliberately leave SQLite and the local service unopened here. A normal
    // image open must not initialize or populate the cache.
}

ImageCache::~ImageCache()
{
    if (m_server) {
        m_server->close();
        if (m_isServer)
            QLocalServer::removeServer(m_serverName);
        delete m_server;
        m_server = nullptr;
    }
    if (m_databaseHolder) {
        m_databaseHolder->database.close();
        m_databaseHolder->database = QSqlDatabase();
        delete m_databaseHolder;
        m_databaseHolder = nullptr;
    }
    QSqlDatabase::removeDatabase(m_connectionName);
    delete m_serverLock;
    m_serverLock = nullptr;
}

bool ImageCache::initialize()
{
    if (m_initialized)
        return true;

    const QString dataDirectory = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (dataDirectory.isEmpty() || !QDir().mkpath(dataDirectory)) {
        m_error = QStringLiteral("Could not create the application data directory.");
        return false;
    }
    m_databasePath = QDir(dataDirectory).filePath(QStringLiteral("image-cache.sqlite"));
    const QByteArray identity = QDir::cleanPath(dataDirectory).toUtf8();
    const QByteArray suffix = QCryptographicHash::hash(identity, QCryptographicHash::Sha256).toHex().left(24);
    m_serverName = QStringLiteral("PhotoSnap-ImageCache-%1").arg(QString::fromLatin1(suffix));

    if (tryConnectToService()) {
        m_initialized = true;
        m_isServer = false;
        m_error.clear();
        return true;
    }

    m_serverLock = new QLockFile(QDir(dataDirectory).filePath(QStringLiteral("image-cache-service.lock")));
    m_serverLock->setStaleLockTime(30000);
    if (!m_serverLock->tryLock(0)) {
        // Another process may be between acquiring the lock and listening.
        for (int attempt = 0; attempt < 30; ++attempt) {
            if (tryConnectToService(100)) {
                m_initialized = true;
                m_isServer = false;
                m_error.clear();
                return true;
            }
        }
        m_error = QStringLiteral("Another Photo Snap process is initializing the shared image cache.");
        return false;
    }

    // Check again after taking the lock to cover a simultaneous first launch.
    if (tryConnectToService()) {
        m_serverLock->unlock();
        delete m_serverLock;
        m_serverLock = nullptr;
        m_initialized = true;
        m_isServer = false;
        m_error.clear();
        return true;
    }

    m_server = new QLocalServer(this);
    m_server->setSocketOptions(QLocalServer::UserAccessOption);
    if (!m_server->listen(m_serverName)) {
        // The lock guarantees no other Photo Snap service can own this name.
        QLocalServer::removeServer(m_serverName);
        if (!m_server->listen(m_serverName)) {
            m_error = m_server->errorString();
            delete m_server;
            m_server = nullptr;
            m_serverLock->unlock();
            delete m_serverLock;
            m_serverLock = nullptr;
            return false;
        }
    }

    if (!openDatabase()) {
        m_server->close();
        QLocalServer::removeServer(m_serverName);
        delete m_server;
        m_server = nullptr;
        m_serverLock->unlock();
        delete m_serverLock;
        m_serverLock = nullptr;
        return false;
    }

    connect(m_server, &QLocalServer::newConnection, this, &ImageCache::acceptConnections);
    m_isServer = true;
    m_initialized = true;
    m_error.clear();
    return true;
}

bool ImageCache::openDatabase()
{
    if (!QSqlDatabase::isDriverAvailable(QStringLiteral("QSQLITE"))) {
        m_error = QStringLiteral("Qt's SQLite driver is unavailable.");
        return false;
    }

    m_databaseHolder->database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    m_databaseHolder->database.setDatabaseName(m_databasePath);
    m_databaseHolder->database.setConnectOptions(QStringLiteral("QSQLITE_BUSY_TIMEOUT=5000"));
    if (!m_databaseHolder->database.open()) {
        m_error = m_databaseHolder->database.lastError().text();
        return false;
    }

    QSqlQuery query(m_databaseHolder->database);
    if (!query.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS image_cache ("
            "path TEXT PRIMARY KEY, "
            "size_bytes INTEGER NOT NULL, "
            "format TEXT NOT NULL, "
            "shooting_datetime TEXT, "
            "creation_datetime TEXT, "
            "update_datetime TEXT, "
            "gps_latitude REAL, "
            "gps_longitude REAL, "
            "modified_ms INTEGER NOT NULL, "
            "thumbnail_png BLOB, "
            "thumbnail_version INTEGER NOT NULL DEFAULT 0, "
            "content_hash BLOB, "
            "is_hidden INTEGER NOT NULL DEFAULT 0, "
            "cached_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP)"))) {
        m_error = query.lastError().text();
        m_databaseHolder->database.close();
        return false;
    }

    bool hasThumbnailColumn = false;
    bool hasThumbnailVersionColumn = false;
    bool hasHiddenColumn = false;
    bool hasCreationDateTimeColumn = false;
    bool hasUpdateDateTimeColumn = false;
    bool hasContentHashColumn = false;
    if (!query.exec(QStringLiteral("PRAGMA table_info(image_cache)"))) {
        m_error = query.lastError().text();
        m_databaseHolder->database.close();
        return false;
    }
    while (query.next()) {
        if (query.value(1).toString() == QStringLiteral("thumbnail_png"))
            hasThumbnailColumn = true;
        else if (query.value(1).toString() == QStringLiteral("thumbnail_version"))
            hasThumbnailVersionColumn = true;
        else if (query.value(1).toString() == QStringLiteral("is_hidden"))
            hasHiddenColumn = true;
        else if (query.value(1).toString() == QStringLiteral("creation_datetime"))
            hasCreationDateTimeColumn = true;
        else if (query.value(1).toString() == QStringLiteral("update_datetime"))
            hasUpdateDateTimeColumn = true;
        else if (query.value(1).toString() == QStringLiteral("content_hash"))
            hasContentHashColumn = true;
    }
    if (!hasThumbnailColumn && !query.exec(QStringLiteral("ALTER TABLE image_cache ADD COLUMN thumbnail_png BLOB"))) {
        m_error = query.lastError().text();
        m_databaseHolder->database.close();
        return false;
    }
    if (!hasThumbnailVersionColumn &&
        !query.exec(QStringLiteral("ALTER TABLE image_cache ADD COLUMN thumbnail_version INTEGER NOT NULL DEFAULT 0"))) {
        m_error = query.lastError().text();
        m_databaseHolder->database.close();
        return false;
    }
    if (!hasHiddenColumn && !query.exec(QStringLiteral("ALTER TABLE image_cache ADD COLUMN is_hidden INTEGER NOT NULL DEFAULT 0"))) {
        m_error = query.lastError().text();
        m_databaseHolder->database.close();
        return false;
    }
    if (!hasCreationDateTimeColumn &&
        !query.exec(QStringLiteral("ALTER TABLE image_cache ADD COLUMN creation_datetime TEXT"))) {
        m_error = query.lastError().text();
        m_databaseHolder->database.close();
        return false;
    }
    if (!hasUpdateDateTimeColumn &&
        !query.exec(QStringLiteral("ALTER TABLE image_cache ADD COLUMN update_datetime TEXT"))) {
        m_error = query.lastError().text();
        m_databaseHolder->database.close();
        return false;
    }
    if (!hasContentHashColumn &&
        !query.exec(QStringLiteral("ALTER TABLE image_cache ADD COLUMN content_hash BLOB"))) {
        m_error = query.lastError().text();
        m_databaseHolder->database.close();
        return false;
    }
    if (!query.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS image_cache_content_hash_idx ON image_cache(content_hash)"))) {
        m_error = query.lastError().text();
        m_databaseHolder->database.close();
        return false;
    }

    if (!hasUpdateDateTimeColumn &&
        !query.exec(QStringLiteral("UPDATE image_cache SET update_datetime = datetime(modified_ms / 1000, 'unixepoch') "
                                   "WHERE update_datetime IS NULL OR update_datetime = ''"))) {
        m_error = query.lastError().text();
        m_databaseHolder->database.close();
        return false;
    }
    if (!hasCreationDateTimeColumn) {
        QStringList paths;
        if (!query.exec(QStringLiteral("SELECT path FROM image_cache WHERE creation_datetime IS NULL OR creation_datetime = ''"))) {
            m_error = query.lastError().text();
            m_databaseHolder->database.close();
            return false;
        }
        while (query.next())
            paths.append(query.value(0).toString());
        query.finish();
        QSqlQuery updateQuery(m_databaseHolder->database);
        updateQuery.prepare(QStringLiteral("UPDATE image_cache SET creation_datetime = :created, update_datetime = "
                                           "COALESCE(NULLIF(:updated, ''), update_datetime) WHERE path = :path"));
        for (const QString &path : std::as_const(paths)) {
            const QFileInfo info(path);
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
            const QString created = info.birthTime().toString(Qt::ISODate);
#else
            const QString created = info.created().toString(Qt::ISODate);
#endif
            const QString updated = info.lastModified().toString(Qt::ISODate);
            updateQuery.bindValue(QStringLiteral(":created"), created.isEmpty() ? QVariant() : QVariant(created));
            updateQuery.bindValue(QStringLiteral(":updated"), updated.isEmpty() ? QVariant() : QVariant(updated));
            updateQuery.bindValue(QStringLiteral(":path"), path);
            if (!updateQuery.exec()) {
                m_error = updateQuery.lastError().text();
                m_databaseHolder->database.close();
                return false;
            }
        }
    }
    return true;
}

bool ImageCache::tryConnectToService(int timeoutMs)
{
    if (m_serverName.isEmpty())
        return false;
    QLocalSocket socket;
    socket.connectToServer(m_serverName);
    if (!socket.waitForConnected(timeoutMs))
        return false;
    socket.disconnectFromServer();
    return true;
}

bool ImageCache::startService()
{
    m_initialized = false;
    return initialize();
}

QVector<ImageMetadata> ImageCache::readImagesFromDatabase(int offset, int limit, const QString &year,
                                                          ImageDateMode dateMode, const QString &month)
{
    QVector<ImageMetadata> images;
    if (!m_databaseHolder || !m_databaseHolder->database.isOpen())
        return images;

    QSqlQuery query(m_databaseHolder->database);
    const QString dateColumn = dateMode == ImageDateMode::CreationDate
        ? QStringLiteral("creation_datetime") : QStringLiteral("shooting_datetime");
    QString sql = QStringLiteral(
            "SELECT path, size_bytes, format, shooting_datetime, creation_datetime, update_datetime, "
            "gps_latitude, gps_longitude, thumbnail_png "
            "FROM image_cache "
            "WHERE thumbnail_png IS NOT NULL AND thumbnail_version = 1 AND is_hidden = 0 ");
    if (year == QStringLiteral("Undated"))
        sql += QStringLiteral("AND (%1 IS NULL OR %1 = '') ").arg(dateColumn);
    else if (!year.isEmpty())
        sql += QStringLiteral("AND substr(%1, 1, 4) = :year ").arg(dateColumn);
    if (month == QStringLiteral("Undated"))
        sql += QStringLiteral("AND (%1 IS NULL OR %1 = '') ").arg(dateColumn);
    else if (!month.isEmpty() && year != QStringLiteral("Undated"))
        sql += QStringLiteral("AND substr(%1, 6, 2) = :month ").arg(dateColumn);
    sql += QStringLiteral(
            "ORDER BY CASE WHEN %1 IS NULL OR %1 = '' THEN 1 ELSE 0 END, "
            "%1 DESC, path COLLATE NOCASE ASC LIMIT :limit OFFSET :offset").arg(dateColumn);
    query.prepare(sql);
    query.bindValue(QStringLiteral(":limit"), qMax(1, limit));
    query.bindValue(QStringLiteral(":offset"), qMax(0, offset));
    if (!year.isEmpty() && year != QStringLiteral("Undated"))
        query.bindValue(QStringLiteral(":year"), year);
    if (!month.isEmpty() && month != QStringLiteral("Undated") && year != QStringLiteral("Undated"))
        query.bindValue(QStringLiteral(":month"), month);
    if (!query.exec()) {
        m_error = query.lastError().text();
        return images;
    }

    while (query.next()) {
        ImageMetadata metadata;
        metadata.path = query.value(0).toString();
        metadata.sizeBytes = query.value(1).toLongLong();
        metadata.format = query.value(2).toString();
        metadata.shootingDateTime = query.value(3).toString();
        metadata.creationDateTime = query.value(4).toString();
        metadata.updateDateTime = query.value(5).toString();
        if (!query.value(6).isNull() && !query.value(7).isNull()) {
            metadata.hasGpsPosition = true;
            metadata.gpsLatitude = query.value(6).toDouble();
            metadata.gpsLongitude = query.value(7).toDouble();
        }
        metadata.thumbnailPng = query.value(8).toByteArray();
        images.append(std::move(metadata));
    }
    return images;
}

QStringList ImageCache::readImageYearsFromDatabase(ImageDateMode dateMode)
{
    QStringList years;
    if (!m_databaseHolder || !m_databaseHolder->database.isOpen())
        return years;
    QSqlQuery query(m_databaseHolder->database);
    const QString dateColumn = dateMode == ImageDateMode::CreationDate
        ? QStringLiteral("creation_datetime") : QStringLiteral("shooting_datetime");
    const QString sql = QStringLiteral(
            "SELECT photo_year FROM ("
            "SELECT DISTINCT CASE WHEN %1 IS NULL OR %1 = '' "
            "THEN 'Undated' ELSE substr(%1, 1, 4) END AS photo_year "
            "FROM image_cache WHERE thumbnail_png IS NOT NULL AND thumbnail_version = 1 AND is_hidden = 0) "
            "ORDER BY CASE WHEN photo_year = 'Undated' THEN 1 ELSE 0 END, photo_year DESC").arg(dateColumn);
    if (!query.exec(sql)) {
        m_error = query.lastError().text();
        return years;
    }
    while (query.next())
        years.append(query.value(0).toString());
    m_error.clear();
    return years;
}

QStringList ImageCache::readImageMonthsFromDatabase(const QString &year, ImageDateMode dateMode)
{
    QStringList months;
    if (!m_databaseHolder || !m_databaseHolder->database.isOpen())
        return months;
    QSqlQuery query(m_databaseHolder->database);
    const QString dateColumn = dateMode == ImageDateMode::CreationDate
        ? QStringLiteral("creation_datetime") : QStringLiteral("shooting_datetime");
    QString sql = QStringLiteral(
        "SELECT photo_month FROM ("
        "SELECT DISTINCT CASE WHEN %1 IS NULL OR %1 = '' "
        "THEN 'Undated' ELSE substr(%1, 6, 2) END AS photo_month "
        "FROM image_cache WHERE thumbnail_png IS NOT NULL AND thumbnail_version = 1 AND is_hidden = 0 ").arg(dateColumn);
    if (year == QStringLiteral("Undated"))
        sql += QStringLiteral("AND (%1 IS NULL OR %1 = '') ").arg(dateColumn);
    else
        sql += QStringLiteral("AND substr(%1, 1, 4) = :year ").arg(dateColumn);
    sql += QStringLiteral(
        ") ORDER BY CASE WHEN photo_month = 'Undated' THEN 1 ELSE 0 END, photo_month DESC");
    query.prepare(sql);
    if (year != QStringLiteral("Undated"))
        query.bindValue(QStringLiteral(":year"), year);
    if (!query.exec()) {
        m_error = query.lastError().text();
        return months;
    }
    while (query.next())
        months.append(query.value(0).toString());
    m_error.clear();
    return months;
}

QStringList ImageCache::cachedImagePaths()
{
    if (!m_initialized && !initialize())
        return {};

    if (m_isServer) {
        QStringList paths;
        QSqlQuery query(m_databaseHolder->database);
        if (!query.exec(QStringLiteral("SELECT path FROM image_cache"))) {
            m_error = query.lastError().text();
            return paths;
        }
        while (query.next())
            paths.append(query.value(0).toString());
        return paths;
    }

    QByteArray request;
    QDataStream requestStream(&request, QIODevice::WriteOnly);
    requestStream << ListPathsRequest;
    QByteArray payload;
    if (!sendRequestToService(request, &payload))
        return {};

    QDataStream response(payload);
    bool success = false;
    QString error;
    quint32 pathCount = 0;
    response >> success >> error >> pathCount;
    if (!success || pathCount > 10000000) {
        m_error = success ? QStringLiteral("The cache service returned an invalid path count.") : error;
        return {};
    }
    QStringList paths;
    paths.reserve(static_cast<qsizetype>(pathCount));
    for (quint32 index = 0; index < pathCount; ++index) {
        QString path;
        response >> path;
        if (response.status() != QDataStream::Ok) {
            m_error = QStringLiteral("The cache service returned invalid path data.");
            return {};
        }
        paths.append(std::move(path));
    }
    m_error.clear();
    return paths;
}

QStringList ImageCache::cachedImagePathsWithoutHash()
{
    if (!m_initialized && !initialize())
        return {};
    if (!m_isServer) {
        QByteArray request;
        QDataStream stream(&request, QIODevice::WriteOnly);
        stream << ListUnhashedPathsRequest;
        QByteArray payload;
        if (!sendRequestToService(request, &payload)) return {};
        QDataStream response(payload);
        bool success = false;
        QString error;
        quint32 count = 0;
        response >> success >> error >> count;
        if (!success || count > 10000000) {
            m_error = success ? QStringLiteral("The cache service returned an invalid path count.") : error;
            return {};
        }
        QStringList paths;
        for (quint32 i = 0; i < count; ++i) {
            QString path;
            response >> path;
            paths.append(std::move(path));
        }
        m_error.clear();
        return paths;
    }
    QStringList paths;
    QSqlQuery query(m_databaseHolder->database);
    if (!query.exec(QStringLiteral("SELECT path FROM image_cache WHERE content_hash IS NULL OR content_hash = X''"))) {
        m_error = query.lastError().text();
        return paths;
    }
    while (query.next()) paths.append(query.value(0).toString());
    m_error.clear();
    return paths;
}

bool ImageCache::addImageIfNew(const ImageMetadata &image)
{
    if (!m_initialized && !initialize())
        return false;
    if (image.path.isEmpty() || image.thumbnailPng.isEmpty()) {
        m_error = QStringLiteral("A cached image requires a path and a thumbnail.");
        return false;
    }

    if (!m_isServer) {
        QByteArray request;
        QDataStream requestStream(&request, QIODevice::WriteOnly);
        requestStream << AddImageRequest << image.path << image.sizeBytes << image.format << image.shootingDateTime
                     << image.creationDateTime << image.updateDateTime
                     << image.hasGpsPosition << image.gpsLatitude << image.gpsLongitude << image.thumbnailPng
                     << image.modifiedMs << image.contentHash;
        QByteArray payload;
        if (!sendRequestToService(request, &payload))
            return false;
        QDataStream response(payload);
        bool success = false;
        QString error;
        bool inserted = false;
        response >> success >> error >> inserted;
        if (!success) {
            m_error = error;
            return false;
        }
        m_error.clear();
        return inserted;
    }

    QSqlQuery query(m_databaseHolder->database);
    query.prepare(QStringLiteral(
        "INSERT OR IGNORE INTO image_cache (path, size_bytes, format, shooting_datetime, creation_datetime, update_datetime, gps_latitude, gps_longitude, modified_ms, thumbnail_png, thumbnail_version, content_hash, cached_at) "
        "VALUES (:path, :size, :format, :shooting_datetime, :creation_datetime, :update_datetime, :latitude, :longitude, :modified_ms, :thumbnail, 1, :hash, CURRENT_TIMESTAMP)"));
    query.bindValue(QStringLiteral(":path"), image.path);
    query.bindValue(QStringLiteral(":size"), image.sizeBytes);
    query.bindValue(QStringLiteral(":format"), image.format);
    query.bindValue(QStringLiteral(":shooting_datetime"), image.shootingDateTime.isEmpty() ? QVariant() : QVariant(image.shootingDateTime));
    query.bindValue(QStringLiteral(":creation_datetime"), image.creationDateTime.isEmpty() ? QVariant() : QVariant(image.creationDateTime));
    query.bindValue(QStringLiteral(":update_datetime"), image.updateDateTime.isEmpty() ? QVariant() : QVariant(image.updateDateTime));
    query.bindValue(QStringLiteral(":latitude"), image.hasGpsPosition ? QVariant(image.gpsLatitude) : QVariant());
    query.bindValue(QStringLiteral(":longitude"), image.hasGpsPosition ? QVariant(image.gpsLongitude) : QVariant());
    query.bindValue(QStringLiteral(":modified_ms"), image.modifiedMs);
    query.bindValue(QStringLiteral(":thumbnail"), image.thumbnailPng);
    query.bindValue(QStringLiteral(":hash"), image.contentHash.isEmpty() ? QVariant() : QVariant(image.contentHash));
    if (!query.exec()) {
        m_error = query.lastError().text();
        return false;
    }
    m_error.clear();
    return query.numRowsAffected() > 0;
}

QString ImageCache::cachedImagePathByHash(const QByteArray &hash)
{
    if (hash.isEmpty() || (!m_initialized && !initialize()))
        return {};
    if (!m_isServer) {
        QByteArray request;
        QDataStream stream(&request, QIODevice::WriteOnly);
        stream << FindHashRequest << hash;
        QByteArray payload;
        if (!sendRequestToService(request, &payload))
            return {};
        QDataStream response(payload);
        bool success = false;
        QString error;
        QString path;
        response >> success >> error >> path;
        m_error = success ? QString() : error;
        return success ? path : QString();
    }
    QSqlQuery query(m_databaseHolder->database);
    query.prepare(QStringLiteral("SELECT path FROM image_cache WHERE content_hash = :hash LIMIT 1"));
    query.bindValue(QStringLiteral(":hash"), hash);
    if (!query.exec()) {
        m_error = query.lastError().text();
        return {};
    }
    m_error.clear();
    return query.next() ? query.value(0).toString() : QString();
}

bool ImageCache::updateImageHash(const QString &path, const QByteArray &hash)
{
    if (path.isEmpty() || hash.isEmpty() || (!m_initialized && !initialize()))
        return false;
    if (!m_isServer) {
        QByteArray request;
        QDataStream stream(&request, QIODevice::WriteOnly);
        stream << UpdateHashRequest << path << hash;
        QByteArray payload;
        if (!sendRequestToService(request, &payload))
            return false;
        QDataStream response(payload);
        bool success = false;
        QString error;
        bool changed = false;
        response >> success >> error >> changed;
        m_error = success ? QString() : error;
        return success;
    }
    QSqlQuery query(m_databaseHolder->database);
    query.prepare(QStringLiteral("UPDATE image_cache SET content_hash=:hash WHERE path=:path"));
    query.bindValue(QStringLiteral(":hash"), hash);
    query.bindValue(QStringLiteral(":path"), path);
    if (!query.exec()) {
        m_error = query.lastError().text();
        return false;
    }
    m_error.clear();
    return true;
}

bool ImageCache::relinkImage(const QString &oldPath, const ImageMetadata &image)
{
    if (oldPath.isEmpty() || image.path.isEmpty() || image.thumbnailPng.isEmpty() || (!m_initialized && !initialize()))
        return false;
    if (!m_isServer) {
        QByteArray request;
        QDataStream stream(&request, QIODevice::WriteOnly);
        stream << RelinkImageRequest << oldPath << image.path << image.sizeBytes << image.format
               << image.shootingDateTime << image.creationDateTime << image.updateDateTime
               << image.hasGpsPosition << image.gpsLatitude << image.gpsLongitude << image.modifiedMs
               << image.thumbnailPng << image.contentHash;
        QByteArray payload;
        if (!sendRequestToService(request, &payload))
            return false;
        QDataStream response(payload);
        bool success = false;
        QString error;
        bool changed = false;
        response >> success >> error >> changed;
        m_error = success ? QString() : error;
        return success && changed;
    }
    QSqlQuery query(m_databaseHolder->database);
    query.prepare(QStringLiteral("UPDATE image_cache SET path=:new_path, size_bytes=:size, format=:format, "
        "shooting_datetime=:shooting, creation_datetime=:creation, update_datetime=:updated, "
        "gps_latitude=:latitude, gps_longitude=:longitude, modified_ms=:modified, thumbnail_png=:thumbnail, "
        "thumbnail_version=1, content_hash=:hash, cached_at=CURRENT_TIMESTAMP WHERE path=:old_path"));
    query.bindValue(QStringLiteral(":new_path"), image.path);
    query.bindValue(QStringLiteral(":size"), image.sizeBytes);
    query.bindValue(QStringLiteral(":format"), image.format);
    query.bindValue(QStringLiteral(":shooting"), image.shootingDateTime.isEmpty() ? QVariant() : QVariant(image.shootingDateTime));
    query.bindValue(QStringLiteral(":creation"), image.creationDateTime.isEmpty() ? QVariant() : QVariant(image.creationDateTime));
    query.bindValue(QStringLiteral(":updated"), image.updateDateTime.isEmpty() ? QVariant() : QVariant(image.updateDateTime));
    query.bindValue(QStringLiteral(":latitude"), image.hasGpsPosition ? QVariant(image.gpsLatitude) : QVariant());
    query.bindValue(QStringLiteral(":longitude"), image.hasGpsPosition ? QVariant(image.gpsLongitude) : QVariant());
    query.bindValue(QStringLiteral(":modified"), image.modifiedMs);
    query.bindValue(QStringLiteral(":thumbnail"), image.thumbnailPng);
    query.bindValue(QStringLiteral(":hash"), image.contentHash);
    query.bindValue(QStringLiteral(":old_path"), oldPath);
    if (!query.exec()) {
        m_error = query.lastError().text();
        return false;
    }
    m_error.clear();
    return query.numRowsAffected() > 0;
}

bool ImageCache::hideImage(const QString &path)
{
    if (!m_initialized && !initialize())
        return false;
    if (!m_isServer) {
        QByteArray request;
        QDataStream requestStream(&request, QIODevice::WriteOnly);
        requestStream << HideImageRequest << path;
        QByteArray payload;
        if (!sendRequestToService(request, &payload))
            return false;
        QDataStream response(payload);
        bool success = false;
        QString error;
        bool changed = false;
        response >> success >> error >> changed;
        m_error = error;
        return success;
    }

    QSqlQuery query(m_databaseHolder->database);
    query.prepare(QStringLiteral("UPDATE image_cache SET is_hidden = 1 WHERE path = :path"));
    query.bindValue(QStringLiteral(":path"), path);
    if (!query.exec()) {
        m_error = query.lastError().text();
        return false;
    }
    m_error.clear();
    return true;
}

bool ImageCache::deleteImageEntry(const QString &path)
{
    if (!m_initialized && !initialize())
        return false;
    if (!m_isServer) {
        QByteArray request;
        QDataStream requestStream(&request, QIODevice::WriteOnly);
        requestStream << DeleteImageRequest << path;
        QByteArray payload;
        if (!sendRequestToService(request, &payload))
            return false;
        QDataStream response(payload);
        bool success = false;
        QString error;
        bool removed = false;
        response >> success >> error >> removed;
        m_error = error;
        return success;
    }

    QSqlQuery query(m_databaseHolder->database);
    query.prepare(QStringLiteral("DELETE FROM image_cache WHERE path = :path"));
    query.bindValue(QStringLiteral(":path"), path);
    if (!query.exec()) {
        m_error = query.lastError().text();
        return false;
    }
    m_error.clear();
    return true;
}

bool ImageCache::unhideImage(const QString &path)
{
    if (!m_initialized && !initialize())
        return false;
    if (!m_isServer) {
        QByteArray request;
        QDataStream requestStream(&request, QIODevice::WriteOnly);
        requestStream << UnhideImageRequest << path;
        QByteArray payload;
        if (!sendRequestToService(request, &payload))
            return false;
        QDataStream response(payload);
        bool success = false;
        QString error;
        bool changed = false;
        response >> success >> error >> changed;
        m_error = error;
        return success;
    }

    QSqlQuery query(m_databaseHolder->database);
    query.prepare(QStringLiteral("UPDATE image_cache SET is_hidden = 0 WHERE path = :path"));
    query.bindValue(QStringLiteral(":path"), path);
    if (!query.exec()) {
        m_error = query.lastError().text();
        return false;
    }
    m_error.clear();
    return true;
}

bool ImageCache::updateImageShootingDateTime(const QString &path, const QString &dateTime,
                                            const QString &updateDateTime)
{
    if (!m_initialized && !initialize())
        return false;
    if (path.isEmpty() || dateTime.isEmpty()) {
        m_error = QStringLiteral("A photo path and shooting date are required.");
        return false;
    }
    if (!m_isServer) {
        QByteArray request;
        QDataStream requestStream(&request, QIODevice::WriteOnly);
        requestStream << UpdateImageDateRequest << path << dateTime << updateDateTime;
        QByteArray payload;
        if (!sendRequestToService(request, &payload))
            return false;
        QDataStream response(payload);
        bool success = false;
        QString error;
        bool changed = false;
        response >> success >> error >> changed;
        m_error = error;
        return success;
    }

    QSqlQuery query(m_databaseHolder->database);
    query.prepare(QStringLiteral("UPDATE image_cache SET shooting_datetime = :date, "
                                 "update_datetime = COALESCE(NULLIF(:updated, ''), update_datetime) "
                                 "WHERE path = :path"));
    query.bindValue(QStringLiteral(":date"), dateTime);
    query.bindValue(QStringLiteral(":updated"), updateDateTime);
    query.bindValue(QStringLiteral(":path"), path);
    if (!query.exec()) {
        m_error = query.lastError().text();
        return false;
    }
    m_error.clear();
    return true;
}

bool ImageCache::sendRequestToService(const QByteArray &request, QByteArray *payload)
{
    QLocalSocket socket;
    socket.connectToServer(m_serverName);
    if (!socket.waitForConnected(1500)) {
        m_error = socket.errorString();
        return false;
    }
    const QByteArray framedRequest = framePayload(request);
    if (socket.write(framedRequest) != framedRequest.size() || !socket.waitForBytesWritten(1500)) {
        m_error = socket.errorString();
        return false;
    }

    QByteArray frame;
    if (!readAvailable(socket, &frame, sizeof(quint32), 5000)) {
        m_error = QStringLiteral("The shared image cache did not return a response.");
        return false;
    }
    QDataStream header(frame);
    quint32 payloadSize = 0;
    header >> payloadSize;
    if (payloadSize > MaximumResponseSize) {
        m_error = QStringLiteral("The shared image cache response is too large.");
        return false;
    }
    if (!readAvailable(socket, &frame, static_cast<qsizetype>(sizeof(quint32)) + payloadSize, 30000)) {
        m_error = QStringLiteral("The shared image cache response was incomplete.");
        return false;
    }

    *payload = frame.mid(sizeof(quint32), payloadSize);
    return true;
}

bool ImageCache::requestImagesFromService(int offset, int limit, const QString &year, ImageDateMode dateMode,
                                          const QString &month, QVector<ImageMetadata> *images)
{
    QByteArray request;
    QDataStream requestStream(&request, QIODevice::WriteOnly);
    const quint8 operation = month.isEmpty() ? ListImagesRequest : ListImagesByMonthRequest;
    requestStream << operation << qMax(0, offset) << qMax(1, limit) << year
                  << static_cast<quint8>(dateMode);
    if (!month.isEmpty())
        requestStream << month;
    QByteArray payload;
    if (!sendRequestToService(request, &payload))
        return false;

    QDataStream response(payload);
    bool success = false;
    QString error;
    quint32 imageCount = 0;
    response >> success >> error >> imageCount;
    if (!success) {
        m_error = error;
        return false;
    }
    if (imageCount > 1000000) {
        m_error = QStringLiteral("The shared image cache returned an invalid image count.");
        return false;
    }

    images->clear();
    images->reserve(static_cast<qsizetype>(imageCount));
    for (quint32 index = 0; index < imageCount; ++index) {
        ImageMetadata metadata;
        response >> metadata.path >> metadata.sizeBytes >> metadata.format >> metadata.shootingDateTime
                 >> metadata.creationDateTime >> metadata.updateDateTime
                 >> metadata.hasGpsPosition >> metadata.gpsLatitude >> metadata.gpsLongitude >> metadata.thumbnailPng;
        if (response.status() != QDataStream::Ok) {
            images->clear();
            m_error = QStringLiteral("The shared image cache returned invalid image data.");
            return false;
        }
        images->append(std::move(metadata));
    }

    m_error.clear();
    return true;
}

bool ImageCache::requestImageYearsFromService(ImageDateMode dateMode, QStringList *years)
{
    QByteArray request;
    QDataStream requestStream(&request, QIODevice::WriteOnly);
    requestStream << ListImageYearsRequest << static_cast<quint8>(dateMode);
    QByteArray payload;
    if (!sendRequestToService(request, &payload))
        return false;
    QDataStream response(payload);
    bool success = false;
    QString error;
    quint32 count = 0;
    response >> success >> error >> count;
    if (!success) {
        m_error = error;
        return false;
    }
    if (count > 1000) {
        m_error = QStringLiteral("The shared image cache returned an invalid year list.");
        return false;
    }
    QStringList result;
    result.reserve(static_cast<qsizetype>(count));
    for (quint32 index = 0; index < count; ++index) {
        QString year;
        response >> year;
        if (response.status() != QDataStream::Ok) {
            m_error = QStringLiteral("The shared image cache returned invalid year data.");
            return false;
        }
        result.append(std::move(year));
    }
    *years = result;
    m_error.clear();
    return true;
}

bool ImageCache::requestImageMonthsFromService(const QString &year, ImageDateMode dateMode, QStringList *months)
{
    QByteArray request;
    QDataStream requestStream(&request, QIODevice::WriteOnly);
    requestStream << ListImageMonthsRequest << year << static_cast<quint8>(dateMode);
    QByteArray payload;
    if (!sendRequestToService(request, &payload))
        return false;
    QDataStream response(payload);
    bool success = false;
    QString error;
    quint32 count = 0;
    response >> success >> error >> count;
    if (!success) {
        m_error = error;
        return false;
    }
    if (count > 13) {
        m_error = QStringLiteral("The shared image cache returned an invalid month list.");
        return false;
    }
    QStringList result;
    result.reserve(static_cast<qsizetype>(count));
    for (quint32 index = 0; index < count; ++index) {
        QString month;
        response >> month;
        if (response.status() != QDataStream::Ok) {
            m_error = QStringLiteral("The shared image cache returned invalid month data.");
            return false;
        }
        result.append(std::move(month));
    }
    *months = result;
    m_error.clear();
    return true;
}

void ImageCache::acceptConnections()
{
    while (m_server && m_server->hasPendingConnections()) {
        QLocalSocket *socket = m_server->nextPendingConnection();
        connect(socket, &QLocalSocket::readyRead, this, [this, socket]() {
            QByteArray frame = socket->property("imageCacheRequestFrame").toByteArray();
            frame.append(socket->readAll());
            if (frame.size() < static_cast<qsizetype>(sizeof(quint32))) {
                socket->setProperty("imageCacheRequestFrame", frame);
                return;
            }
            QDataStream header(frame);
            quint32 requestSize = 0;
            header >> requestSize;
            if (requestSize > MaximumResponseSize) {
                socket->disconnectFromServer();
                return;
            }
            const qsizetype totalSize = static_cast<qsizetype>(sizeof(quint32)) + requestSize;
            if (frame.size() < totalSize) {
                socket->setProperty("imageCacheRequestFrame", frame);
                return;
            }
            socket->setProperty("imageCacheRequestHandled", true);
            handleClient(socket, frame.mid(sizeof(quint32), requestSize));
        });
        connect(socket, &QLocalSocket::disconnected, socket, &QLocalSocket::deleteLater);
    }
}

void ImageCache::handleClient(QLocalSocket *socket, const QByteArray &request)
{
    QDataStream requestStream(request);
    quint8 operation = 0;
    requestStream >> operation;
    QByteArray payload;
    if (operation == ListImagesRequest || operation == ListImagesByMonthRequest) {
        int offset = 0;
        int limit = 1000;
        QString year;
        QString month;
        quint8 mode = 0;
        requestStream >> offset >> limit >> year >> mode;
        if (operation == ListImagesByMonthRequest)
            requestStream >> month;
        if (requestStream.status() != QDataStream::Ok || offset < 0 || limit <= 0 || limit > 10000 || mode > 1)
            payload = encodeError(QStringLiteral("The cache requested an invalid image page."));
        else
            payload = encodeImageList(readImagesFromDatabase(offset, limit, year,
                                                              static_cast<ImageDateMode>(mode), month));
    } else if (operation == ListImageYearsRequest) {
        quint8 mode = 0;
        requestStream >> mode;
        if (requestStream.status() != QDataStream::Ok || mode > 1)
            payload = encodeError(QStringLiteral("The cache requested an invalid date mode."));
        else
            payload = encodeStrings(readImageYearsFromDatabase(static_cast<ImageDateMode>(mode)));
    } else if (operation == ListImageMonthsRequest) {
        QString year;
        quint8 mode = 0;
        requestStream >> year >> mode;
        if (requestStream.status() != QDataStream::Ok || year.isEmpty() || mode > 1)
            payload = encodeError(QStringLiteral("The cache requested an invalid month list."));
        else
            payload = encodeStrings(readImageMonthsFromDatabase(year, static_cast<ImageDateMode>(mode)));
    } else if (operation == ListPathsRequest) {
        QStringList paths;
        QSqlQuery query(m_databaseHolder->database);
        if (!query.exec(QStringLiteral("SELECT path FROM image_cache")))
            payload = encodeError(query.lastError().text());
        else {
            while (query.next())
                paths.append(query.value(0).toString());
            payload = encodePaths(paths);
        }
    } else if (operation == ListUnhashedPathsRequest) {
        QStringList paths;
        QSqlQuery query(m_databaseHolder->database);
        if (!query.exec(QStringLiteral("SELECT path FROM image_cache WHERE content_hash IS NULL OR content_hash = X''")))
            payload = encodeError(query.lastError().text());
        else {
            while (query.next()) paths.append(query.value(0).toString());
            payload = encodePaths(paths);
        }
    } else if (operation == AddImageRequest) {
        ImageMetadata image;
        requestStream >> image.path >> image.sizeBytes >> image.format >> image.shootingDateTime
                      >> image.creationDateTime >> image.updateDateTime
                      >> image.hasGpsPosition >> image.gpsLatitude >> image.gpsLongitude >> image.thumbnailPng
                      >> image.modifiedMs >> image.contentHash;
        bool inserted = false;
        QString error;
        if (requestStream.status() != QDataStream::Ok || image.path.isEmpty() || image.thumbnailPng.isEmpty()) {
            error = QStringLiteral("The scanner sent invalid image metadata.");
        } else {
            QSqlQuery query(m_databaseHolder->database);
            query.prepare(QStringLiteral(
                "INSERT OR IGNORE INTO image_cache (path, size_bytes, format, shooting_datetime, creation_datetime, update_datetime, gps_latitude, gps_longitude, modified_ms, thumbnail_png, thumbnail_version, content_hash, cached_at) "
                "VALUES (:path, :size, :format, :shooting_datetime, :creation_datetime, :update_datetime, :latitude, :longitude, :modified_ms, :thumbnail, 1, :hash, CURRENT_TIMESTAMP)"));
            query.bindValue(QStringLiteral(":path"), image.path);
            query.bindValue(QStringLiteral(":size"), image.sizeBytes);
            query.bindValue(QStringLiteral(":format"), image.format);
            query.bindValue(QStringLiteral(":shooting_datetime"), image.shootingDateTime.isEmpty() ? QVariant() : QVariant(image.shootingDateTime));
            query.bindValue(QStringLiteral(":creation_datetime"), image.creationDateTime.isEmpty() ? QVariant() : QVariant(image.creationDateTime));
            query.bindValue(QStringLiteral(":update_datetime"), image.updateDateTime.isEmpty() ? QVariant() : QVariant(image.updateDateTime));
            query.bindValue(QStringLiteral(":latitude"), image.hasGpsPosition ? QVariant(image.gpsLatitude) : QVariant());
            query.bindValue(QStringLiteral(":longitude"), image.hasGpsPosition ? QVariant(image.gpsLongitude) : QVariant());
            query.bindValue(QStringLiteral(":modified_ms"), image.modifiedMs);
            query.bindValue(QStringLiteral(":thumbnail"), image.thumbnailPng);
            query.bindValue(QStringLiteral(":hash"), image.contentHash.isEmpty() ? QVariant() : QVariant(image.contentHash));
            if (query.exec())
                inserted = query.numRowsAffected() > 0;
            else
                error = query.lastError().text();
        }

        payload = encodeAdded(inserted, error);
    } else if (operation == FindHashRequest) {
        QByteArray hash;
        requestStream >> hash;
        QString path;
        QString error;
        if (requestStream.status() != QDataStream::Ok || hash.isEmpty()) {
            error = QStringLiteral("The scanner sent an invalid image hash.");
        } else {
            QSqlQuery query(m_databaseHolder->database);
            query.prepare(QStringLiteral("SELECT path FROM image_cache WHERE content_hash=:hash LIMIT 1"));
            query.bindValue(QStringLiteral(":hash"), hash);
            if (query.exec()) {
                if (query.next()) path = query.value(0).toString();
            } else error = query.lastError().text();
        }
        QByteArray result;
        QDataStream response(&result, QIODevice::WriteOnly);
        response << error.isEmpty() << error << path;
        payload = result;
    } else if (operation == RelinkImageRequest) {
        ImageMetadata image;
        QString oldPath;
        requestStream >> oldPath >> image.path >> image.sizeBytes >> image.format
                      >> image.shootingDateTime >> image.creationDateTime >> image.updateDateTime
                      >> image.hasGpsPosition >> image.gpsLatitude >> image.gpsLongitude >> image.modifiedMs
                      >> image.thumbnailPng >> image.contentHash;
        bool changed = false;
        QString error;
        if (requestStream.status() != QDataStream::Ok || oldPath.isEmpty() || image.path.isEmpty() || image.thumbnailPng.isEmpty()) {
            error = QStringLiteral("The scanner sent invalid relink metadata.");
        } else {
            QSqlQuery query(m_databaseHolder->database);
            query.prepare(QStringLiteral("UPDATE image_cache SET path=:new_path, size_bytes=:size, format=:format, "
                "shooting_datetime=:shooting, creation_datetime=:creation, update_datetime=:updated, "
                "gps_latitude=:latitude, gps_longitude=:longitude, modified_ms=:modified, thumbnail_png=:thumbnail, "
                "thumbnail_version=1, content_hash=:hash, cached_at=CURRENT_TIMESTAMP WHERE path=:old_path"));
            query.bindValue(QStringLiteral(":new_path"), image.path);
            query.bindValue(QStringLiteral(":size"), image.sizeBytes);
            query.bindValue(QStringLiteral(":format"), image.format);
            query.bindValue(QStringLiteral(":shooting"), image.shootingDateTime.isEmpty() ? QVariant() : QVariant(image.shootingDateTime));
            query.bindValue(QStringLiteral(":creation"), image.creationDateTime.isEmpty() ? QVariant() : QVariant(image.creationDateTime));
            query.bindValue(QStringLiteral(":updated"), image.updateDateTime.isEmpty() ? QVariant() : QVariant(image.updateDateTime));
            query.bindValue(QStringLiteral(":latitude"), image.hasGpsPosition ? QVariant(image.gpsLatitude) : QVariant());
            query.bindValue(QStringLiteral(":longitude"), image.hasGpsPosition ? QVariant(image.gpsLongitude) : QVariant());
            query.bindValue(QStringLiteral(":modified"), image.modifiedMs);
            query.bindValue(QStringLiteral(":thumbnail"), image.thumbnailPng);
            query.bindValue(QStringLiteral(":hash"), image.contentHash);
            query.bindValue(QStringLiteral(":old_path"), oldPath);
            if (query.exec()) changed = query.numRowsAffected() > 0;
            else error = query.lastError().text();
        }
        QByteArray result;
        QDataStream response(&result, QIODevice::WriteOnly);
        response << error.isEmpty() << error << changed;
        payload = result;
    } else if (operation == UpdateHashRequest) {
        QString path;
        QByteArray hash;
        requestStream >> path >> hash;
        QString error;
        bool changed = false;
        if (requestStream.status() != QDataStream::Ok || path.isEmpty() || hash.isEmpty()) {
            error = QStringLiteral("The scanner sent an invalid image hash update.");
        } else {
            QSqlQuery query(m_databaseHolder->database);
            query.prepare(QStringLiteral("UPDATE image_cache SET content_hash=:hash WHERE path=:path"));
            query.bindValue(QStringLiteral(":hash"), hash);
            query.bindValue(QStringLiteral(":path"), path);
            if (query.exec()) changed = query.numRowsAffected() > 0;
            else error = query.lastError().text();
        }
        QByteArray result;
        QDataStream response(&result, QIODevice::WriteOnly);
        response << error.isEmpty() << error << changed;
        payload = result;
    } else if (operation == UpdateImageDateRequest) {
        QString path;
        QString dateTime;
        QString updateDateTime;
        requestStream >> path >> dateTime >> updateDateTime;
        bool changed = false;
        QString error;
        if (requestStream.status() != QDataStream::Ok || path.isEmpty() || dateTime.isEmpty()) {
            error = QStringLiteral("The cache action sent an invalid shooting date.");
        } else {
            QSqlQuery query(m_databaseHolder->database);
            query.prepare(QStringLiteral("UPDATE image_cache SET shooting_datetime = :date, "
                                         "update_datetime = COALESCE(NULLIF(:updated, ''), update_datetime) "
                                         "WHERE path = :path"));
            query.bindValue(QStringLiteral(":date"), dateTime);
            query.bindValue(QStringLiteral(":updated"), updateDateTime);
            query.bindValue(QStringLiteral(":path"), path);
            if (query.exec())
                changed = true;
            else
                error = query.lastError().text();
        }
        payload = encodeAdded(changed, error);
    } else if (operation == HideImageRequest || operation == DeleteImageRequest || operation == UnhideImageRequest) {
        QString path;
        requestStream >> path;
        bool success = false;
        bool changed = false;
        QString error;
        if (requestStream.status() != QDataStream::Ok || path.isEmpty()) {
            error = QStringLiteral("The cache action sent an invalid image path.");
        } else {
            QSqlQuery query(m_databaseHolder->database);
            if (operation == HideImageRequest) {
                query.prepare(QStringLiteral("UPDATE image_cache SET is_hidden = 1 WHERE path = :path"));
            } else if (operation == UnhideImageRequest) {
                query.prepare(QStringLiteral("UPDATE image_cache SET is_hidden = 0 WHERE path = :path"));
            } else {
                query.prepare(QStringLiteral("DELETE FROM image_cache WHERE path = :path"));
            }
            query.bindValue(QStringLiteral(":path"), path);
            if (query.exec()) {
                success = true;
                changed = query.numRowsAffected() > 0;
            } else {
                error = query.lastError().text();
            }
        }
        payload = encodeAdded(changed, error);
        if (!error.isEmpty())
            payload = encodeAdded(false, error);
    } else {
        payload = encodeError(QStringLiteral("Unsupported image cache request."));
    }

    socket->write(framePayload(payload));
    socket->flush();
    socket->disconnectFromServer();
}

QVector<ImageMetadata> ImageCache::cachedImagesOrderedByDate(int offset, int limit, const QString &year,
                                                            ImageDateMode dateMode, const QString &month)
{
    if (!m_initialized && !initialize())
        return {};

    if (m_isServer)
        return readImagesFromDatabase(offset, limit, year, dateMode, month);

    QVector<ImageMetadata> images;
    if (requestImagesFromService(offset, limit, year, dateMode, month, &images))
        return images;

    // The process that owned the service may have exited. Re-elect an owner
    // lazily and retry once so remaining app instances can keep using cache.
    if (!startService())
        return {};
    if (m_isServer)
        return readImagesFromDatabase(offset, limit, year, dateMode, month);
    requestImagesFromService(offset, limit, year, dateMode, month, &images);
    return images;
}

QStringList ImageCache::cachedImageYearsOrdered(ImageDateMode dateMode)
{
    if (!m_initialized && !initialize())
        return {};
    if (m_isServer)
        return readImageYearsFromDatabase(dateMode);

    QStringList years;
    if (requestImageYearsFromService(dateMode, &years))
        return years;
    if (!startService())
        return {};
    if (m_isServer)
        return readImageYearsFromDatabase(dateMode);
    requestImageYearsFromService(dateMode, &years);
    return years;
}

QStringList ImageCache::cachedImageMonthsOrdered(const QString &year, ImageDateMode dateMode)
{
    if (!m_initialized && !initialize())
        return {};
    if (m_isServer)
        return readImageMonthsFromDatabase(year, dateMode);

    QStringList months;
    if (requestImageMonthsFromService(year, dateMode, &months))
        return months;
    if (!startService())
        return {};
    if (m_isServer)
        return readImageMonthsFromDatabase(year, dateMode);
    requestImageMonthsFromService(year, dateMode, &months);
    return months;
}

bool ImageCache::isAvailable() const
{
    return m_initialized;
}

QString ImageCache::databasePath() const
{
    return m_databasePath;
}

QString ImageCache::errorString() const
{
    return m_error;
}
