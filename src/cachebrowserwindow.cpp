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

#include "cachebrowserwindow.h"

#include "imagecache.h"
#include "imagelibrarywidget.h"

#include <QGridLayout>
#include <QFrame>
#include <QEvent>
#include <QCursor>
#include <QCoreApplication>
#include <QDialog>
#include <QDir>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QLabel>
#include <QHBoxLayout>
#include <QLocale>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QProcess>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QSizePolicy>
#include <QSizePolicy>
#include <QShortcut>
#include <QKeySequence>
#include <QShowEvent>
#include <QSizePolicy>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

#include <QTimer>
#include <QTextCursor>

#include <functional>
#include <utility>

namespace {
class OverlayScrollArea final : public QScrollArea
{
public:
    explicit OverlayScrollArea(QWidget *parent = nullptr)
        : QScrollArea(parent), m_overlayBar(new QScrollBar(Qt::Vertical, viewport()))
    {
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        viewport()->setMouseTracking(true);
        viewport()->installEventFilter(this);
        m_overlayBar->setMouseTracking(true);
        m_overlayBar->installEventFilter(this);
        m_overlayBar->setStyleSheet(QStringLiteral(
            "QScrollBar:vertical { background: rgba(16, 18, 22, 150); width: 12px; margin: 2px; border-radius: 6px; }"
            "QScrollBar::handle:vertical { background: rgba(210, 215, 225, 185); min-height: 26px; border-radius: 5px; }"
            "QScrollBar::handle:vertical:hover { background: rgba(240, 242, 248, 225); }"
            "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
            "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }"));
        m_overlayBar->show();

        QScrollBar *nativeBar = verticalScrollBar();
        m_overlayBar->setRange(nativeBar->minimum(), nativeBar->maximum());
        m_overlayBar->setPageStep(nativeBar->pageStep());
        connect(nativeBar, &QScrollBar::rangeChanged, this, [this](int minimum, int maximum) {
            m_overlayBar->setRange(minimum, maximum);
            m_overlayBar->show();
        });
        connect(nativeBar, &QScrollBar::valueChanged, m_overlayBar, &QScrollBar::setValue);
        connect(m_overlayBar, &QScrollBar::valueChanged, nativeBar, &QScrollBar::setValue);
    }

protected:
    void resizeEvent(QResizeEvent *event) override
    {
        QScrollArea::resizeEvent(event);
        positionOverlayBar();
    }

    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (watched == viewport() && event->type() == QEvent::Resize)
            positionOverlayBar();
        return QScrollArea::eventFilter(watched, event);
    }

private:
    void positionOverlayBar()
    {
        if (!m_overlayBar)
            return;
        constexpr int barWidth = 12;
        m_overlayBar->setGeometry(viewport()->width() - barWidth, 0, barWidth, viewport()->height());
        m_overlayBar->setPageStep(verticalScrollBar()->pageStep());
        m_overlayBar->show();
        m_overlayBar->raise();
    }

    QScrollBar *m_overlayBar = nullptr;
};

} // namespace

struct CacheBrowserWindow::YearPane
{
    QString year;
    QWidget *container = nullptr;
    QToolButton *toggle = nullptr;
    QWidget *content = nullptr;
    QVBoxLayout *contentLayout = nullptr;
    QVector<MonthPane *> months;
    bool expanded = false;
};

struct CacheBrowserWindow::MonthPane
{
    QString year;
    QString month;
    QWidget *container = nullptr;
    QToolButton *toggle = nullptr;
    QWidget *content = nullptr;
    QVBoxLayout *contentLayout = nullptr;
    ImageLibraryWidget *library = nullptr;
    bool expanded = false;
};

