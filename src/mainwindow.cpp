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
#include "language.h"
#include "core/PreviewRenderer.h"
#include "core/ImageTransform.h"
#include "ui/CropOverlay.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QCheckBox>
#include <QComboBox>
#include <QCryptographicHash>
#include <QDir>
#include <QDateTime>
#include <QDateTimeEdit>
#include <QDoubleSpinBox>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
#include <QFile>
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
#include <QPainterPath>
#include <QPalette>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStandardPaths>
#include <QStatusBar>
#include <QStyle>
#include <QSettings>
#include <QToolBar>
#include <QTransform>
#include <QToolButton>
#include <QtMath>
#include <QUrl>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include <algorithm>
#include <utility>

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

QIcon settingsIcon()
{
    QPixmap pixmap(20, 20);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    QPolygonF gear;
    for (int tooth = 0; tooth < 8; ++tooth) {
        const qreal base = tooth * 45.0;
        for (const auto &point : {qMakePair(base, 7.0), qMakePair(base + 7.0, 9.2),
                                  qMakePair(base + 20.5, 9.2), qMakePair(base + 27.5, 7.0)}) {
            const qreal radians = qDegreesToRadians(point.first);
            gear << QPointF(10.0 + point.second * qCos(radians),
                            10.0 + point.second * qSin(radians));
        }
    }
    QPainterPath shape;
    shape.setFillRule(Qt::OddEvenFill);
    shape.addPolygon(gear);
    shape.addEllipse(QPointF(10.0, 10.0), 3.0, 3.0);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QApplication::palette().color(QPalette::ButtonText));
    painter.drawPath(shape);
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
    m_cropOverlay = new CropOverlay(m_view->viewport());
    m_cropOverlay->hide();
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
    m_editExifButton->setText(tr("Edit metadata"));
    m_editExifButton->setEnabled(false);
    m_editExifButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    connect(m_editExifButton, &QToolButton::clicked, this, &MainWindow::editExif);
    m_libraryButton = new QToolButton(imageTitleBar);
    m_libraryButton->setAutoRaise(true);
    m_libraryButton->setText(tr("Open library"));
    connect(m_libraryButton, &QToolButton::clicked, m_cachedImagesAction, &QAction::trigger);
    m_settingsButton = new QToolButton(imageTitleBar);
    m_settingsButton->setAutoRaise(true);
    m_settingsButton->setIcon(settingsIcon());
    m_settingsButton->setText(tr("Settings"));
    m_settingsButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    connect(m_settingsButton, &QToolButton::clicked, this, &MainWindow::openSettings);
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
    auto *rotateLeftButton = new QToolButton(imageTitleBar);
    rotateLeftButton->setAutoRaise(true);
    rotateLeftButton->setDefaultAction(m_rotateLeftAction);
    imageTitleLayout->addWidget(rotateLeftButton);
    auto *resetRotationButton = new QToolButton(imageTitleBar);
    resetRotationButton->setAutoRaise(true);
    resetRotationButton->setDefaultAction(m_resetRotationAction);
    imageTitleLayout->addWidget(resetRotationButton);
    m_rotationSpin = new QDoubleSpinBox(imageTitleBar);
    m_rotationSpin->setRange(-180.0, 180.0);
    m_rotationSpin->setDecimals(1);
    m_rotationSpin->setSingleStep(0.1);
    m_rotationSpin->setSuffix(QStringLiteral(" °"));
    m_rotationSpin->setToolTip(tr("Free rotation"));
    imageTitleLayout->addWidget(m_rotationSpin);
    connect(m_rotationSpin, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &MainWindow::setFreeRotation);
    auto *cropButton = new QToolButton(imageTitleBar);
    cropButton->setAutoRaise(true);
    cropButton->setDefaultAction(m_cropAction);
    imageTitleLayout->addWidget(cropButton);
    m_cropRatio = new QComboBox(imageTitleBar);
    m_cropRatio->addItem(tr("Free"), 0.0);
    m_cropRatio->addItem(tr("Original"), -1.0);
    m_cropRatio->addItem(QStringLiteral("1:1"), 1.0);
    m_cropRatio->addItem(QStringLiteral("4:3"), 4.0 / 3.0);
    m_cropRatio->addItem(QStringLiteral("3:2"), 3.0 / 2.0);
    m_cropRatio->addItem(QStringLiteral("16:9"), 16.0 / 9.0);
    m_cropRatio->setVisible(false);
    imageTitleLayout->addWidget(m_cropRatio);
    connect(m_cropRatio, qOverload<int>(&QComboBox::currentIndexChanged),
            this, [this](int index) {
        double ratio = m_cropRatio->itemData(index).toDouble();
        if (ratio < 0.0) {
            const QImage preview = ImageTransform::rotate(
                m_document.originalImage(), m_document.adjustments().rotation);
            ratio = preview.isNull()
                ? 0.0
                : qreal(preview.width()) / preview.height();
        }
        m_cropOverlay->setAspectRatio(ratio);
    });
    m_applyCropButton = new QToolButton(imageTitleBar);
    m_applyCropButton->setAutoRaise(true);
    m_applyCropButton->setDefaultAction(m_confirmCropAction);
    m_applyCropButton->setVisible(false);
    imageTitleLayout->addWidget(m_applyCropButton);
    m_cancelCropButton = new QToolButton(imageTitleBar);
    m_cancelCropButton->setAutoRaise(true);
    m_cancelCropButton->setDefaultAction(m_cancelCropAction);
    m_cancelCropButton->setVisible(false);
    imageTitleLayout->addWidget(m_cancelCropButton);
    connect(m_cropOverlay, &CropOverlay::confirmed, this, &MainWindow::confirmCrop);
    connect(m_cropOverlay, &CropOverlay::cancelled, this, &MainWindow::cancelCrop);
    connect(m_cropOverlay, &CropOverlay::cropChanged, this, [this](const QRectF &rect) {
        m_document.setCropRect(rect);
        PreviewRenderer renderer;
        m_originalPixmap = QPixmap::fromImage(renderer.render(
            m_document.originalImage(), m_document.adjustments()));
        m_view->setPhoto(m_originalPixmap);
    });
    imageTitleLayout->addWidget(copyButton);
    imageTitleLayout->addWidget(m_editExifButton);
    imageTitleLayout->addStretch(1);
    imageTitleLayout->addWidget(m_settingsButton);
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
    connect(m_view, &PhotoView::clicked, this, [this]() {
        if (m_cacheBrowser)
            m_cacheBrowser->clearSelection();
    });
    connect(m_view, &PhotoView::filesDropped, this, [this](const QStringList &paths) {
        if (!paths.isEmpty())
            openImage(paths.constFirst());
    });
    updateNavigationActions();
}

