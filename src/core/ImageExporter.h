#pragma once

#include "ImageAdjustments.h"

#include <QImage>
#include <QString>

class ImageExporter
{
public:
    bool exportImage(const QImage &original,
                     const ImageAdjustments &adjustments,
                     const QString &sourcePath,
                     const QString &targetPath,
                     int jpegQuality,
                     QString *errorString = nullptr);
};
