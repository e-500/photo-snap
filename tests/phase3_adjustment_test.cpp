#include "core/ImageFilters.h"
#include "core/PreviewRenderer.h"

#include <QtTest>

class Phase3AdjustmentTest final : public QObject
{
    Q_OBJECT

private slots:
    void neutralAdjustmentsPreservePixels();
    void adjustmentsLeaveOriginalUnchanged();
    void combinedAdjustmentsAreDeterministic();
    void obsoleteRequestIsNotDelivered();
};

void Phase3AdjustmentTest::neutralAdjustmentsPreservePixels()
{
    QImage original(4, 3, QImage::Format_ARGB32);
    original.fill(QColor(80, 120, 160, 220));

    PreviewRenderer renderer;
    const QImage preview = renderer.render(original, ImageAdjustments());

    QCOMPARE(preview, original);
}

void Phase3AdjustmentTest::adjustmentsLeaveOriginalUnchanged()
{
    QImage original(4, 3, QImage::Format_ARGB32);
    original.fill(QColor(80, 120, 160, 220));
    const QImage before = original.copy();

    ImageAdjustments adjustments;
    adjustments.brightness = 0.25;
    adjustments.contrast = 1.2;
    adjustments.saturation = 0.75;
    const QImage preview = ImageFilters::apply(original, adjustments);

    QVERIFY(!preview.isNull());
    QCOMPARE(original, before);
}

void Phase3AdjustmentTest::combinedAdjustmentsAreDeterministic()
{
    QImage original(3, 2, QImage::Format_ARGB32);
    original.fill(QColor(50, 100, 150, 255));

    ImageAdjustments adjustments;
    adjustments.brightness = -0.1;
    adjustments.contrast = 1.1;
    adjustments.saturation = 1.3;

    PreviewRenderer renderer;
    const QImage first = renderer.render(original, adjustments);
    const QImage second = renderer.render(original, adjustments);

    QCOMPARE(first, second);
}

void Phase3AdjustmentTest::obsoleteRequestIsNotDelivered()
{
    QImage original(2, 2, QImage::Format_ARGB32);
    original.fill(Qt::red);
    PreviewRenderer renderer;

    ImageAdjustments firstAdjustments;
    firstAdjustments.brightness = 0.5;
    const quint64 first = renderer.requestPreview(original, firstAdjustments);

    ImageAdjustments secondAdjustments;
    secondAdjustments.saturation = 0.0;
    const quint64 second = renderer.requestPreview(original, secondAdjustments);

    QVERIFY(second > first);
    quint64 deliveredId = 0;
    connect(&renderer, &PreviewRenderer::previewReady,
            [&deliveredId](quint64 requestId, const QImage &) {
        deliveredId = requestId;
    });
    QTRY_COMPARE(deliveredId, second);
}

QTEST_GUILESS_MAIN(Phase3AdjustmentTest)
#include "phase3_adjustment_test.moc"
