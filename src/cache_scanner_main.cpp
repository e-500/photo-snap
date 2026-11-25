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
#include "imagemetadata.h"

#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QImageReader>
#include <QCryptographicHash>
#include <QFile>
#include <QHash>
#include <QSet>
#include <QTextStream>
#include <QCommandLineParser>
#include <QThread>

#include <future>
#include <utility>

#ifdef Q_OS_WIN
#include <windows.h>
#else
#include <sys/select.h>
#include <unistd.h>
#endif

namespace {

QString pathKey(const QString &path)
{
    QString key = QDir::cleanPath(QDir::fromNativeSeparators(path));
#ifdef Q_OS_WIN
    key = key.toCaseFolded();
#endif
    return key;
}

bool isWithinRoot(const QString &path, const QString &rootKey)
{
    QString prefix = rootKey;
    if (!prefix.endsWith(QLatin1Char('/')))
        prefix.append(QLatin1Char('/'));
    return pathKey(path).startsWith(prefix);
}

QStringList readControlCommands(QString *pending);

QByteArray hashFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    QCryptographicHash hash(QCryptographicHash::Sha256);
    while (!file.atEnd()) {
        const QByteArray chunk = file.read(1024 * 1024);
        if (chunk.isEmpty() && file.error() != QFileDevice::NoError)
            return {};
        hash.addData(chunk);
    }
    return hash.result();
}

QPair<QString, QString> inferPrefixes(const QString &oldPath, const QString &newPath)
{
    QStringList oldParts = QDir::fromNativeSeparators(oldPath).split(QLatin1Char('/'), Qt::SkipEmptyParts);
    QStringList newParts = QDir::fromNativeSeparators(newPath).split(QLatin1Char('/'), Qt::SkipEmptyParts);
    if (!oldParts.isEmpty()) oldParts.removeLast();
    if (!newParts.isEmpty()) newParts.removeLast();
    int shared = 0;
    while (shared < oldParts.size() && shared < newParts.size() &&
           pathKey(oldParts.at(oldParts.size() - shared - 1)) == pathKey(newParts.at(newParts.size() - shared - 1)))
        ++shared;
    const QString oldPrefix = oldParts.mid(0, oldParts.size() - shared).join(QLatin1Char('/'));
    const QString newPrefix = newParts.mid(0, newParts.size() - shared).join(QLatin1Char('/'));
    return {oldPrefix, newPrefix};
}

QString replacePrefix(const QString &path, const QString &from, const QString &to)
{
    if (from.isEmpty())
        return {};
    const QString normalized = QDir::fromNativeSeparators(path);
    const QString normalizedFrom = QDir::fromNativeSeparators(from);
    if (pathKey(normalized) != pathKey(normalizedFrom) &&
        !pathKey(normalized).startsWith(pathKey(normalizedFrom) + QLatin1Char('/')))
        return {};
    return QDir::fromNativeSeparators(to) + normalized.mid(normalizedFrom.size());
}

enum class RelinkDecision { Relink, Skip, Abort };

RelinkDecision waitForRelinkDecision(QString *stopped, QString *pending, QTextStream &output,
                                     bool *rememberChoice)
{
    while (true) {
        for (const QString &command : readControlCommands(pending)) {
            if (command == QStringLiteral("relink")) {
                output << "Relink accepted.\n" << Qt::flush;
                return RelinkDecision::Relink;
            }
            if (command == QStringLiteral("relink-remember")) {
                *rememberChoice = true;
                output << "Relink accepted and remembered.\n" << Qt::flush;
                return RelinkDecision::Relink;
            }
            if (command == QStringLiteral("skip-remember")) {
                *rememberChoice = true;
                output << "Relink declined and remembered; skipping matching photos.\n" << Qt::flush;
                return RelinkDecision::Skip;
            }
            if (command == QStringLiteral("abort") || command == QStringLiteral("stop")) {
                *stopped = QStringLiteral("stop");
                output << "Relink declined; stopping scan.\n" << Qt::flush;
                return RelinkDecision::Abort;
            }
        }
        QThread::msleep(100);
    }
}

