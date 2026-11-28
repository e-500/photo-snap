#include "PreviewRenderer.h"

#include <QTransform>
#include <QtMath>

QImage PreviewRenderer::render(const QImage &original, const ImageAdjustments &adjustments) const
{
    if (original.isNull())
        return {};

    QImage result = original.copy();
    if (!qFuzzyIsNull(adjustments.rotation)) {
        result = result.transformed(
            QTransform().rotate(adjustments.rotation),
            Qt::SmoothTransformation);
    }

    if (adjustments.cropRect != QRectF(0.0, 0.0, 1.0, 1.0)) {
        const QRect crop(
            qRound(adjustments.cropRect.left() * result.width()),
            qRound(adjustments.cropRect.top() * result.height()),
            qRound(adjustments.cropRect.width() * result.width()),
            qRound(adjustments.cropRect.height() * result.height()));
        result = result.copy(crop.intersected(result.rect()));
    }

    return result;
}
