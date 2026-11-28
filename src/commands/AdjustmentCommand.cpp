#include "AdjustmentCommand.h"

#include "../core/ImageDocument.h"

AdjustmentCommand::AdjustmentCommand(ImageDocument *document,
                                     const ImageAdjustments &before,
                                     const ImageAdjustments &after,
                                     const QString &text,
                                     int id)
    : QUndoCommand(text, nullptr),
      m_document(document),
      m_before(before),
      m_after(after),
      m_id(id)
{
}

int AdjustmentCommand::id() const
{
    return m_id;
}

bool AdjustmentCommand::mergeWith(const QUndoCommand *other)
{
    const AdjustmentCommand *command =
        dynamic_cast<const AdjustmentCommand *>(other);
    if (!command || command->id() != id())
        return false;
    m_after = command->m_after;
    return true;
}

void AdjustmentCommand::undo()
{
    m_document->setAdjustments(m_before);
}

void AdjustmentCommand::redo()
{
    m_document->setAdjustments(m_after);
}
