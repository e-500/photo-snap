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

#include "mainwindow.h"

#include "photoview.h"
#include "cachebrowserwindow.h"
#include "imagemetadata.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QDateTime>
#include <QDateTimeEdit>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGraphicsPixmapItem>
#include <QImageReader>
#include <QKeySequence>
#include <QLabel>
#include <QLocale>
#include <QMenu>
#include <QMenuBar>
#include <QMimeData>
#include <QMessageBox>
#include <QPixmap>
#include <QPainter>
#include <QPalette>
#include <QStatusBar>
#include <QStyle>
#include <QToolBar>
#include <QTransform>
#include <QToolButton>
#include <QUrl>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include <algorithm>

namespace {

QIcon copyIcon()
{
    QPixmap pixmap(20, 20);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(QApplication::palette().color(QPalette::ButtonText), 1.5));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(QRectF(7.5, 2.5, 9, 11), 1.2, 1.2);
    painter.drawRoundedRect(QRectF(3.5, 6.5, 9, 11), 1.2, 1.2);
    return QIcon(pixmap);
}

QIcon editTimestampIcon()
{
    QPixmap pixmap(20, 20);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    QPen pen(QApplication::palette().color(QPalette::ButtonText), 1.4);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    QPolygonF pencil;
    pencil << QPointF(4, 15) << QPointF(6, 16) << QPointF(16, 6)
           << QPointF(13, 3) << QPointF(4, 12);
    painter.drawPolygon(pencil);
    painter.drawLine(QPointF(4, 15), QPointF(3, 18));
    painter.drawLine(QPointF(4, 15), QPointF(7, 16));
    return QIcon(pixmap);
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("Photo Snap"));
    resize(1100, 760);
    setMinimumSize(520, 360);
    auto *centralWidget = new QWidget(this);
    m_view = new PhotoView(centralWidget);
    setCentralWidget(centralWidget);

    createActions();
    createToolbar();

    auto *centralLayout = new QVBoxLayout(centralWidget);
    centralLayout->setContentsMargins(0, 0, 0, 0);
    centralLayout->setSpacing(0);
    auto *imageTitleBar = new QWidget(centralWidget);
    auto *imageTitleLayout = new QHBoxLayout(imageTitleBar);
    imageTitleLayout->setContentsMargins(8, 2, 4, 2);
    m_editExifButton = new QToolButton(imageTitleBar);
    m_editExifButton->setAutoRaise(true);
    m_editExifButton->setIcon(editTimestampIcon());
    m_editExifButton->setText(tr("Edit shooting timestamp"));
    m_editExifButton->setEnabled(false);
    m_editExifButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    connect(m_editExifButton, &QToolButton::clicked, this, &MainWindow::editExif);
    m_libraryButton = new QToolButton(imageTitleBar);
    m_libraryButton->setAutoRaise(true);
    m_libraryButton->setText(tr("Open library"));
    connect(m_libraryButton, &QToolButton::clicked, m_cachedImagesAction, &QAction::trigger);
    auto *copyButton = new QToolButton(imageTitleBar);
    copyButton->setAutoRaise(true);
    copyButton->setDefaultAction(m_copyAction);
    copyButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    copyButton->setText(tr("Copy"));
    auto *rotateButton = new QToolButton(imageTitleBar);
    rotateButton->setAutoRaise(true);
    rotateButton->setDefaultAction(m_rotateAction);
    rotateButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    imageTitleLayout->addWidget(rotateButton);
    imageTitleLayout->addWidget(copyButton);
    imageTitleLayout->addStretch(1);
    imageTitleLayout->addWidget(m_editExifButton);
    imageTitleLayout->addWidget(m_libraryButton);
    centralLayout->addWidget(imageTitleBar);
    centralLayout->addWidget(m_view, 1);

    auto *fileMenu = menuBar()->addMenu(tr("&File"));
    QAction *openAction = fileMenu->addAction(tr("&Open…"), this, &MainWindow::chooseImage, QKeySequence::Open);
    addAction(openAction);
    fileMenu->addSeparator();
    QAction *quitAction = fileMenu->addAction(tr("&Quit"), qApp, &QApplication::quit, QKeySequence::Quit);
    addAction(quitAction);

    auto *viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(m_zoomInAction);
    viewMenu->addAction(m_zoomOutAction);
    viewMenu->addAction(m_fitAction);
    viewMenu->addAction(m_actualSizeAction);
    viewMenu->addAction(m_rotateAction);
    viewMenu->addAction(m_copyAction);
    viewMenu->addSeparator();
    viewMenu->addAction(m_fullScreenAction);

    auto *navigateMenu = menuBar()->addMenu(tr("&Navigate"));
    navigateMenu->addAction(m_previousAction);
    navigateMenu->addAction(m_nextAction);

    menuBar()->hide();
    m_toolbar->hide();

    m_zoomLabel = new QLabel(tr("—"), this);
    m_zoomLabel->setMinimumWidth(58);
    m_zoomLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    statusBar()->addPermanentWidget(m_zoomLabel);
    statusBar()->showMessage(tr("Open a photo to get started · Ctrl+O"));

    connect(m_view, &PhotoView::zoomChanged, this, &MainWindow::updateZoomLabel);
    connect(m_view, &PhotoView::filesDropped, this, [this](const QStringList &paths) {
        if (!paths.isEmpty())
            openImage(paths.constFirst());
    });
    updateNavigationActions();
}

