#include "core/ImageDocument.h"
#include "commands/AdjustmentCommand.h"

#include <QtTest>
#include <QUndoStack>

class Phase5UndoTest final : public QObject
{
    Q_OBJECT

private slots:
    void undoRedoBrightness();
    void redoIsClearedByNewCommand();
    void mergedSliderDragCreatesOneCommand();
    void resetCanBeUndone();
    void newDocumentClearsStack();
};

void Phase5UndoTest::undoRedoBrightness()
{
    QImage image(3, 2, QImage::Format_ARGB32);
    image.fill(Qt::red);
    ImageDocument document;
    QVERIFY(document.setImage(image));
    QUndoStack stack;

    const ImageAdjustments before = document.adjustments();
    document.setBrightness(0.25);
    stack.push(new AdjustmentCommand(&document, before, document.adjustments(),
                                     QStringLiteral("Brightness"),
                                     AdjustmentCommand::BrightnessId));

    QVERIFY(qFuzzyCompare(document.adjustments().brightness, 0.25));
    stack.undo();
    QVERIFY(qFuzzyCompare(document.adjustments().brightness, 0.0));
    stack.redo();
    QVERIFY(qFuzzyCompare(document.adjustments().brightness, 0.25));
}

void Phase5UndoTest::redoIsClearedByNewCommand()
{
    ImageDocument document;
    QImage image(2, 2, QImage::Format_ARGB32);
    document.setImage(image);
    QUndoStack stack;

    const ImageAdjustments before = document.adjustments();
    document.setContrast(1.2);
    stack.push(new AdjustmentCommand(&document, before, document.adjustments(),
                                     QStringLiteral("Contrast"),
                                     AdjustmentCommand::ContrastId));
    stack.undo();

    const ImageAdjustments beforeNew = document.adjustments();
    document.setSaturation(0.5);
    stack.push(new AdjustmentCommand(&document, beforeNew, document.adjustments(),
                                     QStringLiteral("Saturation"),
                                     AdjustmentCommand::SaturationId));

    QVERIFY(!stack.canRedo());
}

void Phase5UndoTest::mergedSliderDragCreatesOneCommand()
{
    ImageDocument document;
    QImage image(2, 2, QImage::Format_ARGB32);
    document.setImage(image);
    QUndoStack stack;

    const ImageAdjustments initial = document.adjustments();
    document.setBrightness(0.10);
    stack.push(new AdjustmentCommand(&document, initial, document.adjustments(),
                                     QStringLiteral("Brightness"),
                                     AdjustmentCommand::BrightnessId));

    ImageAdjustments middle = document.adjustments();
    document.setBrightness(0.20);
    stack.push(new AdjustmentCommand(&document, middle, document.adjustments(),
                                     QStringLiteral("Brightness"),
                                     AdjustmentCommand::BrightnessId));

    QCOMPARE(stack.count(), 1);
    stack.undo();
    QVERIFY(qFuzzyCompare(document.adjustments().brightness, 0.0));
}

void Phase5UndoTest::resetCanBeUndone()
{
    ImageDocument document;
    QImage image(2, 2, QImage::Format_ARGB32);
    document.setImage(image);
    QUndoStack stack;

    const ImageAdjustments before = document.adjustments();
    document.setSepia(true);
    document.setTemperature(0.5);
    stack.push(new AdjustmentCommand(&document, before, document.adjustments(),
                                     QStringLiteral("Reset adjustments"),
                                     AdjustmentCommand::ResetId));

    stack.undo();
    QVERIFY(document.adjustments().isNeutral());
}

void Phase5UndoTest::newDocumentClearsStack()
{
    ImageDocument document;
    QUndoStack stack;

    QImage first(2, 2, QImage::Format_ARGB32);
    document.setImage(first);
    const ImageAdjustments before = document.adjustments();
    document.setSharpness(0.5);
    stack.push(new AdjustmentCommand(&document, before, document.adjustments(),
                                     QStringLiteral("Sharpness"),
                                     AdjustmentCommand::SharpnessId));

    QCOMPARE(stack.count(), 1);
    QImage second(4, 3, QImage::Format_ARGB32);
    document.setImage(second);
    stack.clear();

    QCOMPARE(stack.count(), 0);
    QVERIFY(document.adjustments().isNeutral());
}

QTEST_GUILESS_MAIN(Phase5UndoTest)
#include "phase5_undo_test.moc"
