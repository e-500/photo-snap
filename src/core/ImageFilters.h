#pragma once

#include "ImageAdjustments.h"

#include <QImage>

namespace ImageFilters {

QImage apply(const QImage &source, const ImageAdjustments &adjustments);

}
