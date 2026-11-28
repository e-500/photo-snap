#include "ImageTransform.h"

#include <QTransform>
#include <QtMath>
#include <algorithm>

namespace ImageTransform {

double normalizeRotation(double degrees)
{
    double normalized = std::remainder(degrees, 360.0);
    if (normalized <= -180.0)
        normalized += 360.0;
    if (normalized > 180.0)
        normalized -= 360.0;
    return normalized;
}

double addQuarterTurn(double degrees)
{
    return normalizeRotation(degrees + 90.0);
}

QImage rotate(const QImage &source, double degrees)
{
    if (source.isNull())
        return {};
    const double normalized = normalizeRotation(degrees);
    if (qFuzzyIsNull(normalized))
        return source.copy();
    return source.transformed(
        QTransform().rotate(normalized),
        Qt::SmoothTransformation);
}

QImage cropNormalized(const QImage &source, const QRectF &normalizedRect)
{
    if (source.isNull())
        return {};

    const QRectF valid = normalizedRect.intersected(QRectF(0.0, 0.0, 1.0, 1.0));
    if (valid.width() <= 0.0 || valid.height() <= 0.0)
        return source.copy();

    const QRect crop(
        qMax(0, qFloor(valid.left() * source.width())),
        qMax(0, qFloor(valid.top() * source.height())),
        qMin(source.width(), qCeil(valid.width() * source.width())),
        qMin(source.height(), qCeil(valid.height() * source.height())));
    return source.copy(crop.intersected(source.rect()));
}

}