void MainWindow::createActions()
{
    const QStyle *style = QApplication::style();
    m_previousAction = new QAction(style->standardIcon(QStyle::SP_MediaSkipBackward), tr("Previous photo"), this);
    m_previousAction->setShortcut(QKeySequence(Qt::Key_Left));
    connect(m_previousAction, &QAction::triggered, this, &MainWindow::openPreviousImage);

    m_nextAction = new QAction(style->standardIcon(QStyle::SP_MediaSkipForward), tr("Next photo"), this);
    m_nextAction->setShortcut(QKeySequence(Qt::Key_Right));
    connect(m_nextAction, &QAction::triggered, this, &MainWindow::openNextImage);

    m_zoomInAction = new QAction(style->standardIcon(QStyle::SP_ArrowUp), tr("Zoom in"), this);
    m_zoomInAction->setShortcut(QKeySequence::ZoomIn);
    connect(m_zoomInAction, &QAction::triggered, m_view, &PhotoView::zoomIn);

    m_zoomOutAction = new QAction(style->standardIcon(QStyle::SP_ArrowDown), tr("Zoom out"), this);
    m_zoomOutAction->setShortcut(QKeySequence::ZoomOut);
    connect(m_zoomOutAction, &QAction::triggered, m_view, &PhotoView::zoomOut);

    m_fitAction = new QAction(tr("Fit to window"), this);
    m_fitAction->setShortcut(QKeySequence(Qt::Key_F));
    connect(m_fitAction, &QAction::triggered, m_view, &PhotoView::fitPhoto);

    m_actualSizeAction = new QAction(tr("Actual size"), this);
    m_actualSizeAction->setShortcut(QKeySequence(Qt::Key_1));
    connect(m_actualSizeAction, &QAction::triggered, m_view, &PhotoView::showActualSize);

    m_rotateAction = new QAction(style->standardIcon(QStyle::SP_BrowserReload), tr("Rotate"), this);
    m_rotateAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
    connect(m_rotateAction, &QAction::triggered, this, &MainWindow::rotateImage);

    m_copyAction = new QAction(copyIcon(), tr("Copy"), this);
    m_copyAction->setShortcut(QKeySequence::Copy);
    m_copyAction->setEnabled(false);
    connect(m_copyAction, &QAction::triggered, this, &MainWindow::copyImageToClipboard);

    m_fullScreenAction = new QAction(tr("Full screen"), this);
    m_fullScreenAction->setShortcut(QKeySequence(Qt::Key_F11));
    connect(m_fullScreenAction, &QAction::triggered, this, &MainWindow::toggleFullScreen);

    m_cachedImagesAction = new QAction(style->standardIcon(QStyle::SP_FileDialogListView), tr("Library"), this);
    connect(m_cachedImagesAction, &QAction::triggered, this, &MainWindow::showCachedImages);
    m_cachedImagesAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+B")));

    for (QAction *action : {m_previousAction, m_nextAction, m_zoomInAction, m_zoomOutAction,
                            m_fitAction, m_actualSizeAction, m_rotateAction, m_copyAction,
                            m_fullScreenAction, m_cachedImagesAction})
        addAction(action);
}

