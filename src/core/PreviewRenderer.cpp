#include "PreviewRenderer.h"

#include "ImageTransform.h"

QImage PreviewRenderer::render(const QImage &original, const ImageAdjustments &adjustments) const
{
    if (original.isNull())
        return {};

    QImage result = ImageTransform::rotate(original, adjustments.rotation);
    result = ImageTransform::cropNormalized(result, adjustments.cropRect);

    return result;
}
