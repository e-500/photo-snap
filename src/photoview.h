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

#include <QGraphicsView>
#include <QStringList>

class QGraphicsPixmapItem;
class QGraphicsScene;
class QPixmap;

class PhotoView final : public QGraphicsView
{
    Q_OBJECT

public:
    explicit PhotoView(QWidget *parent = nullptr);

    void setPhoto(const QPixmap &pixmap);
    void clearPhoto();
    void fitPhoto();
    void showActualSize();
    void zoomIn();
    void zoomOut();
    qreal zoomFactor() const;
    bool hasPhoto() const;

signals:
    void zoomChanged(qreal factor);
    void filesDropped(const QStringList &paths);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    void applyZoom(qreal factor);

    QGraphicsScene *m_scene = nullptr;
    QGraphicsPixmapItem *m_photoItem = nullptr;
    qreal m_zoom = 1.0;
    bool m_fitToWindow = true;
};
