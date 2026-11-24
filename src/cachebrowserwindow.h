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

#include <QDockWidget>
#include <QStringList>
#include <QVector>

#include "imagecache.h"
#include "selectedimages.h"

class QLabel;
class QCheckBox;
class QDialog;
class QPlainTextEdit;
class QPushButton;
class QProcess;
class QResizeEvent;
class QScrollArea;
class QToolButton;
class QWidget;
class QShowEvent;
class QTimer;
class QVBoxLayout;
class ImageLibraryWidget;

class CacheBrowserWindow final : public QDockWidget
{
    Q_OBJECT

public:
    explicit CacheBrowserWindow(ImageCache *cache, QWidget *parent = nullptr);
    void refresh();
    bool hasSelection() const;
    QVector<ImageMetadata> getSelectedImages();
    void clearSelection();

signals:
    void openImageRequested(const QString &path);
    void selectionChanged();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    void startScan();

private:
    struct YearPane;
    struct MonthPane;
    void setYearExpanded(YearPane *pane, bool expanded);
    void setMonthExpanded(MonthPane *pane, bool expanded);
    void updateSelectionControls(YearPane *pane = nullptr);
    void hideSelectedImages();
    void deleteSelectedImages();
    void checkScrollPrefetch();
    void appendScanOutput(const QString &output);

    ImageCache *m_cache = nullptr;
    QScrollArea *m_scrollArea = nullptr;
    QWidget *m_yearsWidget = nullptr;
    QVBoxLayout *m_yearsLayout = nullptr;
    QLabel *m_emptyLabel = nullptr;
    QToolButton *m_scanButton = nullptr;
    QToolButton *m_dateModeButton = nullptr;
    QProcess *m_scannerProcess = nullptr;
    QDialog *m_scanLogDialog = nullptr;
    QPlainTextEdit *m_scanLog = nullptr;
    QString m_scanOutputBuffer;
    QPushButton *m_pauseResumeButton = nullptr;
    QPushButton *m_stopScanButton = nullptr;
    bool m_scanPaused = false;
    ImageDateMode m_dateMode = ImageDateMode::ShootingDate;
    SelectedImages m_selectedImages;
    QVector<YearPane *> m_yearPanes;
    QStringList m_monthsToRestore;
    QTimer *m_scrollPrefetchTimer = nullptr;
};
