#include "core/ImageExporter.h"

#include <QTemporaryDir>
#include <QImageWriter>
#include <QtTest>

class Phase6ExportTest final : public QObject
{
    Q_OBJECT

private slots:
    void exportsJpeg();
    void exportsPng();
    void exportsTiff();
    void rejectsSourceOverwrite();
    void neutralExportMatchesOriginal();
    void combinedTransformDoesNotModifyOriginal();
};

void Phase6ExportTest::exportsJpeg()
{
    if (!QImageWriter::supportedImageFormats().contains("jpg")
        && !QImageWriter::supportedImageFormats().contains("jpeg"))
        QSKIP("JPEG writer plugin is not available.");

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QImage source(4, 3, QImage::Format_RGB32);
    source.fill(QColor(180, 140, 100));
    const QString sourcePath = directory.filePath(QStringLiteral("source.png"));
    QVERIFY(source.save(sourcePath, "PNG"));
    const QString target = directory.filePath(QStringLiteral("export.jpg"));

    ImageExporter exporter;
    QString error;
    QVERIFY2(exporter.exportImage(source, ImageAdjustments(), sourcePath, target, 92, &error),
             error.toUtf8().constData());
    QVERIFY(!QImage(target).isNull());
}

void Phase6ExportTest::exportsPng()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QImage source(4, 3, QImage::Format_ARGB32);
    source.fill(QColor(100, 120, 140, 200));
    const QString sourcePath = directory.filePath(QStringLiteral("source.png"));
    QVERIFY(source.save(sourcePath, "PNG"));
    const QString target = directory.filePath(QStringLiteral("export.png"));

    ImageExporter exporter;
    QString error;
    QVERIFY2(exporter.exportImage(source, ImageAdjustments(), sourcePath, target, 92, &error),
             error.toUtf8().constData());
    QVERIFY(QImage(target).size() == source.size());
}

void Phase6ExportTest::exportsTiff()
{
    if (!QImageWriter::supportedImageFormats().contains("tiff"))
        QSKIP("TIFF writer plugin is not available.");
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QImage source(3, 5, QImage::Format_ARGB32);
    source.fill(Qt::blue);
    const QString sourcePath = directory.filePath(QStringLiteral("source.png"));
    QVERIFY(source.save(sourcePath, "PNG"));
    const QString target = directory.filePath(QStringLiteral("export.tiff"));

    ImageExporter exporter;
    QString error;
    QVERIFY2(exporter.exportImage(source, ImageAdjustments(), sourcePath, target, 92, &error),
             error.toUtf8().constData());
    QImage exported(target);
    QVERIFY(!exported.isNull());
    QCOMPARE(exported.size(), source.size());
}

void Phase6ExportTest::rejectsSourceOverwrite()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QImage source(3, 3, QImage::Format_ARGB32);
    source.fill(Qt::green);
    const QString sourcePath = directory.filePath(QStringLiteral("source.png"));
    QVERIFY(source.save(sourcePath, "PNG"));

    ImageExporter exporter;
    QString error;
    QVERIFY(!exporter.exportImage(source, ImageAdjustments(), sourcePath, sourcePath, 92, &error));
    QVERIFY(!error.isEmpty());
}

void Phase6ExportTest::neutralExportMatchesOriginal()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QImage source(4, 2, QImage::Format_ARGB32);
    source.fill(QColor(25, 50, 75, 255));
    const QString sourcePath = directory.filePath(QStringLiteral("source.png"));
    const QString target = directory.filePath(QStringLiteral("neutral.png"));

    ImageExporter exporter;
    QString error;
    QVERIFY(exporter.exportImage(source, ImageAdjustments(), sourcePath, target, 92, &error));

    const QImage exported(target);
    QCOMPARE(exported.size(), source.size());
    QCOMPARE(exported.pixelColor(0, 0), source.pixelColor(0, 0));
}

void Phase6ExportTest::combinedTransformDoesNotModifyOriginal()
{
    QImage source(8, 6, QImage::Format_ARGB32);
    source.fill(QColor(80, 100, 120, 255));
    const QImage before = source.copy();

    ImageAdjustments adjustments;
    adjustments.rotation = 90.0;
    adjustments.cropRect = QRectF(0.25, 0.25, 0.5, 0.5);
    adjustments.brightness = 0.10;

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString sourcePath = directory.filePath(QStringLiteral("source.png"));
    const QString target = directory.filePath(QStringLiteral("combined.png"));
    ImageExporter exporter;
    QString error;
    QVERIFY(exporter.exportImage(source, adjustments, sourcePath, target, 92, &error));
    QCOMPARE(source, before);
}

QTEST_GUILESS_MAIN(Phase6ExportTest)
#include "phase6_export_test.moc"