void MainWindow::createToolbar()
{
    m_toolbar = addToolBar(tr("Photo tools"));
    m_toolbar->setMovable(false);
    m_toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_toolbar->addAction(QApplication::style()->standardIcon(QStyle::SP_DialogOpenButton), tr("Open"),
                        this, &MainWindow::chooseImage);
    m_toolbar->addAction(m_cachedImagesAction);
    m_toolbar->addSeparator();
    m_toolbar->addAction(m_previousAction);
    m_toolbar->addAction(m_nextAction);
    m_toolbar->addSeparator();
    m_toolbar->addAction(m_zoomOutAction);
    m_toolbar->addAction(m_zoomInAction);
    m_toolbar->addAction(m_fitAction);
    m_toolbar->addAction(m_rotateAction);
    m_toolbar->addSeparator();
    m_toolbar->addAction(m_fullScreenAction);

}

void MainWindow::showCachedImages()
{
    if (m_cacheBrowser && m_cacheBrowser->isVisible()) {
        m_cacheBrowser->hide();
        updateLibraryButtonText();
        return;
    }

    ImageCache &imageCache = ImageCache::instance();
    if (!imageCache.initialize()) {
        QMessageBox::warning(this, tr("Image cache unavailable"), imageCache.errorString());
        return;
    }

    if (!m_cacheBrowser) {
        m_cacheBrowser = new CacheBrowserWindow(&imageCache, this);
        addDockWidget(Qt::RightDockWidgetArea, m_cacheBrowser);
        resizeDocks({m_cacheBrowser}, {420}, Qt::Horizontal);
        connect(m_cacheBrowser, &CacheBrowserWindow::openImageRequested, this, &MainWindow::openImage);
        connect(m_cacheBrowser, &CacheBrowserWindow::selectionChanged, this, [this]() {
            const bool hasSelection = !m_cacheBrowser->selectedPaths().isEmpty();
            m_copyAction->setEnabled(hasSelection || !m_originalPixmap.isNull());
        });
        connect(m_cacheBrowser, &QDockWidget::visibilityChanged, this,
                [this]() { updateLibraryButtonText(); });
    }
    m_cacheBrowser->refresh();
    m_cacheBrowser->show();
    m_cacheBrowser->raise();
    m_cacheBrowser->activateWindow();
    updateLibraryButtonText();
}

void MainWindow::updateLibraryButtonText()
{
    if (!m_libraryButton)
        return;
    m_libraryButton->setText(tr("Open library"));
    m_libraryButton->setVisible(!m_cacheBrowser || !m_cacheBrowser->isVisible());
}

QString MainWindow::imageFilter() const
{
    QStringList extensions;
    const auto formats = QImageReader::supportedImageFormats();
    extensions.reserve(formats.size() * 2);
    for (const QByteArray &format : formats) {
        const QString extension = QString::fromLatin1(format);
        extensions.append(QStringLiteral("*.%1").arg(extension.toLower()));
        extensions.append(QStringLiteral("*.%1").arg(extension.toUpper()));
    }
    extensions.removeDuplicates();
    extensions.sort(Qt::CaseInsensitive);
    return tr("Images (%1);;All files (*)").arg(extensions.join(QLatin1Char(' ')));
}

void MainWindow::chooseImage()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("Open photo"),
                                                       m_currentPath.isEmpty() ? QDir::homePath()
                                                                               : QFileInfo(m_currentPath).absolutePath(),
                                                       imageFilter());
    if (!path.isEmpty())
        openImage(path);
}

