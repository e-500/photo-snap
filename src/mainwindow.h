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

#include <QMainWindow>
#include <QPixmap>
#include <QStringList>

class QAction;
class CacheBrowserWindow;
class QLabel;
class PhotoView;
class QToolBar;
class QToolButton;
class QWidget;

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    void openImage(const QString &path);

private slots:
    void chooseImage();
    void showCachedImages();
    void openPreviousImage();
    void openNextImage();
    void rotateImage();
    void copyImageToClipboard();
    void editExif();
    void toggleFullScreen();
    void updateZoomLabel(qreal factor);
    void openSettings();

private:
    void createActions();
    void createToolbar();
    void updateLibraryButtonText();
    void updateNavigationActions();
    QStringList imagesInCurrentFolder() const;
    QString imageFilter() const;

    PhotoView *m_view = nullptr;
    QAction *m_previousAction = nullptr;
    QAction *m_nextAction = nullptr;
    QAction *m_zoomInAction = nullptr;
    QAction *m_zoomOutAction = nullptr;
    QAction *m_fitAction = nullptr;
    QAction *m_actualSizeAction = nullptr;
    QAction *m_rotateAction = nullptr;
    QAction *m_copyAction = nullptr;
    QAction *m_fullScreenAction = nullptr;
    QAction *m_cachedImagesAction = nullptr;
    QToolBar *m_toolbar = nullptr;
    QToolButton *m_editExifButton = nullptr;
    QToolButton *m_libraryButton = nullptr;
    QToolButton *m_settingsButton = nullptr;
    CacheBrowserWindow *m_cacheBrowser = nullptr;
    QString m_currentPath;
    QPixmap m_originalPixmap;
    int m_rotation = 0;
    QLabel *m_zoomLabel = nullptr;
    bool m_wasMaximized = false;
};
