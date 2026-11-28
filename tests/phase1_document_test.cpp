#include "core/ImageDocument.h"
#include "core/PreviewRenderer.h"

#include <QTemporaryDir>
#include <QtTest>

class Phase1DocumentTest final : public QObject
{
    Q_OBJECT

private slots:
    void neutralAdjustmentsPreserveOriginal();
    void invalidImageDoesNotReplaceDocument();
    void changingImageResetsAdjustments();
    void rotationLeavesOriginalUnchanged();
};

void Phase1DocumentTest::neutralAdjustmentsPreserveOriginal()
{
    QImage original(5, 3, QImage::Format_RGB32);
    original.fill(QColor(20, 40, 60));

    ImageDocument document;
    QVERIFY(document.setImage(original));

    PreviewRenderer renderer;
    const QImage preview = renderer.render(
        document.originalImage(), document.adjustments());

    QCOMPARE(preview, original);
    QCOMPARE(document.originalImage(), original);
    QVERIFY(!document.isModified());
}

void Phase1DocumentTest::invalidImageDoesNotReplaceDocument()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    QImage original(4, 4, QImage::Format_RGB32);
    original.fill(Qt::blue);

    ImageDocument document;
    QVERIFY(document.setImage(original));
    QVERIFY(!document.load(directory.filePath(QStringLiteral("missing.png"))));
    QCOMPARE(document.originalImage(), original);
}

void Phase1DocumentTest::changingImageResetsAdjustments()
{
    QImage first(3, 3, QImage::Format_RGB32);
    first.fill(Qt::red);
    QImage second(2, 5, QImage::Format_RGB32);
    second.fill(Qt::green);

    ImageDocument document;
    QVERIFY(document.setImage(first));
    document.setRotation(90.0);
    document.setBrightness(0.25);
    QVERIFY(document.isModified());

    QVERIFY(document.setImage(second, QStringLiteral("second.png")));
    QVERIFY(document.adjustments().isNeutral());
    QCOMPARE(document.originalImage(), second);
    QCOMPARE(document.path(), QStringLiteral("second.png"));
    QVERIFY(!document.isModified());
}

void Phase1DocumentTest::rotationLeavesOriginalUnchanged()
{
    QImage original(4, 2, QImage::Format_RGB32);
    original.fill(Qt::black);
    original.setPixelColor(0, 0, QColor(Qt::red));
    const QImage before = original.copy();

    ImageDocument document;
    QVERIFY(document.setImage(original));
    document.setRotation(90.0);

    PreviewRenderer renderer;
    const QImage preview = renderer.render(
        document.originalImage(), document.adjustments());

    QCOMPARE(preview.width(), 2);
    QCOMPARE(preview.height(), 4);
    QCOMPARE(document.originalImage(), before);
    QVERIFY(document.isModified());
}

QTEST_GUILESS_MAIN(Phase1DocumentTest)
#include "phase1_document_test.moc"