void MainWindow::openImage(const QString &path)
{
    QImageReader reader(path);
    reader.setAutoTransform(true);
    const QImage image = reader.read();
    if (image.isNull()) {
        QMessageBox::warning(this, tr("Unable to open photo"),
                             tr("Could not read “%1”.\n%2").arg(QFileInfo(path).fileName(), reader.errorString()));
        return;
    }

    m_currentPath = QFileInfo(path).absoluteFilePath();
    m_editExifButton->setEnabled(true);
    m_copyAction->setEnabled(true);
    m_originalPixmap = QPixmap::fromImage(image);
    m_rotation = 0;
    m_view->setPhoto(m_originalPixmap);
    setWindowTitle(QStringLiteral("%1 — Photo Snap").arg(QFileInfo(m_currentPath).fileName()));
    QStringList details = {
        QFileInfo(m_currentPath).fileName(),
        QStringLiteral("%1 × %2 px").arg(image.width()).arg(image.height()),
        QLocale().formattedDataSize(QFileInfo(m_currentPath).size()),
        QString::fromLatin1(reader.format()).toUpper()
    };
    statusBar()->showMessage(details.join(QStringLiteral("  ·  ")));
    updateNavigationActions();
}

QStringList MainWindow::imagesInCurrentFolder() const
{
    if (m_currentPath.isEmpty())
        return {};

    QStringList patterns;
    for (const QByteArray &format : QImageReader::supportedImageFormats()) {
        const QString extension = QString::fromLatin1(format);
        patterns.append(QStringLiteral("*.%1").arg(extension.toLower()));
        patterns.append(QStringLiteral("*.%1").arg(extension.toUpper()));
    }

    QDir directory(QFileInfo(m_currentPath).absolutePath());
    QStringList files = directory.entryList(patterns, QDir::Files, QDir::Name | QDir::IgnoreCase);
    const QString currentName = QFileInfo(m_currentPath).fileName();
    if (!files.contains(currentName, Qt::CaseInsensitive))
        files.append(currentName);
    files.sort(Qt::CaseInsensitive);
    return files;
}

void MainWindow::updateNavigationActions()
{
    const QStringList files = imagesInCurrentFolder();
    const bool enabled = files.size() > 1;
    m_previousAction->setEnabled(enabled);
    m_nextAction->setEnabled(enabled);
}

void MainWindow::openPreviousImage()
{
    const QStringList files = imagesInCurrentFolder();
    if (files.size() < 2)
        return;
    const QString currentName = QFileInfo(m_currentPath).fileName();
    const int index = files.indexOf(currentName, Qt::CaseInsensitive);
    const int previous = index <= 0 ? files.size() - 1 : index - 1;
    openImage(QDir(QFileInfo(m_currentPath).absolutePath()).filePath(files.at(previous)));
}

void MainWindow::openNextImage()
{
    const QStringList files = imagesInCurrentFolder();
    if (files.size() < 2)
        return;
    const QString currentName = QFileInfo(m_currentPath).fileName();
    const int index = files.indexOf(currentName, Qt::CaseInsensitive);
    const int next = index < 0 || index == files.size() - 1 ? 0 : index + 1;
    openImage(QDir(QFileInfo(m_currentPath).absolutePath()).filePath(files.at(next)));
}

void MainWindow::rotateImage()
{
    if (m_originalPixmap.isNull())
        return;
    m_rotation = (m_rotation + 90) % 360;
    const QPixmap rotated = m_originalPixmap.transformed(QTransform().rotate(m_rotation), Qt::SmoothTransformation);
    m_view->setPhoto(rotated);
}

