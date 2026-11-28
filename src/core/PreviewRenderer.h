#pragma once

#include "ImageAdjustments.h"

#include <QImage>

class PreviewRenderer final
{
public:
    QImage render(const QImage &original, const ImageAdjustments &adjustments) const;
};
