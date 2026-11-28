#pragma once

#include <QImage>
#include <QRectF>

namespace ImageTransform {

double normalizeRotation(double degrees);
double addQuarterTurn(double degrees);
QImage rotate(const QImage &source, double degrees);
QImage cropNormalized(const QImage &source, const QRectF &normalizedRect);

}
