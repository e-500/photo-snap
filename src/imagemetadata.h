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

#include "imagecache.h"

#include <QDateTime>

ImageMetadata makeImageMetadata(const QString &path, const QString &format, const QImage &image);
QString readImageShotDateTime(const QString &path);
bool writeImageShotDateTime(const QString &path, const QDateTime &dateTime, QString *error = nullptr);
QString readImageAuthor(const QString &path);
QString readImageCopyright(const QString &path);
bool writeImageExifMetadata(const QString &path, const QDateTime &dateTime, const QString &author,
                            const QString &copyright, bool updateDateTime, bool updateAuthor,
                            bool updateCopyright, QString *error = nullptr);
