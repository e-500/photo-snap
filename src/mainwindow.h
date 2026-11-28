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
#include "core/ImageDocument.h"
#include "core/PreviewRenderer.h"

#include <QMainWindow>
#include <QPixmap>
#include <QStringList>

class QAction;
class CacheBrowserWindow;
class CropOverlay;
class AdjustmentPanel;
class QUndoStack;
class QDoubleSpinBox;
class QComboBox;
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
    void rotateCounterClockwise();
    void resetRotation();
    void setFreeRotation(double degrees);
    void toggleCrop();
    void confirmCrop();
    void cancelCrop();
    void setBrightness(double value);
    void setContrast(double value);
    void setSaturation(double value);
    void setGrayscale(bool enabled);
    void setSepia(bool enabled);
    void setTemperature(double value);
    void setSharpness(double value);
    void resetAdjustments();
    void undo();
    void redo();
    void copyImageToClipboard();
    void editExif();
    void toggleFullScreen();
    void updateZoomLabel(qreal factor);
    void openSettings();
    void exportImage();

private:
    void createActions();
    void createToolbar();
    void updateLibraryButtonText();
    void updateNavigationActions();
    void schedulePreview();
    void pushAdjustment(const QString &text, int id,
                        const ImageAdjustments &before,
                        const ImageAdjustments &after);
    void updateDocumentTitle();
    void syncAdjustmentPanel();
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
    QAction *m_rotateLeftAction = nullptr;
    QAction *m_resetRotationAction = nullptr;
    QAction *m_cropAction = nullptr;
    QAction *m_confirmCropAction = nullptr;
    QAction *m_cancelCropAction = nullptr;
    QAction *m_copyAction = nullptr;
    QAction *m_exportAction = nullptr;
    QAction *m_fullScreenAction = nullptr;
    QAction *m_cachedImagesAction = nullptr;
    QToolBar *m_toolbar = nullptr;
    QToolButton *m_editExifButton = nullptr;
    QToolButton *m_libraryButton = nullptr;
    QToolButton *m_settingsButton = nullptr;
    QToolButton *m_applyCropButton = nullptr;
    QToolButton *m_cancelCropButton = nullptr;
    QDoubleSpinBox *m_rotationSpin = nullptr;
    QComboBox *m_cropRatio = nullptr;
    CacheBrowserWindow *m_cacheBrowser = nullptr;
    QString m_currentPath;
    ImageDocument m_document;
    QPixmap m_originalPixmap;
    double m_rotation = 0.0;
    CropOverlay *m_cropOverlay = nullptr;
    AdjustmentPanel *m_adjustmentPanel = nullptr;
    PreviewRenderer m_previewRenderer;
    quint64 m_previewRequestId = 0;
    QUndoStack *m_undoStack = nullptr;
    QAction *m_undoAction = nullptr;
    QAction *m_redoAction = nullptr;
    bool m_applyingAdjustmentState = false;
    ImageAdjustments m_cropBefore;
    QLabel *m_zoomLabel = nullptr;
    bool m_wasMaximized = false;
};