void MainWindow::openSettings()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Settings"));
    auto *layout = new QVBoxLayout(&dialog);
    auto *form = new QFormLayout;
    auto *languageCombo = new QComboBox(&dialog);
    for (const QString &language : supportedLanguages())
        languageCombo->addItem(languageName(language), language);
    const QString currentLanguage = normalizedLanguage(
        QSettings().value(QStringLiteral("language"), QStringLiteral("EN-GB")).toString());
    languageCombo->setCurrentIndex(languageCombo->findData(currentLanguage));
    form->addRow(tr("Language"), languageCombo);
    layout->addLayout(form);

    auto *databasePathLabel = new QLabel(
        tr("Database file: %1").arg(QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation))
                                         .filePath(QStringLiteral("image-cache.sqlite"))), &dialog);
    databasePathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    databasePathLabel->setWordWrap(true);
    layout->addWidget(databasePathLabel);
    auto *revealDatabaseButton = new QPushButton(tr("Reveal database in file explorer"), &dialog);
    layout->addWidget(revealDatabaseButton);
    connect(revealDatabaseButton, &QPushButton::clicked, &dialog, [this]() {
        ImageCache &cache = ImageCache::instance();
        if (!cache.initialize()) {
            QMessageBox::warning(this, tr("Image cache unavailable"), cache.errorString());
            return;
        }
        const QString directory = QFileInfo(cache.databasePath()).absolutePath();
        if (!QDesktopServices::openUrl(QUrl::fromLocalFile(directory)))
            QMessageBox::warning(this, tr("Could not open folder"), tr("Could not open %1").arg(directory));
    });
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted)
        return;

    const QString selectedLanguage = languageCombo->currentData().toString();
    if (selectedLanguage != currentLanguage) {
        QSettings().setValue(QStringLiteral("language"), selectedLanguage);
        QMessageBox::information(this, tr("Language changed"),
                                 tr("Restart Photo Snap to apply the language change."));
    }
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

    m_rotateLeftAction = new QAction(tr("Rotate left"), this);
    connect(m_rotateLeftAction, &QAction::triggered, this, &MainWindow::rotateCounterClockwise);

    m_resetRotationAction = new QAction(tr("Reset rotation"), this);
    connect(m_resetRotationAction, &QAction::triggered, this, &MainWindow::resetRotation);

    m_cropAction = new QAction(tr("Crop"), this);
    connect(m_cropAction, &QAction::triggered, this, &MainWindow::toggleCrop);

    m_confirmCropAction = new QAction(tr("Apply crop"), this);
    connect(m_confirmCropAction, &QAction::triggered, this, &MainWindow::confirmCrop);

    m_cancelCropAction = new QAction(tr("Cancel crop"), this);
    connect(m_cancelCropAction, &QAction::triggered, this, &MainWindow::cancelCrop);

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
            const bool hasSelection = m_cacheBrowser->hasSelection();
            m_copyAction->setEnabled(hasSelection || !m_originalPixmap.isNull());
            m_editExifButton->setEnabled(hasSelection || !m_currentPath.isEmpty());
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
        QDialog dialog(this);
        dialog.setWindowTitle(tr("Unable to open photo"));
        dialog.setMinimumWidth(460);
        auto *layout = new QVBoxLayout(&dialog);
        auto *message = new QLabel(
            tr("Could not read “%1”.\n%2").arg(QFileInfo(path).fileName(), reader.errorString()), &dialog);
        message->setWordWrap(true);
        layout->addWidget(message);
        layout->addWidget(new QLabel(tr("Full path"), &dialog));
        auto *pathText = new QPlainTextEdit(&dialog);
        pathText->setReadOnly(true);
        pathText->setPlainText(QFileInfo(path).absoluteFilePath());
        pathText->setLineWrapMode(QPlainTextEdit::WidgetWidth);
        pathText->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        pathText->setMinimumHeight(64);
        pathText->setMaximumHeight(112);
        layout->addWidget(pathText);
        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok, &dialog);
        connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        layout->addWidget(buttons);
        dialog.exec();
        return;
    }

    m_currentPath = QFileInfo(path).absoluteFilePath();
    m_editExifButton->setEnabled(true);
    m_copyAction->setEnabled(true);
    m_originalPixmap = QPixmap::fromImage(image);
    m_document.setImage(image, m_currentPath);
    m_rotation = 0;
    if (m_rotationSpin)
        m_rotationSpin->setValue(0.0);
    if (m_cropOverlay)
        m_cropOverlay->hide();
    PreviewRenderer renderer;
    m_originalPixmap = QPixmap::fromImage(renderer.render(
        m_document.originalImage(), m_document.adjustments()));
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
    if (m_document.originalImage().isNull())
        return;
    m_rotation = ImageTransform::addQuarterTurn(m_rotation);
    if (m_rotationSpin)
        m_rotationSpin->setValue(m_rotation);
    m_document.setRotation(m_rotation);
    PreviewRenderer renderer;
    m_originalPixmap = QPixmap::fromImage(renderer.render(
        m_document.originalImage(), m_document.adjustments()));
    m_view->setPhoto(m_originalPixmap);
}