CacheBrowserWindow::CacheBrowserWindow(ImageCache *cache, QWidget *parent)
    : QDockWidget(parent), m_cache(cache)
{
    setWindowTitle(tr("Library"));
    setMinimumSize(320, 240);
    setAllowedAreas(Qt::AllDockWidgetAreas);
    setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);

    auto *titleBar = new QWidget(this);
    auto *titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(8, 2, 4, 2);
    auto *title = new QLabel(tr("Library"), titleBar);
    m_dateModeButton = new QToolButton(titleBar);
    m_dateModeButton->setAutoRaise(true);
    m_dateModeButton->setCheckable(true);
    m_dateModeButton->setText(tr("By shooting date"));
    m_scanButton = new QToolButton(titleBar);
    m_scanButton->setAutoRaise(true);
    m_scanButton->setText(tr("Scan folder"));
    auto *closeButton = new QToolButton(titleBar);
    closeButton->setAutoRaise(true);
    closeButton->setText(QStringLiteral("×"));
    closeButton->setToolTip(tr("Close cached photos"));
    titleLayout->addWidget(title);
    titleLayout->addWidget(m_dateModeButton);
    titleLayout->addStretch(1);
    titleLayout->addWidget(m_scanButton);
    titleLayout->addWidget(closeButton);
    setTitleBarWidget(titleBar);
    connect(m_scanButton, &QToolButton::clicked, this, &CacheBrowserWindow::startScan);
    connect(m_dateModeButton, &QToolButton::toggled, this, [this](bool creationDate) {
        m_dateMode = creationDate ? ImageDateMode::CreationDate : ImageDateMode::ShootingDate;
        m_dateModeButton->setText(creationDate ? tr("By creation date") : tr("By shooting date"));
        refresh();
    });
    connect(closeButton, &QToolButton::clicked, this, &QDockWidget::hide);

    auto *content = new QWidget(this);
    auto *layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 0, 0, 0);
    m_scrollArea = new OverlayScrollArea(content);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollPrefetchTimer = new QTimer(this);
    m_scrollPrefetchTimer->setSingleShot(true);
    m_scrollPrefetchTimer->setInterval(35);
    connect(m_scrollPrefetchTimer, &QTimer::timeout, this, &CacheBrowserWindow::checkScrollPrefetch);
    connect(m_scrollArea->verticalScrollBar(), &QScrollBar::valueChanged,
            this, [this]() { m_scrollPrefetchTimer->start(); });

    m_yearsWidget = new QWidget(m_scrollArea);
    m_yearsWidget->setMinimumWidth(0);
    m_yearsWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    m_yearsLayout = new QVBoxLayout(m_yearsWidget);
    m_yearsLayout->setContentsMargins(8, 8, 8, 8);
    m_yearsLayout->setSpacing(6);
    m_yearsLayout->setSizeConstraint(QLayout::SetNoConstraint);
    m_emptyLabel = new QLabel(tr("No cached photos yet"), m_yearsWidget);
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setMinimumSize(300, 180);
    m_yearsLayout->addWidget(m_emptyLabel);
    m_scrollArea->setWidget(m_yearsWidget);
    layout->addWidget(m_scrollArea);
    setWidget(content);

    m_scannerProcess = new QProcess(this);
    m_scanLogDialog = new QDialog(this);
    m_scanLogDialog->setWindowTitle(tr("Photo scan log"));
    m_scanLogDialog->setModal(false);
    m_scanLogDialog->resize(720, 420);
    auto *scanLogLayout = new QVBoxLayout(m_scanLogDialog);
    m_scanLog = new QPlainTextEdit(m_scanLogDialog);
    m_scanLog->setReadOnly(true);
    m_scanLog->setMaximumBlockCount(5000);
    auto *scanControls = new QHBoxLayout;
    m_pauseResumeButton = new QPushButton(tr("Pause"), m_scanLogDialog);
    m_stopScanButton = new QPushButton(tr("Stop"), m_scanLogDialog);
    m_pauseResumeButton->setEnabled(false);
    m_stopScanButton->setEnabled(false);
    scanControls->addStretch(1);
    scanControls->addWidget(m_pauseResumeButton);
    scanControls->addWidget(m_stopScanButton);
    scanLogLayout->addWidget(m_scanLog);
    scanLogLayout->addLayout(scanControls);
    connect(m_pauseResumeButton, &QPushButton::clicked, this, [this]() {
        if (!m_scannerProcess || m_scannerProcess->state() == QProcess::NotRunning)
            return;
        m_scanPaused = !m_scanPaused;
        m_scannerProcess->write(m_scanPaused ? "pause\n" : "resume\n");
        m_pauseResumeButton->setText(m_scanPaused ? tr("Resume") : tr("Pause"));
    });
    connect(m_stopScanButton, &QPushButton::clicked, this, [this]() {
        if (!m_scannerProcess || m_scannerProcess->state() == QProcess::NotRunning)
            return;
        m_scannerProcess->write("stop\n");
        m_stopScanButton->setEnabled(false);
        m_pauseResumeButton->setEnabled(false);
        m_scanLog->appendPlainText(tr("Stop requested; waiting for the current image to finish…"));
    });
    connect(m_scannerProcess, &QProcess::readyReadStandardOutput, this, [this]() {
        appendScanOutput(QString::fromUtf8(m_scannerProcess->readAllStandardOutput()));
    });
    connect(m_scannerProcess, &QProcess::readyReadStandardError, this, [this]() {
        const QString output = QString::fromUtf8(m_scannerProcess->readAllStandardError());
        m_scanLog->moveCursor(QTextCursor::End);
        m_scanLog->insertPlainText(output);
    });
    connect(m_scannerProcess, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
            [this](int exitCode, QProcess::ExitStatus exitStatus) {
        const QString output = QString::fromUtf8(m_scannerProcess->readAllStandardOutput()).trimmed();
        const QString error = QString::fromUtf8(m_scannerProcess->readAllStandardError()).trimmed();
        if (!output.isEmpty())
            appendScanOutput(output + QLatin1Char('\n'));
        if (!error.isEmpty())
            m_scanLog->appendPlainText(error);
        m_scanButton->setEnabled(true);
        m_scanButton->setText(tr("Scan folder"));
        m_pauseResumeButton->setEnabled(false);
        m_pauseResumeButton->setText(tr("Pause"));
        m_stopScanButton->setEnabled(false);
        m_scanPaused = false;
        refresh();
        if (exitStatus != QProcess::NormalExit || exitCode != 0) {
            m_scanLog->appendPlainText(tr("Scanner exited with code %1.").arg(exitCode));
        }
    });
    connect(m_scannerProcess, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart)
            return;
        m_scanButton->setEnabled(true);
        m_scanButton->setText(tr("Scan folder"));
        m_pauseResumeButton->setEnabled(false);
        m_stopScanButton->setEnabled(false);
        m_scanLog->appendPlainText(tr("Could not start scanner: %1").arg(m_scannerProcess->errorString()));
        m_scanLogDialog->show();
    });

}

