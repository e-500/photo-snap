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

#include "imagelibrarywidget.h"

#include <QFrame>
#include <QGridLayout>
#include <QImage>
#include <QLabel>
#include <QMouseEvent>
#include <QPixmap>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QShortcut>
#include <QKeySequence>
#include <QSizePolicy>
#include <QSizePolicy>
#include <QVBoxLayout>

#include <functional>
#include <utility>

namespace {
constexpr int DatabasePageSize = 1000;
constexpr int ThumbnailBatchSize = 100;

class ThumbnailLabel final : public QLabel
{
public:
    explicit ThumbnailLabel(std::function<void(QWidget *, Qt::KeyboardModifiers)> selectImage,
                            std::function<void()> openImage, QWidget *parent = nullptr)
        : QLabel(parent), m_selectImage(std::move(selectImage)), m_openImage(std::move(openImage))
    {
        setCursor(Qt::PointingHandCursor);
        setFocusPolicy(Qt::ClickFocus);
    }

protected:
    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton && m_selectImage) {
            m_selectImage(this, event->modifiers());
            setFocus(Qt::MouseFocusReason);
        }
        QLabel::mousePressEvent(event);
    }

    void mouseDoubleClickEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton && m_openImage) {
            m_openImage();
            event->accept();
            return;
        }
        QLabel::mouseDoubleClickEvent(event);
    }

private:
    std::function<void(QWidget *, Qt::KeyboardModifiers)> m_selectImage;
    std::function<void()> m_openImage;
};
}

ImageLibraryWidget::ImageLibraryWidget(ImageCache *cache, SelectedImages *selection, QString year, QString month,
                                       ImageDateMode dateMode,
                                       QScrollArea *scrollArea, QWidget *parent)
    : QWidget(parent), m_cache(cache), m_selection(selection), m_year(std::move(year)), m_month(std::move(month)),
      m_dateMode(dateMode), m_scrollArea(scrollArea)
{
    setMinimumWidth(0);
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSizeConstraint(QLayout::SetNoConstraint);
    m_gridLayout = new QGridLayout;
    m_gridLayout->setContentsMargins(12, 12, 12, 12);
    m_gridLayout->setHorizontalSpacing(12);
    m_gridLayout->setVerticalSpacing(12);
    m_gridLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    auto *gridHost = new QWidget(this);
    gridHost->setMinimumWidth(0);
    gridHost->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    gridHost->setLayout(m_gridLayout);
    layout->addWidget(gridHost);
    m_emptyLabel = new QLabel(tr("No photos in this year"), gridHost);
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setMinimumSize(300, 150);
    m_gridLayout->addWidget(m_emptyLabel, 0, 0);

    auto *hideShortcut = new QShortcut(QKeySequence(Qt::Key_Delete), this);
    hideShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(hideShortcut, &QShortcut::activated, this, &ImageLibraryWidget::hideSelectedThumbnail);
    auto *deleteShortcut = new QShortcut(QKeySequence(Qt::SHIFT | Qt::Key_Delete), this);
    deleteShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(deleteShortcut, &QShortcut::activated, this, &ImageLibraryWidget::deleteSelectedImage);
}

void ImageLibraryWidget::refresh()
{
    const bool hadSelection = m_selection && m_selection->hasSelection();
    m_thumbnailPaths.clear();
    m_thumbnailEntryIndices.clear();
    m_selectionAnchor = -1;
    m_nextUnloadBoundary = ThumbnailBatchSize;
    while (QLayoutItem *item = m_gridLayout->takeAt(0)) {
        if (item->widget() && item->widget() != m_emptyLabel)
            delete item->widget();
        delete item;
    }
    m_thumbnails.clear();
    m_thumbnailImages.clear();
    m_loadedImages.clear();
    m_nextDatabaseOffset = 0;
    m_nextThumbnailEntry = 0;
    m_hasMoreDatabaseEntries = true;
    m_loadingDatabaseEntries = false;
    loadMoreDatabaseEntries();
    if (m_loadedImages.isEmpty()) {
        m_emptyLabel->setVisible(true);
        m_gridLayout->addWidget(m_emptyLabel, 0, 0);
        if (hadSelection)
            emit selectionChanged();
        return;
    }
    loadMoreThumbnails();
    if (hadSelection)
        emit selectionChanged();
}

void ImageLibraryWidget::clearSelection()
{
    m_selectionAnchor = -1;
    updateSelectionAppearance();
}

void ImageLibraryWidget::updateSelectionAppearance()
{
    for (int index = 0; index < m_thumbnails.size(); ++index) {
        const bool selected = m_selection &&
            m_selection->isImageSelected(m_thumbnailPaths.at(index), m_year, m_month);
        m_thumbnails.at(index)->setStyleSheet(selected
            ? QStringLiteral("border: 2px solid #55aaff; background: #202329;")
            : QStringLiteral("border: 2px solid #454b55; background: #202329;"));
    }
}