void MainWindow::rotateCounterClockwise()
{
    if (m_document.originalImage().isNull())
        return;
    m_rotation = ImageTransform::normalizeRotation(m_rotation - 90.0);
    if (m_rotationSpin)
        m_rotationSpin->setValue(m_rotation);
    m_document.setRotation(m_rotation);
    PreviewRenderer renderer;
    m_originalPixmap = QPixmap::fromImage(renderer.render(
        m_document.originalImage(), m_document.adjustments()));
    m_view->setPhoto(m_originalPixmap);
}

void MainWindow::resetRotation()
{
    m_rotation = 0.0;
    if (m_rotationSpin)
        m_rotationSpin->setValue(0.0);
    m_document.setRotation(0.0);
    PreviewRenderer renderer;
    m_originalPixmap = QPixmap::fromImage(renderer.render(
        m_document.originalImage(), m_document.adjustments()));
    m_view->setPhoto(m_originalPixmap);
}

void MainWindow::setFreeRotation(double degrees)
{
    if (m_document.originalImage().isNull())
        return;
    m_rotation = ImageTransform::normalizeRotation(degrees);
    m_document.setRotation(m_rotation);
    PreviewRenderer renderer;
    m_originalPixmap = QPixmap::fromImage(renderer.render(
        m_document.originalImage(), m_document.adjustments()));
    m_view->setPhoto(m_originalPixmap);
}

