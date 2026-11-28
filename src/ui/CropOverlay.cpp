#include "CropOverlay.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <algorithm>

namespace {
constexpr int HandleSize = 12;
}

CropOverlay::CropOverlay(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
    setMouseTracking(true);
    resetSelection();
}

void CropOverlay::resetSelection()
{
    m_selection = QRectF(0.1, 0.1, 0.8, 0.8);
    applyAspectRatio();
    update();
    emit cropChanged(normalizedSelection());
}

void CropOverlay::setAspectRatio(double ratio)
{
    m_aspectRatio = ratio;
    applyAspectRatio();
    update();
    emit cropChanged(normalizedSelection());
}

void CropOverlay::applyAspectRatio()
{
    if (m_aspectRatio <= 0.0)
        return;

    QRect rect = selectionPixels();
    rect.setWidth(qRound(rect.height() * m_aspectRatio));
    rect.moveLeft(qMax(0, qMin(width() - rect.width(), rect.left())));
    rect.moveTop(qMax(0, qMin(height() - rect.height(), rect.top())));
    rect.setWidth(qMin(width(), rect.width()));
    rect.setHeight(qMin(height(), rect.height()));
    m_selection = QRectF(
        qreal(rect.left()) / width(),
        qreal(rect.top()) / height(),
        qreal(rect.width()) / width(),
        qreal(rect.height()) / height());
}

QRect CropOverlay::selectionPixels() const
{
    return QRect(
        qRound(m_selection.left() * width()),
        qRound(m_selection.top() * height()),
        qRound(m_selection.width() * width()),
        qRound(m_selection.height() * height()));
}

QRectF CropOverlay::normalizedSelection() const
{
    return QRectF(
        m_selection.left(),
        m_selection.top(),
        m_selection.width(),
        m_selection.height());
}

CropOverlay::Handle CropOverlay::handleAt(const QPoint &point) const
{
    const QRect rect = selectionPixels();
    const QRect handles[] = {
        QRect(rect.topLeft() - QPoint(HandleSize / 2, HandleSize / 2), QSize(HandleSize, HandleSize)),
        QRect(rect.center().x() - HandleSize / 2, rect.top() - HandleSize / 2, HandleSize, HandleSize),
        QRect(rect.topRight() - QPoint(HandleSize / 2, HandleSize / 2), QSize(HandleSize, HandleSize)),
        QRect(rect.right() - HandleSize / 2, rect.center().y() - HandleSize / 2, HandleSize, HandleSize),
        QRect(rect.bottomRight() - QPoint(HandleSize / 2, HandleSize / 2), QSize(HandleSize, HandleSize)),
        QRect(rect.center().x() - HandleSize / 2, rect.bottom() - HandleSize / 2, HandleSize, HandleSize),
        QRect(rect.bottomLeft() - QPoint(HandleSize / 2, HandleSize / 2), QSize(HandleSize, HandleSize)),
        QRect(rect.left() - HandleSize / 2, rect.center().y() - HandleSize / 2, HandleSize, HandleSize)
    };
    const Handle handlesType[] = {
        Handle::TopLeft, Handle::Top, Handle::TopRight, Handle::Right,
        Handle::BottomRight, Handle::Bottom, Handle::BottomLeft, Handle::Left
    };
    for (int i = 0; i < 8; ++i) {
        if (handles[i].contains(point))
            return handlesType[i];
    }
    return rect.contains(point) ? Handle::Move : Handle::None;
}

void CropOverlay::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    const QRect selection = selectionPixels();
    painter.fillRect(QRect(0, 0, width(), height()), QColor(0, 0, 0, 96));
    painter.setCompositionMode(QPainter::CompositionMode_Clear);
    painter.fillRect(selection, Qt::transparent);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

    painter.setPen(QPen(QColor(255, 255, 255, 230), 1));
    painter.drawRect(selection);
    painter.setPen(QPen(QColor(255, 255, 255, 160), 1));
    for (int i = 1; i < 3; ++i) {
        painter.drawLine(selection.left() + selection.width() * i / 3, selection.top(),
                         selection.left() + selection.width() * i / 3, selection.bottom());
        painter.drawLine(selection.left(), selection.top() + selection.height() * i / 3,
                         selection.right(), selection.top() + selection.height() * i / 3);
    }
    painter.setBrush(QColor(255, 255, 255, 230));
    painter.setPen(Qt::NoPen);
    for (const QPoint &point : {
        selection.topLeft(), QPoint(selection.center().x(), selection.top()),
        selection.topRight(), QPoint(selection.right(), selection.center().y()),
        selection.bottomRight(), QPoint(selection.center().x(), selection.bottom()),
        selection.bottomLeft(), QPoint(selection.left(), selection.center().y())}) {
        painter.drawRect(QRect(point.x() - 4, point.y() - 4, 8, 8));
    }
}

void CropOverlay::mousePressEvent(QMouseEvent *event)
{
    m_activeHandle = handleAt(event->pos());
    m_dragOffset = event->pos() - selectionPixels().topLeft();
}

void CropOverlay::mouseMoveEvent(QMouseEvent *event)
{
    if (!(event->buttons() & Qt::LeftButton) || m_activeHandle == Handle::None)
        return;

    QRect rect = selectionPixels();
    if (m_activeHandle == Handle::Move) {
        const QPoint topLeft = event->pos() - m_dragOffset;
        rect.moveLeft(qMax(0, qMin(width() - rect.width(), topLeft.x())));
        rect.moveTop(qMax(0, qMin(height() - rect.height(), topLeft.y())));
    } else {
        const QPoint pos = event->pos();
        if (m_activeHandle == Handle::TopLeft || m_activeHandle == Handle::Left || m_activeHandle == Handle::BottomLeft)
            rect.setLeft(qBound(0, pos.x(), rect.right() - 40));
        if (m_activeHandle == Handle::TopLeft || m_activeHandle == Handle::Top || m_activeHandle == Handle::TopRight)
            rect.setTop(qBound(0, pos.y(), rect.bottom() - 40));
        if (m_activeHandle == Handle::TopRight || m_activeHandle == Handle::Right || m_activeHandle == Handle::BottomRight)
            rect.setRight(qBound(rect.left() + 40, pos.x(), width()));
        if (m_activeHandle == Handle::BottomLeft || m_activeHandle == Handle::Bottom || m_activeHandle == Handle::BottomRight)
            rect.setBottom(qBound(rect.top() + 40, pos.y(), height()));
    }

    m_selection = QRectF(
        qreal(rect.left()) / width(),
        qreal(rect.top()) / height(),
        qreal(rect.width()) / width(),
        qreal(rect.height()) / height());
    update();
    emit cropChanged(normalizedSelection());
}

void CropOverlay::mouseReleaseEvent(QMouseEvent *)
{
    m_activeHandle = Handle::None;
}

void CropOverlay::resizeEvent(QResizeEvent *)
{
    update();
}