void ImageLibraryWidget::loadMoreDatabaseEntries()
{
    if (!m_cache || !m_hasMoreDatabaseEntries || m_loadingDatabaseEntries)
        return;
    m_loadingDatabaseEntries = true;
    const QVector<ImageMetadata> images =
        m_cache->cachedImagesOrderedByDate(m_nextDatabaseOffset, DatabasePageSize, m_year, m_dateMode, m_month);
    m_loadedImages += images;
    m_nextDatabaseOffset += images.size();
    m_hasMoreDatabaseEntries = images.size() == DatabasePageSize;
    m_loadingDatabaseEntries = false;
}

void ImageLibraryWidget::loadMoreThumbnails()
{
    const int firstNewThumbnail = m_thumbnails.size();
    const int targetEntry = qMin(m_nextThumbnailEntry + ThumbnailBatchSize, m_loadedImages.size());
    while (m_nextThumbnailEntry < targetEntry) {
        const int metadataIndex = m_nextThumbnailEntry++;
        const ImageMetadata metadata = m_loadedImages.at(metadataIndex);
        const QImage thumbnail = QImage::fromData(metadata.thumbnailPng, "PNG");
        if (thumbnail.isNull())
            continue;
        auto *label = new ThumbnailLabel([this, path = metadata.path](QWidget *widget, Qt::KeyboardModifiers modifiers) {
            selectThumbnail(widget, path, modifiers);
        }, [this, path = metadata.path]() {
            emit openImageRequested(path);
        }, this);
        label->setFrameShape(QFrame::NoFrame);
        const bool selected = m_selection && m_selection->isImageSelected(metadata.path, m_year, m_month);
        label->setStyleSheet(selected ? QStringLiteral("border: 2px solid #55aaff; background: #202329;")
                                      : QStringLiteral("border: 2px solid #454b55; background: #202329;"));
        label->setAlignment(Qt::AlignCenter);
        m_thumbnails.append(label);
        m_thumbnailImages.append(thumbnail);
        m_thumbnailPaths.append(metadata.path);
        m_thumbnailEntryIndices.append(metadataIndex);
    }
    if (firstNewThumbnail == 0) {
        arrangeThumbnails();
    } else {
        m_gridLayout->removeWidget(m_emptyLabel);
        m_emptyLabel->hide();
        const int innerSize = qMax(1, m_cellSize - 4);
        for (int index = firstNewThumbnail; index < m_thumbnails.size(); ++index) {
            QLabel *label = m_thumbnails.at(index);
            label->setFixedSize(m_cellSize, m_cellSize);
            const QImage &image = m_thumbnailImages.at(index);
            label->setPixmap(QPixmap::fromImage(image.scaled(
                innerSize, innerSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)));
            m_gridLayout->addWidget(label, index / m_columns, index % m_columns);
        }
    }
}

void ImageLibraryWidget::checkScrollPrefetch()
{
    if (!isVisible() || m_thumbnails.isEmpty() || !m_scrollArea)
        return;
    const QPoint gridTop = mapTo(m_scrollArea->viewport(), QPoint(0, 0));
    const QRect viewportRect = m_scrollArea->viewport()->rect();
    const int gridBottom = gridTop.y() + height();
    if (gridBottom <= viewportRect.top() || gridTop.y() >= viewportRect.bottom())
        return;

    const int rowStride = qMax(1, m_cellSize + m_gridVerticalSpacing);
    const int visibleTop = qMax(0, -gridTop.y() - m_gridTopMargin);
    const int visibleBottom = qMax(visibleTop, viewportRect.bottom() - gridTop.y() - m_gridTopMargin);
    const int firstRow = visibleTop / rowStride;
    const int lastRow = visibleBottom / rowStride;
    const int firstVisibleIndex = qMin(m_thumbnails.size() - 1, firstRow * m_columns);
    const int lastVisibleIndex = qMin(m_thumbnails.size() - 1, (lastRow + 1) * m_columns - 1);

    const int reloadEnd = qMin(m_thumbnails.size(), lastVisibleIndex + ThumbnailBatchSize / 4 + 1);
    for (int index = firstVisibleIndex; index < reloadEnd; ++index) {
        if (!m_thumbnailImages.at(index).isNull())
            continue;
        const int metadataIndex = m_thumbnailEntryIndices.at(index);
        const QImage image = QImage::fromData(m_loadedImages.at(metadataIndex).thumbnailPng, "PNG");
        if (image.isNull())
            continue;
        m_thumbnailImages[index] = image;
        const int innerSize = qMax(1, m_cellSize - 4);
        m_thumbnails.at(index)->setPixmap(QPixmap::fromImage(image.scaled(
            innerSize, innerSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)));
    }

    const int nextThumbnailThreshold = qMax(0, m_thumbnails.size() - ThumbnailBatchSize / 4);
    if (lastVisibleIndex >= nextThumbnailThreshold && m_nextThumbnailEntry < m_loadedImages.size())
        loadMoreThumbnails();
    const int nextDatabaseThreshold = qMax(0, m_loadedImages.size() - DatabasePageSize / 4);
    if (m_hasMoreDatabaseEntries && lastVisibleIndex >= nextDatabaseThreshold)
        loadMoreDatabaseEntries();

    while (firstVisibleIndex >= m_nextUnloadBoundary + ThumbnailBatchSize / 4) {
        const int unloadEnd = qMin(m_nextUnloadBoundary, m_thumbnails.size());
        for (int index = m_nextUnloadBoundary - ThumbnailBatchSize; index < unloadEnd; ++index) {
            if (index < 0 || m_thumbnailImages.at(index).isNull())
                continue;
            m_thumbnailImages[index] = QImage();
            m_thumbnails.at(index)->clear();
        }
        m_nextUnloadBoundary += ThumbnailBatchSize;
    }
}

