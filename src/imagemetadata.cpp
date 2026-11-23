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

#include "imagemetadata.h"

#include <QBuffer>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QSaveFile>

#include <cmath>
#include <limits>

namespace {

struct ExifInfo
{
    QString dateTime;
    bool hasGps = false;
    double latitude = 0.0;
    double longitude = 0.0;
};

class TiffReader
{
public:
    TiffReader(QFile &file, qint64 base, qint64 length) : m_file(file), m_base(base), m_length(length)
    {
        QByteArray header;
        if (!bytes(0, 8, &header))
            return;
        if (header.at(0) == 'I' && header.at(1) == 'I')
            m_little = true;
        else if (header.at(0) == 'M' && header.at(1) == 'M')
            m_little = false;
        else
            return;
        quint16 magic = 0;
        m_valid = u16(2, &magic) && magic == 42 && u32(4, &m_ifd0);
    }

    ExifInfo read()
    {
        ExifInfo result;
        if (!m_valid)
            return result;
        quint32 exifIfd = 0;
        quint32 gpsIfd = 0;
        eachEntry(m_ifd0, [&](quint16 tag, quint16 type, quint32 count, quint32 value, quint32 entry) {
            if (tag == 0x0132 && result.dateTime.isEmpty())
                result.dateTime = ascii(type, count, value, entry);
            if (tag == 0x8769)
                integer(type, count, value, entry, &exifIfd);
            else if (tag == 0x8825)
                integer(type, count, value, entry, &gpsIfd);
        });
        if (exifIfd) {
            QString original;
            QString digitized;
            eachEntry(exifIfd, [&](quint16 tag, quint16 type, quint32 count, quint32 value, quint32 entry) {
                if (tag == 0x9003)
                    original = ascii(type, count, value, entry);
                else if (tag == 0x9004)
                    digitized = ascii(type, count, value, entry);
            });
            if (!original.isEmpty())
                result.dateTime = original;
            else if (!digitized.isEmpty())
                result.dateTime = digitized;
        }
        if (gpsIfd) {
            QString latRef;
            QString lonRef;
            bool hasLat = false;
            bool hasLon = false;
            double lat = 0.0;
            double lon = 0.0;
            eachEntry(gpsIfd, [&](quint16 tag, quint16 type, quint32 count, quint32 value, quint32 entry) {
                if (tag == 1)
                    latRef = ascii(type, count, value, entry).trimmed().toUpper();
                else if (tag == 2)
                    hasLat = coordinate(type, count, value, entry, &lat);
                else if (tag == 3)
                    lonRef = ascii(type, count, value, entry).trimmed().toUpper();
                else if (tag == 4)
                    hasLon = coordinate(type, count, value, entry, &lon);
            });
            if (hasLat && hasLon && (latRef == QStringLiteral("N") || latRef == QStringLiteral("S")) &&
                (lonRef == QStringLiteral("E") || lonRef == QStringLiteral("W"))) {
                result.latitude = latRef == QStringLiteral("S") ? -lat : lat;
                result.longitude = lonRef == QStringLiteral("W") ? -lon : lon;
                result.hasGps = std::abs(result.latitude) <= 90.0 && std::abs(result.longitude) <= 180.0;
            }
        }
        const QDateTime date = QDateTime::fromString(result.dateTime, QStringLiteral("yyyy:MM:dd HH:mm:ss"));
        result.dateTime = date.isValid() ? date.toString(Qt::ISODate) : QString();
        return result;
    }

private:
    template<typename Callback>
    void eachEntry(quint32 offset, Callback callback)
    {
        quint16 count = 0;
        if (!u16(offset, &count) || count > 4096)
            return;
        for (quint32 i = 0; i < count; ++i) {
            const quint32 entry = offset + 2 + i * 12;
            quint16 tag = 0, type = 0;
            quint32 values = 0, value = 0;
            if (!u16(entry, &tag) || !u16(entry + 2, &type) || !u32(entry + 4, &values) || !u32(entry + 8, &value))
                return;
            callback(tag, type, values, value, entry);
        }
    }

