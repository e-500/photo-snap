#pragma once

#include <QRectF>

struct ImageAdjustments
{
    double rotation = 0.0;
    QRectF cropRect = QRectF(0.0, 0.0, 1.0, 1.0);
    double brightness = 0.0;
    double contrast = 1.0;
    double saturation = 1.0;
    bool grayscale = false;
    bool sepia = false;
    double temperature = 0.0;
    double sharpness = 0.0;

    bool isNeutral() const
    {
        return rotation == 0.0
            && cropRect == QRectF(0.0, 0.0, 1.0, 1.0)
            && brightness == 0.0
            && contrast == 1.0
            && saturation == 1.0
            && !grayscale
            && !sepia
            && temperature == 0.0
            && sharpness == 0.0;
    }
};