QStringList readControlCommands(QString *pending)
{
    QByteArray bytes;
#ifdef Q_OS_WIN
    HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    DWORD available = 0;
    if (input != INVALID_HANDLE_VALUE && input != nullptr && PeekNamedPipe(input, nullptr, 0, nullptr, &available, nullptr) && available > 0) {
        char buffer[512];
        DWORD read = 0;
        if (ReadFile(input, buffer, qMin<DWORD>(available, sizeof(buffer)), &read, nullptr))
            bytes = QByteArray(buffer, static_cast<qsizetype>(read));
    }
#else
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(STDIN_FILENO, &readSet);
    timeval timeout{0, 0};
    if (select(STDIN_FILENO + 1, &readSet, nullptr, nullptr, &timeout) > 0) {
        char buffer[512];
        const ssize_t count = ::read(STDIN_FILENO, buffer, sizeof(buffer));
        if (count > 0)
            bytes = QByteArray(buffer, static_cast<qsizetype>(count));
    }
#endif
    *pending += QString::fromUtf8(bytes);
    QStringList commands;
    qsizetype newline = -1;
    while ((newline = pending->indexOf(QLatin1Char('\n'))) >= 0) {
        commands.append(pending->left(newline).trimmed().toLower());
        pending->remove(0, newline + 1);
    }
    return commands;
}

