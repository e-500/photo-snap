#include "ImageExporter.h"

#include "PreviewRenderer.h"

#include <QDir>
#include <QFileInfo>
#include <QImageWriter>

bool ImageExporter::exportImage(const QImage &original,
                                const ImageAdjustments &adjustments,
                                const QString &sourcePath,
                                const QString &targetPath,
                                int jpegQuality,
                                QString *errorString)
{
    if (original.isNull()) {
        if (errorString)
            *errorString = QStringLiteral("No image is loaded.");
        return false;
    }
    if (targetPath.trimmed().isEmpty()) {
        if (errorString)
            *errorString = QStringLiteral("No export path was selected.");
        return false;
    }

    QFileInfo source(sourcePath);
    if (!source.isAbsolute() && !sourcePath.isEmpty())
        source = QFileInfo(QDir::current().filePath(sourcePath));
    QFileInfo target(targetPath);
    if (!target.isAbsolute())
        target = QFileInfo(QDir::current().filePath(targetPath));

    const QString sourceAbsolute = !source.canonicalFilePath().isEmpty()
        ? QFileInfo(source.canonicalFilePath()).absoluteFilePath()
        : source.absoluteFilePath();
    const QString targetAbsolute = target.exists() && !target.canonicalFilePath().isEmpty()
        ? QFileInfo(target.canonicalFilePath()).absoluteFilePath()
        : target.absoluteFilePath();
    if (!sourcePath.isEmpty() && sourceAbsolute == targetAbsolute) {
        if (errorString)
            *errorString = QStringLiteral("The source image cannot be overwritten.");
        return false;
    }

    PreviewRenderer renderer;
    QImage rendered = renderer.render(original, adjustments);
    if (rendered.isNull()) {
        if (errorString)
            *errorString = QStringLiteral("The image could not be rendered.");
        return false;
    }
    for (const QString &key : original.textKeys())
        rendered.setText(key, original.text(key));

    QImageWriter writer(target.absoluteFilePath());
    const QByteArray suffix = target.suffix().toLower().toUtf8();
    if (!suffix.isEmpty())
        writer.setFormat(suffix);
    if (suffix == "jpg" || suffix == "jpeg")
        writer.setQuality(qBound(1, jpegQuality, 100));

    if (!writer.write(rendered)) {
        if (errorString)
            *errorString = writer.errorString();
        return false;
    }
    return true;
}
