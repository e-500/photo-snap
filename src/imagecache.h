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

#pragma once

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>

class QLocalServer;
class QLocalSocket;
class QLockFile;

struct ImageMetadata
{
    QString path;
    qint64 sizeBytes = 0;
    qint64 modifiedMs = 0;
    QString format;
    QString shootingDateTime;
    QString creationDateTime;
    QString updateDateTime;
    bool hasGpsPosition = false;
    double gpsLatitude = 0.0;
    double gpsLongitude = 0.0;
    QByteArray thumbnailPng;
    QByteArray contentHash;
};

enum class ImageDateMode : quint8
{
    ShootingDate = 0,
    CreationDate = 1
};

// One service per operating-system user. Each Photo Snap process uses this
// local singleton as a client; the first process to initialize it owns SQLite
// and serves later clients over a per-user local socket.
class ImageCache final : public QObject
{
public:
    static ImageCache &instance();

    ImageCache(const ImageCache &) = delete;
    ImageCache &operator=(const ImageCache &) = delete;

    bool initialize();
    QVector<ImageMetadata> cachedImagesOrderedByDate(int offset = 0, int limit = 1000,
                                                      const QString &year = QString(),
                                                      ImageDateMode dateMode = ImageDateMode::ShootingDate,
                                                      const QString &month = QString());
    QStringList cachedImageYearsOrdered(ImageDateMode dateMode = ImageDateMode::ShootingDate);
    QStringList cachedImageMonthsOrdered(const QString &year, ImageDateMode dateMode);
    QStringList cachedImagePaths();
    QStringList cachedImagePathsWithoutHash();
    bool addImageIfNew(const ImageMetadata &image);
    QString cachedImagePathByHash(const QByteArray &hash);
    bool updateImageHash(const QString &path, const QByteArray &hash);
    bool relinkImage(const QString &oldPath, const ImageMetadata &image);
    bool hideImage(const QString &path);
    bool unhideImage(const QString &path);
    bool deleteImageEntry(const QString &path);
    bool updateImageShootingDateTime(const QString &path, const QString &dateTime,
                                    const QString &updateDateTime = QString());
    bool isAvailable() const;
    QString databasePath() const;
    QString errorString() const;

private:
    ImageCache();
    ~ImageCache() override;

    bool openDatabase();
    bool tryConnectToService(int timeoutMs = 150);
    bool startService();
    QVector<ImageMetadata> readImagesFromDatabase(int offset, int limit, const QString &year,
                                                   ImageDateMode dateMode, const QString &month = QString());
    QStringList readImageYearsFromDatabase(ImageDateMode dateMode);
    QStringList readImageMonthsFromDatabase(const QString &year, ImageDateMode dateMode);
    bool requestImagesFromService(int offset, int limit, const QString &year, ImageDateMode dateMode,
                                  const QString &month, QVector<ImageMetadata> *images);
    bool requestImageYearsFromService(ImageDateMode dateMode, QStringList *years);
    bool requestImageMonthsFromService(const QString &year, ImageDateMode dateMode, QStringList *months);
    bool sendRequestToService(const QByteArray &request, QByteArray *response);
    void acceptConnections();
    void handleClient(QLocalSocket *socket, const QByteArray &request);

    QString m_connectionName;
    QString m_databasePath;
    QString m_serverName;
    QString m_error;
    class QSqlDatabaseHolder;
    QSqlDatabaseHolder *m_databaseHolder = nullptr;
    QLocalServer *m_server = nullptr;
    QLockFile *m_serverLock = nullptr;
    bool m_initialized = false;
    bool m_isServer = false;
};
