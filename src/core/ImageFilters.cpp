#include "ImageFilters.h"

#include <algorithm>
#include <cmath>

namespace {

int clampByte(double value)
{
    return std::clamp(static_cast<int>(std::lround(value)), 0, 255);
}

}

namespace ImageFilters {

QImage apply(const QImage &source, const ImageAdjustments &adjustments)
{
    if (source.isNull())
        return {};
    if (adjustments.brightness == 0.0
        && adjustments.contrast == 1.0
        && adjustments.saturation == 1.0)
        return source.copy();

    QImage result = source.convertToFormat(QImage::Format_ARGB32);
    for (int y = 0; y < result.height(); ++y) {
        QRgb *line = reinterpret_cast<QRgb *>(result.scanLine(y));
        for (int x = 0; x < result.width(); ++x) {
            const QRgb pixel = line[x];
            const int alpha = qAlpha(pixel);
            double red = qRed(pixel);
            double green = qGreen(pixel);
            double blue = qBlue(pixel);

            red += adjustments.brightness * 255.0;
            green += adjustments.brightness * 255.0;
            blue += adjustments.brightness * 255.0;

            red = ((red - 127.5) * adjustments.contrast) + 127.5;
            green = ((green - 127.5) * adjustments.contrast) + 127.5;
            blue = ((blue - 127.5) * adjustments.contrast) + 127.5;

            const double luminance = 0.2126 * red + 0.7152 * green + 0.0722 * blue;
            red = luminance + (red - luminance) * adjustments.saturation;
            green = luminance + (green - luminance) * adjustments.saturation;
            blue = luminance + (blue - luminance) * adjustments.saturation;

            line[x] = qRgba(clampByte(red), clampByte(green),
                            clampByte(blue), alpha);
        }
    }
    return result;
}

}