void MainWindow::toggleCrop()
{
    if (m_document.originalImage().isNull())
        return;
    if (m_cropOverlay->isVisible()) {
        cancelCrop();
        return;
    }
    m_cropOverlay->setGeometry(m_view->viewport()->rect());
    m_cropOverlay->resetSelection();
    m_cropOverlay->show();
    m_cropOverlay->raise();
    m_applyCropButton->setVisible(true);
    m_cancelCropButton->setVisible(true);
    m_cropRatio->setVisible(true);
}

void MainWindow::confirmCrop()
{
    if (!m_cropOverlay->isVisible())
        return;
    m_cropOverlay->hide();
    m_applyCropButton->setVisible(false);
    m_cancelCropButton->setVisible(false);
    m_cropRatio->setVisible(false);
}

void MainWindow::cancelCrop()
{
    if (m_cropOverlay->isVisible()) {
        m_document.setCropRect(QRectF(0.0, 0.0, 1.0, 1.0));
        PreviewRenderer renderer;
        m_originalPixmap = QPixmap::fromImage(renderer.render(
            m_document.originalImage(), m_document.adjustments()));
        m_view->setPhoto(m_originalPixmap);
    }
    m_cropOverlay->hide();
    m_applyCropButton->setVisible(false);
    m_cancelCropButton->setVisible(false);
    m_cropRatio->setVisible(false);
}

