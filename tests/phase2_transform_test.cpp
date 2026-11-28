#include "core/ImageTransform.h"

#include <QtTest>

class Phase2TransformTest final : public QObject
{
    Q_OBJECT

private slots:
    void quarterTurnsNormalize();
    void ninetyDegreeRotationKeepsOriginal();
    void centralCropIsDeterministic();
};

void Phase2TransformTest::quarterTurnsNormalize()
{
    QCOMPARE(ImageTransform::normalizeRotation(360.0), 0.0);
    QCOMPARE(ImageTransform::normalizeRotation(-270.0), 90.0);
    QCOMPARE(ImageTransform::normalizeRotation(270.0), -90.0);
    QCOMPARE(ImageTransform::addQuarterTurn(270.0), 0.0);
}

void Phase2TransformTest::ninetyDegreeRotationKeepsOriginal()
{
    QImage original(4, 2, QImage::Format_RGB32);
    original.fill(Qt::black);
    original.setPixelColor(0, 0, QColor(Qt::red));
    const QImage before = original.copy();

    const QImage rotated = ImageTransform::rotate(original, 90.0);

    QCOMPARE(rotated.width(), 2);
    QCOMPARE(rotated.height(), 4);
    QCOMPARE(original, before);
}

void Phase2TransformTest::centralCropIsDeterministic()
{
    QImage original(8, 6, QImage::Format_RGB32);
    original.fill(Qt::white);

    const QImage cropped = ImageTransform::cropNormalized(
        original, QRectF(0.25, 0.25, 0.5, 0.5));

    QCOMPARE(cropped.width(), 4);
    QCOMPARE(cropped.height(), 3);
    QCOMPARE(original.size(), QSize(8, 6));
}

QTEST_GUILESS_MAIN(Phase2TransformTest)
#include "phase2_transform_test.moc"
