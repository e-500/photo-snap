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

#include "photoview.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QMimeData>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QScrollBar>
#include <QUrl>
#include <QWheelEvent>

#include <algorithm>

PhotoView::PhotoView(QWidget *parent)
    : QGraphicsView(parent), m_scene(new QGraphicsScene(this))
{
    setScene(m_scene);
    setBackgroundBrush(QColor(QStringLiteral("#111318")));
    setFrameShape(QFrame::NoFrame);
    setRenderHint(QPainter::Antialiasing, false);
    setRenderHint(QPainter::SmoothPixmapTransform, true);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setAcceptDrops(true);
}

void PhotoView::setPhoto(const QPixmap &pixmap)
{
    m_scene->clear();
    m_photoItem = m_scene->addPixmap(pixmap);
    m_photoItem->setTransformationMode(Qt::SmoothTransformation);
    m_scene->setSceneRect(m_photoItem->boundingRect());
    m_fitToWindow = true;
    fitPhoto();
}

void PhotoView::clearPhoto()
{
    m_scene->clear();
    m_photoItem = nullptr;
    resetTransform();
    m_zoom = 1.0;
    emit zoomChanged(m_zoom);
    viewport()->update();
}

bool PhotoView::hasPhoto() const
{
    return m_photoItem != nullptr;
}

void PhotoView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        emit clicked();
    QGraphicsView::mousePressEvent(event);
}

qreal PhotoView::zoomFactor() const
{
    return m_zoom;
}

void PhotoView::fitPhoto()
{
    if (!hasPhoto())
        return;

    m_fitToWindow = true;
    fitInView(m_photoItem, Qt::KeepAspectRatio);
    m_zoom = transform().m11();
    emit zoomChanged(m_zoom);
}

void PhotoView::showActualSize()
{
    if (!hasPhoto())
        return;
    m_fitToWindow = false;
    resetTransform();
    m_zoom = 1.0;
    emit zoomChanged(m_zoom);
    centerOn(m_photoItem);
}

void PhotoView::zoomIn()
{
    if (hasPhoto())
        applyZoom(m_zoom * 1.25);
}

void PhotoView::zoomOut()
{
    if (hasPhoto())
        applyZoom(m_zoom / 1.25);
}

void PhotoView::applyZoom(qreal factor)
{
    if (!hasPhoto())
        return;

    const qreal clamped = std::clamp(factor, qreal(0.05), qreal(32.0));
    if (qFuzzyCompare(clamped, m_zoom))
        return;

    m_fitToWindow = false;
    const qreal ratio = clamped / m_zoom;
    scale(ratio, ratio);
    m_zoom = clamped;
    emit zoomChanged(m_zoom);
}

void PhotoView::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
    if (m_fitToWindow)
        fitPhoto();
}

void PhotoView::wheelEvent(QWheelEvent *event)
{
    if (!hasPhoto()) {
        QGraphicsView::wheelEvent(event);
        return;
    }

    if (event->angleDelta().y() > 0)
        zoomIn();
    else if (event->angleDelta().y() < 0)
        zoomOut();
    event->accept();
}

void PhotoView::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void PhotoView::dropEvent(QDropEvent *event)
{
    QStringList paths;
    for (const QUrl &url : event->mimeData()->urls()) {
        if (url.isLocalFile() && QFileInfo(url.toLocalFile()).isFile())
            paths.append(url.toLocalFile());
    }
    if (!paths.isEmpty()) {
        emit filesDropped(paths);
        event->acceptProposedAction();
    }
}