bool waitWhilePaused(bool *paused, QString *stopped, QString *pending, QTextStream &output)
{
    while (true) {
        for (const QString &command : readControlCommands(pending)) {
            if (command == QStringLiteral("pause")) {
                if (!*paused) {
                    *paused = true;
                    output << "Scan paused.\n" << Qt::flush;
                }
            } else if (command == QStringLiteral("resume")) {
                if (*paused) {
                    *paused = false;
                    output << "Scan resumed.\n" << Qt::flush;
                }
            } else if (command == QStringLiteral("stop")) {
                *stopped = QStringLiteral("stop");
                *paused = false;
                output << "Stopping scan…\n" << Qt::flush;
            }
        }
        if (!stopped->isEmpty())
            return false;
        if (!*paused)
            return true;
        QThread::msleep(100);
    }
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("Photo Snap"));
    QCoreApplication::setOrganizationName(QStringLiteral("Photo Snap"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Recursively scan a folder and add new photos to the Photo Snap cache."));
    parser.addHelpOption();
    QCommandLineOption unhideOption(QStringLiteral("unhide-existing"),
                                   QStringLiteral("Make previously hidden cached photos visible when their files still exist."));
    parser.addOption(unhideOption);
    parser.addPositionalArgument(QStringLiteral("folder"), QStringLiteral("Folder to scan recursively."));
    parser.process(app);
    const QStringList arguments = parser.positionalArguments();
    if (arguments.size() != 1) {
        QTextStream(stderr) << "A folder path is required.\n";
        return 2;
    }

    const QFileInfo rootInfo(arguments.constFirst());
    if (!rootInfo.exists() || !rootInfo.isDir()) {
        QTextStream(stderr) << "The scan path is not an existing folder: " << arguments.constFirst() << '\n';
        return 2;
    }
    QString rootPath = rootInfo.canonicalFilePath();
    if (rootPath.isEmpty())
        rootPath = rootInfo.absoluteFilePath();
    const QString rootKey = pathKey(rootPath);
    const bool unhideExisting = parser.isSet(unhideOption);

    ImageCache &cache = ImageCache::instance();
    if (!cache.initialize()) {
        QTextStream(stderr) << "Could not connect to the per-user image cache: " << cache.errorString() << '\n';
        return 3;
    }

    QSet<QString> knownPaths;
    QHash<QString, QString> originalPathByKey;
    const QStringList cachedPaths = cache.cachedImagePaths();
    for (const QString &path : cachedPaths) {
        knownPaths.insert(pathKey(path));
        originalPathByKey.insert(pathKey(path), path);
    }
    QVector<QPair<QString, QString>> prefixChanges;
    bool hasRememberedRelinkChoice = false;
    bool rememberedRelinkChoice = false;

    quint64 scanned = 0;
    quint64 added = 0;
    quint64 skipped = 0;
    quint64 unreadable = 0;
    quint64 failed = 0;
    quint64 removed = 0;
    quint64 restored = 0;
    QStringList staleCandidates;
    bool paused = false;
    QString stopped;
    QString pendingCommands;
    QTextStream output(stdout);
    bool canContinue = true;
    const QStringList unhashedPaths = cache.cachedImagePathsWithoutHash();
    quint64 hashesBackfilled = 0;
    for (const QString &path : unhashedPaths) {
        if (!waitWhilePaused(&paused, &stopped, &pendingCommands, output)) {
            canContinue = false;
            break;
        }
        if (!QFileInfo::exists(path))
            continue;
        const QByteArray hash = hashFile(path);
        if (!hash.isEmpty() && cache.updateImageHash(path, hash)) {
            ++hashesBackfilled;
            output << "Indexed existing photo hash: " << path << '\n' << Qt::flush;
        }
    }
    if (hashesBackfilled > 0)
        output << "Indexed " << hashesBackfilled << " existing photo hash(es).\n" << Qt::flush;
    for (const QString &cachedPath : cachedPaths) {
        if (!waitWhilePaused(&paused, &stopped, &pendingCommands, output)) {
            canContinue = false;
            break;
        }
        if (!isWithinRoot(cachedPath, rootKey))
            continue;
        if (!QFileInfo::exists(cachedPath)) {
            staleCandidates.append(cachedPath);
        } else if (unhideExisting) {
            if (cache.unhideImage(cachedPath)) {
                ++restored;
                output << "Made cached photo visible: " << cachedPath << '\n' << Qt::flush;
            } else {
                ++failed;
                output << "Failed to unhide cached photo: " << cachedPath << " ("
                       << cache.errorString() << ")\n" << Qt::flush;
            }
        }
    }

    quint64 totalFiles = 0;
    QDirIterator counter(rootPath, QDir::Files | QDir::Readable, QDirIterator::Subdirectories);
    while (counter.hasNext()) {
        counter.next();
        ++totalFiles;
    }
    quint64 processedFiles = 0;
    int lastReportedProgress = -1;
    const auto reportProgress = [&]() {
        ++processedFiles;
        const int tenths = totalFiles == 0 ? 1000
                                           : qMin(1000, qRound(1000.0 * processedFiles / totalFiles));
        if (tenths == lastReportedProgress)
            return;
        lastReportedProgress = tenths;
        output << "Progress: " << QString::number(tenths / 10.0, 'f', 1) << "%\n" << Qt::flush;
    };

    QDirIterator iterator(rootPath, QDir::Files | QDir::Readable,
                          QDirIterator::Subdirectories);
    while (canContinue && iterator.hasNext() && waitWhilePaused(&paused, &stopped, &pendingCommands, output)) {
        const QString filePath = iterator.next();
        const QFileInfo fileInfo(filePath);
        QString canonicalPath = fileInfo.canonicalFilePath();
        if (canonicalPath.isEmpty())
            canonicalPath = fileInfo.absoluteFilePath();
        const QString key = pathKey(canonicalPath);
        if (knownPaths.contains(key)) {
            ++skipped;
            output << "Skipped (already cached): " << canonicalPath << '\n' << Qt::flush;
            reportProgress();
            continue;
        }

        QString relinkFromPath;
        for (const auto &change : std::as_const(prefixChanges)) {
            const QString candidate = replacePrefix(canonicalPath, change.second, change.first);
            if (!candidate.isEmpty() && knownPaths.contains(pathKey(candidate))) {
                relinkFromPath = originalPathByKey.value(pathKey(candidate), candidate);
                break;
            }
        }

        const QByteArray detectedFormat = QImageReader::imageFormat(canonicalPath);
        if (detectedFormat.isEmpty()) {
            reportProgress();
            continue;
        }
        ++scanned;
        output << "Reading: " << canonicalPath << '\n' << Qt::flush;
        auto hashFuture = std::async(std::launch::async, hashFile, canonicalPath);
        QImageReader reader(canonicalPath);
        reader.setAutoTransform(true);
        const QImage image = reader.read();
        if (image.isNull()) {
            ++unreadable;
            output << "Unreadable: " << canonicalPath << " (" << reader.errorString() << ")\n" << Qt::flush;
            reportProgress();
            continue;
        }

        QByteArray imageFormat = reader.format();
        if (imageFormat.isEmpty())
            imageFormat = detectedFormat;
        const ImageMetadata metadata = makeImageMetadata(canonicalPath, QString::fromLatin1(imageFormat), image);
        if (metadata.thumbnailPng.isEmpty()) {
            ++unreadable;
            output << "Unreadable: " << canonicalPath << " (could not create thumbnail)\n" << Qt::flush;
            reportProgress();
            continue;
        }
        ImageMetadata hashedMetadata = metadata;
        hashedMetadata.contentHash = hashFuture.get();
        if (hashedMetadata.contentHash.isEmpty()) {
            ++failed;
            output << "Failed to hash: " << canonicalPath << '\n' << Qt::flush;
            reportProgress();
            continue;
        }
        if (!relinkFromPath.isEmpty()) {
            if (cache.relinkImage(relinkFromPath, hashedMetadata)) {
                ++added;
                knownPaths.remove(pathKey(relinkFromPath));
                originalPathByKey.remove(pathKey(relinkFromPath));
                knownPaths.insert(key);
                originalPathByKey.insert(key, canonicalPath);
                output << "Relinked from prefix rule: " << relinkFromPath << " → " << canonicalPath << '\n' << Qt::flush;
            } else {
                ++failed;
                output << "Failed to relink: " << canonicalPath << " (" << cache.errorString() << ")\n" << Qt::flush;
            }
        } else {
            const QString matchingPath = cache.cachedImagePathByHash(hashedMetadata.contentHash);
            if (!matchingPath.isEmpty() && pathKey(matchingPath) != key) {
                if (QFileInfo::exists(matchingPath)) {
                    ++skipped;
                    output << "Skipped (duplicate; original still exists): " << canonicalPath
                           << " (original: " << matchingPath << ")\n" << Qt::flush;
                    reportProgress();
                    continue;
                }
                const auto prefixes = inferPrefixes(matchingPath, canonicalPath);
                RelinkDecision decision = RelinkDecision::Abort;
                if (hasRememberedRelinkChoice) {
                    decision = rememberedRelinkChoice ? RelinkDecision::Relink : RelinkDecision::Skip;
                    output << (rememberedRelinkChoice ? "Relink choice remembered; accepting: "
                                                       : "Relink choice remembered; skipping: ")
                           << canonicalPath << '\n' << Qt::flush;
                } else {
                    output << "MovedMatch:\t" << matchingPath << '\t' << canonicalPath << '\t'
                           << prefixes.first << '\t' << prefixes.second << '\n' << Qt::flush;
                    bool rememberChoice = false;
                    decision = waitForRelinkDecision(&stopped, &pendingCommands, output, &rememberChoice);
                    if (rememberChoice) {
                        hasRememberedRelinkChoice = true;
                        rememberedRelinkChoice = decision == RelinkDecision::Relink;
                    }
                }
                if (decision == RelinkDecision::Abort) {
                    reportProgress();
                    break;
                }
                if (decision == RelinkDecision::Skip) {
                    ++skipped;
                    output << "Skipped moved photo: " << canonicalPath << '\n' << Qt::flush;
                    reportProgress();
                    continue;
                }
                if (!prefixes.first.isEmpty() && !prefixes.second.isEmpty() && prefixes.first != prefixes.second)
                    prefixChanges.append(prefixes);
                if (cache.relinkImage(matchingPath, hashedMetadata)) {
                    ++added;
                    knownPaths.remove(pathKey(matchingPath));
                    originalPathByKey.remove(pathKey(matchingPath));
                    knownPaths.insert(key);
                    originalPathByKey.insert(key, canonicalPath);
                    output << "Relinked moved photo: " << matchingPath << " → " << canonicalPath << '\n' << Qt::flush;
                } else {
                    ++failed;
                    output << "Failed to relink moved photo: " << canonicalPath << " (" << cache.errorString() << ")\n" << Qt::flush;
                }
            } else if (cache.addImageIfNew(hashedMetadata)) {
                ++added;
                knownPaths.insert(key);
                originalPathByKey.insert(key, canonicalPath);
                output << "Added: " << canonicalPath << '\n' << Qt::flush;
            } else if (cache.errorString().isEmpty()) {
                ++skipped;
                knownPaths.insert(key);
                originalPathByKey.insert(key, canonicalPath);
                output << "Skipped (already cached): " << canonicalPath << '\n' << Qt::flush;
            } else {
                ++failed;
                output << "Failed: " << canonicalPath << " (" << cache.errorString() << ")\n" << Qt::flush;
            }
        }
        reportProgress();
    }

    if (stopped.isEmpty()) {
        for (const QString &cachedPath : std::as_const(staleCandidates)) {
            if (!knownPaths.contains(pathKey(cachedPath)))
                continue;
            if (cache.deleteImageEntry(cachedPath)) {
                ++removed;
                knownPaths.remove(pathKey(cachedPath));
                output << "Removed stale cache entry: " << cachedPath << '\n' << Qt::flush;
            } else {
                ++failed;
                output << "Failed to remove stale cache entry: " << cachedPath << " ("
                       << cache.errorString() << ")\n" << Qt::flush;
            }
        }
    }

    if (stopped.isEmpty() && lastReportedProgress < 1000)
        output << "Progress: 100.0%\n" << Qt::flush;

    output << (stopped.isEmpty() ? "Scan complete. " : "Scan interrupted. ")
           << "Added " << added << ", skipped existing " << skipped << ", removed stale entries " << removed
           << ", made visible " << restored
           << ", unreadable " << unreadable << ", failed " << failed
           << " (new files checked: " << scanned << ").\n" << Qt::flush;
    return !stopped.isEmpty() ? 130 : (failed == 0 ? 0 : 4);
}
