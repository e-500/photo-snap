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

#include <QImage>
#include <QSet>
#include <QVector>
#include <QWidget>

class QLabel;
class QGridLayout;
class QResizeEvent;
class QScrollArea;

class ImageLibraryWidget final : public QWidget
{
    Q_OBJECT

public:
    ImageLibraryWidget(ImageCache *cache, QString year, QString month, ImageDateMode dateMode,
                       QScrollArea *scrollArea, QWidget *parent = nullptr);
    void refresh();
    void checkScrollPrefetch();
    QStringList selectedPaths() const;
    void clearSelection();

signals:
    void openImageRequested(const QString &path);
    void refreshRequested();
    void selectionChanged();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void arrangeThumbnails();
    void loadMoreDatabaseEntries();
    void loadMoreThumbnails();
    void selectThumbnail(QWidget *thumbnail, const QString &path, Qt::KeyboardModifiers modifiers);
    void hideSelectedThumbnail();
    void deleteSelectedImage();

    ImageCache *m_cache = nullptr;
    QString m_year;
    QString m_month;
    ImageDateMode m_dateMode = ImageDateMode::ShootingDate;
    QScrollArea *m_scrollArea = nullptr;
    QGridLayout *m_gridLayout = nullptr;
    QLabel *m_emptyLabel = nullptr;
    QSet<QString> m_selectedPaths;
    QVector<QString> m_thumbnailPaths;
    QVector<int> m_thumbnailEntryIndices;
    QVector<ImageMetadata> m_loadedImages;
    QVector<QLabel *> m_thumbnails;
    QVector<QImage> m_thumbnailImages;
    int m_selectionAnchor = -1;
    int m_nextDatabaseOffset = 0;
    int m_nextThumbnailEntry = 0;
    int m_nextUnloadBoundary = 100;
    int m_columns = 1;
    int m_cellSize = 132;
    int m_gridTopMargin = 12;
    int m_gridVerticalSpacing = 12;
    bool m_hasMoreDatabaseEntries = true;
    bool m_loadingDatabaseEntries = false;
};