void ImageLibraryWidget::arrangeThumbnails()
{
    if (m_thumbnails.isEmpty()) {
        m_gridLayout->removeWidget(m_emptyLabel);
        m_emptyLabel->setVisible(true);
        m_gridLayout->addWidget(m_emptyLabel, 0, 0);
        return;
    }
    m_gridLayout->removeWidget(m_emptyLabel);
    m_emptyLabel->hide();
    int left = 0, top = 0, right = 0, bottom = 0;
    m_gridLayout->getContentsMargins(&left, &top, &right, &bottom);
    Q_UNUSED(bottom);
    m_gridTopMargin = top;
    m_gridVerticalSpacing = m_gridLayout->verticalSpacing();
    constexpr int imageSize = 128;
    constexpr int borderWidth = 2;
    const int gap = m_gridLayout->horizontalSpacing();
    const int availableWidth = qMax(1, width() - left - right);
    const int minCellWidth = imageSize + borderWidth * 2;
    m_columns = qMax(1, (availableWidth + gap) / (minCellWidth + gap));
    m_cellSize = qMax(minCellWidth, (availableWidth - gap * (m_columns - 1)) / m_columns);
    const int innerSize = m_cellSize - borderWidth * 2;
    for (int index = 0; index < m_thumbnails.size(); ++index) {
        QLabel *label = m_thumbnails.at(index);
        m_gridLayout->removeWidget(label);
        label->setFixedSize(m_cellSize, m_cellSize);
        const bool selected = m_selection &&
            m_selection->isImageSelected(m_thumbnailPaths.at(index), m_year, m_month);
        label->setStyleSheet(selected
                                 ? QStringLiteral("border: 2px solid #55aaff; background: #202329;")
                                 : QStringLiteral("border: 2px solid #454b55; background: #202329;"));
        if (m_thumbnailImages.at(index).isNull()) {
            label->clear();
        } else {
            label->setPixmap(QPixmap::fromImage(m_thumbnailImages.at(index).scaled(
                innerSize, innerSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)));
        }
        m_gridLayout->addWidget(label, index / m_columns, index % m_columns);
    }
}

void ImageLibraryWidget::selectThumbnail(QWidget *thumbnail, const QString &path,
                                         Qt::KeyboardModifiers modifiers)
{
    Q_UNUSED(thumbnail);
    const int clickedIndex = m_thumbnailPaths.indexOf(path);
    if (clickedIndex < 0)
        return;
    if (!m_selection)
        return;
    const bool control = modifiers.testFlag(Qt::ControlModifier) || modifiers.testFlag(Qt::MetaModifier);
    const bool shift = modifiers.testFlag(Qt::ShiftModifier);
    if (shift && m_selectionAnchor >= 0) {
        if (!control)
            m_selection->clear();
        const int first = qMin(m_selectionAnchor, clickedIndex);
        const int last = qMax(m_selectionAnchor, clickedIndex);
        for (int index = first; index <= last; ++index)
            m_selection->setImageSelected(m_thumbnailPaths.at(index), m_year, m_month, true);
    } else if (control) {
        m_selection->toggleImage(path, m_year, m_month);
        m_selectionAnchor = clickedIndex;
    } else {
        m_selection->clear();
        m_selection->setImageSelected(path, m_year, m_month, true);
        m_selectionAnchor = clickedIndex;
    }
    updateSelectionAppearance();
    emit selectionChanged();
}

void ImageLibraryWidget::hideSelectedThumbnail()
{
    if (m_selection && m_selection->hasSelection())
        emit hideSelectionRequested();
}

void ImageLibraryWidget::deleteSelectedImage()
{
    if (m_selection && m_selection->hasSelection())
        emit deleteSelectionRequested();
}

void ImageLibraryWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    arrangeThumbnails();
}
