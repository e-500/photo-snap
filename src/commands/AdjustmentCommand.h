#pragma once

#include "../core/ImageAdjustments.h"

#include <QUndoCommand>

class ImageDocument;

class AdjustmentCommand final : public QUndoCommand
{
public:
    enum Id {
        RotationId = 1,
        CropId,
        BrightnessId,
        ContrastId,
        SaturationId,
        GrayscaleId,
        SepiaId,
        TemperatureId,
        SharpnessId,
        ResetId
    };

    AdjustmentCommand(ImageDocument *document,
                      const ImageAdjustments &before,
                      const ImageAdjustments &after,
                      const QString &text,
                      int id);

    int id() const override;
    bool mergeWith(const QUndoCommand *other) override;
    void undo() override;
    void redo() override;

private:
    ImageDocument *m_document;
    ImageAdjustments m_before;
    ImageAdjustments m_after;
    int m_id = 0;
};