void MainWindow::copyImageToClipboard()
{
    const QStringList selectedPaths = m_cacheBrowser ? m_cacheBrowser->selectedPaths() : QStringList();
    if (!selectedPaths.isEmpty()) {
        auto *mimeData = new QMimeData;
        QList<QUrl> urls;
        urls.reserve(selectedPaths.size());
        for (const QString &path : selectedPaths)
            urls.append(QUrl::fromLocalFile(path));
        mimeData->setUrls(urls);
#ifdef Q_OS_LINUX
        QByteArray copiedFiles = "copy\n";
        for (const QUrl &url : urls) {
            copiedFiles += url.toString(QUrl::FullyEncoded).toUtf8();
            copiedFiles += '\n';
        }
        mimeData->setData("x-special/gnome-copied-files", copiedFiles);
#endif
        if (selectedPaths.size() == 1) {
            QImageReader reader(selectedPaths.constFirst());
            reader.setAutoTransform(true);
            const QImage selectedImage = reader.read();
            if (!selectedImage.isNull())
                mimeData->setImageData(selectedImage);
        }
        QApplication::clipboard()->setMimeData(mimeData);
        statusBar()->showMessage(tr("%1 selected photo(s) copied to clipboard").arg(selectedPaths.size()), 2500);
        return;
    }
    if (m_originalPixmap.isNull())
        return;
    const QPixmap displayedPixmap = m_originalPixmap.transformed(
        QTransform().rotate(m_rotation), Qt::SmoothTransformation);
    auto *mimeData = new QMimeData;
    const QUrl currentFileUrl = QUrl::fromLocalFile(m_currentPath);
    mimeData->setUrls({currentFileUrl});
#ifdef Q_OS_LINUX
    mimeData->setData("x-special/gnome-copied-files",
                      QByteArray("copy\n") + currentFileUrl.toString(QUrl::FullyEncoded).toUtf8() + '\n');
#endif
    mimeData->setImageData(displayedPixmap.toImage());
    QApplication::clipboard()->setMimeData(mimeData);
    statusBar()->showMessage(tr("Photo copied to clipboard"), 2500);
}

void MainWindow::editExif()
{
    if (m_currentPath.isEmpty())
        return;

    QDateTime initialDateTime = QDateTime::fromString(readImageShotDateTime(m_currentPath), Qt::ISODate);
    if (!initialDateTime.isValid())
        initialDateTime = QFileInfo(m_currentPath).lastModified();

    QDialog dialog(this);
    dialog.setWindowTitle(tr("Edit EXIF"));
    auto *layout = new QVBoxLayout(&dialog);
    auto *form = new QFormLayout;
    auto *shotDateTime = new QDateTimeEdit(initialDateTime, &dialog);
    shotDateTime->setDisplayFormat(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    shotDateTime->setCalendarPopup(true);
    form->addRow(tr("Shot date/time"), shotDateTime);
    layout->addLayout(form);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);
    if (dialog.exec() != QDialog::Accepted)
        return;

    const QString shootingDateTime = shotDateTime->dateTime().toString(Qt::ISODate);
    QString error;
    if (!writeImageShotDateTime(m_currentPath, shotDateTime->dateTime(), &error)) {
        QMessageBox::warning(this, tr("Could not update EXIF"), error);
        return;
    }

    ImageCache &cache = ImageCache::instance();
    const QString updateDateTime = QFileInfo(m_currentPath).lastModified().toString(Qt::ISODate);
    if (cache.initialize() && !cache.updateImageShootingDateTime(m_currentPath, shootingDateTime, updateDateTime))
        QMessageBox::warning(this, tr("Photo saved"),
                             tr("The EXIF date was saved, but the library entry could not be updated: %1")
                                 .arg(cache.errorString()));
    if (m_cacheBrowser && m_cacheBrowser->isVisible())
        m_cacheBrowser->refresh();
}

void MainWindow::toggleFullScreen()
{
    if (isFullScreen()) {
        showNormal();
        if (m_wasMaximized)
            showMaximized();
        m_fullScreenAction->setText(tr("Full screen"));
    } else {
        m_wasMaximized = isMaximized();
        showFullScreen();
        m_fullScreenAction->setText(tr("Exit full screen"));
    }
}

void MainWindow::updateZoomLabel(qreal factor)
{
    m_zoomLabel->setText(m_view->hasPhoto() ? QStringLiteral("%1%").arg(qRound(factor * 100))
                                            : QStringLiteral("—"));
}
