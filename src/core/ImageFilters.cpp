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
        && adjustments.saturation == 1.0
        && !adjustments.grayscale
        && !adjustments.sepia
        && adjustments.temperature == 0.0
        && adjustments.sharpness == 0.0)
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

            red += adjustments.temperature * 40.0;
            blue -= adjustments.temperature * 40.0;

            if (adjustments.grayscale) {
                const double gray = 0.2126 * red + 0.7152 * green + 0.0722 * blue;
                red = green = blue = gray;
            }
            if (adjustments.sepia) {
                const double originalRed = red;
                const double originalGreen = green;
                const double originalBlue = blue;
                red = originalRed * 0.393 + originalGreen * 0.769 + originalBlue * 0.189;
                green = originalRed * 0.349 + originalGreen * 0.686 + originalBlue * 0.168;
                blue = originalRed * 0.272 + originalGreen * 0.534 + originalBlue * 0.131;
            }

            line[x] = qRgba(clampByte(red), clampByte(green),
                            clampByte(blue), alpha);
        }
    }

    if (adjustments.sharpness != 0.0) {
        const QImage blurred = result.copy();
        const double amount = std::clamp(adjustments.sharpness, -1.0, 1.0);
        for (int y = 1; y + 1 < result.height(); ++y) {
            QRgb *line = reinterpret_cast<QRgb *>(result.scanLine(y));
            for (int x = 1; x + 1 < result.width(); ++x) {
                const QRgb center = blurred.pixel(x, y);
                const QRgb neighbors =
                    qRgb((qRed(blurred.pixel(x - 1, y)) + qRed(blurred.pixel(x + 1, y))
                          + qRed(blurred.pixel(x, y - 1)) + qRed(blurred.pixel(x, y + 1))) / 4,
                         (qGreen(blurred.pixel(x - 1, y)) + qGreen(blurred.pixel(x + 1, y))
                          + qGreen(blurred.pixel(x, y - 1)) + qGreen(blurred.pixel(x, y + 1))) / 4,
                         (qBlue(blurred.pixel(x - 1, y)) + qBlue(blurred.pixel(x + 1, y))
                          + qBlue(blurred.pixel(x, y - 1)) + qBlue(blurred.pixel(x, y + 1))) / 4);
                line[x] = qRgba(
                    clampByte(qRed(center) + amount * (qRed(center) - qRed(neighbors))),
                    clampByte(qGreen(center) + amount * (qGreen(center) - qGreen(neighbors))),
                    clampByte(qBlue(center) + amount * (qBlue(center) - qBlue(neighbors))),
                    qAlpha(center));
            }
        }
    }
    return result;
}

}