    bool bytes(quint32 offset, quint32 count, QByteArray *out)
    {
        if (static_cast<quint64>(offset) + count > static_cast<quint64>(m_length) || !m_file.seek(m_base + offset))
            return false;
        *out = m_file.read(count);
        return out->size() == static_cast<qsizetype>(count);
    }

    bool u16(quint32 offset, quint16 *out)
    {
        QByteArray b;
        if (!bytes(offset, 2, &b)) return false;
        const quint8 a = static_cast<quint8>(b.at(0));
        const quint8 c = static_cast<quint8>(b.at(1));
        *out = m_little ? static_cast<quint16>(a | (c << 8)) : static_cast<quint16>((a << 8) | c);
        return true;
    }

    bool u32(quint32 offset, quint32 *out)
    {
        QByteArray b;
        if (!bytes(offset, 4, &b)) return false;
        quint32 value = 0;
        if (m_little) {
            for (int i = 3; i >= 0; --i) value = (value << 8) | static_cast<quint8>(b.at(i));
        } else {
            for (int i = 0; i < 4; ++i) value = (value << 8) | static_cast<quint8>(b.at(i));
        }
        *out = value;
        return true;
    }

    bool valueOffset(quint16 type, quint32 count, quint32 value, quint32 entry, quint32 *offset)
    {
        quint32 unit = 0;
        switch (type) {
        case 1: case 2: case 7: unit = 1; break;
        case 3: unit = 2; break;
        case 4: case 9: unit = 4; break;
        case 5: case 10: unit = 8; break;
        default: return false;
        }
        const quint64 size = static_cast<quint64>(unit) * count;
        if (size > static_cast<quint64>(m_length)) return false;
        *offset = size <= 4 ? entry + 8 : value;
        return static_cast<quint64>(*offset) + size <= static_cast<quint64>(m_length);
    }

    QString ascii(quint16 type, quint32 count, quint32 value, quint32 entry)
    {
        quint32 offset = 0;
        if (type != 2 || count == 0 || count > 128 || !valueOffset(type, count, value, entry, &offset)) return {};
        QByteArray b;
        if (!bytes(offset, count, &b)) return {};
        const int nul = b.indexOf('\0');
        if (nul >= 0) b.truncate(nul);
        return QString::fromLatin1(b).trimmed();
    }

    bool integer(quint16 type, quint32 count, quint32 value, quint32 entry, quint32 *out)
    {
        quint32 offset = 0;
        if (count != 1 || !valueOffset(type, count, value, entry, &offset)) return false;
        if (type == 3) {
            quint16 shortValue = 0;
            if (!u16(offset, &shortValue)) return false;
            *out = shortValue;
            return true;
        }
        return type == 4 && u32(offset, out);
    }

    bool rational(quint32 offset, double *out)
    {
        quint32 numerator = 0, denominator = 0;
        if (!u32(offset, &numerator) || !u32(offset + 4, &denominator) || denominator == 0) return false;
        *out = static_cast<double>(numerator) / denominator;
        return true;
    }

    bool coordinate(quint16 type, quint32 count, quint32 value, quint32 entry, double *out)
    {
        quint32 offset = 0;
        if (type != 5 || count != 3 || !valueOffset(type, count, value, entry, &offset)) return false;
        double degrees = 0.0, minutes = 0.0, seconds = 0.0;
        if (!rational(offset, &degrees) || !rational(offset + 8, &minutes) || !rational(offset + 16, &seconds)) return false;
        *out = degrees + minutes / 60.0 + seconds / 3600.0;
        return true;
    }

