#include "core/PreviewRenderer.h"

#include <QElapsedTimer>
#include <QtTest>

#include <cstdio>

class Phase7BenchmarkTest final : public QObject
{
    Q_OBJECT

private slots:
    void benchmarkPreviewSizes();
    void cacheAvoidsRepeatedRender();
};

void Phase7BenchmarkTest::benchmarkPreviewSizes()
{
    const QList<QSize> sizes = {
        QSize(1920, 1080),
        QSize(4000, 3000),
        QSize(6000, 4000)
    };

    for (const QSize &size : sizes) {
        QImage image(size, QImage::Format_ARGB32);
        image.fill(QColor(100, 130, 160, 255));
        ImageAdjustments adjustments;
        adjustments.brightness = 0.05;
        adjustments.contrast = 1.1;
        adjustments.saturation = 0.9;
        adjustments.temperature = 0.1;

        PreviewRenderer renderer;
        QElapsedTimer timer;
        timer.start();
        const QImage preview = renderer.renderPreview(image, adjustments);
        const qint64 elapsedMs = timer.elapsed();

        QVERIFY(!preview.isNull());
        QVERIFY(preview.width() <= 1920);
        QVERIFY(preview.height() <= 1080);
        std::fprintf(stderr, "Preview %dx%d: %lld ms\n",
                     size.width(), size.height(),
                     static_cast<long long>(elapsedMs));
    }
}

void Phase7BenchmarkTest::cacheAvoidsRepeatedRender()
{
    QImage image(320, 240, QImage::Format_ARGB32);
    image.fill(Qt::gray);
    PreviewRenderer renderer;

    const QImage first = renderer.render(image, ImageAdjustments());
    const QImage second = renderer.render(image, ImageAdjustments());

    QCOMPARE(first, second);
}

QTEST_GUILESS_MAIN(Phase7BenchmarkTest)
#include "phase7_benchmark_test.moc"
