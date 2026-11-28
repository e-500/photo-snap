#include "ImageDocument.h"

#include <QImageReader>

ImageDocument::ImageDocument(QObject *parent)
    : QObject(parent)
{
}

void ImageDocument::clear()
{
    m_originalImage = QImage();
    m_adjustments = ImageAdjustments();
    m_path.clear();
    emit documentChanged();
    emit adjustmentsChanged();
}

bool ImageDocument::load(const QString &path)
{
    QImageReader reader(path);
    reader.setAutoTransform(true);
    const QImage image = reader.read();
    if (!setImage(image, path))
        return false;
    return true;
}

bool ImageDocument::setImage(const QImage &image, const QString &path)
{
    if (image.isNull())
        return false;

    m_originalImage = image.copy();
    m_adjustments = ImageAdjustments();
    m_path = path;
    emit documentChanged();
    emit adjustmentsChanged();
    return true;
}

const QImage &ImageDocument::originalImage() const
{
    return m_originalImage;
}

const ImageAdjustments &ImageDocument::adjustments() const
{
    return m_adjustments;
}

const QString &ImageDocument::path() const
{
    return m_path;
}

bool ImageDocument::isModified() const
{
    return !m_originalImage.isNull() && !isAdjustmentNeutral();
}

bool ImageDocument::isAdjustmentNeutral() const
{
    return m_adjustments.isNeutral();
}

void ImageDocument::setRotation(double degrees)
{
    if (m_adjustments.rotation == degrees)
        return;
    m_adjustments.rotation = degrees;
    emit adjustmentsChanged();
}

void ImageDocument::setCropRect(const QRectF &cropRect)
{
    if (m_adjustments.cropRect == cropRect)
        return;
    m_adjustments.cropRect = cropRect;
    emit adjustmentsChanged();
}

void ImageDocument::setBrightness(double brightness)
{
    if (m_adjustments.brightness == brightness)
        return;
    m_adjustments.brightness = brightness;
    emit adjustmentsChanged();
}

void ImageDocument::setContrast(double contrast)
{
    if (m_adjustments.contrast == contrast)
        return;
    m_adjustments.contrast = contrast;
    emit adjustmentsChanged();
}

void ImageDocument::setSaturation(double saturation)
{
    if (m_adjustments.saturation == saturation)
        return;
    m_adjustments.saturation = saturation;
    emit adjustmentsChanged();
}

void ImageDocument::resetAdjustments()
{
    if (m_adjustments.isNeutral())
        return;
    m_adjustments = ImageAdjustments();
    emit adjustmentsChanged();
}

void ImageDocument::setGrayscale(bool enabled)
{
    if (m_adjustments.grayscale == enabled)
        return;
    m_adjustments.grayscale = enabled;
    emit adjustmentsChanged();
}

void ImageDocument::setSepia(bool enabled)
{
    if (m_adjustments.sepia == enabled)
        return;
    m_adjustments.sepia = enabled;
    emit adjustmentsChanged();
}

void ImageDocument::setTemperature(double temperature)
{
    if (m_adjustments.temperature == temperature)
        return;
    m_adjustments.temperature = temperature;
    emit adjustmentsChanged();
}

void ImageDocument::setSharpness(double sharpness)
{
    if (m_adjustments.sharpness == sharpness)
        return;
    m_adjustments.sharpness = sharpness;
    emit adjustmentsChanged();
}