    QFile &m_file;
    qint64 m_base = 0;
    qint64 m_length = 0;
    quint32 m_ifd0 = 0;
    bool m_little = true;
    bool m_valid = false;
};

ExifInfo readExif(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return {};
    const QByteArray head = file.read(8);
    if (head.size() == 8 && ((head.at(0) == 'I' && head.at(1) == 'I' && head.at(2) == '*' && head.at(3) == '\0') ||
                             (head.at(0) == 'M' && head.at(1) == 'M' && head.at(2) == '\0' && head.at(3) == '*'))) {
        TiffReader reader(file, 0, file.size());
        return reader.read();
    }
    if (head == QByteArray::fromHex("89504e470d0a1a0a")) {
        while (!file.atEnd()) {
            const QByteArray chunk = file.read(8);
            if (chunk.size() != 8) return {};
            const quint32 length = (static_cast<quint32>(static_cast<quint8>(chunk[0])) << 24) |
                                   (static_cast<quint32>(static_cast<quint8>(chunk[1])) << 16) |
                                   (static_cast<quint32>(static_cast<quint8>(chunk[2])) << 8) |
                                   static_cast<quint8>(chunk[3]);
            const qint64 data = file.pos();
            if (file.size() - data < 4 || length > static_cast<quint64>(file.size() - data - 4)) return {};
            if (chunk.mid(4, 4) == QByteArrayLiteral("eXIf")) {
                TiffReader reader(file, data, length);
                return reader.read();
            }
            if (!file.seek(data + length + 4)) return {};
        }
        return {};
    }
    if (head.size() < 2 || static_cast<quint8>(head[0]) != 0xff || static_cast<quint8>(head[1]) != 0xd8 || !file.seek(2))
        return {};
    while (!file.atEnd()) {
        char byte = 0;
        do { if (file.read(&byte, 1) != 1) return {}; } while (static_cast<quint8>(byte) != 0xff);
        do { if (file.read(&byte, 1) != 1) return {}; } while (static_cast<quint8>(byte) == 0xff);
        const quint8 marker = static_cast<quint8>(byte);
        if (marker == 0xda || marker == 0xd9) break;
        if (marker == 0x01 || (marker >= 0xd0 && marker <= 0xd7)) continue;
        const QByteArray lengthBytes = file.read(2);
        if (lengthBytes.size() != 2) return {};
        const quint16 segmentLength = (static_cast<quint16>(static_cast<quint8>(lengthBytes[0])) << 8) |
                                      static_cast<quint8>(lengthBytes[1]);
        if (segmentLength < 2) return {};
        const qint64 segmentStart = file.pos();
        const qint64 contentLength = segmentLength - 2;
        if (marker == 0xe1 && contentLength >= 14) {
            if (file.read(6) == QByteArray("Exif\0\0", 6)) {
                TiffReader reader(file, segmentStart + 6, contentLength - 6);
                return reader.read();
            }
        }
        if (!file.seek(segmentStart + contentLength)) return {};
    }
    return {};
}

void write16(QByteArray *bytes, int offset, quint16 value, bool littleEndian)
{
    if (littleEndian) {
        (*bytes)[offset] = static_cast<char>(value & 0xff);
        (*bytes)[offset + 1] = static_cast<char>((value >> 8) & 0xff);
    } else {
        (*bytes)[offset] = static_cast<char>((value >> 8) & 0xff);
        (*bytes)[offset + 1] = static_cast<char>(value & 0xff);
    }
}

void write32(QByteArray *bytes, int offset, quint32 value, bool littleEndian)
{
    for (int index = 0; index < 4; ++index) {
        const int shift = littleEndian ? index * 8 : (3 - index) * 8;
        (*bytes)[offset + index] = static_cast<char>((value >> shift) & 0xff);
    }
}

quint32 read32(const QByteArray &bytes, int offset, bool littleEndian)
{
    quint32 value = 0;
    if (littleEndian) {
        for (int index = 3; index >= 0; --index)
            value = (value << 8) | static_cast<quint8>(bytes.at(offset + index));
    } else {
        for (int index = 0; index < 4; ++index)
            value = (value << 8) | static_cast<quint8>(bytes.at(offset + index));
    }
    return value;
}

QByteArray makeDateExif(const QString &date, bool littleEndian)
{
    QByteArray tiff(130, '\0');
    tiff[0] = littleEndian ? 'I' : 'M';
    tiff[1] = littleEndian ? 'I' : 'M';
    write16(&tiff, 2, 42, littleEndian);
    write32(&tiff, 4, 8, littleEndian);

    write16(&tiff, 8, 2, littleEndian);
    write16(&tiff, 10, 0x0132, littleEndian);
    write16(&tiff, 12, 2, littleEndian);
    write32(&tiff, 14, 20, littleEndian);
    write32(&tiff, 18, 38, littleEndian);
    write16(&tiff, 22, 0x8769, littleEndian);
    write16(&tiff, 24, 4, littleEndian);
    write32(&tiff, 26, 1, littleEndian);
    write32(&tiff, 30, 58, littleEndian);

    write16(&tiff, 58, 2, littleEndian);
    write16(&tiff, 60, 0x9003, littleEndian);
    write16(&tiff, 62, 2, littleEndian);
    write32(&tiff, 64, 20, littleEndian);
    write32(&tiff, 68, 90, littleEndian);
    write16(&tiff, 72, 0x9004, littleEndian);
    write16(&tiff, 74, 2, littleEndian);
    write32(&tiff, 76, 20, littleEndian);
    write32(&tiff, 80, 110, littleEndian);

    const QByteArray value = date.toLatin1() + '\0';
    tiff.replace(38, value.size(), value);
    tiff.replace(90, value.size(), value);
    tiff.replace(110, value.size(), value);
    return tiff;
}

quint32 pngCrc32(const QByteArray &bytes)
{
    quint32 crc = 0xffffffffu;
    for (const unsigned char byte : bytes) {
        crc ^= byte;
        for (int bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ ((crc & 1u) ? 0xedb88320u : 0u);
    }
    return crc ^ 0xffffffffu;
}

void appendBigEndian32(QByteArray *bytes, quint32 value)
{
    bytes->append(static_cast<char>((value >> 24) & 0xff));
    bytes->append(static_cast<char>((value >> 16) & 0xff));
    bytes->append(static_cast<char>((value >> 8) & 0xff));
    bytes->append(static_cast<char>(value & 0xff));
}

bool fail(QString *error, const QString &message)
{
    if (error)
        *error = message;
    return false;
}

QByteArray makeThumbnail(const QImage &image)
{
    if (image.isNull()) return {};
    constexpr int size = 128;
    const QImage expanded = image.scaled(size, size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    const QImage thumbnail = expanded.copy((expanded.width() - size) / 2, (expanded.height() - size) / 2, size, size);
    QByteArray png;
    QBuffer buffer(&png);
    if (!buffer.open(QIODevice::WriteOnly) || !thumbnail.save(&buffer, "PNG")) return {};
    return png;
}

} // namespace

ImageMetadata makeImageMetadata(const QString &path, const QString &format, const QImage &image)
{
    const QFileInfo info(path);
    ImageMetadata metadata;
    metadata.path = info.canonicalFilePath();
    if (metadata.path.isEmpty()) metadata.path = info.absoluteFilePath();
    metadata.sizeBytes = info.size();
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    const QDateTime creationDateTime = info.birthTime();
#else
    const QDateTime creationDateTime = info.created();
#endif
    const QDateTime updateDateTime = info.lastModified();
    metadata.creationDateTime = creationDateTime.toString(Qt::ISODate);
    metadata.updateDateTime = updateDateTime.toString(Qt::ISODate);
    metadata.modifiedMs = updateDateTime.toMSecsSinceEpoch();
    metadata.format = format.toUpper();
    if (metadata.format.isEmpty()) metadata.format = info.suffix().toUpper();

    const ExifInfo exif = readExif(metadata.path);
    metadata.shootingDateTime = exif.dateTime;
    metadata.hasGpsPosition = exif.hasGps;
    metadata.gpsLatitude = exif.latitude;
    metadata.gpsLongitude = exif.longitude;
    metadata.thumbnailPng = makeThumbnail(image);
    return metadata;
}

QString readImageShotDateTime(const QString &path)
{
    return readExif(path).dateTime;
}

bool writeImageShotDateTime(const QString &path, const QDateTime &dateTime, QString *error)
{
    if (!dateTime.isValid())
        return fail(error, QStringLiteral("Enter a valid date and time."));

    QFile source(path);
    if (!source.open(QIODevice::ReadOnly))
        return fail(error, source.errorString());
    QByteArray bytes = source.readAll();
    const QFileDevice::Permissions permissions = source.permissions();
    source.close();
    if (bytes.isEmpty())
        return fail(error, QStringLiteral("The image file is empty or could not be read."));

    const QString exifDate = dateTime.toString(QStringLiteral("yyyy:MM:dd HH:mm:ss"));
    const QByteArray dateExif = makeDateExif(exifDate, true);

    if (bytes.size() >= 2 && static_cast<quint8>(bytes.at(0)) == 0xff &&
        static_cast<quint8>(bytes.at(1)) == 0xd8) {
        QByteArray payload("Exif\0\0", 6);
        payload.append(dateExif);
        const quint16 segmentLength = static_cast<quint16>(payload.size() + 2);
        QByteArray segment;
        segment.append(static_cast<char>(0xff));
        segment.append(static_cast<char>(0xe1));
        segment.append(static_cast<char>((segmentLength >> 8) & 0xff));
        segment.append(static_cast<char>(segmentLength & 0xff));
        segment.append(payload);
        bytes.insert(2, segment);
    } else if (bytes.startsWith(QByteArray::fromHex("89504e470d0a1a0a"))) {
        if (bytes.size() < 33 || bytes.mid(12, 4) != QByteArrayLiteral("IHDR"))
            return fail(error, QStringLiteral("The PNG image header is invalid."));
        const auto readBigEndian32 = [&bytes](int offset) {
            return (static_cast<quint32>(static_cast<quint8>(bytes.at(offset))) << 24) |
                   (static_cast<quint32>(static_cast<quint8>(bytes.at(offset + 1))) << 16) |
                   (static_cast<quint32>(static_cast<quint8>(bytes.at(offset + 2))) << 8) |
                   static_cast<quint8>(bytes.at(offset + 3));
        };
        const quint32 headerLength = readBigEndian32(8);
        const quint64 insertAt64 = 8ull + 12ull + headerLength;
        if (insertAt64 > static_cast<quint64>(bytes.size()))
            return fail(error, QStringLiteral("The PNG image header is incomplete."));
        QByteArray chunk;
        appendBigEndian32(&chunk, static_cast<quint32>(dateExif.size()));
        const QByteArray typeAndData = QByteArrayLiteral("eXIf") + dateExif;
        chunk.append(typeAndData);
        appendBigEndian32(&chunk, pngCrc32(typeAndData));
        bytes.insert(static_cast<qsizetype>(insertAt64), chunk);
    } else if (bytes.size() >= 8 &&
               ((bytes.at(0) == 'I' && bytes.at(1) == 'I' && bytes.at(2) == '*' && bytes.at(3) == '\0') ||
                (bytes.at(0) == 'M' && bytes.at(1) == 'M' && bytes.at(2) == '\0' && bytes.at(3) == '*'))) {
        const bool littleEndian = bytes.at(0) == 'I';
        const quint32 oldIfd = read32(bytes, 4, littleEndian);
        const quint64 base64 = static_cast<quint64>(bytes.size()) + (bytes.size() % 2);
        if (base64 + dateExif.size() - 8 > std::numeric_limits<quint32>::max())
            return fail(error, QStringLiteral("The TIFF image is too large to update."));
        const quint32 base = static_cast<quint32>(base64);
        if (bytes.size() % 2)
            bytes.append('\0');
        QByteArray appendedExif = makeDateExif(exifDate, littleEndian);
        write32(&appendedExif, 4, base, littleEndian);
        write32(&appendedExif, 18, base + 30, littleEndian);
        write32(&appendedExif, 30, base + 50, littleEndian);
        write32(&appendedExif, 34, oldIfd, littleEndian);
        write32(&appendedExif, 68, base + 82, littleEndian);
        write32(&appendedExif, 80, base + 102, littleEndian);
        bytes.append(appendedExif.mid(8));
        write32(&bytes, 4, base, littleEndian);
    } else {
        return fail(error, QStringLiteral("EXIF date editing supports JPEG, PNG, and TIFF images."));
    }

    QSaveFile destination(path);
    if (!destination.open(QIODevice::WriteOnly))
        return fail(error, destination.errorString());
    if (destination.write(bytes) != bytes.size()) {
        destination.cancelWriting();
        return fail(error, destination.errorString());
    }
    destination.setPermissions(permissions);
    if (!destination.commit())
        return fail(error, destination.errorString());
    if (error)
        error->clear();
    return true;
}