void MainWindow::copyImageToClipboard()
{
    const bool hasLibrarySelection = m_cacheBrowser && m_cacheBrowser->hasSelection();
    const QVector<ImageMetadata> selectedImages = m_cacheBrowser ? m_cacheBrowser->getSelectedImages()
                                                                  : QVector<ImageMetadata>();
    if (hasLibrarySelection) {
        if (selectedImages.isEmpty())
            return;
        QStringList selectedPaths;
        selectedPaths.reserve(selectedImages.size());
        for (const ImageMetadata &image : selectedImages)
            selectedPaths.append(image.path);
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
    if (m_document.originalImage().isNull())
        return;
    PreviewRenderer renderer;
    const QPixmap displayedPixmap = QPixmap::fromImage(renderer.render(
        m_document.originalImage(), m_document.adjustments()));
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
    QStringList targetPaths;
    if (m_cacheBrowser && m_cacheBrowser->hasSelection()) {
        const QVector<ImageMetadata> selectedImages = m_cacheBrowser->getSelectedImages();
        for (const ImageMetadata &image : selectedImages)
            targetPaths.append(image.path);
    } else if (!m_currentPath.isEmpty()) {
        targetPaths.append(m_currentPath);
    }
    targetPaths.removeDuplicates();
    if (targetPaths.isEmpty())
        return;
    if (targetPaths.size() > 1 &&
        QMessageBox::question(this, tr("Edit selected photo metadata?"),
                              tr("Apply the same shooting date, GPS location, author, and copyright to all %n selected photos?",
                                 nullptr, targetPaths.size()),
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
        return;

    const QString initialPath = targetPaths.constFirst();
    QDateTime initialDateTime = QDateTime::fromString(readImageShotDateTime(initialPath), Qt::ISODate);
    if (!initialDateTime.isValid())
        initialDateTime = QFileInfo(initialPath).lastModified();
    double initialLatitude = 0.0;
    double initialLongitude = 0.0;
    const bool hasInitialGpsPosition = readImageGpsPosition(initialPath, &initialLatitude, &initialLongitude);

    QDialog dialog(this);
    dialog.setWindowTitle(tr("Edit metadata"));
    auto *layout = new QVBoxLayout(&dialog);
    auto *form = new QFormLayout;
    auto *shotDateTime = new QDateTimeEdit(initialDateTime, &dialog);
    shotDateTime->setDisplayFormat(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    shotDateTime->setCalendarPopup(true);
    const auto addUpdateField = [&dialog, form](const QString &label, QWidget *field, bool checked = true) {
        auto *row = new QWidget(&dialog);
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        auto *enabled = new QCheckBox(tr("Update"), row);
        enabled->setChecked(checked);
        enabled->setToolTip(tr("When unchecked, keep each photo's existing value."));
        field->setEnabled(checked);
        rowLayout->addWidget(enabled);
        rowLayout->addWidget(field, 1);
        QObject::connect(enabled, &QCheckBox::toggled, field, &QWidget::setEnabled);
        form->addRow(label, row);
        return enabled;
    };
    auto *updateDateTime = addUpdateField(tr("Shot date/time"), shotDateTime);

    auto *latitude = new QDoubleSpinBox(&dialog);
    latitude->setRange(-90.0, 90.0);
    latitude->setDecimals(7);
    latitude->setSingleStep(0.0001);
    latitude->setValue(initialLatitude);
    auto *longitude = new QDoubleSpinBox(&dialog);
    longitude->setRange(-180.0, 180.0);
    longitude->setDecimals(7);
    longitude->setSingleStep(0.0001);
    longitude->setValue(initialLongitude);
    auto *removeGps = new QCheckBox(tr("Remove GPS location"), &dialog);
    auto *gpsEditor = new QWidget(&dialog);
    auto *gpsLayout = new QHBoxLayout(gpsEditor);
    gpsLayout->setContentsMargins(0, 0, 0, 0);
    gpsLayout->addWidget(new QLabel(tr("Latitude"), gpsEditor));
    gpsLayout->addWidget(latitude, 1);
    gpsLayout->addWidget(new QLabel(tr("Longitude"), gpsEditor));
    gpsLayout->addWidget(longitude, 1);
    gpsLayout->addWidget(removeGps);
    auto *updateGps = addUpdateField(tr("GPS location"), gpsEditor, hasInitialGpsPosition);
    connect(removeGps, &QCheckBox::toggled, &dialog, [latitude, longitude](bool remove) {
        latitude->setEnabled(!remove);
        longitude->setEnabled(!remove);
    });
    connect(updateGps, &QCheckBox::toggled, &dialog, [latitude, longitude, removeGps](bool enabled) {
        removeGps->setEnabled(enabled);
        latitude->setEnabled(enabled && !removeGps->isChecked());
        longitude->setEnabled(enabled && !removeGps->isChecked());
    });

    QSettings settings;
    auto makeSuggestionCombo = [&dialog](QStringList suggestions, const QString &currentValue) {
        auto *combo = new QComboBox(&dialog);
        combo->setEditable(true);
        combo->setInsertPolicy(QComboBox::NoInsert);
        suggestions.removeDuplicates();
        if (!currentValue.trimmed().isEmpty() && !suggestions.contains(currentValue))
            suggestions.prepend(currentValue);
        combo->addItems(suggestions);
        combo->setCurrentText(currentValue);
        return combo;
    };
    QStringList authorSuggestions = settings.value(QStringLiteral("metadata/authorSuggestions")).toStringList();
    if (!authorSuggestions.contains(QStringLiteral("Photographer")))
        authorSuggestions.append(tr("Photographer"));
    auto *author = makeSuggestionCombo(authorSuggestions, readImageAuthor(initialPath));
    auto *updateAuthor = addUpdateField(tr("Author"), author);

    QStringList copyrightSuggestions = settings.value(QStringLiteral("metadata/copyrightSuggestions")).toStringList();
    copyrightSuggestions << tr("All rights reserved") << QStringLiteral("CC BY 4.0")
                        << QStringLiteral("CC BY-SA 4.0");
    auto *copyright = makeSuggestionCombo(copyrightSuggestions, readImageCopyright(initialPath));
    auto *updateCopyright = addUpdateField(tr("Copyright"), copyright);
    layout->addLayout(form);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
    QPushButton *saveButton = buttons->button(QDialogButtonBox::Save);
    const auto updateSaveEnabled = [updateDateTime, updateAuthor, updateCopyright, updateGps, saveButton]() {
        saveButton->setEnabled(updateDateTime->isChecked() || updateAuthor->isChecked() ||
                                updateCopyright->isChecked() || updateGps->isChecked());
    };
    connect(updateDateTime, &QCheckBox::toggled, &dialog, updateSaveEnabled);
    connect(updateAuthor, &QCheckBox::toggled, &dialog, updateSaveEnabled);
    connect(updateCopyright, &QCheckBox::toggled, &dialog, updateSaveEnabled);
    connect(updateGps, &QCheckBox::toggled, &dialog, updateSaveEnabled);
    updateSaveEnabled();
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);
    if (dialog.exec() != QDialog::Accepted)
        return;

    const QString shootingDateTime = updateDateTime->isChecked()
        ? shotDateTime->dateTime().toString(Qt::ISODate) : QString();
    const bool gpsPositionEnabled = updateGps->isChecked();
    const bool hasGpsPosition = gpsPositionEnabled && !removeGps->isChecked();
    QString error;
    const QString authorValue = author->currentText().trimmed();
    const QString copyrightValue = copyright->currentText().trimmed();

    auto rememberSuggestion = [](const QString &key, const QString &value) {
        if (value.isEmpty())
            return;
        QSettings recentSettings;
        QStringList values = recentSettings.value(key).toStringList();
        values.removeAll(value);
        values.prepend(value);
        while (values.size() > 20)
            values.removeLast();
        recentSettings.setValue(key, values);
    };
    rememberSuggestion(QStringLiteral("metadata/authorSuggestions"), authorValue);
    rememberSuggestion(QStringLiteral("metadata/copyrightSuggestions"), copyrightValue);

    ImageCache &cache = ImageCache::instance();
    const bool cacheAvailable = cache.initialize();
    QStringList failures;
    int saved = 0;
    for (const QString &path : std::as_const(targetPaths)) {
        error.clear();
        if (!writeImageExifMetadata(path, shotDateTime->dateTime(), authorValue, copyrightValue,
                                    updateDateTime->isChecked(), updateAuthor->isChecked(),
                                    updateCopyright->isChecked(), gpsPositionEnabled, hasGpsPosition,
                                    latitude->value(), longitude->value(), &error)) {
            failures.append(tr("%1: %2").arg(path, error));
            continue;
        }
        ++saved;
        if (!cacheAvailable)
            continue;
        QFile file(path);
        QCryptographicHash hash(QCryptographicHash::Sha256);
        if (!file.open(QIODevice::ReadOnly)) {
            failures.append(tr("%1: metadata saved, but hash could not be refreshed (%2)").arg(path, file.errorString()));
            continue;
        }
        bool hashRead = true;
        while (!file.atEnd()) {
            const QByteArray chunk = file.read(1024 * 1024);
            if (chunk.isEmpty() && file.error() != QFileDevice::NoError) {
                hashRead = false;
                failures.append(tr("%1: metadata saved, but hash could not be refreshed (%2)").arg(path, file.errorString()));
                break;
            }
            hash.addData(chunk);
        }
        if (!hashRead)
            continue;
        const QString updatedAt = QFileInfo(path).lastModified().toString(Qt::ISODate);
        const bool dateUpdated = cache.updateImageShootingDateTime(path, shootingDateTime, updatedAt);
        QString cacheError = dateUpdated ? QString() : cache.errorString();
        const bool gpsUpdated = !gpsPositionEnabled ||
            cache.updateImageGpsPosition(path, hasGpsPosition, latitude->value(), longitude->value());
        if (!gpsUpdated && cacheError.isEmpty())
            cacheError = cache.errorString();
        const bool hashUpdated = cache.updateImageHash(path, hash.result());
        if (!hashUpdated && cacheError.isEmpty())
            cacheError = cache.errorString();
        if (!dateUpdated || !gpsUpdated || !hashUpdated)
            failures.append(tr("%1: metadata saved, but the library entry could not be updated (%2)")
                                .arg(path, cacheError));
    }
    if (!failures.isEmpty())
        QMessageBox::warning(this, saved == targetPaths.size() ? tr("Some library entries could not be updated")
                                                               : tr("Some photos could not be updated"),
                             failures.join(QLatin1Char('\n')));
    if (m_cacheBrowser) {
        m_cacheBrowser->clearSelection();
        if (m_cacheBrowser->isVisible())
            m_cacheBrowser->refresh();
    }
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