void CacheBrowserWindow::startScan()
{
    if (!m_scannerProcess || m_scannerProcess->state() != QProcess::NotRunning)
        return;

    const QString folder = QFileDialog::getExistingDirectory(this, tr("Choose a folder to scan"), QDir::homePath());
    if (folder.isEmpty())
        return;

    const auto restoreHidden = QMessageBox::question(
        this, tr("Restore hidden cached photos?"),
        tr("If a cached photo is still present in this folder, should it be made visible in the cache browser?"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes;

#ifdef Q_OS_WIN
    const QString scannerName = QStringLiteral("photo-snap-cache-scanner.exe");
#else
    const QString scannerName = QStringLiteral("photo-snap-cache-scanner");
#endif
    QDir applicationDirectory(QCoreApplication::applicationDirPath());
#if defined(Q_OS_MACOS) || defined(Q_OS_MAC)
    const QString scannerPath = applicationDirectory.absoluteFilePath(QStringLiteral("../../../bin/%1").arg(scannerName));
#else
    const QString scannerPath = applicationDirectory.filePath(scannerName);
#endif
    if (!QFileInfo::exists(scannerPath)) {
        QMessageBox::warning(this, tr("Scanner not found"),
                             tr("Could not find the scanner application:\n%1").arg(scannerPath));
        return;
    }

    m_scanButton->setEnabled(false);
    m_scanButton->setText(tr("Scanning…"));
    m_scanPaused = false;
    m_scanLog->clear();
    m_scanOutputBuffer.clear();
    m_scanLogDialog->setWindowTitle(tr("Photo scan log — 0.0 %"));
    m_scanLog->appendPlainText(tr("Scanning folder: %1").arg(folder));
    m_pauseResumeButton->setText(tr("Pause"));
    m_pauseResumeButton->setEnabled(true);
    m_stopScanButton->setEnabled(true);
    m_scanLogDialog->show();
    m_scanLogDialog->raise();
    QStringList arguments{folder};
    if (restoreHidden)
        arguments.append(QStringLiteral("--unhide-existing"));
    m_scannerProcess->start(scannerPath, arguments);
}

void CacheBrowserWindow::appendScanOutput(const QString &output)
{
    m_scanOutputBuffer += output;
    qsizetype newline = -1;
    while ((newline = m_scanOutputBuffer.indexOf(QLatin1Char('\n'))) >= 0) {
        QString line = m_scanOutputBuffer.left(newline);
        m_scanOutputBuffer.remove(0, newline + 1);
        if (line.endsWith(QLatin1Char('\r')))
            line.chop(1);
        if (line.startsWith(QStringLiteral("Progress: ")) && line.endsWith(QLatin1Char('%'))) {
            bool valid = false;
            const double progress = line.mid(10, line.size() - 11).toDouble(&valid);
            if (valid)
                m_scanLogDialog->setWindowTitle(tr("Photo scan log — %1 %")
                                                    .arg(QString::number(progress, 'f', 1)));
        } else if (!line.isEmpty()) {
            m_scanLog->appendPlainText(line);
        }
    }
}

void CacheBrowserWindow::refresh()
{
    QStringList expandedYears;
    m_monthsToRestore.clear();
    for (YearPane *pane : std::as_const(m_yearPanes)) {
        if (pane->expanded) {
            expandedYears.append(pane->year);
            for (MonthPane *monthPane : std::as_const(pane->months)) {
                if (monthPane->expanded)
                    m_monthsToRestore.append(pane->year + QLatin1Char('/') + monthPane->month);
            }
        }
        for (MonthPane *monthPane : std::as_const(pane->months))
            delete monthPane;
        delete pane;
    }
    m_yearPanes.clear();
    while (QLayoutItem *item = m_yearsLayout->takeAt(0)) {
        if (item->widget() && item->widget() != m_emptyLabel)
            delete item->widget();
        delete item;
    }

    const QStringList years = m_cache ? m_cache->cachedImageYearsOrdered(m_dateMode) : QStringList();
    if (years.isEmpty()) {
        m_emptyLabel->setVisible(true);
        m_yearsLayout->addWidget(m_emptyLabel);
        m_monthsToRestore.clear();
        emit selectionChanged();
        return;
    }
    m_emptyLabel->hide();

    for (const QString &year : years) {
        auto *pane = new YearPane;
        pane->year = year;
        pane->container = new QWidget(m_yearsWidget);
        pane->container->setMinimumWidth(0);
        pane->container->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        auto *sectionLayout = new QVBoxLayout(pane->container);
        sectionLayout->setContentsMargins(0, 0, 0, 0);
        sectionLayout->setSpacing(0);
        sectionLayout->setSizeConstraint(QLayout::SetNoConstraint);
        pane->toggle = new QToolButton(pane->container);
        pane->toggle->setCheckable(true);
        pane->toggle->setAutoRaise(true);
        pane->toggle->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        pane->toggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        pane->toggle->setArrowType(Qt::RightArrow);
        pane->toggle->setText(year == QStringLiteral("Undated") ? tr("Undated") : year);
        pane->toggle->setStyleSheet(QStringLiteral(
            "QToolButton { text-align: left; font-weight: 600; padding: 2px 4px; background: #282c33; border-radius: 3px; }"
            "QToolButton:hover { background: #343942; }"));
        pane->content = new QWidget(pane->container);
        pane->content->setMinimumWidth(0);
        pane->content->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        pane->contentLayout = new QVBoxLayout(pane->content);
        pane->contentLayout->setContentsMargins(0, 6, 0, 0);
        pane->contentLayout->setSpacing(6);
        pane->contentLayout->setSizeConstraint(QLayout::SetNoConstraint);
        pane->content->hide();
        sectionLayout->addWidget(pane->toggle);
        sectionLayout->addWidget(pane->content);
        m_yearsLayout->addWidget(pane->container);
        m_yearPanes.append(pane);
        connect(pane->toggle, &QToolButton::toggled, this,
                [this, pane](bool expanded) { setYearExpanded(pane, expanded); });
        if (expandedYears.contains(year))
            pane->toggle->setChecked(true);
    }
    m_yearsLayout->addStretch(1);
    m_monthsToRestore.clear();
    m_scrollPrefetchTimer->start();
    emit selectionChanged();
}

QStringList CacheBrowserWindow::selectedPaths() const
{
    QStringList paths;
    for (const YearPane *pane : m_yearPanes) {
        if (!pane)
            continue;
        for (const MonthPane *monthPane : pane->months) {
            if (monthPane && monthPane->library)
                paths.append(monthPane->library->selectedPaths());
        }
    }
    paths.removeDuplicates();
    paths.sort(Qt::CaseInsensitive);
    return paths;
}

void CacheBrowserWindow::clearSelection()
{
    for (YearPane *pane : std::as_const(m_yearPanes)) {
        if (!pane)
            continue;
        for (MonthPane *monthPane : std::as_const(pane->months)) {
            if (monthPane && monthPane->library)
                monthPane->library->clearSelection();
        }
    }
}

void CacheBrowserWindow::setYearExpanded(YearPane *pane, bool expanded)
{
    if (!pane)
        return;
    pane->expanded = expanded;
    pane->toggle->setArrowType(expanded ? Qt::DownArrow : Qt::RightArrow);
    pane->content->setVisible(expanded);
    if (expanded && pane->months.isEmpty()) {
        const QStringList months = m_cache ? m_cache->cachedImageMonthsOrdered(pane->year, m_dateMode) : QStringList();
        for (const QString &month : months) {
            auto *monthPane = new MonthPane;
            monthPane->year = pane->year;
            monthPane->month = month;
            monthPane->container = new QWidget(pane->content);
            monthPane->container->setMinimumWidth(0);
            monthPane->container->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
            auto *monthLayout = new QVBoxLayout(monthPane->container);
            monthLayout->setContentsMargins(8, 0, 0, 0);
            monthLayout->setSpacing(0);
            monthLayout->setSizeConstraint(QLayout::SetNoConstraint);
            monthPane->toggle = new QToolButton(monthPane->container);
            monthPane->toggle->setCheckable(true);
            monthPane->toggle->setAutoRaise(true);
            monthPane->toggle->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            monthPane->toggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
            monthPane->toggle->setArrowType(Qt::RightArrow);
            const int monthNumber = month.toInt();
            monthPane->toggle->setText(month == QStringLiteral("Undated")
                                           ? tr("Undated")
                                           : QLocale().monthName(monthNumber, QLocale::LongFormat));
            monthPane->toggle->setStyleSheet(QStringLiteral(
                "QToolButton { text-align: left; padding: 2px 4px; background: #24282e; border-radius: 3px; }"
                "QToolButton:hover { background: #30353d; }"));
            monthPane->content = new QWidget(monthPane->container);
            monthPane->content->setMinimumWidth(0);
            monthPane->content->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
            monthPane->contentLayout = new QVBoxLayout(monthPane->content);
            monthPane->contentLayout->setContentsMargins(0, 0, 0, 0);
            monthPane->contentLayout->setSizeConstraint(QLayout::SetNoConstraint);
            monthPane->content->hide();
            monthLayout->addWidget(monthPane->toggle);
            monthLayout->addWidget(monthPane->content);
            pane->contentLayout->addWidget(monthPane->container);
            pane->months.append(monthPane);
            connect(monthPane->toggle, &QToolButton::toggled, this,
                    [this, monthPane](bool monthExpanded) { setMonthExpanded(monthPane, monthExpanded); });
            if (m_monthsToRestore.contains(pane->year + QLatin1Char('/') + month))
                monthPane->toggle->setChecked(true);
        }
    }
    if (m_scrollPrefetchTimer)
        m_scrollPrefetchTimer->start();
}

void CacheBrowserWindow::setMonthExpanded(MonthPane *pane, bool expanded)
{
    if (!pane)
        return;
    pane->expanded = expanded;
    pane->toggle->setArrowType(expanded ? Qt::DownArrow : Qt::RightArrow);
    pane->content->setVisible(expanded);
    if (expanded && !pane->library) {
        pane->library = new ImageLibraryWidget(m_cache, pane->year, pane->month, m_dateMode,
                                               m_scrollArea, pane->content);
        pane->contentLayout->addWidget(pane->library);
        connect(pane->library, &ImageLibraryWidget::openImageRequested, this, [this](const QString &path) {
            clearSelection();
            emit openImageRequested(path);
        });
        connect(pane->library, &ImageLibraryWidget::selectionChanged,
                this, &CacheBrowserWindow::selectionChanged);
        connect(pane->library, &ImageLibraryWidget::refreshRequested,
                this, &CacheBrowserWindow::refresh, Qt::QueuedConnection);
        pane->library->refresh();
    }
    if (m_scrollPrefetchTimer)
        m_scrollPrefetchTimer->start();
}

void CacheBrowserWindow::checkScrollPrefetch()
{
    for (YearPane *pane : std::as_const(m_yearPanes)) {
        if (!pane->expanded)
            continue;
        for (MonthPane *monthPane : std::as_const(pane->months)) {
            if (monthPane->expanded && monthPane->library)
                monthPane->library->checkScrollPrefetch();
        }
    }
}
void CacheBrowserWindow::resizeEvent(QResizeEvent *event)
{
    QDockWidget::resizeEvent(event);
    if (m_scrollPrefetchTimer)
        m_scrollPrefetchTimer->start();
}

void CacheBrowserWindow::showEvent(QShowEvent *event)
{
    QDockWidget::showEvent(event);
    QTimer::singleShot(0, this, &CacheBrowserWindow::checkScrollPrefetch);
}
