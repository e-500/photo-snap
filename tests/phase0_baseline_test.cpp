#include <QCoreApplication>
#include <QImage>
#include <QTemporaryDir>
#include <QtTest>

class Phase0BaselineTest final : public QObject
{
    Q_OBJECT

private slots:
    void pngRoundTripKeepsPixels();
    void ninetyDegreeRotationPreservesOriginal();
};

void Phase0BaselineTest::pngRoundTripKeepsPixels()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    QImage source(3, 2, QImage::Format_RGB32);
    source.fill(QColor(30, 90, 150));
    source.setPixelColor(0, 0, QColor(Qt::red));

    const QString path = directory.filePath(QStringLiteral("baseline.png"));
    QVERIFY(source.save(path, "PNG"));

    QImage loaded(path);
    QVERIFY(!loaded.isNull());
    QCOMPARE(loaded.size(), source.size());
    QCOMPARE(loaded.pixelColor(0, 0), source.pixelColor(0, 0));
}

void Phase0BaselineTest::ninetyDegreeRotationPreservesOriginal()
{
    QImage original(4, 2, QImage::Format_RGB32);
    original.fill(Qt::black);
    original.setPixelColor(0, 0, QColor(Qt::red));
    const QImage before = original.copy();

    const QImage rotated = original.transformed(QTransform().rotate(90), Qt::SmoothTransformation);

    QCOMPARE(rotated.width(), 2);
    QCOMPARE(rotated.height(), 4);
    QCOMPARE(original, before);
    QCOMPARE(rotated.pixelColor(1, 0), QColor(Qt::red));
}

QTEST_GUILESS_MAIN(Phase0BaselineTest)
#include "phase0_baseline_test.moc"
