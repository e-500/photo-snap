#include "core/ImageFilters.h"

#include <QtTest>

class Phase4FilterTest final : public QObject
{
    Q_OBJECT

private slots:
    void neutralFiltersPreserveImage();
    void grayscaleProducesNeutralChannels();
    void sepiaIsDeterministic();
    void temperaturePreservesAlpha();
    void sharpnessPreservesOriginal();
};

void Phase4FilterTest::neutralFiltersPreserveImage()
{
    QImage original(3, 3, QImage::Format_ARGB32);
    original.fill(QColor(100, 120, 140, 180));

    const QImage result = ImageFilters::apply(original, ImageAdjustments());

    QCOMPARE(result, original);
}

void Phase4FilterTest::grayscaleProducesNeutralChannels()
{
    QImage original(2, 2, QImage::Format_ARGB32);
    original.fill(QColor(220, 40, 20, 255));

    ImageAdjustments adjustments;
    adjustments.grayscale = true;
    const QImage result = ImageFilters::apply(original, adjustments);

    const QRgb pixel = result.pixel(0, 0);
    QCOMPARE(qRed(pixel), qGreen(pixel));
    QCOMPARE(qGreen(pixel), qBlue(pixel));
}

void Phase4FilterTest::sepiaIsDeterministic()
{
    QImage original(3, 2, QImage::Format_ARGB32);
    original.fill(QColor(100, 150, 200, 255));

    ImageAdjustments adjustments;
    adjustments.sepia = true;
    const QImage first = ImageFilters::apply(original, adjustments);
    const QImage second = ImageFilters::apply(original, adjustments);

    QCOMPARE(first, second);
}

void Phase4FilterTest::temperaturePreservesAlpha()
{
    QImage original(4, 3, QImage::Format_ARGB32);
    original.fill(QColor(100, 120, 140, 73));

    ImageAdjustments adjustments;
    adjustments.temperature = 1.0;
    const QImage result = ImageFilters::apply(original, adjustments);

    QCOMPARE(result.width(), original.width());
    QCOMPARE(result.height(), original.height());
    QCOMPARE(qAlpha(result.pixel(0, 0)), 73);
}

void Phase4FilterTest::sharpnessPreservesOriginal()
{
    QImage original(5, 5, QImage::Format_ARGB32);
    original.fill(QColor(60, 80, 100, 255));
    const QImage before = original.copy();

    ImageAdjustments adjustments;
    adjustments.sharpness = 0.5;
    const QImage result = ImageFilters::apply(original, adjustments);

    QVERIFY(!result.isNull());
    QCOMPARE(original, before);
}

QTEST_GUILESS_MAIN(Phase4FilterTest)
#include "phase4_filter_test.moc"
